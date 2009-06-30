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
#include "resinst.h"
#include "races.h"
#include "state.h"
#include "grpinfo.h"
#include "planets/genall.h"
#include "libs/mathlib.h"


static void
BuildUrquanGuard (void)
{
	BYTE ship1, ship2;
	BYTE b0, b1;
	POINT org;
	HIPGROUP hGroup, hNextGroup;

	GLOBAL (BattleGroupRef) = GET_GAME_STATE_32 (SAMATRA_GRPOFFS0);

	if (!GET_GAME_STATE (KOHR_AH_FRENZY))
		ship1 = URQUAN_SHIP, ship2 = BLACK_URQUAN_SHIP;
	else
		ship1 = BLACK_URQUAN_SHIP, ship2 = URQUAN_SHIP;

	assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

	for (b0 = 0; b0 < MAX_SHIPS_PER_SIDE; ++b0)
		CloneShipFragment (ship1,
					&GLOBAL (npc_built_ship_q), 0);

	if (GLOBAL (BattleGroupRef) == 0)
	{
		GLOBAL (BattleGroupRef) = PutGroupInfo (GROUPS_ADD_NEW, 1);
		SET_GAME_STATE_32 (SAMATRA_GRPOFFS0, GLOBAL (BattleGroupRef));
	}

#define NUM_URQUAN_GUARDS0 12
	for (b0 = 1; b0 <= NUM_URQUAN_GUARDS0; ++b0)
		PutGroupInfo (GLOBAL (BattleGroupRef), b0);

	ReinitQueue (&GLOBAL (npc_built_ship_q));
	for (b0 = 0; b0 < MAX_SHIPS_PER_SIDE; ++b0)
		CloneShipFragment (ship2,
					&GLOBAL (npc_built_ship_q), 0);
#define NUM_URQUAN_GUARDS1 4
	for (b0 = 1; b0 <= NUM_URQUAN_GUARDS1; ++b0)
		PutGroupInfo (GLOBAL (BattleGroupRef), (BYTE)(NUM_URQUAN_GUARDS0 + b0));

	ReinitQueue (&GLOBAL (npc_built_ship_q));

	GetGroupInfo (GLOBAL (BattleGroupRef), GROUP_INIT_IP);

	XFormIPLoc (&pSolarSysState->PlanetDesc[4].image.origin,
			&org, FALSE);
	hGroup = GetHeadLink (&GLOBAL (ip_group_q));
	for (b0 = 0, b1 = 0;
			b0 < NUM_URQUAN_GUARDS0;
			++b0, b1 += FULL_CIRCLE / (NUM_URQUAN_GUARDS0 + NUM_URQUAN_GUARDS1))
	{
		IP_GROUP *GroupPtr;

		if (b1 % (FULL_CIRCLE / NUM_URQUAN_GUARDS1) == 0)
			b1 += FULL_CIRCLE / (NUM_URQUAN_GUARDS0 + NUM_URQUAN_GUARDS1);

		GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
		hNextGroup = _GetSuccLink (GroupPtr);
		GroupPtr->task = ON_STATION | IGNORE_FLAGSHIP;
		GroupPtr->sys_loc = 0;
		GroupPtr->dest_loc = 4 + 1;
		GroupPtr->orbit_pos = NORMALIZE_FACING (ANGLE_TO_FACING (b1));
		GroupPtr->group_counter = 0;
		GroupPtr->loc.x = org.x + COSINE (b1, STATION_RADIUS);
		GroupPtr->loc.y = org.y + SINE (b1, STATION_RADIUS);
		UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);
		hGroup = hNextGroup;
	}

	for (b0 = 0, b1 = 0;
			b0 < NUM_URQUAN_GUARDS1;
			++b0, b1 += FULL_CIRCLE / NUM_URQUAN_GUARDS1)
	{
		IP_GROUP *GroupPtr;

		GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
		hNextGroup = _GetSuccLink (GroupPtr);
		GroupPtr->task = ON_STATION | IGNORE_FLAGSHIP;
		GroupPtr->sys_loc = 0;
		GroupPtr->dest_loc = 4 + 1;
		GroupPtr->orbit_pos = NORMALIZE_FACING (ANGLE_TO_FACING (b1));
		GroupPtr->group_counter = 0;
		GroupPtr->loc.x = org.x + COSINE (b1, STATION_RADIUS);
		GroupPtr->loc.y = org.y + SINE (b1, STATION_RADIUS);
		UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);
		hGroup = hNextGroup;
	}
}

void
GenerateSamatra (BYTE control)
{
	switch (control)
	{
		case INIT_NPCS:
			if (!GET_GAME_STATE (URQUAN_MESSED_UP))
				BuildUrquanGuard ();
			else
			{	// Exorcise Ur-Quan ghosts upon system reentry
				PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP); // wipe out the group
			}
			break;
		case REINIT_NPCS:
			GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);
			EncounterGroup = 0;
			EncounterRace = -1; // Do not want guards to chase the player
			{
				BOOLEAN GuardEngaged;
				HIPGROUP hGroup, hNextGroup;

				GuardEngaged = FALSE;
				for (hGroup = GetHeadLink (&GLOBAL (ip_group_q));
						hGroup; hGroup = hNextGroup)
				{
					IP_GROUP *GroupPtr;

					GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
					hNextGroup = _GetSuccLink (GroupPtr);

					if (GET_GAME_STATE (URQUAN_MESSED_UP))
					{
						GroupPtr->task &= REFORM_GROUP;
						GroupPtr->task |= FLEE | IGNORE_FLAGSHIP;
						GroupPtr->dest_loc = 0;
					}
					else if (GroupPtr->task & REFORM_GROUP)
					{
						GroupPtr->task &= ~REFORM_GROUP;
						GroupPtr->group_counter = 0;

						GuardEngaged = TRUE;
					}

					UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);
				}

				if (GuardEngaged)
				{
					COUNT angle;
					POINT org;

					XFormIPLoc (
							&pSolarSysState->PlanetDesc[4].image.origin,
							&org, FALSE);
					angle = ARCTAN (GLOBAL (ip_location.x) - org.x,
							GLOBAL (ip_location.y) - org.y);
					GLOBAL (ip_location.x) = org.x + COSINE (angle, 3000);
					GLOBAL (ip_location.y) = org.y + SINE (angle, 3000);
					XFormIPLoc (&GLOBAL (ip_location),
							&GLOBAL (ShipStamp.origin), TRUE);
				}
			}
			break;
		case GENERATE_MOONS:
			GenerateRandomIP (GENERATE_MOONS);
			if (pSolarSysState->pBaseDesc == &pSolarSysState->PlanetDesc[4])
			{
				COUNT angle;
				DWORD rand_val;

				pSolarSysState->MoonDesc[0].data_index = SA_MATRA;
				pSolarSysState->MoonDesc[0].radius = MIN_MOON_RADIUS
						+ (2 * MOON_DELTA);
				rand_val = TFB_Random ();
				angle = NORMALIZE_ANGLE (LOWORD (rand_val));
				pSolarSysState->MoonDesc[0].location.x =
						COSINE (angle, pSolarSysState->MoonDesc[0].radius);
				pSolarSysState->MoonDesc[0].location.y =
						SINE (angle, pSolarSysState->MoonDesc[0].radius);
			}
			break;
		case GENERATE_PLANETS:
			GenerateRandomIP (GENERATE_PLANETS);
			pSolarSysState->PlanetDesc[4].NumPlanets = 1;
			break;
		case GENERATE_ORBITAL:
				/* Samatra */
			if (pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->PlanetDesc[4]
					&& pSolarSysState->pOrbitalDesc == &pSolarSysState->MoonDesc[0])
			{
				PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
				ReinitQueue (&GLOBAL (ip_group_q));
				assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

				if (!GET_GAME_STATE (URQUAN_MESSED_UP))
					CloneShipFragment (!GET_GAME_STATE (KOHR_AH_FRENZY) ?
							URQUAN_SHIP : BLACK_URQUAN_SHIP,
							&GLOBAL (npc_built_ship_q), INFINITE_FLEET);
				else
				{
#define URQUAN_REMNANTS 3
					BYTE i;

					for (i = 0; i < URQUAN_REMNANTS; ++i)
					{
						CloneShipFragment (URQUAN_SHIP,
								&GLOBAL (npc_built_ship_q), 0);
						CloneShipFragment (BLACK_URQUAN_SHIP,
								&GLOBAL (npc_built_ship_q), 0);
					}
				}

				pSolarSysState->MenuState.Initialized += 2;
				GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
				SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
				SET_GAME_STATE (URQUAN_PROTECTING_SAMATRA, 1);
				InitCommunication (URQUAN_CONVERSATION);

				if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
				{
					BOOLEAN UrquanSurvivors;

					UrquanSurvivors = GetHeadLink (
							&GLOBAL (npc_built_ship_q)
							) != 0;

					GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
					ReinitQueue (&GLOBAL (npc_built_ship_q));
					GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);
					if (UrquanSurvivors)
					{
						SET_GAME_STATE (URQUAN_PROTECTING_SAMATRA, 0);
					}
					else
					{
						EncounterGroup = 0;
						EncounterRace = -1;
						GLOBAL (CurrentActivity) = IN_LAST_BATTLE | START_ENCOUNTER;
						if (GET_GAME_STATE (YEHAT_CIVIL_WAR)
								&& ActivateStarShip (YEHAT_SHIP, SPHERE_TRACKING)
								&& ActivateStarShip (YEHAT_REBEL_SHIP, FEASIBILITY_STUDY))
							InitCommunication (YEHAT_REBEL_CONVERSATION);
					}
				}
				pSolarSysState->MenuState.Initialized -= 2;
				break;
			}
		default:
			GenerateRandomIP (control);
			break;
	}
}

