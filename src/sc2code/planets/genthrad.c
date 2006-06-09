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
#include "encount.h"
#include "globdata.h"
#include "nameref.h"
#include "resinst.h"
#include "setup.h"
#include "state.h"
#include "planets/genall.h"
#include "libs/mathlib.h"


void
GenerateThradd (BYTE control)
{
	switch (control)
	{
		case GENERATE_ENERGY:
			if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
			{
				DWORD rand_val, old_rand;

				if (CurStarDescPtr->Index != AQUA_HELIX_DEFINED)
				{
					COUNT i, which_node;

					old_rand = TFB_SeedRandom (
							pSolarSysState->SysInfo.PlanetInfo.ScanSeed[ENERGY_SCAN]
							);

					which_node = i = 0;
					do
					{
						rand_val = TFB_Random ();
						pSolarSysState->SysInfo.PlanetInfo.CurPt.x =
								(LOBYTE (LOWORD (rand_val)) % (MAP_WIDTH - (8 << 1))) + 8;
						pSolarSysState->SysInfo.PlanetInfo.CurPt.y =
								(HIBYTE (LOWORD (rand_val)) % (MAP_HEIGHT - (8 << 1))) + 8;
						pSolarSysState->SysInfo.PlanetInfo.CurType = 1;
						pSolarSysState->SysInfo.PlanetInfo.CurDensity = 0;
						if (which_node >= pSolarSysState->CurNode
								&& !(pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
								& (1L << i)))
							break;
						++which_node;
					} while (++i < 16);
					pSolarSysState->CurNode = which_node;

					TFB_SeedRandom (old_rand);
					break;
				}
				else if (!GET_GAME_STATE (AQUA_HELIX))
				{
					old_rand = TFB_SeedRandom (
							pSolarSysState->SysInfo.PlanetInfo.ScanSeed[ENERGY_SCAN]
							);

					rand_val = TFB_Random ();
					pSolarSysState->SysInfo.PlanetInfo.CurPt.x =
							(LOBYTE (LOWORD (rand_val)) % (MAP_WIDTH - (8 << 1))) + 8;
					pSolarSysState->SysInfo.PlanetInfo.CurPt.y =
							(HIBYTE (LOWORD (rand_val)) % (MAP_HEIGHT - (8 << 1))) + 8;
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
							SET_GAME_STATE (HELIX_VISITS, 0);
							SET_GAME_STATE (AQUA_HELIX, 1);
							SET_GAME_STATE (AQUA_HELIX_ON_SHIP, 1);
							SET_GAME_STATE (HELIX_UNPROTECTED, 1);
						}
					}

					TFB_SeedRandom (old_rand);
					break;
				}
			}
			pSolarSysState->CurNode = 0;
			break;
		case GENERATE_PLANETS:
		{
			COUNT angle;

			GenerateRandomIP (GENERATE_PLANETS);
			if (CurStarDescPtr->Index == AQUA_HELIX_DEFINED)
			{
				pSolarSysState->PlanetDesc[0].data_index = PRIMORDIAL_WORLD;
				pSolarSysState->PlanetDesc[0].radius = EARTH_RADIUS * 65L / 100;
				angle = ARCTAN (
						pSolarSysState->PlanetDesc[0].location.x,
						pSolarSysState->PlanetDesc[0].location.y
						);
				pSolarSysState->PlanetDesc[0].location.x =
						COSINE (angle, pSolarSysState->PlanetDesc[0].radius);
				pSolarSysState->PlanetDesc[0].location.y =
						SINE (angle, pSolarSysState->PlanetDesc[0].radius);
			}
			else
			{
				pSolarSysState->PlanetDesc[0].data_index = WATER_WORLD;
				pSolarSysState->PlanetDesc[0].NumPlanets = 0;
				pSolarSysState->PlanetDesc[0].radius = EARTH_RADIUS * 98L / 100;
				angle = ARCTAN (
						pSolarSysState->PlanetDesc[0].location.x,
						pSolarSysState->PlanetDesc[0].location.y
						);
				pSolarSysState->PlanetDesc[0].location.x =
						COSINE (angle, pSolarSysState->PlanetDesc[0].radius);
				pSolarSysState->PlanetDesc[0].location.y =
						SINE (angle, pSolarSysState->PlanetDesc[0].radius);
			}
			break;
		}
		case GENERATE_ORBITAL:
			if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
			{
				if (ActivateStarShip (THRADDASH_SHIP, SPHERE_TRACKING)
						&& (CurStarDescPtr->Index == THRADD_DEFINED
						|| (!GET_GAME_STATE (HELIX_UNPROTECTED)
						&& (BYTE)(GET_GAME_STATE (THRADD_MISSION) - 1) >= 3)))
				{
					NotifyOthers (THRADDASH_SHIP, (BYTE)~0);
					PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
					ReinitQueue (&GLOBAL (npc_built_ship_q));

					CloneShipFragment (THRADDASH_SHIP,
							&GLOBAL (npc_built_ship_q), INFINITE_FLEET);

					pSolarSysState->MenuState.Initialized += 2;
					GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
					if (CurStarDescPtr->Index == THRADD_DEFINED)
					{
						SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
					}
					else
					{
						SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 6);
					}
					InitCommunication (THRADD_CONVERSATION);
					pSolarSysState->MenuState.Initialized -= 2;

					if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
						break;

					GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
					ReinitQueue (&GLOBAL (npc_built_ship_q));
					GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);

					if (CurStarDescPtr->Index == THRADD_DEFINED
							|| (!GET_GAME_STATE (HELIX_UNPROTECTED)
							&& (BYTE)(GET_GAME_STATE (THRADD_MISSION) - 1) >= 3))
						break;
					LockMutex (GraphicsLock);
					RepairSISBorder ();
					UnlockMutex (GraphicsLock);
				}

				if (CurStarDescPtr->Index == AQUA_HELIX_DEFINED
						&& !GET_GAME_STATE (AQUA_HELIX))
				{
					LoadStdLanderFont (&pSolarSysState->SysInfo.PlanetInfo);
					pSolarSysState->PlanetSideFrame[1] =
							CaptureDrawable (
									LoadGraphic (AQUA_MASK_PMAP_ANIM)
									);
					pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
							CaptureStringTable (
									LoadStringTable (AQUA_STRTAB)
									);
				}
				else if (CurStarDescPtr->Index == THRADD_DEFINED)
				{
					LoadStdLanderFont (&pSolarSysState->SysInfo.PlanetInfo);
					pSolarSysState->PlanetSideFrame[1] =
							CaptureDrawable (
							LoadGraphic (RUINS_MASK_PMAP_ANIM)
							);
					pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
							CaptureStringTable (
									LoadStringTable (RUINS_STRTAB)
									);
				}
			}
		default:
			GenerateRandomIP (control);
			break;
	}
}

