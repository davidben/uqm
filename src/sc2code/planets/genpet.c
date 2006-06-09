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


static void
ZapToUrquanEncounter (void)
{
	HENCOUNTER hEncounter;

	if ((hEncounter = AllocEncounter ()) || (hEncounter = GetHeadEncounter ()))
	{
		SIZE dx, dy;
		ENCOUNTERPTR EncounterPtr;
		HSTARSHIP hStarShip;
		EXTENDED_SHIP_FRAGMENTPTR TemplatePtr;

		LockEncounter (hEncounter, &EncounterPtr);

		if (hEncounter == GetHeadEncounter ())
			RemoveEncounter (hEncounter);
		memset (EncounterPtr, 0, sizeof (*EncounterPtr));

		InsertEncounter (hEncounter, GetHeadEncounter ());

		hStarShip = GetStarShipFromIndex (&GLOBAL (avail_race_q), URQUAN_SHIP);
		TemplatePtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (avail_race_q),
				hStarShip
				);
		EncounterPtr->origin = TemplatePtr->ShipInfo.loc;
		EncounterPtr->radius = TemplatePtr->ShipInfo.actual_strength;
		EncounterPtr->SD.Type = URQUAN_SHIP;
		EncounterPtr->SD.Index = MAKE_BYTE (1, 0) | ONE_SHOT_ENCOUNTER;
		EncounterPtr->SD.ShipList[0] =
				((SHIP_FRAGMENTPTR)TemplatePtr)->ShipInfo;
		EncounterPtr->SD.ShipList[0].var1 = URQUAN_SHIP;
		EncounterPtr->SD.star_pt.x = 5288;
		EncounterPtr->SD.star_pt.y = 4892;
		EncounterPtr->log_x = UNIVERSE_TO_LOGX (EncounterPtr->SD.star_pt.x);
		EncounterPtr->log_y = UNIVERSE_TO_LOGY (EncounterPtr->SD.star_pt.y);
		GLOBAL_SIS (log_x) = EncounterPtr->log_x;
		GLOBAL_SIS (log_y) = EncounterPtr->log_y;
		UnlockStarShip (
				&GLOBAL (avail_race_q),
				hStarShip
				);

		{
#define LOST_DAYS 15
			COUNT i;
			BYTE black_buf[] = {FadeAllToBlack};

			SleepThreadUntil (XFormColorMap ((COLORMAPPTR)black_buf, ONE_SECOND * 2));
			for (i = 0; i < LOST_DAYS; ++i)
			{
				while (ClockTick () > 0)
					;

				ResumeGameClock ();
				SleepThread (ONE_SECOND / 60);
				SuspendGameClock ();
			}
		}

		GLOBAL (CurrentActivity) = MAKE_WORD (IN_HYPERSPACE, 0) | START_ENCOUNTER;

		dx = CurStarDescPtr->star_pt.x - EncounterPtr->SD.star_pt.x;
		dy = CurStarDescPtr->star_pt.y - EncounterPtr->SD.star_pt.y;
		dx = (SIZE)square_root ((long)dx * dx + (long)dy * dy)
				+ (FUEL_TANK_SCALE >> 1);

		LockMutex (GraphicsLock);
		DeltaSISGauges (0, -dx, 0);
		if (GLOBAL_SIS (FuelOnBoard) < 5 * FUEL_TANK_SCALE)
		{
			dx = ((5 + ((COUNT)TFB_Random () % 5)) * FUEL_TANK_SCALE)
					- (SIZE)GLOBAL_SIS (FuelOnBoard);
			DeltaSISGauges (0, dx, 0);
		}
		DrawSISMessage (NULL_PTR);
		DrawHyperCoords (EncounterPtr->SD.star_pt);
		UnlockMutex (GraphicsLock);

		UnlockEncounter (hEncounter);
	}
}

void
GenerateTalkingPet (BYTE control)
{
	switch (control)
	{
		case GENERATE_ENERGY:
			if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
			{
				COUNT i, which_node;
				DWORD rand_val, old_rand;

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
		case GENERATE_PLANETS:
		{
			COUNT angle;

			GenerateRandomIP (GENERATE_PLANETS);
			pSolarSysState->PlanetDesc[0].data_index = TELLURIC_WORLD;
			pSolarSysState->PlanetDesc[0].radius = EARTH_RADIUS * 204L / 100;
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
			if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0]
					&& (GET_GAME_STATE (UMGAH_ZOMBIE_BLOBBIES)
					|| !GET_GAME_STATE (TALKING_PET)
					|| ActivateStarShip (UMGAH_SHIP, SPHERE_TRACKING)))
			{
				NotifyOthers (UMGAH_SHIP, (BYTE)~0);
				PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
				ReinitQueue (&GLOBAL (npc_built_ship_q));

				if (ActivateStarShip (UMGAH_SHIP, SPHERE_TRACKING))
				{
					pSolarSysState->MenuState.Initialized += 2;
					GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
					SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
					if (!GET_GAME_STATE (UMGAH_ZOMBIE_BLOBBIES))
					{
						CloneShipFragment (UMGAH_SHIP,
								&GLOBAL (npc_built_ship_q), INFINITE_FLEET);
						InitCommunication (UMGAH_CONVERSATION);
					}
					else
					{
						COUNT i;

						for (i = 0; i < 10; ++i)
							CloneShipFragment (UMGAH_SHIP,
									&GLOBAL (npc_built_ship_q), 0);
						InitCommunication (TALKING_PET_CONVERSATION);
					}
					pSolarSysState->MenuState.Initialized -= 2;
				}
		
				if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
				{
					BOOLEAN UmgahSurvivors;

					UmgahSurvivors = GetHeadLink (
							&GLOBAL (npc_built_ship_q)
							) != 0;
					GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;

					if (GET_GAME_STATE (PLAYER_HYPNOTIZED))
						ZapToUrquanEncounter ();
					else if (GET_GAME_STATE (UMGAH_ZOMBIE_BLOBBIES)
							&& !UmgahSurvivors)
					{
						// Defeated the zombie fleet.
						InitCommunication (TALKING_PET_CONVERSATION);
					}
					else if (!(ActivateStarShip (UMGAH_SHIP, SPHERE_TRACKING)))
					{
						// The Kohr-Ah have destroyed the Umgah, but the
						// talking pet survived.
						InitCommunication (TALKING_PET_CONVERSATION);
					}

					ReinitQueue (&GLOBAL (npc_built_ship_q));
					GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);
				}
				break;
			}

			if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
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

			GenerateRandomIP (GENERATE_ORBITAL);
			if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
				pSolarSysState->SysInfo.PlanetInfo.Weather = 0;
			break;
		default:
			GenerateRandomIP (control);
			break;
	}
}

