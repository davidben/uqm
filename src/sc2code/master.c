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
	InitQueue (&master_q, num_entries, sizeof (MASTER_SHIP_INFO));
	while (num_entries--)
	{
		HMASTERSHIP hBuiltShip;
		char built_buf[30];
		HMASTERSHIP hStarShip, hNextShip;
		MASTER_SHIP_INFO *BuiltPtr;
		RACE_DESC *RDPtr;

		hBuiltShip = AllocLink (&master_q);
		if (!hBuiltShip)
			continue;

		// Allow other things to run
		//  supposedly, loading ship packages and data takes some time
		if (YieldProcessing)
			YieldProcessing ();

		BuiltPtr = LockMasterShip (&master_q, hBuiltShip);
		BuiltPtr->RaceResIndex = MAKE_RESOURCE (rp++, rt, ri++);
		RDPtr = load_ship (BuiltPtr->RaceResIndex, FALSE);
		if (!RDPtr)
		{
			UnlockMasterShip (&master_q, hBuiltShip);
			continue;
		}

		// Grab a copy of loaded icons, strings and info
		// XXX: SHIP_INFO implicitly referenced here
		BuiltPtr->ShipInfo = RDPtr->ship_info;
		BuiltPtr->Fleet = RDPtr->fleet;
		free_ship (RDPtr, FALSE, FALSE);

		GetStringContents (SetAbsStringTableIndex (
				BuiltPtr->ShipInfo.race_strings, 2
				), (STRINGPTR)built_buf, FALSE);
		UnlockMasterShip (&master_q, hBuiltShip);

		// Insert the ship in the master queue in the right location
		// to keep the list sorted on the name of the race.
		for (hStarShip = GetHeadLink (&master_q);
				hStarShip; hStarShip = hNextShip)
		{
			char ship_buf[30];
			MASTER_SHIP_INFO *MasterPtr;

			MasterPtr = LockMasterShip (&master_q, hStarShip);
			hNextShip = _GetSuccLink (MasterPtr);
			GetStringContents (SetAbsStringTableIndex (
					MasterPtr->ShipInfo.race_strings, 2
					), (STRINGPTR)ship_buf, FALSE);
			UnlockMasterShip (&master_q, hStarShip);

			if (strcmp (built_buf, ship_buf) < 0)
				break;
		}
		InsertQueue (&master_q, hBuiltShip, hStarShip);
	}
}

void
FreeMasterShipList (void)
{
	HMASTERSHIP hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&master_q);
			hStarShip != 0; hStarShip = hNextShip)
	{
		MASTER_SHIP_INFO *MasterPtr;

		MasterPtr = LockMasterShip (&master_q, hStarShip);
		hNextShip = _GetSuccLink (MasterPtr);

		DestroyDrawable (ReleaseDrawable (MasterPtr->ShipInfo.melee_icon));
		DestroyDrawable (ReleaseDrawable (MasterPtr->ShipInfo.icons));
		DestroyStringTable (ReleaseStringTable (
				MasterPtr->ShipInfo.race_strings));

		UnlockMasterShip (&master_q, hStarShip);
	}

	UninitQueue (&master_q);
}

HMASTERSHIP
FindMasterShip (DWORD ship_ref)
{
	HMASTERSHIP hStarShip, hNextShip;
	
	for (hStarShip = GetHeadLink (&master_q); hStarShip; hStarShip = hNextShip)
	{
		DWORD ref;
		MASTER_SHIP_INFO *MasterPtr;

		MasterPtr = LockMasterShip (&master_q, hStarShip);
		hNextShip = _GetSuccLink (MasterPtr);
		ref = MasterPtr->RaceResIndex;
		UnlockMasterShip (&master_q, hStarShip);

		if (ref == ship_ref)
			break;
	}

	return (hStarShip);
}

int
FindMasterShipIndex (DWORD ship_ref)
{
	HMASTERSHIP hStarShip, hNextShip;
	int index;
	
	for (index = 0, hStarShip = GetHeadLink (&master_q); hStarShip;
			++index, hStarShip = hNextShip)
	{
		DWORD ref;
		MASTER_SHIP_INFO *MasterPtr;

		MasterPtr = LockMasterShip (&master_q, hStarShip);
		hNextShip = _GetSuccLink (MasterPtr);
		ref = MasterPtr->RaceResIndex;
		UnlockMasterShip (&master_q, hStarShip);

		if (ref == ship_ref)
			break;
	}

	return hStarShip ? index : -1;
}
