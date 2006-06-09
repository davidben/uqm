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
#include "globdata.h"
#include "state.h"
#include "planets/genall.h"


static void
check_old_shofixti (void)
{
	HSTARSHIP hStarShip;

	if (GLOBAL (BattleGroupRef)
			&& (hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q)))
			&& GET_GAME_STATE (SHOFIXTI_RECRUITED))
	{
		BYTE task;
		SHIP_FRAGMENTPTR FragPtr;

		FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (npc_built_ship_q), hStarShip);
		task = GET_GROUP_MISSION (FragPtr);

		SET_GROUP_MISSION (FragPtr,
				FLEE | IGNORE_FLAGSHIP | (task & REFORM_GROUP));
		SET_GROUP_DEST (FragPtr, 0);

		UnlockStarShip (&GLOBAL (npc_built_ship_q), hStarShip);
	}
}

void
GenerateShofixti (BYTE control)
{
	switch (control)
	{
		case INIT_NPCS:
		if (!GET_GAME_STATE (SHOFIXTI_RECRUITED)
				&& (!GET_GAME_STATE (SHOFIXTI_KIA)
				|| (!GET_GAME_STATE (SHOFIXTI_BRO_KIA)
				&& GET_GAME_STATE (MAIDENS_ON_SHIP))))
		{
			GLOBAL (BattleGroupRef) = GET_GAME_STATE_32 (SHOFIXTI_GRPOFFS0);
			if (GLOBAL (BattleGroupRef) == 0
					|| !GetGroupInfo (GLOBAL (BattleGroupRef), GROUP_INIT_IP))
			{
				if (GLOBAL (BattleGroupRef) == 0)
					GLOBAL (BattleGroupRef) = ~0L;

				CloneShipFragment (SHOFIXTI_SHIP,
						&GLOBAL (npc_built_ship_q), 1);

				GLOBAL (BattleGroupRef) = PutGroupInfo (
						GLOBAL (BattleGroupRef), 1);
				SET_GAME_STATE_32 (SHOFIXTI_GRPOFFS0,
						GLOBAL (BattleGroupRef));
			}
		}
		case REINIT_NPCS:
			GenerateRandomIP (control);
			check_old_shofixti ();
			break;
		case UNINIT_NPCS:
			if (GLOBAL (BattleGroupRef)
					&& !GET_GAME_STATE (SHOFIXTI_RECRUITED)
					&& GetHeadLink (&GLOBAL (npc_built_ship_q)) == 0)
			{
				if (!GET_GAME_STATE (SHOFIXTI_KIA))
				{
					SET_GAME_STATE (SHOFIXTI_KIA, 1);
					SET_GAME_STATE (SHOFIXTI_VISITS, 0);
				}
				else if (GET_GAME_STATE (MAIDENS_ON_SHIP))
				{
					SET_GAME_STATE (SHOFIXTI_BRO_KIA, 1);
				}
			}
			GenerateRandomIP (UNINIT_NPCS);
			break;
		case GENERATE_PLANETS:
		{
			COUNT i;
			PPLANET_DESC pCurDesc;

#define NUM_PLANETS 6
			pSolarSysState->SunDesc[0].NumPlanets = NUM_PLANETS;
			for (i = 0, pCurDesc = pSolarSysState->PlanetDesc;
					i < NUM_PLANETS; ++i, ++pCurDesc)
			{
				pCurDesc->NumPlanets = 0;
				if (i < (NUM_PLANETS >> 1))
					pCurDesc->data_index = SELENIC_WORLD;
				else
					pCurDesc->data_index = METAL_WORLD;
			}

			FillOrbits (pSolarSysState,
					NUM_PLANETS, &pSolarSysState->PlanetDesc[0], TRUE);
			break;
		}
		default:
			GenerateRandomIP (control);
			break;
	}
}


