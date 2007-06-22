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

#include "races.h"
#include "setup.h"
#include "libs/compiler.h"
#include "libs/mathlib.h"


// Allocate a new STARSHIP and put it in the queue.
HSTARSHIP
Build (QUEUE *pQueue, DWORD RaceResIndex, COUNT which_player, BYTE
		captains_name_index)
{
	HSTARSHIP hNewShip;

	hNewShip = AllocStarShip (pQueue);
	if (hNewShip != 0)
	{
		STARSHIP *StarShipPtr;

		// XXX: STARSHIP refactor; The last remaining aliasing between
		//   STARSHIP and SHIP_FRAGMENT structs
		StarShipPtr = LockStarShip (pQueue, hNewShip);
		memset (StarShipPtr, 0, GetLinkSize (pQueue));

		StarShipPtr->RaceResIndex = RaceResIndex;
		OwnStarShip (StarShipPtr, which_player, captains_name_index);

		UnlockStarShip (pQueue, hNewShip);
		PutQueue (pQueue, hNewShip);
	}

	return (hNewShip);
}

HLINK
GetStarShipFromIndex (QUEUE *pShipQ, COUNT Index)
{
	HLINK hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (pShipQ);
			Index > 0 && hStarShip; hStarShip = hNextShip, --Index)
	{
		LINK *StarShipPtr;

		StarShipPtr = LockLink (pShipQ, hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);
		UnlockLink (pShipQ, hStarShip);
	}

	return (hStarShip);
}

/*
 * What this function does depends on the value of the 'state' argument:
 * SPHERE_TRACKING:
 * 	The sphere of influence for the race for 'which_ship' will be shown
 * 	on the starmap in the future.
 * 	The value returned is 'which_ship', unless the type of ship is only
 * 	available in SuperMelee, in which case 0 is returned.
 * SPHERE_KNOWN:
 * 	The size of the fleet of the race of 'which_ship' when the starmap was
 * 	last checked is returned.
 * ESCORT_WORTH:
 * 	The total value of all the ships escorting the SIS is returned.
 * 	'which_ship' is ignored.
 * ESCORTING_FLAGSHIP:
 * 	Test if a ship of type 'which_ship' is among the escorts of the SIS
 * 	0 is returned if false, 1 if true.
 * FEASIBILITY_STUDY:
 * 	Test if the SIS can have an escort of type 'which_ship'.
 * 	0 is returned if 'which_ship' is not available.
 * 	Otherwise, the number of ships that can be added is returned.
 * CHECK_ALLIANCE:
 * 	Test the alliance status of the race of 'which_ship'.
 *      Either GOOD_GUY (allied) or BAD_GUY (not allied) is returned.
 * SET_ALLIED (0):
 * 	Ally with the race of 'which_ship'. This makes their ship available
 *  for building in the shipyard.
 * SET_NOT_ALLIED:
 * 	End an alliance with the race of 'which_ship'. This ends the possibility
 * 	of building their ships in the shipyard.
 * REMOVE_BUILT: 
 *  Make the already built escorts of the race of 'which_ship' disappear.
 *   (as for the Orz when the alliance with them ends)
 * any other positive number:
 * 	Give the player this many ships of type 'which_ship'.
 */
COUNT
ActivateStarShip (COUNT which_ship, SIZE state)
{
	HSTARSHIP hStarShip, hNextShip;

	hStarShip = GetStarShipFromIndex (&GLOBAL (avail_race_q), which_ship);
	if (!hStarShip)
		return 0;

	switch (state)
	{
		case SPHERE_TRACKING:
		case SPHERE_KNOWN:
		{
			EXTENDED_SHIP_FRAGMENT *StarShipPtr;

			StarShipPtr = (EXTENDED_SHIP_FRAGMENT*) LockStarShip (
					&GLOBAL (avail_race_q), hStarShip);
			if (state == SPHERE_KNOWN)
				which_ship = StarShipPtr->ShipInfo.known_strength;
			else if (StarShipPtr->ShipInfo.actual_strength == 0)
			{
				if (!(StarShipPtr->ShipInfo.ship_flags & (GOOD_GUY | BAD_GUY)))
					which_ship = 0;
			}
			else if (StarShipPtr->ShipInfo.known_strength == 0
					&& StarShipPtr->ShipInfo.actual_strength != (COUNT)~0)
			{
				StarShipPtr->ShipInfo.known_strength = 1;
				StarShipPtr->ShipInfo.known_loc = StarShipPtr->ShipInfo.loc;
			}
			UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);
			return (which_ship);
		}
		case ESCORT_WORTH:
		{
			COUNT ShipCost[] =
			{
				RACE_SHIP_COST
			};
			COUNT total = 0;

			for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
					hStarShip; hStarShip = hNextShip)
			{
				SHIP_FRAGMENT *StarShipPtr;

				StarShipPtr = (SHIP_FRAGMENT*) LockStarShip (
						&GLOBAL (built_ship_q), hStarShip);
				hNextShip = _GetSuccLink (StarShipPtr);
				total += ShipCost[GET_RACE_ID (StarShipPtr)];
				UnlockStarShip (&GLOBAL (built_ship_q), hStarShip);
			}
			return total;
		}
		case ESCORTING_FLAGSHIP:
		{
			for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
					hStarShip; hStarShip = hNextShip)
			{
				BYTE ship_type;
				SHIP_FRAGMENT *StarShipPtr;

				StarShipPtr = (SHIP_FRAGMENT*) LockStarShip (
						&GLOBAL (built_ship_q), hStarShip);
				hNextShip = _GetSuccLink (StarShipPtr);
				ship_type = GET_RACE_ID (StarShipPtr);
				UnlockStarShip (&GLOBAL (built_ship_q), hStarShip);

				if (ship_type == which_ship)
					return 1;
			}
			return 0;
		}
		case FEASIBILITY_STUDY:
		{
			return (MAX_BUILT_SHIPS - CountLinks (&GLOBAL (built_ship_q)));
		}
		case CHECK_ALLIANCE:
		{
			COUNT flags;
			EXTENDED_SHIP_FRAGMENT *StarShipPtr = (EXTENDED_SHIP_FRAGMENT*)
					LockStarShip (&GLOBAL (avail_race_q), hStarShip);
			flags = StarShipPtr->ShipInfo.ship_flags & (GOOD_GUY | BAD_GUY);
			UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);
			return flags;
		}
		case SET_ALLIED:
		case SET_NOT_ALLIED:
		{
			EXTENDED_SHIP_FRAGMENT *StarShipPtr = (EXTENDED_SHIP_FRAGMENT*)
					LockStarShip (&GLOBAL (avail_race_q), hStarShip);

			if (!(StarShipPtr->ShipInfo.ship_flags & (GOOD_GUY | BAD_GUY)))
			{	/* Strange request, silently ignore it */
				UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);
				break;
			}

			StarShipPtr->ShipInfo.ship_flags &= ~(GOOD_GUY | BAD_GUY);
			if (state == SET_ALLIED)
				StarShipPtr->ShipInfo.ship_flags |= GOOD_GUY;
			else
				StarShipPtr->ShipInfo.ship_flags |= BAD_GUY;
			
			UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);
			break;
		}
		case REMOVE_BUILT:
		{
			BOOLEAN ShipRemoved = FALSE;

			for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
					hStarShip; hStarShip = hNextShip)
			{
				BOOLEAN RemoveShip;
				SHIP_FRAGMENT *StarShipPtr;

				StarShipPtr = (SHIP_FRAGMENT*) LockStarShip (
						&GLOBAL (built_ship_q), hStarShip);
				hNextShip = _GetSuccLink (StarShipPtr);
				RemoveShip = (GET_RACE_ID (StarShipPtr) == which_ship);
				UnlockStarShip (&GLOBAL (built_ship_q), hStarShip);

				if (RemoveShip)
				{
					ShipRemoved = TRUE;

					RemoveQueue (&GLOBAL (built_ship_q), hStarShip);
					FreeStarShip (&GLOBAL (built_ship_q), hStarShip);
				}
			}
			
			if (ShipRemoved)
			{
				LockMutex (GraphicsLock);
				DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA,
						UNDEFINED_DELTA);
				UnlockMutex (GraphicsLock);
			}
			break;
		}
		default:
		{
			BYTE which_window;
			COUNT i;

			assert (state > 0);
			/* Add ships to the escorts */
			which_window = 0;
			for (i = 0; i < (COUNT)state; i++)
			{
				HSTARSHIP hOldShip;
				SHIP_FRAGMENT *StarShipPtr;

				hStarShip = CloneShipFragment (which_ship,
						&GLOBAL (built_ship_q), 0);
				if (!hStarShip)
					break;

				RemoveQueue (&GLOBAL (built_ship_q), hStarShip);

				/* Find first available escort window */
				while ((hOldShip = GetStarShipFromIndex (
						&GLOBAL (built_ship_q), which_window++)))
				{
					BYTE win_loc;

					StarShipPtr = (SHIP_FRAGMENT*) LockStarShip (
							&GLOBAL (built_ship_q), hOldShip);
					// XXX: hack; escort window is not group loc,
					//   should just use queue index (already var2)
					win_loc = GET_GROUP_LOC (StarShipPtr);
					UnlockStarShip (&GLOBAL (built_ship_q), hOldShip);
					if (which_window <= win_loc)
						break;
				}

				StarShipPtr = (SHIP_FRAGMENT*) LockStarShip (
						&GLOBAL (built_ship_q), hStarShip);
				// XXX: hack; escort window is not group loc,
				//   should just use queue index (already var2)
				SET_GROUP_LOC (StarShipPtr, which_window - 1);
				UnlockStarShip (&GLOBAL (built_ship_q), hStarShip);

				InsertQueue (&GLOBAL (built_ship_q), hStarShip, hOldShip);
			}

			LockMutex (GraphicsLock);
			DeltaSISGauges (UNDEFINED_DELTA,
					UNDEFINED_DELTA, UNDEFINED_DELTA);
			UnlockMutex (GraphicsLock);
			return i;
		}
	}

	return 1;
}

COUNT
GetIndexFromStarShip (QUEUE *pShipQ, HLINK hStarShip)
{
	COUNT Index;

	Index = 0;
	while (hStarShip != GetHeadLink (pShipQ))
	{
		HLINK hNextShip;
		LINK *StarShipPtr;

		StarShipPtr = LockLink (pShipQ, hStarShip);
		hNextShip = _GetPredLink (StarShipPtr);
		UnlockLink (pShipQ, hStarShip);

		hStarShip = hNextShip;
		++Index;
	}

	return Index;
}

BYTE
NameCaptain (QUEUE *pQueue, DWORD RaceResIndex)
{
	BYTE name_index;
	HSTARSHIP hStarShip;

	do
	{
		HSTARSHIP hNextShip;

		name_index = PickCaptainName ();
		for (hStarShip = GetHeadLink (pQueue); hStarShip; hStarShip = hNextShip)
		{
			STARSHIP *TestShipPtr;

			TestShipPtr = LockStarShip (pQueue, hStarShip);
			hNextShip = _GetSuccLink (TestShipPtr);
			if (TestShipPtr->RaceResIndex == RaceResIndex)
			{
				BOOLEAN SameName;

				// XXX: STARSHIP refactor; The last remaining aliasing between
				//   STARSHIP and SHIP_FRAGMENT structs
				// This hack will not be needed once
				//   STARSHIP maintains captain/side permanently
				if (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE)
					SameName = (name_index == TestShipPtr->captains_name_index);
				else
					SameName = (name_index == StarShipCaptain (TestShipPtr));

				if (SameName)
				{
					UnlockStarShip (pQueue, hStarShip);
					break;
				}
			}
			UnlockStarShip (pQueue, hStarShip);
		}
	} while (hStarShip);

	return name_index;
}

// crew_level can be set to INFINITE_FLEET for a ship which is to
// represent an infinite number of ships.
HSTARSHIP
CloneShipFragment (COUNT shipIndex, QUEUE *pDstQueue, COUNT crew_level)
{
	HSTARSHIP hStarShip, hBuiltShip;
	EXTENDED_SHIP_FRAGMENT *TemplatePtr;

	hStarShip = GetStarShipFromIndex (&GLOBAL (avail_race_q), shipIndex);
	if (hStarShip == 0)
		return 0;

	TemplatePtr = (EXTENDED_SHIP_FRAGMENT *) LockStarShip (
			&GLOBAL (avail_race_q), hStarShip);
	hBuiltShip = Build (pDstQueue, TemplatePtr->RaceResIndex,
			TemplatePtr->ShipInfo.ship_flags & (GOOD_GUY | BAD_GUY),
			(BYTE)(shipIndex == SAMATRA_SHIP ?
					0 : NameCaptain (pDstQueue, TemplatePtr->RaceResIndex)));
	if (hBuiltShip)
	{
		SHIP_FRAGMENT *ShipFragPtr;

		ShipFragPtr = (SHIP_FRAGMENT*) LockStarShip (pDstQueue, hBuiltShip);
		// XXX: SHIP_INFO struct copy
		ShipFragPtr->ShipInfo.race_strings = TemplatePtr->ShipInfo.race_strings;
		ShipFragPtr->ShipInfo.icons = TemplatePtr->ShipInfo.icons;
		ShipFragPtr->ShipInfo.melee_icon = TemplatePtr->ShipInfo.melee_icon;
		if (crew_level)
			ShipFragPtr->ShipInfo.crew_level = crew_level;
		else
			ShipFragPtr->ShipInfo.crew_level = TemplatePtr->ShipInfo.crew_level;
		ShipFragPtr->ShipInfo.max_crew = TemplatePtr->ShipInfo.max_crew;
		ShipFragPtr->ShipInfo.energy_level = 0;
		ShipFragPtr->ShipInfo.max_energy = TemplatePtr->ShipInfo.max_energy;
		ShipFragPtr->ShipInfo.ship_flags = 0;
		ShipFragPtr->ShipInfo.var1 = 0;
		ShipFragPtr->ShipInfo.var2 = 0;
		ShipFragPtr->ShipInfo.loc.x = 0;
		ShipFragPtr->ShipInfo.loc.y = 0;
		SET_RACE_ID (ShipFragPtr, (BYTE)shipIndex);
		UnlockStarShip (pDstQueue, hBuiltShip);
	}
	UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);

	return hBuiltShip;
}

/* Set the crew and captain's name on the first fully-crewed escort
 * ship of race 'which_ship' */
int
SetEscortCrewComplement (COUNT which_ship, COUNT crew_level, BYTE captain)
{
	HSTARSHIP hTemplateShip;
	EXTENDED_SHIP_FRAGMENT *TemplatePtr;
	HSTARSHIP hStarShip;
	HSTARSHIP hNextShip;
	SHIP_FRAGMENT *StarShipPtr = 0;
	int Index;

	hTemplateShip = GetStarShipFromIndex (&GLOBAL (avail_race_q), which_ship);
	if (!hTemplateShip)
		return -1;
	TemplatePtr = (EXTENDED_SHIP_FRAGMENT*) LockStarShip (
			&GLOBAL (avail_race_q), hTemplateShip);

	/* Find first ship of which_ship race */
	for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q)), Index = 0;
			hStarShip; hStarShip = hNextShip, ++Index)
	{
		StarShipPtr = (SHIP_FRAGMENT*) LockStarShip (
				&GLOBAL (built_ship_q), hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);
		if (which_ship == GET_RACE_ID (StarShipPtr) &&
				StarShipPtr->ShipInfo.crew_level ==
				TemplatePtr->ShipInfo.crew_level)
			break; /* found one */
		UnlockStarShip (&GLOBAL (built_ship_q), hStarShip);
	}
	if (hStarShip)
	{
		StarShipPtr->ShipInfo.crew_level = crew_level;
		OwnStarShip (StarShipPtr, StarShipPlayer (StarShipPtr), captain);
		UnlockStarShip (&GLOBAL (built_ship_q), hStarShip);
	}
	else
		Index = -1;

	UnlockStarShip (&GLOBAL (avail_race_q), hTemplateShip);
	return Index;
}
