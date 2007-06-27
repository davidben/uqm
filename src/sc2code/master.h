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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MASTER_H
#define _MASTER_H

#include "races.h"
#include "libs/compiler.h"

typedef HLINK HMASTERSHIP;

typedef struct
{
	// LINK elements; must be first
	HMASTERSHIP pred;
	HMASTERSHIP succ;

	DWORD RaceResIndex;

	SHIP_INFO ShipInfo;
} MASTER_SHIP_INFO;

extern QUEUE master_q;
		/* List of ships available in SuperMelee;
		 * queue element is MASTER_SHIP_INFO */

static inline MASTER_SHIP_INFO *
LockMasterShip (const QUEUE *pq, HMASTERSHIP h)
{
	assert (GetLinkSize (pq) == sizeof (MASTER_SHIP_INFO));
	return (MASTER_SHIP_INFO *) LockLink (pq, h);
}

#define UnlockMasterShip(pq, h) UnlockLink (pq, h)
#define FreeMasterShip(pq, h) FreeLink (pq, h)

extern void LoadMasterShipList (void (* YieldProcessing)(void));
extern void FreeMasterShipList (void);
extern HMASTERSHIP FindMasterShip (DWORD ship_ref);
extern int FindMasterShipIndex (DWORD ship_ref);


#endif  /* _MASTER_H */

