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

#ifndef _PLANETS_H
#define _PLANETS_H

#define END_INTERPLANETARY START_INTERPLANETARY

enum
{
	SCAN = 0,
	STARMAP,
	EQUIP_DEVICE,
	CARGO,
	ROSTER,
	GAME_MENU,
	NAVIGATION
};

enum
{
	MINERAL_SCAN = 0,
	ENERGY_SCAN,
	BIOLOGICAL_SCAN,

	NUM_SCAN_TYPES,
	EXIT_SCAN = NUM_SCAN_TYPES,
	AUTO_SCAN,
	DISPATCH_SHUTTLE
};

#define MAP_WIDTH SIS_SCREEN_WIDTH
#define MAP_HEIGHT (75 - SAFE_Y)

enum
{
	GENERATE_PLANETS = 0,
	GENERATE_MOONS,
	GENERATE_ORBITAL,

	INIT_NPCS,
	REINIT_NPCS,
	UNINIT_NPCS,

	GENERATE_MINERAL,
	GENERATE_ENERGY,
	GENERATE_LIFE,

	GENERATE_NAME
};

enum
{
	BIOLOGICAL_DISASTER = 0,
	EARTHQUAKE_DISASTER,
	LIGHTNING_DISASTER,
	LAVASPOT_DISASTER,

		/* additional lander sounds */
	LANDER_INJURED,
	LANDER_SHOOTS,
	LANDER_HITS,
	LIFEFORM_CANNED,
	LANDER_PICKUP,
	LANDER_FULL,
	LANDER_DEPARTS,
	LANDER_RETURNS,
	LANDER_DESTROYED
};
#define MAX_SCROUNGED 50 /* max lander can hold */

#define SCALE_RADIUS(r) ((r) << 6)
#define UNSCALE_RADIUS(r) ((r) >> 6)
#define MAX_ZOOM_RADIUS SCALE_RADIUS(128)
#define MIN_ZOOM_RADIUS (MAX_ZOOM_RADIUS>>3)
#define EARTH_RADIUS SCALE_RADIUS(8)

#define MIN_PLANET_RADIUS SCALE_RADIUS (4)
#define MAX_PLANET_RADIUS SCALE_RADIUS (124)

#define DISPLAY_FACTOR ((SIS_SCREEN_WIDTH >> 1) - 8)

#define NUM_SCANDOT_TRANSITIONS 4

#define MIN_MOON_RADIUS 35
#define MOON_DELTA 20

#define PLANET_SHIELDED (1 << 7)

typedef struct planet_desc
{
	DWORD rand_seed;

	BYTE data_index;
	BYTE NumPlanets;
	SIZE radius;
	POINT location;

	COLOR temp_color;
	COUNT NextIndex;
	STAMP image;

	struct planet_desc *pPrevDesc;
} PLANET_DESC;
typedef PLANET_DESC *PPLANET_DESC;

typedef struct
{
	POINT star_pt;
	BYTE Type, Index;
	BYTE Prefix, Postfix;
} STAR_DESC;
typedef STAR_DESC *PSTAR_DESC;
#define STAR_DESCPTR PSTAR_DESC

#define MAX_SUNS 1
#define MAX_PLANETS 16
#define MAX_MOONS 4

#include "elemdata.h"
#include "plandata.h"
#include "sundata.h"

typedef void (*PLAN_GEN_FUNC) (BYTE control);

typedef struct solarsys_state
{
	MENU_STATE MenuState;

	COUNT WaitIntersect;
	PLANET_DESC SunDesc[MAX_SUNS],
						PlanetDesc[MAX_PLANETS],
						MoonDesc[MAX_MOONS];
	PPLANET_DESC pBaseDesc, pOrbitalDesc;
	SIZE FirstPlanetIndex, LastPlanetIndex;

	BYTE turn_counter, turn_wait;
	BYTE thrust_counter, max_ship_speed;

	FRAME TopoFrame;
	MEM_HANDLE hTopoData;
	PBYTE lpTopoData;
	STRING XlatRef;
	STRINGPTR XlatPtr;
	COLORMAP OrbitalCMap;

	SYSTEM_INFO SysInfo;

	COUNT CurNode;
	PLAN_GEN_FUNC GenFunc;

	FRAME PlanetSideFrame[6];
	FRAME PlanetFrameArray;
	BYTE *isPFADefined;
	FRAME ScaleFrame[2];
	FRAME ShieldFrame;
	FRAME TintFrame[2];
	UWORD Tint_rgb;
	DWORD **lpTopoMap;
	UBYTE PauseRotate;
} SOLARSYS_STATE;
typedef SOLARSYS_STATE *PSOLARSYS_STATE;

extern PSOLARSYS_STATE pSolarSysState;

extern void LoadPlanet (BOOLEAN IsDefined);
extern void FreePlanet (void);

extern void ExploreSolarSys (void);
extern void DrawStarBackGround (BOOLEAN ForPlanet);
extern void XFormIPLoc (PPOINT pIn, PPOINT pOut, BOOLEAN ToDisplay);
extern void GenerateRandomIP (BYTE control);
extern PLAN_GEN_FUNC GenerateIP (BYTE Index);
extern void DrawSystem (SIZE radius, BOOLEAN IsInnerSystem);
extern void DrawOval (PRECT pRect, BYTE num_off_pixels);
extern void DrawFilledOval (PRECT pRect);
extern void DoMissions (void);
extern void FillOrbits (BYTE NumPlanets, PPLANET_DESC pBaseDesc, BOOLEAN
		TypesDefined);
extern void ScanSystem (void);
extern void ChangeSolarSys (void);
extern BOOLEAN DoFlagshipCommands (INPUT_STATE InputState, PMENU_STATE
		pMS);
extern void ZoomSystem (void);
extern void LoadSolarSys (void);
extern void InitLander (BYTE LanderFlags);

#endif /* _PLANETS_H */

