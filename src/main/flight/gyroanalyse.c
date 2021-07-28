/*
 * This file is part of Cleanflight and Betaflight.
 *
 * Cleanflight and Betaflight are free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Cleanflight and Betaflight are distributed in the hope that they
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

/* original work by Rav
 * 2018_07 updated by ctzsnooze to post filter, wider Q, different peak detection
 * coding assistance and advice from DieHertz, Rav, eTracer
 * test pilots icr4sh, UAV Tech, Flint723
 */
#include <stdint.h>

#include "platform.h"
FILE_COMPILE_FOR_SPEED

#ifdef USE_DYNAMIC_FILTERS

#include "build/debug.h"

#include "common/filter.h"
#include "common/maths.h"
#include "common/time.h"
#include "common/utils.h"
#include "config/feature.h"

#include "drivers/accgyro/accgyro.h"
#include "drivers/time.h"

#include "sensors/gyro.h"
#include "fc/config.h"

#include "gyroanalyse.h"

// The FFT splits the frequency domain into an number of bins
// A sampling frequency of 1000 and max frequency of 500 at a window size of 32 gives 16 frequency bins each 31.25Hz wide
// Eg [0,31), [31,62), [62, 93) etc
// for gyro loop >= 4KHz, sample rate 2000 defines FFT range to 1000Hz, 16 bins each 62.5 Hz wide
// NB  FFT_WINDOW_SIZE is set to 32 in gyroanalyse.h
#define FFT_BIN_COUNT             (FFT_WINDOW_SIZE / 2)
// smoothing frequency for FFT centre frequency
#define DYN_NOTCH_SMOOTH_FREQ_HZ  50
// we need 4 steps for each axis
#define DYN_NOTCH_CALC_TICKS      (XYZ_AXIS_COUNT * 4)

void gyroDataAnalyseStateInit(
    gyroAnalyseState_t *state, 
    uint16_t minFrequency,
    uint8_t range,
    uint32_t targetLooptimeUs
) {
    state->fftSamplingRateHz = DYN_NOTCH_RANGE_HZ_LOW;
    state->minFrequency = minFrequency;

    if (range == DYN_NOTCH_RANGE_HIGH) {
        state->fftSamplingRateHz = DYN_NOTCH_RANGE_HZ_HIGH;
    }
    else if (range == DYN_NOTCH_RANGE_MEDIUM) {
        state->fftSamplingRateHz = DYN_NOTCH_RANGE_HZ_MEDIUM;
    }

    // If we get at least 3 samples then use the default FFT sample frequency
    // otherwise we need to calculate a FFT sample frequency to ensure we get 3 samples (gyro loops < 4K)
    const int gyroLoopRateHz = lrintf((1.0f / targetLooptimeUs) * 1e6f);
    
    state->fftSamplingRateHz = MIN((gyroLoopRateHz / 3), state->fftSamplingRateHz);

    state->fftResolution = (float)state->fftSamplingRateHz / FFT_WINDOW_SIZE;

    state->fftStartBin = state->minFrequency / lrintf(state->fftResolution);

    state->maxFrequency = state->fftSamplingRateHz / 2; //Nyquist

    for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
        state->hanningWindow[i] = (0.5f - 0.5f * cos_approx(2 * M_PIf * i / (FFT_WINDOW_SIZE - 1)));
    }

    const uint16_t samplingFrequency = 1000000 / targetLooptimeUs;
    state->maxSampleCount = samplingFrequency / state->fftSamplingRateHz;
    state->maxSampleCountRcp = 1.f / state->maxSampleCount;

    arm_rfft_fast_init_f32(&state->fftInstance, FFT_WINDOW_SIZE);

//    recalculation of filters takes 4 calls per axis => each filter gets updated every DYN_NOTCH_CALC_TICKS calls
//    at 4khz gyro loop rate this means 4khz / 4 / 3 = 333Hz => update every 3ms
//    for gyro rate > 16kHz, we have update frequency of 1kHz => 1ms
    const float looptime = MAX(1000000u / state->fftSamplingRateHz, targetLooptimeUs * DYN_NOTCH_CALC_TICKS);
    for (int axis = 0; axis < XYZ_AXIS_COUNT; axis++) {
        // any init value
        state->centerFreq[axis] = state->maxFrequency;
        state->prevCenterFreq[axis] = state->maxFrequency;
        biquadFilterInitLPF(&state->detectedFrequencyFilter[axis], DYN_NOTCH_SMOOTH_FREQ_HZ, looptime);
    }
}

void gyroDataAnalysePush(gyroAnalyseState_t *state, const int axis, const float sample)
{
    state->oversampledGyroAccumulator[axis] += sample;
}

static void gyroDataAnalyseUpdate(gyroAnalyseState_t *state);

/*
 * Collect gyro data, to be analysed in gyroDataAnalyseUpdate function
 */
void gyroDataAnalyse(gyroAnalyseState_t *state)
{
    state->filterUpdateExecute = false; //This will be changed to true only if new data is present

    // samples should have been pushed by `gyroDataAnalysePush`
    // if gyro sampling is > 1kHz, accumulate multiple samples
    state->sampleCount++;

    // this runs at 1kHz
    if (state->sampleCount == state->maxSampleCount) {
        state->sampleCount = 0;

        // calculate mean value of accumulated samples
        for (int axis = 0; axis < XYZ_AXIS_COUNT; axis++) {
            float sample = state->oversampledGyroAccumulator[axis] * state->maxSampleCountRcp;
            state->downsampledGyroData[axis][state->circularBufferIdx] = sample;

            state->oversampledGyroAccumulator[axis] = 0;
        }

        state->circularBufferIdx = (state->circularBufferIdx + 1) % FFT_WINDOW_SIZE;

        // We need DYN_NOTCH_CALC_TICKS tick to update all axis with newly sampled value
        state->updateTicks = DYN_NOTCH_CALC_TICKS;
    }

    // calculate FFT and update filters
    if (state->updateTicks > 0) {
        gyroDataAnalyseUpdate(state);
        --state->updateTicks;
    }
}

void stage_rfft_f32(arm_rfft_fast_instance_f32 *S, float32_t *p, float32_t *pOut);
void arm_cfft_radix8by2_f32(arm_cfft_instance_f32 *S, float32_t *p1);
void arm_cfft_radix8by4_f32(arm_cfft_instance_f32 *S, float32_t *p1);
void arm_radix8_butterfly_f32(float32_t *pSrc, uint16_t fftLen, const float32_t *pCoef, uint16_t twidCoefModifier);
void arm_bitreversal_32(uint32_t *pSrc, const uint16_t bitRevLen, const uint16_t *pBitRevTable);

/*
 * Analyse last gyro data from the last FFT_WINDOW_SIZE milliseconds
 */
static NOINLINE void gyroDataAnalyseUpdate(gyroAnalyseState_t *state)
{
    enum {
        STEP_ARM_CFFT_F32,
        STEP_BITREVERSAL,
        STEP_STAGE_RFFT_F32,
        STEP_ARM_CMPLX_MAG_F32,
        STEP_CALC_FREQUENCIES,
        STEP_UPDATE_FILTERS,
        STEP_HANNING,
        STEP_COUNT
    };

    arm_cfft_instance_f32 *Sint = &(state->fftInstance.Sint);

    switch (state->updateStep) {
        case STEP_ARM_CFFT_F32:
        {
            switch (FFT_BIN_COUNT) {
            case 16:
                // 16us
                arm_cfft_radix8by2_f32(Sint, state->fftData);
                break;
            case 32:
                // 35us
                arm_cfft_radix8by4_f32(Sint, state->fftData);
                break;
            case 64:
                // 70us
                arm_radix8_butterfly_f32(state->fftData, FFT_BIN_COUNT, Sint->pTwiddle, 1);
                break;
            }
            break;
        }
        case STEP_BITREVERSAL:
        {
            // 6us
            arm_bitreversal_32((uint32_t*) state->fftData, Sint->bitRevLength, Sint->pBitRevTable);
            state->updateStep++;
            FALLTHROUGH;
        }
        case STEP_STAGE_RFFT_F32:
        {
            // 14us
            // this does not work in place => fftData AND rfftData needed
            stage_rfft_f32(&state->fftInstance, state->fftData, state->rfftData);
            break;
        }
        case STEP_ARM_CMPLX_MAG_F32:
        {
            // 8us
            arm_cmplx_mag_f32(state->rfftData, state->fftData, FFT_BIN_COUNT);
            state->updateStep++;
            FALLTHROUGH;
        }
        case STEP_CALC_FREQUENCIES:
        {
            bool fftIncreased = false;
            float dataMax = 0;
            uint8_t binStart = 0;
            uint8_t binMax = 0;
            //for bins after initial decline, identify start bin and max bin 
            for (int i = state->fftStartBin; i < FFT_BIN_COUNT; i++) {
                if (fftIncreased || (state->fftData[i] > state->fftData[i - 1])) {
                    if (!fftIncreased) {
                        binStart = i; // first up-step bin
                        fftIncreased = true;
                    }
                    if (state->fftData[i] > dataMax) {
                        dataMax = state->fftData[i];
                        binMax = i;  // tallest bin
                    }
                }
            }
            // accumulate fftSum and fftWeightedSum from peak bin, and shoulder bins either side of peak
            float cubedData = state->fftData[binMax] * state->fftData[binMax] * state->fftData[binMax];
            float fftSum = cubedData;
            float fftWeightedSum = cubedData * (binMax + 1);
            // accumulate upper shoulder
            for (int i = binMax; i < FFT_BIN_COUNT - 1; i++) {
                if (state->fftData[i] > state->fftData[i + 1]) {
                    cubedData = state->fftData[i] * state->fftData[i] * state->fftData[i];
                    fftSum += cubedData;
                    fftWeightedSum += cubedData * (i + 1);
                } else {
                break;
                }
            }
            // accumulate lower shoulder
            for (int i = binMax; i > binStart + 1; i--) {
                if (state->fftData[i] > state->fftData[i - 1]) {
                    cubedData = state->fftData[i] * state->fftData[i] * state->fftData[i];
                    fftSum += cubedData;
                    fftWeightedSum += cubedData * (i + 1);
                } else {
                break;
                }
            }
            // get weighted center of relevant frequency range (this way we have a better resolution than 31.25Hz)
            float centerFreq = state->maxFrequency;
            float fftMeanIndex = 0;
             // idx was shifted by 1 to start at 1, not 0
            if (fftSum > 0) {
                fftMeanIndex = (fftWeightedSum / fftSum) - 1;
                // the index points at the center frequency of each bin so index 0 is actually 16.125Hz
                centerFreq = fftMeanIndex * state->fftResolution;
            } else {
                centerFreq = state->prevCenterFreq[state->updateAxis];
            }
            centerFreq = fmax(centerFreq, state->minFrequency);
            centerFreq = biquadFilterApply(&state->detectedFrequencyFilter[state->updateAxis], centerFreq);
            state->prevCenterFreq[state->updateAxis] = state->centerFreq[state->updateAxis];
            state->centerFreq[state->updateAxis] = centerFreq;
            break;
        }
        case STEP_UPDATE_FILTERS:
        {
            // 7us
            // calculate cutoffFreq and notch Q, update notch filter  =1.8+((A2-150)*0.004)
            if (state->prevCenterFreq[state->updateAxis] != state->centerFreq[state->updateAxis]) {
                /*
                 * Filters will be updated inside dynamicGyroNotchFiltersUpdate()
                 */
                state->filterUpdateExecute = true;
                state->filterUpdateAxis = state->updateAxis;
                state->filterUpdateFrequency = state->centerFreq[state->updateAxis];
            }

            state->updateAxis = (state->updateAxis + 1) % XYZ_AXIS_COUNT;
            state->updateStep++;
            FALLTHROUGH;
        }
        case STEP_HANNING:
        {
            // 5us
            // apply hanning window to gyro samples and store result in fftData
            // hanning starts and ends with 0, could be skipped for minor speed improvement
            const uint8_t ringBufIdx = FFT_WINDOW_SIZE - state->circularBufferIdx;
            arm_mult_f32(&state->downsampledGyroData[state->updateAxis][state->circularBufferIdx], &state->hanningWindow[0], &state->fftData[0], ringBufIdx);
            if (state->circularBufferIdx > 0) {
                arm_mult_f32(&state->downsampledGyroData[state->updateAxis][0], &state->hanningWindow[ringBufIdx], &state->fftData[ringBufIdx], state->circularBufferIdx);
            }
        }
    }

    state->updateStep = (state->updateStep + 1) % STEP_COUNT;
}

#endif // USE_DYNAMIC_FILTERS
