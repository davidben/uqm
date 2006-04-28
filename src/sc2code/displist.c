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

#include "displist.h"
#include "libs/log.h"

/*
 * This file contains code for generic doubly linked lists.
 * If QUEUE_TABLE is defined, each lists has its own preallocated
 * pool of link structures. The size is specific on InitQueue(),
 * and poses a hard limit on the number of elements in the list.
 */

BOOLEAN
InitQueue (PQUEUE pq, COUNT num_elements, OBJ_SIZE size)
{
	SetHeadLink (pq, NULL_HANDLE);
	SetTailLink (pq, NULL_HANDLE);
	SetLinkSize (pq, size);
#ifndef QUEUE_TABLE
	return (TRUE);
#else /* QUEUE_TABLE */
	SetFreeList (pq, NULL_HANDLE);
#if 0	
	log_add (log_Debug, "InitQueue(): num_elements = %d (%d)",
			num_elements, (BYTE)num_elements);
#endif
	if (AllocQueueTab (pq, num_elements) && LockQueueTab (pq))
	{
		do
			FreeLink (pq, GetLinkAddr (pq, num_elements));
		while (--num_elements);

		return (TRUE);
	}

	return (FALSE);
#endif /* QUEUE_TABLE */
}

BOOLEAN
UninitQueue (PQUEUE pq)
{
#ifdef QUEUE_TABLE
	SetHeadLink (pq, NULL_HANDLE);
	SetTailLink (pq, NULL_HANDLE);
	SetFreeList (pq, NULL_HANDLE);
	UnlockQueueTab (pq);
	FreeQueueTab (pq);

	return (TRUE);
#else /* !QUEUE_TABLE */
	HLINK hLink;

	while ((hLink = GetHeadLink (pq)) != NULL_HANDLE)
	{
		RemoveQueue (pq, hLink);
		if (!FreeLink (pq, hLink))
			return (FALSE);
	}

	return (TRUE);
#endif /* QUEUE_TABLE */
}

void
ReinitQueue (PQUEUE pq)
{
	SetHeadLink (pq, NULL_HANDLE);
	SetTailLink (pq, NULL_HANDLE);
#ifdef QUEUE_TABLE
	{
		COUNT num_elements;

		SetFreeList (pq, NULL_HANDLE);

		num_elements = SizeQueueTab (pq);
		if (num_elements)
		{
			do
				FreeLink (pq, GetLinkAddr (pq, num_elements));
			while (--num_elements);
		}
	}
#endif /* QUEUE_TABLE */
}

#ifdef QUEUE_TABLE
HLINK
AllocLink (PQUEUE pq)
{
	HLINK hLink;

	hLink = GetFreeList (pq);
	if (hLink)
	{
		LINKPTR LinkPtr;

		LinkPtr = LockLink (pq, hLink);
		SetFreeList (pq, _GetSuccLink (LinkPtr));
		UnlockLink (pq, hLink);
	}
	else
		log_add (log_Debug, "AllocLink(): No more elements");

	return (hLink);
}

void
FreeLink (PQUEUE pq, HLINK hLink)
{
	LINKPTR LinkPtr;

	LinkPtr = LockLink (pq, hLink);
	_SetSuccLink (LinkPtr, GetFreeList (pq));
	UnlockLink (pq, hLink);

	SetFreeList (pq, hLink);
}
#endif /* QUEUE_TABLE */

void
PutQueue (PQUEUE pq, HLINK hLink)
{
	LINKPTR LinkPtr;

	if (GetHeadLink (pq) == 0)
		SetHeadLink (pq, hLink);
	else
	{
		HLINK hTail;
		LINKPTR lpTail;

		hTail = GetTailLink (pq);
		lpTail = LockLink (pq, hTail);
		_SetSuccLink (lpTail, hLink);
		UnlockLink (pq, hTail);
	}

	LinkPtr = LockLink (pq, hLink);
	_SetPredLink (LinkPtr, GetTailLink (pq));
	_SetSuccLink (LinkPtr, NULL_HANDLE);
	UnlockLink (pq, hLink);

	SetTailLink (pq, hLink);
}

void
InsertQueue (PQUEUE pq, HLINK hLink, HLINK hRefLink)
{
	if (hRefLink == 0)
		PutQueue (pq, hLink);
	else
	{
		LINKPTR LinkPtr, RefLinkPtr;

		LinkPtr = LockLink (pq, hLink);
		RefLinkPtr = LockLink (pq, hRefLink);
		_SetPredLink (LinkPtr, _GetPredLink (RefLinkPtr));
		_SetPredLink (RefLinkPtr, hLink);
		_SetSuccLink (LinkPtr, hRefLink);

		if (GetHeadLink (pq) == hRefLink)
			SetHeadLink (pq, hLink);
		else
		{
			HLINK hPredLink;
			LINKPTR PredLinkPtr;

			hPredLink = _GetPredLink (LinkPtr);
			PredLinkPtr = LockLink (pq, hPredLink);
			_SetSuccLink (PredLinkPtr, hLink);
			UnlockLink (pq, hPredLink);
		}
		UnlockLink (pq, hRefLink);
		UnlockLink (pq, hLink);
	}
}

void
RemoveQueue (PQUEUE pq, HLINK hLink)
{
	LINKPTR LinkPtr;

	LinkPtr = LockLink (pq, hLink);
	if (GetHeadLink (pq) == hLink)
	{
		SetHeadLink (pq, _GetSuccLink (LinkPtr));
	}
	else
	{
		HLINK hPredLink;
		LINKPTR PredLinkPtr;

		hPredLink = _GetPredLink (LinkPtr);
		PredLinkPtr = LockLink (pq, hPredLink);
		_SetSuccLink (PredLinkPtr, _GetSuccLink (LinkPtr));
		UnlockLink (pq, hPredLink);
	}
	if (GetTailLink (pq) == hLink)
	{
		SetTailLink (pq, _GetPredLink (LinkPtr));
	}
	else
	{
		HLINK hSuccLink, hPredLink;
		LINKPTR SuccLinkPtr;

		hSuccLink = _GetSuccLink (LinkPtr);
		SuccLinkPtr = LockLink (pq, hSuccLink);
		hPredLink = _GetPredLink (LinkPtr);
		_SetPredLink (SuccLinkPtr, hPredLink);
		UnlockLink (pq, hSuccLink);
	}
	UnlockLink (pq, hLink);
}

COUNT
CountLinks (PQUEUE pq)
{
	COUNT LinkCount;
	HLINK hLink, hNextLink;

	LinkCount = 0;
	for (hLink = GetHeadLink (pq); hLink; hLink = hNextLink)
	{
		LINKPTR LinkPtr;

		++LinkCount;

		LinkPtr = LockLink (pq, hLink);
		hNextLink = _GetSuccLink (LinkPtr);
		UnlockLink (pq, hLink);
	}

	return (LinkCount);
}

void
ForAllLinks (PQUEUE pq, void (*callback)(LINKPTR, void *), void *arg)
{
	HLINK hLink, hNextLink;

	for (hLink = GetHeadLink (pq); hLink; hLink = hNextLink)
	{
		LINKPTR LinkPtr;
		LinkPtr = LockLink (pq, hLink);
		hNextLink = _GetSuccLink (LinkPtr);
		(*callback) (LinkPtr, arg);
		UnlockLink (pq, hLink);
	}
}


