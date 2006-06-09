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
#include "lander.h"
#include "nameref.h"
#include "resinst.h"
#include "setup.h"
#include "state.h"
#include "sounds.h"
#include "planets/genall.h"
#include "libs/mathlib.h"


static void
GenerateAndrosynth (BYTE control)
{
	switch (control)
	{
		case GENERATE_ENERGY:
		{
			DWORD rand_val, old_rand;

			if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[1])
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
					pSolarSysState->SysInfo.PlanetInfo.CurType = 0;
					pSolarSysState->SysInfo.PlanetInfo.CurDensity = 0;

					if (pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
							& (1L << i))
					{
						pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
								&= ~(1L << i);
							/* so won't get freed in lander.c */
						CaptureStringTable (GetStringTable (
								pSolarSysState->SysInfo.PlanetInfo.DiscoveryString
								));
						if (!(pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
								& (1L << (i + 16))))
						{
							SET_GAME_STATE (PLANETARY_CHANGE, 1);

							pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
									|= (1L << (i + 16));
							if (pSolarSysState->SysInfo.PlanetInfo.DiscoveryString)
							{
								PPLANETSIDE_DESC pPSD;

								pPSD = (PPLANETSIDE_DESC)pMenuState->ModuleFrame;
								UnbatchGraphics ();
								DoDiscoveryReport (MenuSounds);
								BatchGraphics ();
								pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
										SetRelStringTableIndex (
										pSolarSysState->SysInfo.PlanetInfo.DiscoveryString,
										1
										);
								if (GetStringTableIndex (
										pSolarSysState->SysInfo.PlanetInfo.DiscoveryString
										) == 0)
										/* so will get freed in lander.c */
									ReleaseStringTable (
											pSolarSysState->SysInfo.PlanetInfo.DiscoveryString
											);
							}
						}
					}

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
			pSolarSysState->CurNode = 0;
			break;
		}
		case GENERATE_PLANETS:
		{
			COUNT angle;

			GenerateRandomIP (GENERATE_PLANETS);
			pSolarSysState->PlanetDesc[1].data_index = TELLURIC_WORLD;
			pSolarSysState->PlanetDesc[1].radius = EARTH_RADIUS * 204L / 100;
			angle = ARCTAN (
					pSolarSysState->PlanetDesc[1].location.x,
					pSolarSysState->PlanetDesc[1].location.y
					);
			pSolarSysState->PlanetDesc[1].location.x =
					COSINE (angle, pSolarSysState->PlanetDesc[1].radius);
			pSolarSysState->PlanetDesc[1].location.y =
					SINE (angle, pSolarSysState->PlanetDesc[1].radius);
			break;
		}
		case GENERATE_ORBITAL:
			if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[1])
			{
				UWORD retval;

				LoadStdLanderFont (&pSolarSysState->SysInfo.PlanetInfo);
				pSolarSysState->PlanetSideFrame[1] =
						CaptureDrawable (
						LoadGraphic (RUINS_MASK_PMAP_ANIM)
						);
				pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
						CaptureStringTable (
								LoadStringTable (ANDROSYNTH_RUINS_STRTAB)
								);
				retval = HIWORD (
						pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[ENERGY_SCAN]
						);
				while (retval)
				{
					if (retval & 1)
					{
						pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
								SetRelStringTableIndex (
								pSolarSysState->SysInfo.PlanetInfo.DiscoveryString,
								1
								);
						if (GetStringTableIndex (
								pSolarSysState->SysInfo.PlanetInfo.DiscoveryString
								) == 0)
						{
							DestroyStringTable (ReleaseStringTable (
									pSolarSysState->SysInfo.PlanetInfo.DiscoveryString
									));
							pSolarSysState->SysInfo.PlanetInfo.DiscoveryString = 0;
						}
					}

					retval >>= 1;
				}
			}
			GenerateRandomIP (GENERATE_ORBITAL);
			if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[1])
			{
				pSolarSysState->SysInfo.PlanetInfo.AtmoDensity =
						EARTH_ATMOSPHERE * 144 / 100;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = 28;
				pSolarSysState->SysInfo.PlanetInfo.Weather = 1;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 1;
			}
			break;
		default:
			GenerateRandomIP (control);
			break;
	}
}

void
GenerateOrz (BYTE control)
{
	if (CurStarDescPtr->Index == ANDROSYNTH_DEFINED)
	{
		GenerateAndrosynth (control);
		return;
	}

	switch (control)
	{
		case GENERATE_ENERGY:
		{
			DWORD rand_val, old_rand;

			if (CurStarDescPtr->Index != ORZ_DEFINED
					&& pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->PlanetDesc[1]
					&& pSolarSysState->pOrbitalDesc == &pSolarSysState->MoonDesc[2]
					&& !GET_GAME_STATE (TAALO_PROTECTOR))
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
						SET_GAME_STATE (TAALO_PROTECTOR, 1);
						SET_GAME_STATE (TAALO_PROTECTOR_ON_SHIP, 1);
					}
				}

				TFB_SeedRandom (old_rand);
				break;
			}
			else if (CurStarDescPtr->Index == ORZ_DEFINED
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
			pSolarSysState->CurNode = 0;
			break;
		}
		case GENERATE_PLANETS:
		{
			COUNT angle;

			GenerateRandomIP (GENERATE_PLANETS);
			if (CurStarDescPtr->Index == ORZ_DEFINED)
			{
				pSolarSysState->PlanetDesc[0].data_index = WATER_WORLD;
				pSolarSysState->PlanetDesc[0].radius = EARTH_RADIUS * 156L / 100;
				pSolarSysState->PlanetDesc[0].NumPlanets = 0;
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
			if ((CurStarDescPtr->Index == ORZ_DEFINED
					&& pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
					|| (CurStarDescPtr->Index == TAALO_PROTECTOR_DEFINED
					&& pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->PlanetDesc[1]
					&& pSolarSysState->pOrbitalDesc == &pSolarSysState->MoonDesc[2]
					&& !GET_GAME_STATE (TAALO_PROTECTOR)))
			{
				COUNT i;

				if ((CurStarDescPtr->Index == ORZ_DEFINED
						|| !GET_GAME_STATE (TAALO_UNPROTECTED))
						&& ActivateStarShip (ORZ_SHIP, SPHERE_TRACKING))
				{
					NotifyOthers (ORZ_SHIP, (BYTE)~0);
					PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
					ReinitQueue (&GLOBAL (npc_built_ship_q));

					if (CurStarDescPtr->Index == ORZ_DEFINED)
					{
						CloneShipFragment (ORZ_SHIP,
								&GLOBAL (npc_built_ship_q), INFINITE_FLEET);
						SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
					}
					else
					{
						for (i = 0; i < 14; ++i)
							CloneShipFragment (ORZ_SHIP,
									&GLOBAL (npc_built_ship_q), 0);
						SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 6);
					}
					pSolarSysState->MenuState.Initialized += 2;
					GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
					InitCommunication (ORZ_CONVERSATION);
					pSolarSysState->MenuState.Initialized -= 2;

					if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
						break;
					else
					{
						BOOLEAN OrzSurvivors;

						OrzSurvivors = GetHeadLink (&GLOBAL (npc_built_ship_q))
								&& (CurStarDescPtr->Index == ORZ_DEFINED
								|| !GET_GAME_STATE (TAALO_UNPROTECTED));

						GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
						ReinitQueue (&GLOBAL (npc_built_ship_q));
						GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);

						if (OrzSurvivors)
							break;

						LockMutex (GraphicsLock);
						RepairSISBorder ();
						UnlockMutex (GraphicsLock);
					}
				}

				SET_GAME_STATE (TAALO_UNPROTECTED, 1);
				if (CurStarDescPtr->Index == TAALO_PROTECTOR_DEFINED)
				{
					LoadStdLanderFont (&pSolarSysState->SysInfo.PlanetInfo);
					pSolarSysState->PlanetSideFrame[1] =
							CaptureDrawable (
									LoadGraphic (TAALO_DEVICE_MASK_PMAP_ANIM)
									);
					pSolarSysState->SysInfo.PlanetInfo.DiscoveryString =
							CaptureStringTable (
									LoadStringTable (TAALO_DEVICE_STRTAB)
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
		default:
			GenerateRandomIP (control);
			break;
	}
}


