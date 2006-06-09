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
			BYTE b0, b1, b2, b3;
			HSTARSHIP hStarShip;

			b0 = GET_GAME_STATE (COLONY_GRPOFFS0);
			b1 = GET_GAME_STATE (COLONY_GRPOFFS1);
			b2 = GET_GAME_STATE (COLONY_GRPOFFS2);
			b3 = GET_GAME_STATE (COLONY_GRPOFFS3);
			GLOBAL (BattleGroupRef) = MAKE_DWORD (
					MAKE_WORD (b0, b1), MAKE_WORD (b2, b3)
					);
			if (GLOBAL (BattleGroupRef) == 0)
			{
					CloneShipFragment (URQUAN_SHIP,
							&GLOBAL (npc_built_ship_q), 0);

					GLOBAL (BattleGroupRef) = PutGroupInfo (GROUPS_ADD_NEW, 1);
					b0 = LOBYTE (LOWORD (GLOBAL (BattleGroupRef)));
					b1 = HIBYTE (LOWORD (GLOBAL (BattleGroupRef)));
					b2 = LOBYTE (HIWORD (GLOBAL (BattleGroupRef)));
					b3 = HIBYTE (HIWORD (GLOBAL (BattleGroupRef)));
					SET_GAME_STATE (COLONY_GRPOFFS0, b0);
					SET_GAME_STATE (COLONY_GRPOFFS1, b1);
					SET_GAME_STATE (COLONY_GRPOFFS2, b2);
					SET_GAME_STATE (COLONY_GRPOFFS3, b3);
			}

			GenerateRandomIP (INIT_NPCS);

			if (GLOBAL (BattleGroupRef)
					&& (hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q))))
			{
				SHIP_FRAGMENTPTR FragPtr;

				FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (
						&GLOBAL (npc_built_ship_q), hStarShip
						);
				SET_GROUP_MISSION (FragPtr, IN_ORBIT);
				SET_GROUP_LOC (FragPtr, 0 + 1); /* orbitting colony */
				SET_GROUP_DEST (FragPtr, 0 + 1); /* orbitting colony */
				FragPtr->ShipInfo.loc.x = FragPtr->ShipInfo.loc.y = 0;
				FragPtr->ShipInfo.group_counter = 0;
				UnlockStarShip (
						&GLOBAL (npc_built_ship_q), hStarShip
						);
			}
			break;
		}
		case GENERATE_PLANETS:
		{
			COUNT angle;
			PPLANET_DESC pMinPlanet;

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


