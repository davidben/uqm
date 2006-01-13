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

#include "planets/elemdata.h"
#include "libs/compiler.h"
#include "libs/gfxlib.h"
#include "libs/sndlib.h"
#include "menustat.h"


#define NUM_TEXT_FRAMES 32

typedef struct
{
	BOOLEAN InTransit;
			// Landing on or taking of from a planet.
			// Setting it while landed will initiate takeoff.

	SOUND OldMenuSounds;

	COUNT ElementLevel, MaxElementLevel,
				BiologicalLevel;
	COUNT ElementAmounts[NUM_ELEMENT_CATEGORIES];

	COUNT NumFrames;
	UNICODE AmountBuf[40];
	TEXT MineralText[3];

	COLOR ColorCycle[NUM_TEXT_FRAMES >> 1];

	BYTE TectonicsChance, WeatherChance, FireChance;
} PLANETSIDE_DESC;
typedef PLANETSIDE_DESC *PPLANETSIDE_DESC;

extern CONTEXT ScanContext;
extern MUSIC_REF LanderMusic;

extern void PlanetSide (PMENU_STATE pMS);
extern void DoDiscoveryReport (SOUND ReadOutSounds);
extern void SetPlanetMusic (BYTE planet_type);
extern void LoadLanderData (void);
extern void FreeLanderData (void);


#endif /* _LANDER_H */

