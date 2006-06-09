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
GenerateUtwig (BYTE control)
{
	switch (control)
	{
		case INIT_NPCS:
			if (CurStarDescPtr->Index == BOMB_DEFINED
					&& !GET_GAME_STATE (UTWIG_BOMB))
				ReinitQueue (&GLOBAL (npc_built_ship_q));
			else
				GenerateRandomIP (INIT_NPCS);
			break;
		case GENERATE_PLANETS:
		{
			COUNT angle;

			GenerateRandomIP (GENERATE_PLANETS);
			if (CurStarDescPtr->Index == UTWIG_DEFINED)
			{
				pSolarSysState->PlanetDesc[0].data_index = WATER_WORLD;
				pSolarSysState->PlanetDesc[0].NumPlanets = 1;
				pSolarSysState->PlanetDesc[0].radius = EARTH_RADIUS * 174L / 100;
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
		case GENERATE_ENERGY:
		{
			DWORD rand_val, old_rand;

			if (CurStarDescPtr->Index == UTWIG_DEFINED
					&& pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
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
			else if (CurStarDescPtr->Index == BOMB_DEFINED
					&& pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->PlanetDesc[5]
					&& pSolarSysState->pOrbitalDesc == &pSolarSysState->MoonDesc[1]
					&& !GET_GAME_STATE (UTWIG_BOMB))
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
						SET_GAME_STATE (UTWIG_BOMB, 1);
						SET_GAME_STATE (UTWIG_BOMB_ON_SHIP, 1);
						SET_GAME_STATE (DRUUGE_MANNER, 1);
						SET_GAME_STATE (DRUUGE_VISITS, 0);
						SET_GAME_STATE (DRUUGE_HOME_VISITS, 0);
					}
				}

				TFB_SeedRandom (old_rand);
				break;
			}
			pSolarSysState->CurNode = 0;
			break;
		}
		case GENERATE_ORBITAL:
			if ((CurStarDescPtr->Index == UTWIG_DEFINED
					&& pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
					|| (CurStarDescPtr->Index == BOMB_DEFINED
					&& pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->PlanetDesc[5]
					&& pSolarSysState->pOrbitalDesc == &pSolarSysState->MoonDesc[1]
					&& !GET_GAME_STATE (UTWIG_BOMB)))
			{
				if ((CurStarDescPtr->Index == UTWIG_DEFINED
						|| !GET_GAME_STATE (UTWIG_HAVE_ULTRON))
						&& ActivateStarShip (UTWIG_SHIP, SPHERE_TRACKING))
				{
					NotifyOthers (UTWIG_SHIP, (BYTE)~0);
					PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
					ReinitQueue (&GLOBAL (npc_built_ship_q));

					CloneShipFragment (UTWIG_SHIP,
							&GLOBAL (npc_built_ship_q), INFINITE_FLEET);

					pSolarSysState->MenuState.Initialized += 2;
					GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
					if (CurStarDescPtr->Index == UTWIG_DEFINED)
					{
						SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
					}
					else
					{
						SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 6);
					}
					InitCommunication (UTWIG_CONVERSATION);
					pSolarSysState->MenuState.Initialized -= 2;

					if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
					{
						GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
						ReinitQueue (&GLOBAL (npc_built_ship_q));
						GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);
					}
					break;
				}
				else if (CurStarDescPtr->Index == BOMB_DEFINED
						&& !GET_GAME_STATE (BOMB_UNPROTECTED)
						&& ActivateStarShip (DRUUGE_SHIP, SPHERE_TRACKING))
				{
					COUNT i;

					PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
					ReinitQueue (&GLOBAL (npc_built_ship_q));

					for (i = 0; i < 5; ++i)
						CloneShipFragment (DRUUGE_SHIP,
								&GLOBAL (npc_built_ship_q), 0);
					pSolarSysState->MenuState.Initialized += 2;
					GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
					SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 6);
					InitCommunication (DRUUGE_CONVERSATION);
					pSolarSysState->MenuState.Initialized -= 2;

					if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
						break;
					else
					{
						BOOLEAN DruugeSurvivors;

						DruugeSurvivors = GetHeadLink (
								&GLOBAL (npc_built_ship_q)
								) != 0;

						GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
						ReinitQueue (&GLOBAL (npc_built_ship_q));
						GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);

						if (DruugeSurvivors)
							break;

						LockMutex (GraphicsLock);
						RepairSISBorder ();
						UnlockMutex (GraphicsLock);
						SET_GAME_STATE (BOMB_UNPROTECTED, 1);
					}
				}

				if (CurStarDescPtr->Index == BOMB_DEFINED)
				{
					LoadStdLanderFont (&pSolarSysState->SysInfo.PlanetInfo);
					pSolarSysState->PlanetSideFrame[1] =
							CaptureDrawable (
									LoadGraphic (BOMB_MASK_PMAP_ANIM)
									);
					pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
							CaptureStringTable (
									LoadStringTable (BOMB_STRTAB)
									);
				}
				else
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
			GenerateRandomIP (GENERATE_ORBITAL);
			if (CurStarDescPtr->Index == UTWIG_DEFINED
					&& pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
			{
				pSolarSysState->SysInfo.PlanetInfo.Weather = 1;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 1;
			}
			break;
		default:
			GenerateRandomIP (control);
			break;
	}
}


