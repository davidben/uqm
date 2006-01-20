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

#include "master.h"

#include "build.h"
#include "resinst.h"
#include "displist.h"
#include "melee.h"


QUEUE master_q;

void
LoadMasterShipList (void)
{
	COUNT num_entries;
	RES_TYPE rt;
	RES_INSTANCE ri;
	RES_PACKAGE rp;

	rt = GET_TYPE (ARILOU_SHIP_INDEX);
	ri = GET_INSTANCE (ARILOU_SHIP_INDEX);
	rp = GET_PACKAGE (ARILOU_SHIP_INDEX);
	InitQueue (&master_q, num_entries = NUM_MELEE_SHIPS, sizeof (SHIP_FRAGMENT));
	while (num_entries--)
	{
		HSTARSHIP hBuiltShip;

		hBuiltShip = Build (&master_q, MAKE_RESOURCE (rp++, rt, ri++), 0, 0);
		if (hBuiltShip)
		{
			char built_buf[30];
			HSTARSHIP hStarShip, hNextShip;
			STARSHIPPTR BuiltShipPtr;
			SHIP_INFOPTR ShipInfoPtr;

			TaskSwitch ();
					// XXX: what is this doing here?

			BuiltShipPtr = LockStarShip (&master_q, hBuiltShip);
			load_ship (BuiltShipPtr, FALSE);
			ShipInfoPtr = &((SHIP_FRAGMENTPTR)BuiltShipPtr)->ShipInfo;
			*ShipInfoPtr = BuiltShipPtr->RaceDescPtr->ship_info;
			BuiltShipPtr->RaceDescPtr->ship_info.melee_icon = 0;
			BuiltShipPtr->RaceDescPtr->ship_info.icons = 0;
			BuiltShipPtr->RaceDescPtr->ship_info.race_strings = 0;
			free_ship (BuiltShipPtr, FALSE);
			BuiltShipPtr->RaceDescPtr = (RACE_DESCPTR)ShipInfoPtr;

			GetStringContents (SetAbsStringTableIndex (
					BuiltShipPtr->RaceDescPtr->ship_info.race_strings, 2
					), (STRINGPTR)built_buf, FALSE);
			UnlockStarShip (&master_q, hBuiltShip);

			RemoveQueue (&master_q, hBuiltShip);

			// Insert the ship in the master queue in the right location
			// to keep the list sorted on the name of the race.
			for (hStarShip = GetHeadLink (&master_q);
					hStarShip; hStarShip = hNextShip)
			{
				char ship_buf[30];
				STARSHIPPTR StarShipPtr;

				StarShipPtr = LockStarShip (&master_q, hStarShip);
				hNextShip = _GetSuccLink (StarShipPtr);
				GetStringContents (SetAbsStringTableIndex (
						StarShipPtr->RaceDescPtr->ship_info.race_strings, 2
						), (STRINGPTR)ship_buf, FALSE);
				UnlockStarShip (&master_q, hStarShip);

				if (strcmp (built_buf, ship_buf) < 0)
					break;
			}
			InsertQueue (&master_q, hBuiltShip, hStarShip);
		}
	}
}

void
FreeMasterShipList (void)
{
	HSTARSHIP hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&master_q);
			hStarShip != 0; hStarShip = hNextShip)
	{
		STARSHIPPTR StarShipPtr;

		StarShipPtr = LockStarShip (&master_q, hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);

		DestroyDrawable (ReleaseDrawable (
				StarShipPtr->RaceDescPtr->ship_info.melee_icon));
		DestroyDrawable (ReleaseDrawable (
				StarShipPtr->RaceDescPtr->ship_info.icons));
		DestroyStringTable (ReleaseStringTable (
				StarShipPtr->RaceDescPtr->ship_info.race_strings));

		UnlockStarShip (&master_q, hStarShip);
	}

	UninitQueue (&master_q);
}

HSTARSHIP
FindMasterShip (DWORD ship_ref)
{
	HSTARSHIP hStarShip;
	
	hStarShip = GetHeadLink (&master_q);
	if (hStarShip)
	{
		do
		{
			DWORD ref;
			HSTARSHIP hNextShip;
			STARSHIPPTR StarShipPtr;

			StarShipPtr = LockStarShip (&master_q, hStarShip);
			hNextShip = _GetSuccLink (StarShipPtr);
			ref = StarShipPtr->RaceResIndex;
			UnlockStarShip (&master_q, hStarShip);

			if (ref == ship_ref)
				break;

			hStarShip = hNextShip;
		} while (hStarShip);
	}

	return (hStarShip);
}
