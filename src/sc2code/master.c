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
LoadMasterShipList (void (* YieldProcessing)(void))
{
	COUNT num_entries;
	RES_TYPE rt;
	RES_INSTANCE ri;
	RES_PACKAGE rp;

	rt = GET_TYPE (ARILOU_SHIP_INDEX);
	ri = GET_INSTANCE (ARILOU_SHIP_INDEX);
	rp = GET_PACKAGE (ARILOU_SHIP_INDEX);
	num_entries = NUM_MELEE_SHIPS;
	InitQueue (&master_q, num_entries, sizeof (SHIP_FRAGMENT));
	while (num_entries--)
	{
		HSTARSHIP hBuiltShip;
		char built_buf[30];
		HSTARSHIP hStarShip, hNextShip;
		SHIP_FRAGMENT *BuiltFragPtr;
		RACE_DESC *RDPtr;

		hBuiltShip = Build (&master_q, MAKE_RESOURCE (rp++, rt, ri++), 0, 0);
		if (!hBuiltShip)
			continue;

		// Allow other things to run
		//  supposedly, loading ship packages and data takes some time
		if (YieldProcessing)
			YieldProcessing ();

		BuiltFragPtr = (SHIP_FRAGMENT *) LockStarShip (&master_q, hBuiltShip);
		RDPtr = load_ship (BuiltFragPtr->RaceResIndex, FALSE);
		if (!RDPtr)
		{
			UnlockStarShip (&master_q, hBuiltShip);
			RemoveQueue (&master_q, hBuiltShip);
			continue;
		}

		// Grab a copy of loaded icons and strings
		BuiltFragPtr->ShipInfo = RDPtr->ship_info;
		free_ship (RDPtr, FALSE, FALSE);
		BuiltFragPtr->ShipInfoPtr = &BuiltFragPtr->ShipInfo;

		GetStringContents (SetAbsStringTableIndex (
				BuiltFragPtr->ShipInfo.race_strings, 2
				), (STRINGPTR)built_buf, FALSE);
		UnlockStarShip (&master_q, hBuiltShip);

		RemoveQueue (&master_q, hBuiltShip);

		// Insert the ship in the master queue in the right location
		// to keep the list sorted on the name of the race.
		for (hStarShip = GetHeadLink (&master_q);
				hStarShip; hStarShip = hNextShip)
		{
			char ship_buf[30];
			SHIP_FRAGMENT *FragPtr;

			FragPtr = (SHIP_FRAGMENT *) LockStarShip (&master_q, hStarShip);
			hNextShip = _GetSuccLink (FragPtr);
			GetStringContents (SetAbsStringTableIndex (
					FragPtr->ShipInfo.race_strings, 2
					), (STRINGPTR)ship_buf, FALSE);
			UnlockStarShip (&master_q, hStarShip);

			if (strcmp (built_buf, ship_buf) < 0)
				break;
		}
		InsertQueue (&master_q, hBuiltShip, hStarShip);
	}
}

void
FreeMasterShipList (void)
{
	HSTARSHIP hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&master_q);
			hStarShip != 0; hStarShip = hNextShip)
	{
		SHIP_FRAGMENT *FragPtr;

		FragPtr = (SHIP_FRAGMENT *) LockStarShip (&master_q, hStarShip);
		hNextShip = _GetSuccLink (FragPtr);

		DestroyDrawable (ReleaseDrawable (FragPtr->ShipInfo.melee_icon));
		DestroyDrawable (ReleaseDrawable (FragPtr->ShipInfo.icons));
		DestroyStringTable (ReleaseStringTable (
				FragPtr->ShipInfo.race_strings));

		UnlockStarShip (&master_q, hStarShip);
	}

	UninitQueue (&master_q);
}

HSTARSHIP
FindMasterShip (DWORD ship_ref)
{
	HSTARSHIP hStarShip;
	HSTARSHIP hNextShip;
	
	for (hStarShip = GetHeadLink (&master_q); hStarShip; hStarShip = hNextShip)
	{
		DWORD ref;
		SHIP_FRAGMENT *FragPtr;

		FragPtr = (SHIP_FRAGMENT *) LockStarShip (&master_q, hStarShip);
		hNextShip = _GetSuccLink (FragPtr);
		ref = FragPtr->RaceResIndex;
		UnlockStarShip (&master_q, hStarShip);

		if (ref == ship_ref)
			break;
	}

	return (hStarShip);
}
