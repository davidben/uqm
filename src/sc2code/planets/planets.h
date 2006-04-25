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

#include "menustat.h"
#include "units.h"

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

typedef struct planet_orbit
{
	FRAME TopoZoomFrame;
			// 4x scaled topo image for planet-side
	PBYTE lpTopoData;
			// normal topo data; expressed in elevation levels
			// data is signed for planets other than gas giants
			// transformed to light variance map for 3d planet
	FRAME PlanetFrameArray;
			// rotating 3d planet frames (current and next)
	FRAME ObjectFrame;
			// any extra planetary object (shield, atmo, rings)
			// automatically drawn if present
	FRAME TintFrame;
			// tinted topo images for current scan type (dynamic)
	DWORD *lpTopoMap;
			// RGBA version of topo image; for 3d planet
	DWORD *ScratchArray;
			// temp RGBA data for whatever transforms (nuked often)
	FRAME WorkFrame;
			// any extra frame workspace (for dynamic objects)
}  PLANET_ORBIT;

// See doc/devel/generate for information on how this structure is
// filled.
typedef struct solarsys_state
{
	MENU_STATE MenuState;

	COUNT WaitIntersect;
	PLANET_DESC SunDesc[MAX_SUNS];
	PLANET_DESC PlanetDesc[MAX_PLANETS];
			// Description of the planets in the system.
			// Only defined after a call to GenFunc with GENERATE_PLANETS
			// as its argument, and overwritten by subsequent calls.
	PLANET_DESC MoonDesc[MAX_MOONS];
			// Description of the moons orbiting the planet pointed to
			// by pBaseDesc.
			// Only defined after a call to GenFunc with GENERATE_MOONS
			// as its argument, and overwritten by subsequent calls.
	PPLANET_DESC pBaseDesc;
	PPLANET_DESC pOrbitalDesc;
	SIZE FirstPlanetIndex, LastPlanetIndex;
			// The planets get sorted on their image.origin.y value.
			// PlanetDesc[FirstPlanetIndex] is the planet with the lowest
			// image.origin.y, and PlanetDesc[LastPlanetIndex] has the
			// highest image.origin.y.
			// PlanetDesc[PlanetDesc[i].NextIndex] is the next planet
			// after PlanetDesc[i] in the ordering.

	BYTE turn_counter, turn_wait;
	BYTE thrust_counter, max_ship_speed;

	STRING XlatRef;
	STRINGPTR XlatPtr;
	COLORMAP OrbitalCMap;

	SYSTEM_INFO SysInfo;

	COUNT CurNode;
	PLAN_GEN_FUNC GenFunc;
			// Function to call to fill in various parts of this structure.
			// See doc/devel/generate.

	FRAME PlanetSideFrame[6];
	UWORD Tint_rgb;
	UBYTE PauseRotate;
	FRAME TopoFrame;
	PLANET_ORBIT Orbit;
} SOLARSYS_STATE;
typedef SOLARSYS_STATE *PSOLARSYS_STATE;

extern PSOLARSYS_STATE pSolarSysState;
extern MUSIC_REF SpaceMusic;

extern void LoadPlanet (FRAME SurfDefFrame);
extern void DrawPlanet (int x, int y, int dy, unsigned int rgb);
extern void FreePlanet (void);
extern void LoadStdLanderFont (PLANET_INFO *info);
extern void FreeLanderFont (PLANET_INFO *info);

extern void ExploreSolarSys (void);
extern void DrawStarBackGround (BOOLEAN ForPlanet);
extern void XFormIPLoc (PPOINT pIn, PPOINT pOut, BOOLEAN ToDisplay);
extern void GenerateRandomIP (BYTE control);
extern PLAN_GEN_FUNC GenerateIP (BYTE Index);
extern void DrawSystem (SIZE radius, BOOLEAN IsInnerSystem);
extern void DrawOval (PRECT pRect, BYTE num_off_pixels);
extern void DrawFilledOval (PRECT pRect);
extern void DoMissions (void);
extern void FillOrbits (PSOLARSYS_STATE system,
		BYTE NumPlanets, PPLANET_DESC pBaseDesc, BOOLEAN TypesDefined);
extern void ScanSystem (void);
extern void ChangeSolarSys (void);
extern BOOLEAN DoFlagshipCommands (PMENU_STATE pMS);
extern void ZoomSystem (void);
extern void LoadSolarSys (void);
extern void InitLander (BYTE LanderFlags);
extern BOOLEAN ValidateOrbits (void);
extern void IP_reset (void);
extern void IP_frame (void);

extern PRECT RotatePlanet (int x, int dx, int dy, COUNT scale_amt,
		UBYTE zoom_from, PRECT r);
extern void SetPlanetTilt (int da);
extern void DrawScannedObjects (BOOLEAN Reversed);
extern void GeneratePlanetMask (PPLANET_DESC pPlanetDesc, FRAME SurfDefFrame);
extern void DeltaTopography (COUNT num_iterations, PSBYTE DepthArray,
		PRECT pRect, SIZE depth_delta);

#endif /* _PLANETS_H */

