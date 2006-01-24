/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This file contains definitions relating to using strings specific to
 * the game. libs/strlib.h is for the string library.
 */

#ifndef _GAMESTR_H
#define _GAMESTR_H


#include "libs/strlib.h"

#define STAR_STRING_BASE 0
#define DEVICE_STRING_BASE 133
#define CARGO_STRING_BASE 162
#define ELEMENTS_STRING_BASE 172
#define SCAN_STRING_BASE 305
#define STAR_NUMBER_BASE 361
#define PLANET_NUMBER_BASE 375
#define MONTHS_STRING_BASE 407
#define FEEDBACK_STRING_BASE 419
#define STARBASE_STRING_BASE 421
#define ENCOUNTER_STRING_BASE 426
#define NAVIGATION_STRING_BASE 434
#define NAMING_STRING_BASE 440
#define MELEE_STRING_BASE 444
#define SAVEGAME_STRING_BASE 453
#define OPTION_STRING_BASE 458
#define QUITMENU_STRING_BASE 463
#define STATUS_STRING_BASE 468
#define FLAGSHIP_STRING_BASE 474
#define ORBITSCAN_STRING_BASE 487
#define MAINMENU_STRING_BASE 506

#define GAME_STRING(i) ((UNICODE *)GetStringAddress (SetAbsStringTableIndex (GameStrings, (i))))

extern STRING GameStrings;



#endif  /* _GAMESTR_H */

