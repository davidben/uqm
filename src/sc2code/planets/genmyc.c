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
GenerateMycon (BYTE control)
{
	switch (control)
	{
		case GENERATE_LIFE:
			pSolarSysState->CurNode = 0;
			break;
		case GENERATE_ENERGY:
			if (CurStarDescPtr->Index != MYCON_DEFINED
					&& pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
			{
				DWORD rand_val, old_rand;

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
				if (CurStarDescPtr->Index == SUN_DEVICE_DEFINED)
				{
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
							SET_GAME_STATE (SUN_DEVICE, 1);
							SET_GAME_STATE (SUN_DEVICE_ON_SHIP, 1);
							SET_GAME_STATE (MYCON_VISITS, 0);
						}
					}
				}
				else
				{
					if (!(pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
							& (1L << 0))
							&& pSolarSysState->CurNode == (COUNT)~0)
						pSolarSysState->CurNode = 1;
					else
					{
						pSolarSysState->CurNode = 0;
						if ((pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
								& (1L << 0))
								&& pSolarSysState->SysInfo.PlanetInfo.DiscoveryString)
						{
							switch (CurStarDescPtr->Index)
							{
								case EGG_CASE0_DEFINED:
									SET_GAME_STATE (EGG_CASE0_ON_SHIP, 1);
									break;
								case EGG_CASE1_DEFINED:
									SET_GAME_STATE (EGG_CASE1_ON_SHIP, 1);
									break;
								case EGG_CASE2_DEFINED:
									SET_GAME_STATE (EGG_CASE2_ON_SHIP, 1);
									break;
							}
						}
					}
				}

				TFB_SeedRandom (old_rand);
				break;
			}
			pSolarSysState->CurNode = 0;
			break;
		case GENERATE_PLANETS:
		{
			COUNT angle;

			GenerateRandomIP (GENERATE_PLANETS);
			pSolarSysState->PlanetDesc[0].data_index = CRACKED_WORLD;
			pSolarSysState->PlanetDesc[0].radius = EARTH_RADIUS * 80L / 100;
			if (pSolarSysState->PlanetDesc[0].NumPlanets > 2)
				pSolarSysState->PlanetDesc[0].NumPlanets = 2;
			angle = ARCTAN (
					pSolarSysState->PlanetDesc[0].location.x,
					pSolarSysState->PlanetDesc[0].location.y
					);
			pSolarSysState->PlanetDesc[0].location.x =
					COSINE (angle, pSolarSysState->PlanetDesc[0].radius);
			pSolarSysState->PlanetDesc[0].location.y =
					SINE (angle, pSolarSysState->PlanetDesc[0].radius);
			break;
		}
		case GENERATE_ORBITAL:
			if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
			{
				if ((CurStarDescPtr->Index == MYCON_DEFINED
						|| CurStarDescPtr->Index == SUN_DEVICE_DEFINED)
						&& ActivateStarShip (MYCON_SHIP, SPHERE_TRACKING))
				{
					if (CurStarDescPtr->Index == MYCON_DEFINED
							|| !GET_GAME_STATE (SUN_DEVICE_UNGUARDED))
					{
						NotifyOthers (MYCON_SHIP, (BYTE)~0);
						PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
						ReinitQueue (&GLOBAL (npc_built_ship_q));

						if (CurStarDescPtr->Index == MYCON_DEFINED
								|| !GET_GAME_STATE (MYCON_FELL_FOR_AMBUSH))
							CloneShipFragment (MYCON_SHIP,
									&GLOBAL (npc_built_ship_q),
									INFINITE_FLEET);
						else
						{
							COUNT i;

							for (i = 0; i < 5; ++i)
								CloneShipFragment (MYCON_SHIP,
										&GLOBAL (npc_built_ship_q), 0);
						}

						pSolarSysState->MenuState.Initialized += 2;
						GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
						if (CurStarDescPtr->Index == MYCON_DEFINED)
						{
							SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
						}
						else
						{
							SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 6);
						}
						InitCommunication (MYCON_CONVERSATION);
						pSolarSysState->MenuState.Initialized -= 2;

						if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
							break;

						{
							BOOLEAN MyconSurvivors;

							MyconSurvivors = GetHeadLink (
									&GLOBAL (npc_built_ship_q)
									) != 0;

							GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
							ReinitQueue (&GLOBAL (npc_built_ship_q));
							GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);

							if (MyconSurvivors)
								break;

							SET_GAME_STATE (SUN_DEVICE_UNGUARDED, 1);
							LockMutex (GraphicsLock);
							RepairSISBorder ();
							UnlockMutex (GraphicsLock);
						}
					}
				}

				switch (CurStarDescPtr->Index)
				{
					case SUN_DEVICE_DEFINED:
						if (!GET_GAME_STATE (SUN_DEVICE))
						{
							LoadStdLanderFont (&pSolarSysState->SysInfo.PlanetInfo);
							pSolarSysState->PlanetSideFrame[1] =
									CaptureDrawable (
											LoadGraphic (SUN_DEVICE_MASK_PMAP_ANIM)
											);
							pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
									CaptureStringTable (
											LoadStringTable (SUN_DEVICE_STRTAB)
											);
						}
						break;
					case EGG_CASE0_DEFINED:
					case EGG_CASE1_DEFINED:
					case EGG_CASE2_DEFINED:
						if (GET_GAME_STATE (KNOW_ABOUT_SHATTERED) == 0)
						{
							SET_GAME_STATE (KNOW_ABOUT_SHATTERED, 1);
						}
						if (!(pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
								& (1L << 0)))
						{
							LoadStdLanderFont (&pSolarSysState->SysInfo.PlanetInfo);
							pSolarSysState->PlanetSideFrame[1] =
									CaptureDrawable (
											LoadGraphic (EGG_CASE_MASK_PMAP_ANIM)
											);
							pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
									CaptureStringTable (
											LoadStringTable (EGG_CASE_STRTAB)
											);
						}
						break;
				}
			}
		default:
			GenerateRandomIP (control);
			break;
	}
}

