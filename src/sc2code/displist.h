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

#ifndef _DISPLIST_H
#define _DISPLIST_H

#include "memlib.h"

#define QUEUE_TABLE

#ifdef QUEUE_TABLE
typedef PVOID QUEUE_HANDLE;
#else /* !QUEUE_TABLE */
typedef MEM_HANDLE QUEUE_HANDLE;
#endif /* QUEUE_TABLE */

typedef UWORD OBJ_SIZE;
typedef QUEUE_HANDLE HLINK;

typedef struct link
{
	HLINK pred;
	HLINK succ;
} LINK;
typedef LINK *PLINK;

typedef PBYTE BYTEPTR;
typedef PUWORD WORDPTR;
typedef PVOID VOIDPTR;
typedef PLINK LINKPTR;

#ifdef QUEUE_TABLE
#define LockLink(pq, h) (LINKPTR)(h)
#define UnlockLink(pq, h)
#define GetFreeList(pq) (pq)->free_list
#define SetFreeList(pq, h) (pq)->free_list = (h)
#define AllocQueueTab(pq,n) \
		((pq)->hq_tab = mem_allocate ((MEM_SIZE)((COUNT)(pq)->object_size * \
		(COUNT)((pq)->num_objects = (BYTE)(n))), \
		MEM_PRIMARY, DEFAULT_MEM_PRIORITY, MEM_SIMPLE))
#define LockQueueTab(pq) ((pq)->pq_tab = (BYTEPTR)mem_lock ((pq)->hq_tab))
#define UnlockQueueTab(pq) mem_unlock ((pq)->hq_tab)
#define FreeQueueTab(pq) mem_release ((pq)->hq_tab); (pq)->hq_tab = 0
#define SizeQueueTab(pq) (COUNT)((pq)->num_objects)
#define GetLinkAddr(pq,i) (HLINK)((pq)->pq_tab + ((pq)->object_size * ((i) - 1)))
#else /* !QUEUE_TABLE */
#define AllocLink(pq) (HLINK)mem_request ((pq)->object_size)
#define LockLink(pq, h) (LINKPTR)mem_lock (h)
#define UnlockLink(pq, h) mem_unlock (h)
#define FreeLink(pq,h) mem_release (h)
#endif /* QUEUE_TABLE */

typedef struct queue
{
	HLINK head;
	HLINK tail;
#ifdef QUEUE_TABLE
	BYTEPTR pq_tab;
	HLINK free_list;
	MEM_HANDLE hq_tab;
#endif
//    BYTE object_size;
	COUNT object_size;
#ifdef QUEUE_TABLE
	BYTE num_objects;
#endif /* QUEUE_TABLE */
} QUEUE;
typedef QUEUE *PQUEUE;

// #define SetLinkSize(pq,s) ((pq)->object_size = (BYTE)(s))
#define SetLinkSize(pq,s) ((pq)->object_size = (COUNT)(s))
#define GetLinkSize(pq) (COUNT)((pq)->object_size)
#define GetHeadLink(pq) ((pq)->head)
#define SetHeadLink(pq,h) ((pq)->head = (h))
#define GetTailLink(pq) ((pq)->tail)
#define SetTailLink(pq,h) ((pq)->tail = (h))
#define _GetPredLink(lpE) ((lpE)->pred)
#define _SetPredLink(lpE,h) ((lpE)->pred = (h))
#define _GetSuccLink(lpE) ((lpE)->succ)
#define _SetSuccLink(lpE,h) ((lpE)->succ = (h))

extern BOOLEAN InitQueue (PQUEUE pq, COUNT num_elements, OBJ_SIZE size);
extern BOOLEAN UninitQueue (PQUEUE pq);
extern void ReinitQueue (PQUEUE pq);
extern void PutQueue (PQUEUE pq, HLINK hLink);
extern void InsertQueue (PQUEUE pq, HLINK hLink, HLINK hRefLink);
extern void RemoveQueue (PQUEUE pq, HLINK hLink);
extern COUNT CountLinks (PQUEUE pq);
void ForAllLinks(PQUEUE pq, void (*callback)(LINKPTR, void *), void *arg);
#ifdef QUEUE_TABLE
extern HLINK AllocLink (PQUEUE pq);
extern void FreeLink (PQUEUE pq, HLINK hLink);
#endif /* QUEUE_TABLE */

#endif /* _DISPLIST_H */

