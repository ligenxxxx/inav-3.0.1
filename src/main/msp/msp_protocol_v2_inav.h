/*
 * This file is part of INAV
 *
 * INAV is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * INAV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with INAV.  If not, see <http://www.gnu.org/licenses/>.
 */

// iNav specific IDs start from 0x2000
// See https://github.com/iNavFlight/inav/wiki/MSP-V2#msp-v2-message-catalogue

#define MSP2_INAV_STATUS                        0x2000
#define MSP2_INAV_OPTICAL_FLOW                  0x2001
#define MSP2_INAV_ANALOG                        0x2002
#define MSP2_INAV_MISC                          0x2003
#define MSP2_INAV_SET_MISC                      0x2004
#define MSP2_INAV_BATTERY_CONFIG                0x2005
#define MSP2_INAV_SET_BATTERY_CONFIG            0x2006
#define MSP2_INAV_RATE_PROFILE                  0x2007
#define MSP2_INAV_SET_RATE_PROFILE              0x2008
#define MSP2_INAV_AIR_SPEED                     0x2009
#define MSP2_INAV_OUTPUT_MAPPING                0x200A
#define MSP2_INAV_MC_BRAKING                    0x200B
#define MSP2_INAV_SET_MC_BRAKING                0x200C

#define MSP2_INAV_MIXER                         0x2010
#define MSP2_INAV_SET_MIXER                     0x2011

#define MSP2_INAV_OSD_LAYOUTS                   0x2012
#define MSP2_INAV_OSD_SET_LAYOUT_ITEM           0x2013
#define MSP2_INAV_OSD_ALARMS                    0x2014
#define MSP2_INAV_OSD_SET_ALARMS                0x2015
#define MSP2_INAV_OSD_PREFERENCES               0x2016
#define MSP2_INAV_OSD_SET_PREFERENCES           0x2017

#define MSP2_INAV_SELECT_BATTERY_PROFILE        0x2018

#define MSP2_INAV_DEBUG                         0x2019

#define MSP2_BLACKBOX_CONFIG                    0x201A
#define MSP2_SET_BLACKBOX_CONFIG                0x201B

#define MSP2_INAV_TEMP_SENSOR_CONFIG            0x201C
#define MSP2_INAV_SET_TEMP_SENSOR_CONFIG        0x201D
#define MSP2_INAV_TEMPERATURES                  0x201E

#define MSP2_INAV_SERVO_MIXER                   0x2020
#define MSP2_INAV_SET_SERVO_MIXER               0x2021
#define MSP2_INAV_LOGIC_CONDITIONS              0x2022
#define MSP2_INAV_SET_LOGIC_CONDITIONS          0x2023
#define MSP2_INAV_GLOBAL_FUNCTIONS              0x2024
#define MSP2_INAV_SET_GLOBAL_FUNCTIONS          0x2025
#define MSP2_INAV_LOGIC_CONDITIONS_STATUS       0x2026
#define MSP2_INAV_GVAR_STATUS                   0x2027
#define MSP2_INAV_PROGRAMMING_PID               0x2028
#define MSP2_INAV_SET_PROGRAMMING_PID           0x2029
#define MSP2_INAV_PROGRAMMING_PID_STATUS        0x202A

#define MSP2_PID                                0x2030
#define MSP2_SET_PID                            0x2031

#define MSP2_INAV_OPFLOW_CALIBRATION            0x2032

#define MSP2_INAV_FWUPDT_PREPARE                0x2033
#define MSP2_INAV_FWUPDT_STORE                  0x2034
#define MSP2_INAV_FWUPDT_EXEC                   0x2035
#define MSP2_INAV_FWUPDT_ROLLBACK_PREPARE       0x2036
#define MSP2_INAV_FWUPDT_ROLLBACK_EXEC          0x2037

#define MSP2_INAV_SAFEHOME                      0x2038
#define MSP2_INAV_SET_SAFEHOME                  0x2039

#define MSP2_INAV_MISC2                         0x203A
