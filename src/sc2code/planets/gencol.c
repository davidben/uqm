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


void
GenerateColony (BYTE control)
{
	switch (control)
	{
		case INIT_NPCS:
		{
			HSHIPFRAG hStarShip;

			GLOBAL (BattleGroupRef) = GET_GAME_STATE_32 (COLONY_GRPOFFS0);
			if (GLOBAL (BattleGroupRef) == 0)
			{
				CloneShipFragment (URQUAN_SHIP,
						&GLOBAL (npc_built_ship_q), 0);

				GLOBAL (BattleGroupRef) = PutGroupInfo (GROUPS_ADD_NEW, 1);
				SET_GAME_STATE_32 (COLONY_GRPOFFS0, GLOBAL (BattleGroupRef));
			}

			GenerateRandomIP (INIT_NPCS);

			if (GLOBAL (BattleGroupRef)
					&& (hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q))))
			{
				SHIP_FRAGMENT *FragPtr;

				FragPtr = LockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
				SET_GROUP_MISSION (FragPtr, IN_ORBIT);
				SET_GROUP_LOC (FragPtr, 0 + 1); /* orbitting colony */
				SET_GROUP_DEST (FragPtr, 0 + 1); /* orbitting colony */
				FragPtr->loc.x = 0;
				FragPtr->loc.y = 0;
				FragPtr->group_counter = 0;
				UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
			}
			break;
		}
		case GENERATE_PLANETS:
		{
			COUNT angle;
			PLANET_DESC *pMinPlanet;

			pMinPlanet = &pSolarSysState->PlanetDesc[0];
			FillOrbits (pSolarSysState, (BYTE)~0, pMinPlanet, FALSE);

			pMinPlanet->radius = EARTH_RADIUS * 115L / 100;
			angle = ARCTAN (pMinPlanet->location.x, pMinPlanet->location.y);
			pMinPlanet->location.x =
					COSINE (angle, pMinPlanet->radius);
			pMinPlanet->location.y =
					SINE (angle, pMinPlanet->radius);
			pMinPlanet->data_index = WATER_WORLD | PLANET_SHIELDED;
			break;
		}
		case GENERATE_ORBITAL:
			if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
			{
				DoPlanetaryAnalysis (
						&pSolarSysState->SysInfo, pSolarSysState->pOrbitalDesc
						);

				pSolarSysState->SysInfo.PlanetInfo.AtmoDensity =
						EARTH_ATMOSPHERE * 98 / 100;
				pSolarSysState->SysInfo.PlanetInfo.Weather = 0;
				pSolarSysState->SysInfo.PlanetInfo.Tectonics = 0;
				pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature = 28;

				LoadPlanet (NULL);
				break;
			}
		default:
			GenerateRandomIP (control);
			break;
	}
}


