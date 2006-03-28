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

#ifndef _BUILD_H
#define _BUILD_H

#include "races.h"
#include "displist.h"
#include "libs/compiler.h"

#define CREW_XOFFS 4
#define ENERGY_XOFFS 52
#define GAUGE_YOFFS (SHIP_INFO_HEIGHT - 10)
#define UNIT_WIDTH 2
#define UNIT_HEIGHT 1
#define STAT_WIDTH (1 + UNIT_WIDTH + 1 + UNIT_WIDTH + 1)

#define NAME_OFFSET 5
#define NUM_CAPTAINS_NAMES 16

#define OwnStarShip(S,p,n) \
		(S)->s.Player = (p); \
		(S)->s.Captain = (n);
#define StarShipCaptain(S) \
		(S)->s.Captain
#define StarShipPlayer(S) \
		(S)->s.Player
#define PickCaptainName() (((COUNT)TFB_Random () \
								& (NUM_CAPTAINS_NAMES - 1)) \
								+ NAME_OFFSET)

extern HSTARSHIP Build (PQUEUE pQueue, DWORD RaceResIndex, COUNT
		which_player, BYTE captains_name_index);
extern HSTARSHIP CloneShipFragment (COUNT shipIndex, PQUEUE pDstQueue,
		COUNT crew_level);
extern HSTARSHIP GetStarShipFromIndex (PQUEUE pShipQ, COUNT Index);
extern BYTE NameCaptain (PQUEUE pQueue, STARSHIPPTR StarShipPtr);

/* Possible values for the 'state' argument of
 * COUNT ActivateStarShip (COUNT which_ship, SIZE state)
 * See that function for more information.
 */
#define CHECK_ALLIANCE -2
#define ESCORT_WORTH 9995
#define SPHERE_KNOWN 9996
#define SPHERE_TRACKING 9997
#define ESCORTING_FLAGSHIP 9998
#define FEASIBILITY_STUDY 9999
		/* Check for how many escort ships there's still space */

extern COUNT ActivateStarShip (COUNT which_ship, SIZE state);
extern COUNT GetIndexFromStarShip (PQUEUE pShipQ, HSTARSHIP hStarShip);

extern MEM_HANDLE load_ship (STARSHIPPTR StarShipPtr, BOOLEAN
		LoadBattleData);
extern void free_ship (STARSHIPPTR StarShipPtr, BOOLEAN FreeBattleData);

extern void DrawCrewFuelString (COORD y, SIZE state);
extern void ClearShipStatus (COORD y);
extern void OutlineShipStatus (COORD y);
extern void InitShipStatus (STARSHIPPTR StarShipPtr, PRECT pClipRect);
extern void DeltaStatistics (STARSHIPPTR StarShipPtr, SIZE crew_delta,
		SIZE energy_delta);
extern void DrawBattleCrewAmount (STARSHIPPTR StarShipPtr);

#endif /* _BUILD_H */

