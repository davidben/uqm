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

#include "encount.h"
#include "globdata.h"
#include "planets/genall.h"


void
GenerateRainbow (BYTE control)
{
	switch (control)
	{
		case GENERATE_PLANETS:
		{
			COUNT angle;

			GenerateRandomIP (GENERATE_PLANETS);
			pSolarSysState->PlanetDesc[0].data_index = RAINBOW_WORLD;
			pSolarSysState->PlanetDesc[0].NumPlanets = 0;
			pSolarSysState->PlanetDesc[0].radius = EARTH_RADIUS * 50L / 100;
			angle = ARCTAN (
					pSolarSysState->PlanetDesc[0].location.x,
					pSolarSysState->PlanetDesc[0].location.y
					);
			if (angle <= QUADRANT)
				angle += QUADRANT;
			else if (angle >= FULL_CIRCLE - QUADRANT)
				angle -= QUADRANT;
			pSolarSysState->PlanetDesc[0].location.x =
					COSINE (angle, pSolarSysState->PlanetDesc[0].radius);
			pSolarSysState->PlanetDesc[0].location.y =
					SINE (angle, pSolarSysState->PlanetDesc[0].radius);
			break;
		}
		case GENERATE_ORBITAL:
			if (pSolarSysState->pOrbitalDesc == &pSolarSysState->PlanetDesc[0])
			{
				BYTE which_rainbow;
				UWORD rainbow_mask;
				STAR_DESCPTR SDPtr;

				rainbow_mask = MAKE_WORD (
						GET_GAME_STATE (RAINBOW_WORLD0),
						GET_GAME_STATE (RAINBOW_WORLD1)
						);

				which_rainbow = 0;
				SDPtr = &star_array[0];
				while (SDPtr != CurStarDescPtr)
				{
					if (SDPtr->Index == RAINBOW_DEFINED)
						++which_rainbow;
					++SDPtr;
				}
				rainbow_mask |= 1 << which_rainbow;
				SET_GAME_STATE (RAINBOW_WORLD0, LOBYTE (rainbow_mask));
				SET_GAME_STATE (RAINBOW_WORLD1, HIBYTE (rainbow_mask));
			}
		default:
			GenerateRandomIP (control);
			break;
	}
}

