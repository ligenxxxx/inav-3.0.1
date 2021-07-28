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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "platform.h"

#ifdef USE_CMS
#include "cms/cms.h"
#include "cms/cms_types.h"
#include "cms/cms_menu_saveexit.h"

#include "config/feature.h"

#include "fc/config.h"
static const OSD_Entry cmsx_menuSaveExitEntries[] =
{
    OSD_LABEL_ENTRY("-- SAVE/EXIT --"),

    {"EXIT", {.func = cmsMenuExit}, (void *)CMS_EXIT, OME_OSD_Exit, 0},
    {"SAVE+EXIT", {.func = cmsMenuExit}, (void *)CMS_POPUP_SAVE, OME_OSD_Exit, 0},
    {"SAVE+REBOOT", {.func = cmsMenuExit}, (void *)CMS_POPUP_SAVEREBOOT, OME_OSD_Exit, 0},
    {"BACK", {.func = NULL}, NULL, OME_Back, 0 },
    OSD_END_ENTRY,
};

const CMS_Menu cmsx_menuSaveExit = {
#ifdef CMS_MENU_DEBUG
    .GUARD_text = "MENUSAVE",
    .GUARD_type = OME_MENU,
#endif
    .entries = cmsx_menuSaveExitEntries
};

#endif
