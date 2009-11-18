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

#ifndef _LANDER_H
#define _LANDER_H

#include "elemdata.h"
#include "libs/compiler.h"
#include "libs/gfxlib.h"
#include "libs/sndlib.h"
#include "libs/timelib.h"
#include "../element.h"


#define NUM_TEXT_FRAMES 32

typedef struct
{
	BOOLEAN InTransit;
			// Landing on or taking of from a planet.
			// Setting it while landed will initiate takeoff.

	COUNT ElementLevel;
	COUNT MaxElementLevel;
	COUNT BiologicalLevel;
	COUNT ElementAmounts[NUM_ELEMENT_CATEGORIES];

	COUNT NumFrames;
	UNICODE AmountBuf[40];
	TEXT MineralText[3];

	COLOR ColorCycle[NUM_TEXT_FRAMES >> 1];

	BYTE TectonicsChance;
	BYTE WeatherChance;
	BYTE FireChance;
} PLANETSIDE_DESC;

// This is a derived type from INPUT_STATE_DESC.
// Originally, the general MENU_STATE structure was used. Now, only the
// fields which are relevant are put in here. In the MENU_STATE structure,
// these fields were reused for all sorts of purposes, which had nothing
// to do with what the name suggests. Not all fields have been renamed yet
// in LanderInputState.
typedef struct LanderInputState LanderInputState;
struct LanderInputState {
	BOOLEAN (*InputFunc) (LanderInputState *pMS);
	COUNT MenuRepeatDelay;

	PLANETSIDE_DESC *planetSideDesc;
	SIZE Initialized;
	TimeCount NextTime;
			// Frame rate control
};

extern LanderInputState *pLanderInputState;
		// Temporary, to replace the references to pMenuState.
		// TODO: Many functions depend on pLanderInputState. In particular,
		//   the ELEMENT property functions. Fields in LanderInputState
		//   should either be made static vars, or ELEMENT should carry
		//   something like an 'intptr_t private' field
		// TODO: Generation functions use pLanderInputState to locate
		//   the PLANETSIDE_DESC.InTransit. Make those instances into
		//   a function call instead.


extern CONTEXT ScanContext;
extern MUSIC_REF LanderMusic;

extern void PlanetSide (POINT planetLoc);
extern void DoDiscoveryReport (SOUND ReadOutSounds);
extern void SetPlanetMusic (BYTE planet_type);
extern void LoadLanderData (void);
extern void FreeLanderData (void);

extern void object_animation (ELEMENT *ElementPtr);

// ELEMENT.playerNr constants
enum
{
	PS_HUMAN_PLAYER,
	PS_NON_PLAYER,
};

#endif /* _LANDER_H */

