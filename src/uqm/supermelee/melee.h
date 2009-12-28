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

#include "../init.h"
#include "libs/gfxlib.h"
#include "libs/mathlib.h"
#include "libs/sndlib.h"
#include "libs/timelib.h"
#include "libs/reslib.h"
#include "../netplay/packet.h"
		// for NetplayAbortReason and NetplayResetReason.

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

#define PICK_BG_COLOR    BUILD_COLOR (MAKE_RGB15 (0x00, 0x01, 0x0F), 0x01)
#define PICK_VALUE_COLOR BUILD_COLOR (MAKE_RGB15 (0x13, 0x00, 0x00), 0x2C)
		// Used for the current fleet value in the next ship selection
		// in SuperMelee.

#define MAX_TEAM_CHARS 30
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

#include "loadmele.h"

struct melee_side_state
{
	TEAM_IMAGE TeamImage;
	COUNT star_bucks;
};

struct melee_state
{
	BOOLEAN (*InputFunc) (struct melee_state *pInputState);

	BOOLEAN Initialized;
	MELEE_OPTIONS MeleeOption;
	COUNT side, row, col;
	struct melee_side_state SideState[NUM_SIDES];
	struct melee_load_state load;
	COUNT CurIndex;
	RandomContext *randomContext;
			/* RNG state for all local random decisions, i.e. those
			 * decisions that are not shared among network parties. */
	TimeCount LastInputTime;

	MUSIC_REF hMusic;
};

extern void Melee (void);

// Some prototypes for use by loadmele.c:
BOOLEAN DoMelee (MELEE_STATE *pMS);
void DrawMeleeIcon (COUNT which_icon);
COUNT GetTeamValue (TEAM_IMAGE *pTI);
void RepairMeleeFrame (RECT *pRect);
extern FRAME MeleeFrame;

void teamStringChanged (MELEE_STATE *pMS, int player);
void entireFleetChanged (MELEE_STATE *pMS, int player);
void fleetShipChanged (MELEE_STATE *pMS, int player, size_t index);

void updateTeamName (MELEE_STATE *pMS, COUNT side, const char *name,
		size_t len);
bool updateFleetShip (MELEE_STATE *pMS, COUNT side, COUNT index, BYTE ship);
void updateRandomSeed (MELEE_STATE *pMS, COUNT side, DWORD seed);
void confirmationCancelled(MELEE_STATE *pMS, COUNT side);
void connectedFeedback (NetConnection *conn);
void abortFeedback (NetConnection *conn, NetplayAbortReason reason);
void resetFeedback (NetConnection *conn, NetplayResetReason reason,
		bool byRemote);
void errorFeedback (NetConnection *conn);
void closeFeedback (NetConnection *conn);

#endif /* _MELEE_H */

