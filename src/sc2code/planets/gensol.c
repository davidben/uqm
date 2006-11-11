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

#include "build.h"
#include "gamestr.h"
#include "globdata.h"
#include "lander.h"
#include "lifeform.h"
#include "nameref.h"
#include "races.h"
#include "resinst.h"
#include "state.h"
#include "encount.h"
#include "planets/genall.h"
#include "libs/mathlib.h"


static int
init_probe (void)
{
	HSTARSHIP hStarShip;

	if (!GET_GAME_STATE (PROBE_MESSAGE_DELIVERED)
			&& GetGroupInfo (GLOBAL (BattleGroupRef), GROUP_INIT_IP)
			&& (hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q))))
	{
		SHIP_FRAGMENTPTR FragPtr;

		FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (npc_built_ship_q), hStarShip);
		SET_GROUP_MISSION (FragPtr, IN_ORBIT);
		SET_GROUP_LOC (FragPtr, 2 + 1); /* orbitting earth */
		SET_GROUP_DEST (FragPtr, 2 + 1); /* orbitting earth */
		FragPtr->ShipInfo.loc.x = FragPtr->ShipInfo.loc.y = 0;
		FragPtr->ShipInfo.group_counter = 0;
		UnlockStarShip (&GLOBAL (npc_built_ship_q), hStarShip);

		return 1;
	}
	else
		return 0;
}

static void
generate_energy_nodes (void)
{
				/* Pluto */
	if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[8])
	{
		if (!GET_GAME_STATE (FOUND_PLUTO_SPATHI))
		{
			pSolarSysState->SysInfo.PlanetInfo.CurPt.x = 20;
			pSolarSysState->SysInfo.PlanetInfo.CurPt.y = MAP_HEIGHT - 8;
			pSolarSysState->SysInfo.PlanetInfo.CurDensity = 0;
			pSolarSysState->SysInfo.PlanetInfo.CurType = 2;
			if (pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
					& (1L << 0))
			{
				SET_GAME_STATE (FOUND_PLUTO_SPATHI, 1);
				pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
						&= ~(1L << 0);
				((PPLANETSIDE_DESC)pMenuState->ModuleFrame)->InTransit = TRUE;
			}
			else if (pSolarSysState->CurNode == (COUNT)~0)
				pSolarSysState->CurNode = 1;
			return;
		}
	}
				/* Earth Moon */
	else if (pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->PlanetDesc[2]
			&& pSolarSysState->pOrbitalDesc == &pSolarSysState->MoonDesc[1]
			&& !GET_GAME_STATE (MOONBASE_DESTROYED))
	{
		pSolarSysState->SysInfo.PlanetInfo.CurPt.x = MAP_WIDTH * 3 / 4;
		pSolarSysState->SysInfo.PlanetInfo.CurPt.y = MAP_HEIGHT * 1 / 4;
		pSolarSysState->SysInfo.PlanetInfo.CurDensity = 0;
		pSolarSysState->SysInfo.PlanetInfo.CurType = 0;
		if (!(pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
				& (1L << 0))
				&& pSolarSysState->CurNode == (COUNT)~0)
			pSolarSysState->CurNode = 1;
		else
		{
			pSolarSysState->CurNode = 0;
			if (pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
					& (1L << 0))
			{
				SET_GAME_STATE (MOONBASE_DESTROYED, 1);
				SET_GAME_STATE (MOONBASE_ON_SHIP, 1);
			}
		}
		return;
	}
	pSolarSysState->CurNode = 0;
}

static void
generate_tractors (void)
{
	if (pSolarSysState->pOrbitalDesc->pPrevDesc != &pSolarSysState->PlanetDesc[2]
			|| pSolarSysState->pOrbitalDesc != &pSolarSysState->MoonDesc[1])
		pSolarSysState->CurNode = 0;
	else /* Earth Moon */
	{
		COUNT i, which_node;
		DWORD old_rand, rand_val;

		old_rand = TFB_SeedRandom (pSolarSysState->SysInfo.PlanetInfo.ScanSeed[BIOLOGICAL_SCAN]);

		which_node = i = 0;
		do
		{
			rand_val = TFB_Random ();
			pSolarSysState->SysInfo.PlanetInfo.CurPt.x =
					(LOBYTE (LOWORD (rand_val)) % (MAP_WIDTH - (8 << 1))) + 8;
			pSolarSysState->SysInfo.PlanetInfo.CurPt.y =
					(HIBYTE (LOWORD (rand_val)) % (MAP_HEIGHT - (8 << 1))) + 8;
			pSolarSysState->SysInfo.PlanetInfo.CurType = NUM_CREATURE_TYPES + 1;
			if (which_node >= pSolarSysState->CurNode
					&& !(pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[BIOLOGICAL_SCAN]
					& (1L << i)))
				break;
			++which_node;
		} while (++i < 10);
		pSolarSysState->CurNode = which_node;

		TFB_SeedRandom (old_rand);
	}
}

static void
generate_orbital (void)
{
	COUNT i;
	DWORD rand_val;

		/* Starbase */
	if (pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->PlanetDesc[2]
			&& pSolarSysState->pOrbitalDesc == &pSolarSysState->MoonDesc[0])
	{
		PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
		ReinitQueue (&GLOBAL (npc_built_ship_q));

		EncounterGroup = 0;
		GLOBAL (CurrentActivity) |= START_ENCOUNTER;
		SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, (BYTE)~0);
		return;
	}

	rand_val = DoPlanetaryAnalysis (&pSolarSysState->SysInfo,
			pSolarSysState->pOrbitalDesc);
	if (rand_val)
	{
		pSolarSysState->SysInfo.PlanetInfo.ScanSeed[MINERAL_SCAN] =
				rand_val;
		i = (COUNT)~0;
		rand_val = GenerateMineralDeposits (&pSolarSysState->SysInfo, &i);
	}

	if (pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->SunDesc[0])
	{
		i = pSolarSysState->pOrbitalDesc - pSolarSysState->PlanetDesc;
		switch (i)
		{
			case 0: /* MERCURY */
				pSolarSysState->SysInfo.PlanetInfo.AtmoDensity = 0;
				pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 98;
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 38;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 3;
				pSolarSysState->SysInfo.PlanetInfo.Weather = 0;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 2;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 59 * 240;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = 165;
				break;
			case 1: /* VENUS */
				pSolarSysState->SysInfo.PlanetInfo.AtmoDensity = 90 *
						EARTH_ATMOSPHERE;
				pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 95;
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 95;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 177;
				pSolarSysState->SysInfo.PlanetInfo.Weather = 7;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 1;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 243 * 240;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = 457;
				break;
			case 2: /* EARTH */
				pSolarSysState->SysInfo.PlanetInfo.AtmoDensity =
						EARTH_ATMOSPHERE;
				pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 100;
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 100;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 23;
				pSolarSysState->SysInfo.PlanetInfo.Weather = 1;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 1;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 240;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = 22;
				break;
			case 3: /* MARS */
				// XXX: Mars atmo should actually be 1/2 in current units
				pSolarSysState->SysInfo.PlanetInfo.AtmoDensity = 1;
				pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 72;
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 53;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 24;
				pSolarSysState->SysInfo.PlanetInfo.Weather = 1;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 1;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 246;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = -53;
				break;
			case 4: /* JUPITER */
				pSolarSysState->SysInfo.PlanetInfo.AtmoDensity =
						GAS_GIANT_ATMOSPHERE;
				pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 24;
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 1120;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 3;
				pSolarSysState->SysInfo.PlanetInfo.Weather = 7;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 0;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 98;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = -143;
				pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 520L / 100;
				break;
			case 5: /* SATURN */
				pSolarSysState->SysInfo.PlanetInfo.AtmoDensity =
						GAS_GIANT_ATMOSPHERE;
				pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 13;
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 945;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 27;
				pSolarSysState->SysInfo.PlanetInfo.Weather = 7;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 0;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 102;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = -197;
				pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 952L / 100;
				break;
			case 6: /* URANUS */
				pSolarSysState->SysInfo.PlanetInfo.AtmoDensity =
						GAS_GIANT_ATMOSPHERE;
				pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 21;
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 411;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 98;
				pSolarSysState->SysInfo.PlanetInfo.Weather = 7;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 0;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 172;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = -217;
				pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 1916L / 100;
				break;
			case 7: /* NEPTUNE */
				pSolarSysState->SysInfo.PlanetInfo.AtmoDensity =
						GAS_GIANT_ATMOSPHERE;
				pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 28;
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 396;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 30;
				pSolarSysState->SysInfo.PlanetInfo.Weather = 7;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 0;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 182;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = -229;
				pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 2999L / 100;
				break;
			case 8: /* PLUTO */
				if (!GET_GAME_STATE (FOUND_PLUTO_SPATHI))
				{
					LoadStdLanderFont (&pSolarSysState->SysInfo.PlanetInfo);
					pSolarSysState->PlanetSideFrame[1] =
							CaptureDrawable (
							LoadGraphic (SPAPLUTO_MASK_PMAP_ANIM));
					pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
							CaptureStringTable (
							LoadStringTable (SPAPLUTO_STRTAB));
				}

				pSolarSysState->SysInfo.PlanetInfo.AtmoDensity = 0;
				pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 33;
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 18;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 119;
				pSolarSysState->SysInfo.PlanetInfo.Weather = 0;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 0;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 1533;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = -235;
				pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 3937L / 100;
				break;
		}

		pSolarSysState->SysInfo.PlanetInfo.SurfaceGravity =
				CalcGravity (pSolarSysState->SysInfo.PlanetInfo.PlanetDensity,
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius);
		LoadPlanet (i == 2 ? CaptureDrawable (LoadGraphic (EARTH_MASK_ANIM))
				: NULL);
	}
	else
	{
		i = pSolarSysState->pOrbitalDesc->pPrevDesc
				- pSolarSysState->PlanetDesc;
		pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 0;
		pSolarSysState->SysInfo.PlanetInfo.AtmoDensity = 0;
		pSolarSysState->SysInfo.PlanetInfo.Weather = 0;
		switch (i)
		{
			case 2: /* moons of EARTH */
				pSolarSysState->SysInfo.PlanetInfo.ScanSeed[BIOLOGICAL_SCAN] =
						rand_val;

				if (!GET_GAME_STATE (MOONBASE_DESTROYED))
				{
					LoadStdLanderFont (&pSolarSysState->SysInfo.PlanetInfo);
					pSolarSysState->PlanetSideFrame[1] =
							CaptureDrawable (
							LoadGraphic (MOONBASE_MASK_PMAP_ANIM));
					pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
							CaptureStringTable (
							LoadStringTable (MOONBASE_STRTAB));
				}
				pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 60;
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 25;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 0;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 0;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 240 * 29;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = -18;
				break;
			case 4: /* moons of JUPITER */
				pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 520L / 100;
				switch (pSolarSysState->pOrbitalDesc - pSolarSysState->MoonDesc)
				{
					case 0: /* Io */
						pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 69;
						pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 25;
						pSolarSysState->SysInfo.PlanetInfo.Tectonics = 3;
						pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 390;
						pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = -163;
						break;
					case 1: /* Europa */
						pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 54;
						pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 25;
						pSolarSysState->SysInfo.PlanetInfo.Tectonics = 1;
						pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 840;
						pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = -161;
						break;
					case 2: /* Ganymede */
						pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 35;
						pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 41;
						pSolarSysState->SysInfo.PlanetInfo.Tectonics = 0;
						pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 1728;
						pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = -164;
						break;
					case 3: /* Callisto */
						pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 35;
						pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 38;
						pSolarSysState->SysInfo.PlanetInfo.Tectonics = 1;
						pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 4008;
						pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = -167;
						break;
				}
				break;
			case 5: /* moon of SATURN: Titan */
				pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist = EARTH_RADIUS * 952L / 100;
				pSolarSysState->SysInfo.PlanetInfo.AtmoDensity = 160;
				pSolarSysState->SysInfo.PlanetInfo.Weather = 2;
				pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 34;
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 40;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 1;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 3816;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = -178;
				break;
			case 7: /* moon of NEPTUNE: Triton */
				pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist = EARTH_RADIUS * 2999L / 100;
				pSolarSysState->SysInfo.PlanetInfo.AtmoDensity = 10;
				pSolarSysState->SysInfo.PlanetInfo.Weather = 1;
				pSolarSysState->SysInfo.PlanetInfo.PlanetDensity = 95;
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius = 27;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 0;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 4300;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = -216;
				break;
		}

		pSolarSysState->SysInfo.PlanetInfo.SurfaceGravity =
				CalcGravity (pSolarSysState->SysInfo.PlanetInfo.PlanetDensity,
				pSolarSysState->SysInfo.PlanetInfo.PlanetRadius);
		LoadPlanet (NULL);
	}
}

void
GenerateSOL (BYTE control)
{
	COUNT i;
	DWORD rand_val;
	PPLANET_DESC pCurDesc;

	switch (control)
	{
		case INIT_NPCS:
			GLOBAL (BattleGroupRef) =
					GET_GAME_STATE_32 (URQUAN_PROBE_GRPOFFS0);
			if (GLOBAL (BattleGroupRef) == 0)
			{
				CloneShipFragment (URQUAN_PROBE_SHIP,
						&GLOBAL (npc_built_ship_q), 0);

				GLOBAL (BattleGroupRef) = PutGroupInfo (GROUPS_ADD_NEW, 1);
				SET_GAME_STATE_32 (URQUAN_PROBE_GRPOFFS0,
						GLOBAL (BattleGroupRef));
			}
			if (!init_probe ())
				GenerateRandomIP (INIT_NPCS);
			break;
		case REINIT_NPCS:
			if (GET_GAME_STATE (CHMMR_BOMB_STATE) != 3)
				GenerateRandomIP (REINIT_NPCS);
			else
			{
				GLOBAL (BattleGroupRef) = 0;
				ReinitQueue (&GLOBAL (npc_built_ship_q));
			}
			break;
		case GENERATE_ENERGY:
			generate_energy_nodes ();
			break;
		case GENERATE_LIFE:
			generate_tractors ();
			break;
		case GENERATE_ORBITAL:
			generate_orbital ();
			break;
		case GENERATE_NAME:
			i = pSolarSysState->pBaseDesc - pSolarSysState->PlanetDesc;
			utf8StringCopy (GLOBAL_SIS (PlanetName),
					sizeof (GLOBAL_SIS (PlanetName)),
					GAME_STRING (PLANET_NUMBER_BASE + i));
			SET_GAME_STATE (BATTLE_PLANET,
					pSolarSysState->PlanetDesc[i].data_index);
			break;
		case GENERATE_MOONS:
		{
			GenerateRandomIP (GENERATE_MOONS);

			i = pSolarSysState->pBaseDesc - pSolarSysState->PlanetDesc;
			switch (i)
			{
				case 2: /* moons of EARTH */
				{
					COUNT angle;

					/* Starbase: */
					pSolarSysState->MoonDesc[0].data_index = (BYTE)~0;
					pSolarSysState->MoonDesc[0].radius = MIN_MOON_RADIUS;
					angle = HALF_CIRCLE + QUADRANT;
					pSolarSysState->MoonDesc[0].location.x =
							COSINE (angle, pSolarSysState->MoonDesc[0].radius);
					pSolarSysState->MoonDesc[0].location.y =
							SINE (angle, pSolarSysState->MoonDesc[0].radius);

					/* Luna: */
					pSolarSysState->MoonDesc[1].data_index = SELENIC_WORLD;
					pSolarSysState->MoonDesc[1].radius = MIN_MOON_RADIUS
							+ (MAX_MOONS - 1) * MOON_DELTA;
					rand_val = TFB_Random ();
					angle = NORMALIZE_ANGLE (LOWORD (rand_val));
					pSolarSysState->MoonDesc[1].location.x =
							COSINE (angle, pSolarSysState->MoonDesc[1].radius);
					pSolarSysState->MoonDesc[1].location.y =
							SINE (angle, pSolarSysState->MoonDesc[1].radius);
					break;
				}
				case 4: /* moons of JUPITER */
					pSolarSysState->MoonDesc[0].data_index = RADIOACTIVE_WORLD;
							/* Io */
					pSolarSysState->MoonDesc[1].data_index = HALIDE_WORLD;
							/* Europa */
					pSolarSysState->MoonDesc[2].data_index = CYANIC_WORLD;
							/* Ganymede */
					pSolarSysState->MoonDesc[3].data_index = PELLUCID_WORLD;
							/* Callisto */
					break;
				case 5: /* moons of SATURN */
					pSolarSysState->MoonDesc[0].data_index = ALKALI_WORLD;
							/* Titan */
					break;
				case 7: /* moons of NEPTUNE */
					pSolarSysState->MoonDesc[0].data_index = VINYLOGOUS_WORLD;
							/* Triton */
					break;
			}
			break;
		}
		case GENERATE_PLANETS:
		{
#define SOL_SEED 334241042L
			TFB_SeedRandom (SOL_SEED);

			pSolarSysState->SunDesc[0].NumPlanets = 9;
			for (i = 0, pCurDesc = pSolarSysState->PlanetDesc; i < 9; ++i, ++pCurDesc)
			{
				COUNT angle;
				UWORD word_val;

				pCurDesc->rand_seed = rand_val = TFB_Random ();
				word_val = LOWORD (rand_val);
				angle = NORMALIZE_ANGLE ((COUNT)HIBYTE (word_val));

				switch (i)
				{
					case 0: /* MERCURY */
						pCurDesc->data_index = METAL_WORLD;
						pCurDesc->radius = EARTH_RADIUS * 39L / 100;
						pCurDesc->NumPlanets = 0;
						break;
					case 1: /* VENUS */
						pCurDesc->data_index = PRIMORDIAL_WORLD;
						pCurDesc->radius = EARTH_RADIUS * 72L / 100;
						pCurDesc->NumPlanets = 0;
						angle = NORMALIZE_ANGLE (FULL_CIRCLE - angle);
						break;
					case 2: /* EARTH */
						pCurDesc->data_index = WATER_WORLD | PLANET_SHIELDED;
						pCurDesc->radius = EARTH_RADIUS;
						pCurDesc->NumPlanets = 2;
						break;
					case 3: /* MARS */
						pCurDesc->data_index = DUST_WORLD;
						pCurDesc->radius = EARTH_RADIUS * 152L / 100;
						pCurDesc->NumPlanets = 0;
						break;
					case 4: /* JUPITER */
						pCurDesc->data_index = RED_GAS_GIANT;
						pCurDesc->radius = EARTH_RADIUS * 500L /* 520L */ / 100;
						pCurDesc->NumPlanets = 4;
						break;
					case 5: /* SATURN */
						pCurDesc->data_index = ORA_GAS_GIANT;
						pCurDesc->radius = EARTH_RADIUS * 750L /* 952L */ / 100;
						pCurDesc->NumPlanets = 1;
						break;
					case 6: /* URANUS */
						pCurDesc->data_index = GRN_GAS_GIANT;
						pCurDesc->radius = EARTH_RADIUS * 1000L /* 1916L */ / 100;
						pCurDesc->NumPlanets = 0;
						break;
					case 7: /* NEPTUNE */
						pCurDesc->data_index = BLU_GAS_GIANT;
						pCurDesc->radius = EARTH_RADIUS * 1250L /* 2999L */ / 100;
						pCurDesc->NumPlanets = 1;
						break;
					case 8: /* PLUTO */
						pCurDesc->data_index = PELLUCID_WORLD;
						pCurDesc->radius = EARTH_RADIUS * 1550L /* 3937L */ / 100;
						pCurDesc->NumPlanets = 0;
						angle = FULL_CIRCLE - OCTANT;
						break;
				}

				pCurDesc->location.x =
						COSINE (angle, pCurDesc->radius);
				pCurDesc->location.y =
						SINE (angle, pCurDesc->radius);
			}
			break;
		}
		default:
			GenerateRandomIP (control);
			break;
	}
}


