/*
 * This file is part of iNav.
 *
 * iNav free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * iNav distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * An implementation of persistent data storage utilizing RTC backup data register.
 * Retains values written across software resets and boot loader activities.
 */

#include <stdint.h>
#include "platform.h"

#include "drivers/persistent.h"
#include "drivers/system.h"

#define PERSISTENT_OBJECT_MAGIC_VALUE (('i' << 24)|('N' << 16)|('a' << 8)|('v' << 0))

#ifdef USE_HAL_DRIVER

uint32_t persistentObjectRead(persistentObjectId_e id)
{
    RTC_HandleTypeDef rtcHandle = { .Instance = RTC };

    uint32_t value = HAL_RTCEx_BKUPRead(&rtcHandle, id);

    return value;
}

void persistentObjectWrite(persistentObjectId_e id, uint32_t value)
{
    RTC_HandleTypeDef rtcHandle = { .Instance = RTC };

    HAL_RTCEx_BKUPWrite(&rtcHandle, id, value);
}

void persistentObjectRTCEnable(void)
{
    RTC_HandleTypeDef rtcHandle = { .Instance = RTC };

#if !defined(STM32H7)
    __HAL_RCC_PWR_CLK_ENABLE(); // Enable Access to PWR
#endif
    HAL_PWR_EnableBkUpAccess(); // Disable backup domain protection

#if defined(__HAL_RCC_RTC_CLK_ENABLE)
    // For those MCUs with RTCAPBEN bit in RCC clock enable register, turn it on.
    __HAL_RCC_RTC_CLK_ENABLE(); // Enable RTC module
#endif

    // We don't need a clock source for RTC itself. Skip it.

    __HAL_RTC_WRITEPROTECTION_ENABLE(&rtcHandle);  // Reset sequence
    __HAL_RTC_WRITEPROTECTION_DISABLE(&rtcHandle); // Apply sequence
}

#else
uint32_t persistentObjectRead(persistentObjectId_e id)
{
    uint32_t value = RTC_ReadBackupRegister(id);

    return value;
}

void persistentObjectWrite(persistentObjectId_e id, uint32_t value)
{
    RTC_WriteBackupRegister(id, value);
}

void persistentObjectRTCEnable(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE); // Enable Access to PWR
    PWR_BackupAccessCmd(ENABLE); // Disable backup domain protection

    // We don't need a clock source for RTC itself. Skip it.

    RTC_WriteProtectionCmd(ENABLE);  // Reset sequence
    RTC_WriteProtectionCmd(DISABLE); // Apply sequence
}
#endif

void persistentObjectInit(void)
{
    // Configure and enable RTC for backup register access

    persistentObjectRTCEnable();

    // XXX Magic value checking may be sufficient

    uint32_t wasSoftReset;

#ifdef STM32H7
    wasSoftReset = RCC->RSR & RCC_RSR_SFTRSTF;
#else
    wasSoftReset = RCC->CSR & RCC_CSR_SFTRSTF;
#endif

    if (!wasSoftReset || (persistentObjectRead(PERSISTENT_OBJECT_MAGIC) != PERSISTENT_OBJECT_MAGIC_VALUE)) {
        for (int i = 1; i < PERSISTENT_OBJECT_COUNT; i++) {
            persistentObjectWrite(i, 0);
        }
        persistentObjectWrite(PERSISTENT_OBJECT_MAGIC, PERSISTENT_OBJECT_MAGIC_VALUE);
    }
}
