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

#include "init.h"
#include "libs/tasklib.h"
#include "libs/gfxlib.h"
#include "libs/sndlib.h"
#include "libs/reslib.h"
#include "netplay/packet.h"
		// for NetplayAbortREason and NetplayResetReason.

typedef struct melee_state MELEE_STATE;

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
	
	NUM_MELEE_SHIPS,

	MELEE_NONE = (BYTE) ~0
};

#define NUM_MELEE_ROWS 2
#define NUM_MELEE_COLUMNS 7
//#define NUM_MELEE_COLUMNS 6
#define MELEE_FLEET_SIZE (NUM_MELEE_ROWS * NUM_MELEE_COLUMNS)
#define ICON_WIDTH 16
#define ICON_HEIGHT 16

extern FRAME PickMeleeFrame;

#define PICK_BG_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x01, 0x0F), 0x01)

#define MAX_TEAM_CHARS 30
#define MAX_VIS_TEAMS 5
#define NUM_PREBUILT 15
#define NUM_PICK_COLS 5
#define NUM_PICK_ROWS 5

typedef BYTE MELEE_OPTIONS;
typedef COUNT FleetShipIndex;

typedef struct
{
	BYTE ShipList[MELEE_FLEET_SIZE];
	UNICODE TeamName[MAX_TEAM_CHARS + 1 + 24];
			/* The +1 is for the terminating \0; the +24 is in case some
			 * default name in starcon.txt is unknowingly mangled. */
} TEAM_IMAGE;

struct melee_state
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

	MUSIC_REF hMusic;
};
typedef MELEE_STATE *PMELEE_STATE;

extern PMELEE_STATE volatile pMeleeState;

extern void Melee (void);

void teamStringChanged (PMELEE_STATE pMS, int player);
void entireFleetChanged (PMELEE_STATE pMS, int player);
void fleetShipChanged (PMELEE_STATE pMS, int player, size_t index);

void updateTeamName (PMELEE_STATE pMS, COUNT side, const char *name,
		size_t len);
bool updateFleetShip (PMELEE_STATE pMS, COUNT side, COUNT index, BYTE ship);
void updateRandomSeed (PMELEE_STATE pMS, COUNT side, DWORD seed);
void confirmationCancelled(PMELEE_STATE pMS, COUNT side);
void connectedFeedback (NetConnection *conn);
void abortFeedback (NetConnection *conn, NetplayAbortReason reason);
void resetFeedback (NetConnection *conn, NetplayResetReason reason,
		bool byRemote);
void errorFeedback (NetConnection *conn);
void closeFeedback (NetConnection *conn);

#endif /* _MELEE_H */

