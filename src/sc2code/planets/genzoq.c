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

#include "starcon.h"

static void
check_scout (void)
{
	HSTARSHIP hStarShip;

	if (GLOBAL (BattleGroupRef)
			&& (hStarShip = GetHeadLink (
					&GLOBAL (npc_built_ship_q)
					)))
	{
		BYTE task;
		SHIP_FRAGMENTPTR FragPtr;

		FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (npc_built_ship_q), hStarShip
				);
		task = GET_GROUP_MISSION (FragPtr);

		if (task & REFORM_GROUP)
		{
			SET_GROUP_MISSION (FragPtr,
					FLEE | IGNORE_FLAGSHIP | REFORM_GROUP);
			SET_GROUP_DEST (FragPtr, 0);
		}

		UnlockStarShip (
				&GLOBAL (npc_built_ship_q), hStarShip
				);
	}
}

static void
GenerateScout (BYTE control)
{
	switch (control)
	{
		case INIT_NPCS:
			if (!GET_GAME_STATE (MET_ZOQFOT))
			{
				BYTE b0, b1, b2, b3;

				b0 = GET_GAME_STATE (ZOQFOT_GRPOFFS0);
				b1 = GET_GAME_STATE (ZOQFOT_GRPOFFS1);
				b2 = GET_GAME_STATE (ZOQFOT_GRPOFFS2);
				b3 = GET_GAME_STATE (ZOQFOT_GRPOFFS3);
				GLOBAL (BattleGroupRef) = MAKE_DWORD (
							MAKE_WORD (b0, b1), MAKE_WORD (b2, b3)
							);
				if (GLOBAL (BattleGroupRef) == 0)
				{
						CloneShipFragment (ZOQFOTPIK_SHIP,
								&GLOBAL (npc_built_ship_q), 0);

						GLOBAL (BattleGroupRef) = PutGroupInfo (~0L, 1);
						b0 = LOBYTE (LOWORD (GLOBAL (BattleGroupRef)));
						b1 = HIBYTE (LOWORD (GLOBAL (BattleGroupRef)));
						b2 = LOBYTE (HIWORD (GLOBAL (BattleGroupRef)));
						b3 = HIBYTE (HIWORD (GLOBAL (BattleGroupRef)));
						SET_GAME_STATE (ZOQFOT_GRPOFFS0, b0);
						SET_GAME_STATE (ZOQFOT_GRPOFFS1, b1);
						SET_GAME_STATE (ZOQFOT_GRPOFFS2, b2);
						SET_GAME_STATE (ZOQFOT_GRPOFFS3, b3);
				}
			}
			GenerateRandomIP (INIT_NPCS);
			break;
		case REINIT_NPCS:
			GenerateRandomIP (REINIT_NPCS);
			check_scout ();
			break;
		default:
			GenerateRandomIP (control);
			break;
	}
}

void
GenerateZoqFotPik (BYTE control)
{
	if (CurStarDescPtr->Index == ZOQ_SCOUT_DEFINED)
	{
		GenerateScout (control);
		return;
	}

	switch (control)
	{
		case INIT_NPCS:
			if (GET_GAME_STATE (ZOQFOT_DISTRESS) != 1)
				GenerateRandomIP (INIT_NPCS);
			break;
		case GENERATE_ENERGY:
			if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
			{
				COUNT i, which_node;
				DWORD rand_val, old_rand;

				old_rand = SeedRandom (
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

				SeedRandom (old_rand);
				break;
			}
			pSolarSysState->CurNode = 0;
			break;
		case GENERATE_PLANETS:
		{
			COUNT angle;

			GenerateRandomIP (GENERATE_PLANETS);
			pSolarSysState->PlanetDesc[0].data_index = REDUX_WORLD;
			pSolarSysState->PlanetDesc[0].NumPlanets = 1;
			pSolarSysState->PlanetDesc[0].radius = EARTH_RADIUS * 138L / 100;
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
				if (ActivateStarShip (ZOQFOTPIK_SHIP, SPHERE_TRACKING))
				{
					PutGroupInfo (0L, (BYTE)~0);
					ReinitQueue (&GLOBAL (npc_built_ship_q));

					if (GET_GAME_STATE (ZOQFOT_DISTRESS))
					{
						CloneShipFragment (BLACK_URQUAN_SHIP,
								&GLOBAL (npc_built_ship_q), 0);

						pSolarSysState->MenuState.Initialized += 2;
						GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
						SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
						InitCommunication (BLACKURQ_CONVERSATION);
						pSolarSysState->MenuState.Initialized -= 2;

						if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
							break;

						if (GetHeadLink (&GLOBAL (npc_built_ship_q)))
						{
							GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
							ReinitQueue (&GLOBAL (npc_built_ship_q));
							GetGroupInfo (0L, 0);

							break;
						}
					}

					CloneShipFragment (ZOQFOTPIK_SHIP,
							&GLOBAL (npc_built_ship_q), (BYTE)~0);

					pSolarSysState->MenuState.Initialized += 2;
					GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
					SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
					InitCommunication (ZOQFOTPIK_CONVERSATION);
					pSolarSysState->MenuState.Initialized -= 2;
					if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
					{
						GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
						ReinitQueue (&GLOBAL (npc_built_ship_q));
						GetGroupInfo (0L, 0);
					}
					break;
				}
				else
				{
					pSolarSysState->SysInfo.PlanetInfo.LanderFont =
							CaptureFont (
									LoadGraphic (LANDER_FONT)
									);
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

