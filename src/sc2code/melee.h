//Copyright Paul Reiche, Fred Ford. 1992-2002

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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _MELEE_H
#define _MELEE_H

#include "libs/tasklib.h"

enum
{
	MELEE_ANDROSYNTH,
	MELEE_ARILOU,
	MELEE_CHENJESU,
	MELEE_CHMMR,
	MELEE_DRUUGE,
	MELEE_EARTHLING,
	MELEE_ILWRATH,
	MELEE_KOHR_AH,
	MELEE_MELNORME,
	MELEE_MMRNMHRM,
	MELEE_MYCON,
	MELEE_ORZ,
	MELEE_PKUNK,
	MELEE_SHOFIXTI,
	MELEE_SLYLANDRO,
	MELEE_SPATHI,
	MELEE_SUPOX,
	MELEE_SYREEN,
	MELEE_THRADDASH,
	MELEE_UMGAH,
	MELEE_URQUAN,
	MELEE_UTWIG,
	MELEE_VUX,
	MELEE_YEHAT,
	MELEE_ZOQFOTPIK,
	
	NUM_MELEE_SHIPS
};

#define NUM_MELEE_ROWS 2
// #define NUM_MELEE_COLUMNS 7
#define NUM_MELEE_COLUMNS 6
#define ICON_WIDTH 16
#define ICON_HEIGHT 16

extern FRAME PickMeleeFrame;

#define PICK_BG_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x01, 0x0F), 0x1)

#define MAX_TEAM_CHARS 15
#define MAX_VIS_TEAMS 5
#define NUM_PREBUILT 5
#define NUM_PICK_COLS 5
#define NUM_PICK_ROWS 5

typedef BYTE MELEE_OPTIONS;

typedef struct
{
	BYTE ShipList[NUM_MELEE_ROWS][NUM_MELEE_COLUMNS];
	UNICODE TeamName[MAX_TEAM_CHARS + 25]; /* in case default names in starcon.txt
												   are unknowingly mangled */
} TEAM_IMAGE;

typedef struct melee_state
{
	BOOLEAN (*InputFunc) (struct melee_state *pInputState);
	COUNT MenuRepeatDelay;

	BOOLEAN Initialized;
	MELEE_OPTIONS MeleeOption;
	DIRENTRY TeamDE;
	COUNT TopTeamIndex, BotTeamIndex;
	COUNT side, row, col;
	TEAM_IMAGE TeamImage[NUM_SIDES];
	COUNT star_bucks[NUM_SIDES];
	COUNT CurIndex;
	Task flash_task;
	TEAM_IMAGE FileList[MAX_VIS_TEAMS];
	TEAM_IMAGE PreBuiltList[NUM_PREBUILT];
} MELEE_STATE;
typedef MELEE_STATE *PMELEE_STATE;

extern PMELEE_STATE volatile pMeleeState;

#endif /* _MELEE_H */

