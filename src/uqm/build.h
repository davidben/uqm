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

#if defined(__cplusplus)
extern "C" {
#endif

#define NAME_OFFSET 5
#define NUM_CAPTAINS_NAMES 16

#define PickCaptainName() (((COUNT)TFB_Random () \
								& (NUM_CAPTAINS_NAMES - 1)) \
								+ NAME_OFFSET)

extern HLINK Build (QUEUE *pQueue, SPECIES_ID SpeciesID);
extern HSHIPFRAG CloneShipFragment (COUNT shipIndex, QUEUE *pDstQueue,
		COUNT crew_level);
extern HLINK GetStarShipFromIndex (QUEUE *pShipQ, COUNT Index);
extern HSHIPFRAG GetEscortByStarShipIndex (COUNT index);
extern BYTE NameCaptain (QUEUE *pQueue, SPECIES_ID SpeciesID);

extern COUNT ActivateStarShip (COUNT which_ship, SIZE state);
extern COUNT GetIndexFromStarShip (QUEUE *pShipQ, HLINK hStarShip);
extern int SetEscortCrewComplement (COUNT which_ship, COUNT crew_level,
		BYTE captain);

extern COUNT AddEscortShips (COUNT race, SIZE count);
extern COUNT CalculateEscortsWorth (void);
//extern COUNT GetRaceKnownSize (COUNT race);
extern COUNT SetRaceAllied (COUNT race, BOOLEAN flag);
extern COUNT StartSphereTracking (COUNT race);
extern BOOLEAN HaveEscortShip (COUNT race);
extern COUNT EscortFeasibilityStudy (COUNT race);
extern COUNT CheckAlliance (COUNT race);
extern void RemoveEscortShips (COUNT race);

extern RACE_DESC *load_ship (SPECIES_ID SpeciesID, BOOLEAN LoadBattleData);
extern void free_ship (RACE_DESC *RaceDescPtr, BOOLEAN FreeIconData,
		BOOLEAN FreeBattleData);

#if defined(__cplusplus)
}
#endif

#endif /* _BUILD_H */

