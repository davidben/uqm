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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
//#include "3d.h"
//#include "tool.h"
//#include "utils.h"
//#include "pcwin.h"
#include "compiler.h"
#include "libs/threadlib.h"
#include "libs/memlib.h"

#define GetToolFrame() 1

//#define MEM_DEBUG

#ifdef MEM_DEBUG
#include <crtdbg.h>
#endif

#define LEAK_DEBUG

#ifdef LEAK_DEBUG
BOOLEAN leak_debug;
int leak_idx = -1;
int leak_size = -1;
#endif
static Mutex _MemoryLock;

//#define MAX_EXTENTS 100000
#define MAX_EXTENTS (MEM_HANDLE) 32766
		// Changed to 32766 to get rid of warnings that an expression
		// 'h <= MAX_EXTENTS' is always true due to limited range of
		// MEM_HANDLE.

/* Keep track of memory allocations. */

typedef struct _szMemoryNode {
	void *memory;
	struct _szMemoryNode *next;
	MEM_HANDLE handle;
	MEM_SIZE size;
	int refcount;
} szMemoryNode;

static szMemoryNode extents[MAX_EXTENTS];
static szMemoryNode *freeListHead = NULL;

/*****************************************************************************/
/*FUNCTION
**
** SYNOPSIS
**      Message(format, parms)
**
** DESCRIPTION
** Presents a windows message box to the user and echoes the
** message with printf().
**
** INPUT
** As for printf().
**
** OUTPUT
**
** HISTORY
**      10-Jul-96:AKL Creation.
**
** ASSUMPTIONS
**END*/

static void Message(char *fmt, ...)
{
		va_list ap;
		char buffer[256];


		va_start(ap, fmt);
		vsprintf(buffer, fmt, ap);
		va_end(ap);

#ifndef FINAL
		fprintf (stderr, "%s\n", buffer);
		//fflush (stderr);
#endif

#if 0
	if (GetToolFrame ())
	{
		ShowWindow(GetToolFrame (),SW_HIDE);
	}
		MessageBox(NULL, buffer, "Message From Programmer Dude", MB_OK);

	if (GetToolFrame ())
	{
		ShowWindow(GetToolFrame (),SW_NORMAL);
	}
#endif
}

#if 0
/*****************************************************************************/
/*FUNCTION
**
** SYNOPSIS
**      ok = MessageWithRetry(format, parms)
**
** DESCRIPTION
** Presents a windows message box to the user and echoes the
** message with printf(). The users gets to choose OK or CANCEL.
** Returns TRUE if the user says OK.
**
** INPUT
** As for printf().
**
** OUTPUT
** int ok = TRUE for OK, FALSE for CANCEL.
**
** HISTORY
**      6-Nov-96:AKL Creation.
**
** ASSUMPTIONS
**END*/

static int MessageWithRetry(char *fmt, ...)
{
		va_list ap;
		char buffer[256];
#if 0
		int ret;
#endif


		va_start(ap, fmt);
		vsprintf(buffer, fmt, ap);
		va_end(ap);

#ifndef FINAL
		fprintf (stderr, "%s\n", buffer);
		// fflush(stderr);
#endif

#if 0
	if (GetToolFrame ())
	{
		ShowWindow(GetToolFrame (),SW_HIDE);
	}
		ret = MessageBox(NULL, buffer, "Message From Programmer Dude",
												MB_OKCANCEL);

		/* Return value:
						0 => not enough memory for message. Abort game.
						IDCANCEL => user selected CANCEL.
						IDOK => user selected OK.
		*/

	if (GetToolFrame ())
	{
		ShowWindow(GetToolFrame (),SW_NORMAL);
	}

		if ( ret == IDOK )
				return TRUE;
		else
#endif
				return FALSE;
}
#endif

/*****************************************************************************/
/*FUNCTION
**
** SYNOPSIS
** CheckMemory()
**
** DESCRIPTION
** Checks integrity of memory allocation: diagnostic only.
** Either aborts or spits messages to stdout if there's a
** problem.
**
** INPUT
**
** OUTPUT
**
** HISTORY
**      04-Sept-96:AKL Creation.
**
** ASSUMPTIONS
**
**END*/

#ifdef MEM_DEBUG

void CheckMemory(void)
{

   // Send all reports to STDOUT
   _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
   _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
   _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
   _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
   _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
   _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );

		_CrtCheckMemory();
		fflush(stdout);
}
#endif

/*****************************************************************************/
/*FUNCTION
**
** SYNOPSIS
** mem = MallocWithRetry(coreSize, diagnostic)
**
** DESCRIPTION
** Mallocs the required memory. If the malloc fails, the user is prompted
** to shutdown other applications and retry, or kill the game.
** If the game is killed, the diagnostic string is printed.
**
** INPUT
** int coreSize = How much memory to malloc (in bytes).
** char *diagnostic = String identifying the caller.
**
** OUTPUT
** void *mem = Ptr to memory, or NULL if user wants to kill us.
**
** HISTORY
**      06-Nov-96:AKL Creation.
**
** ASSUMPTIONS
**
**END*/

void *
MallocWithRetry(int bytes, char *diagStr)
{
	while (1)
	{
		void *ptr;
		
		ptr = malloc (bytes);
		if (ptr)
			return (ptr);
#ifndef FINAL
		fprintf (stderr, "Malloc failed for %s. #Bytes %d.\n", diagStr, bytes);
//		fflush (stderr);
#endif

		abort();
#if 0
		/* The user gets a chance to close other applications and try again. */
		if (!MessageWithRetry ("I'm out of memory!  "
				"Please close other applications and click OK to try again"))
		{
			if (MessageWithRetry ("Really OK to stop tfbtool?"))
			{
				/* User says "die". */
				fprintf (stderr, "Killed by the user (%s).\n", diagStr);
				exit (0);
			}
		}
#endif
	}
}

MEM_HANDLE
mem_allocate (MEM_SIZE coreSize, MEM_FLAGS flags, MEM_PRIORITY priority,
		MEM_USAGE usage)
{
	szMemoryNode *node;

	LockMutex (_MemoryLock);

#ifdef MEM_DEBUG
	CheckMemory();
#endif

	if ((node = freeListHead) == NULL)
		Message ("mem_allocate: out of extents.");
	else if ((node->memory = MallocWithRetry (coreSize, "mem_allocate:")) == 0
			&& coreSize)
		Message ("mem_allocate: couldn't allocate %u bytes.", coreSize);
	else
	{
		freeListHead = node->next;

		node->size = coreSize;
		node->handle = node - extents + 1;
		node->refcount = 0;
		if (flags & MEM_ZEROINIT)
				memset (node->memory, 0, node->size);
#ifdef LEAK_DEBUG
		if (leak_debug)
		{
			fprintf (stderr, "alloc %d: %p, %lu\n", (int) node->handle,
					(void *) node->memory, node->size);
			// Prefered form:
			//fprintf (stderr, "alloc %d: %#8" PRIxPTR ", %lu\n",
			//		(int) node->handle,	(intptr_t) node->memory, node->size);
		}
		if (node->handle == leak_idx && node->size == leak_size)
			node = node;
		if (node->size == leak_size)
			node = node;
#endif /* LEAK_DEBUG */

		UnlockMutex (_MemoryLock);
		return (node->handle);
	}

	UnlockMutex (_MemoryLock);
	(void) priority;  /* Satisfying compiler (unused parameter) */
	(void) usage;  /* Satisfying compiler (unused parameter) */
	return (0);
}

/*****************************************************************************/
/*FUNCTION
**
** SYNOPSIS
** size = mem_get_size(h)
**
** DESCRIPTION
**      Returns the number of bytes in the given memory allocation.
**
** INPUT
**      int h = Handle to memory allocation.
**
** OUTPUT
**      int size = Number of bytes in allocation or zero on error.
**
** HISTORY
**      09-Jul-96:AKL Creation. Original version in LIBS\MEMORY\DATAINFO.C.
**
** ASSUMPTIONS
**
**END*/

MEM_SIZE
mem_get_size (MEM_HANDLE h)
{
	LockMutex (_MemoryLock);
	if (h > 0 && h <= MAX_EXTENTS && extents[h - 1].handle == h)
	{
		UnlockMutex (_MemoryLock);
		return ((int)extents[h - 1].size);
	}
	UnlockMutex (_MemoryLock);

	return (0);
}

/*****************************************************************************/
/*FUNCTION
**
** SYNOPSIS
** done = mem_init(coreSize, pma, diskName)
**
** DESCRIPTION
** Initialize data that tracks allocation extents.
**
** INPUT
** int coreSize = IGNORED!
** int * pma = IGNORED!
** PSTR diskName = IGNORED!
**
** OUTPUT
**      int done = TRUE.
**
** HISTORY
**      09-Jul-96:AKL Creation. Original version in LIBS\MEMORY\INIT.C.
**
** ASSUMPTIONS
**
**END*/

MEM_BOOL
mem_init (void)
{
	int i;

	_MemoryLock = CreateMutex ("memory lock", SYNC_CLASS_RESOURCE);
	LockMutex (_MemoryLock);

	freeListHead = &extents[0];
	for (i=0; i<MAX_EXTENTS; i++)
	{
		extents[i].memory = NULL;
		extents[i].handle = -1;
		extents[i].next = &extents[i+1];
		extents[i].size = 0;
	}
	extents[MAX_EXTENTS-1].next = NULL;

	UnlockMutex (_MemoryLock);

	return TRUE;
}

/*****************************************************************************/
/*FUNCTION
**
** SYNOPSIS
** mem_uninit()
**
** DESCRIPTION
** Free allocated memory.
**
** INPUT
**
** OUTPUT
**
** HISTORY
**      14-Oct-96:AKL Creation.
**
** ASSUMPTIONS
**
**END*/

MEM_BOOL
mem_uninit(void)
{
	int i;

	LockMutex (_MemoryLock);

	for (i=0; i<MAX_EXTENTS; i++)
	{
		if (extents[i].handle != -1)
		{
			fprintf (stderr, "LEAK: unreleased extent %d: %p, %lu\n",
					extents[i].handle, (void *) extents[i].memory,
					extents[i].size);
			// Prefered form:
			//fprintf (stderr, "LEAK: unreleased extent %d: %#8" PRIxPTR
			//		", %lu\n", extents[i].handle,
			//		(intptr_t) extents[i].memory, extents[i].size);
			fflush (stderr);
			extents[i].handle = -1;
			if (extents[i].memory)
			{
				free (extents[i].memory);
				extents[i].memory = 0;
			}
		}
		extents[i].size = 0;
	}
	freeListHead = 0;

	UnlockMutex (_MemoryLock);

	return (TRUE);
}

/*****************************************************************************/
/*FUNCTION
**
** SYNOPSIS
** done = mem_release(h)
**
** DESCRIPTION
**      Frees the given memory.
**
** INPUT
** int h = Handle to memory to be released.
**
** OUTPUT
**      int done = TRUE.
**
** HISTORY
**      09-Jul-96:AKL Creation. Original version in LIBS\MEMORY\ALLOC.C.
**
** ASSUMPTIONS
**
**END*/

MEM_BOOL
mem_release(MEM_HANDLE h)
{
	if (h == 0)
		return (TRUE);

	LockMutex (_MemoryLock);

	--h;
	if (h < 0 || h >= MAX_EXTENTS)
		fprintf (stderr, "LEAK: attempt to release invalid extent %d\n", h);
	else if (extents[h].handle == -1)
		fprintf (stderr, "LEAK: attempt to release unallocated extent %d\n",h);
	else if (extents[h].refcount == 0)
	{
#ifdef LEAK_DEBUG
		if (leak_debug)
		{
			fprintf (stderr, "free %d: %p\n",
					extents[h].handle, (void *) extents[h].memory);
			// Prefered form:
			//fprintf (stderr, "free %d: %#8" PRIxPTR "\n",
			//		extents[h].handle, (intptr_t) extents[h].memory);
		}
#endif
		if (extents[h].memory)
		{
			free (extents[h].memory);
			extents[h].memory = 0;
		}
		extents[h].handle = -1;
		extents[h].next = freeListHead;
		freeListHead = &extents[h];
		UnlockMutex (_MemoryLock);
		return TRUE;
	}

	UnlockMutex (_MemoryLock);
	return FALSE;
}

/*****************************************************************************/
/*FUNCTION
**
** SYNOPSIS
** memp = mem_simple_access(h)
**
** DESCRIPTION
**      Obtains access to the given memory allocation. For now, we simply
** return the handle itself, because it is the actual address!
**
** INPUT
** int h = Handle to memory to be accessed.
**
** OUTPUT
**      void * memp = Ptr to memory.
**
** HISTORY
**      09-Jul-96:AKL Creation. Original version in LIBS\MEMORY\SIMPLE.C.
**
** ASSUMPTIONS
**
**END*/

void *
mem_simple_access(MEM_HANDLE h)
{
	LockMutex (_MemoryLock);

	if (h > 0 && h <= MAX_EXTENTS && extents[h - 1].handle == h)
	{
		++extents[h - 1].refcount;
		UnlockMutex (_MemoryLock);
		return (extents[h - 1].memory);
	}

	UnlockMutex (_MemoryLock);
	return (0);
}

/*****************************************************************************/
/*FUNCTION
**
** SYNOPSIS
** done = mem_simple_unaccess(h)
**
** DESCRIPTION
**      Release access to the given memory allocation. For now, we simply
** do nothing!
**
** INPUT
** int h = Handle to memory to be unaccessed.
**
** OUTPUT
**      int done = TRUE.
**
** HISTORY
**      09-Jul-96:AKL Creation. Original version in LIBS\MEMORY\SIMPLE.C.
**
** ASSUMPTIONS
**
**END*/

MEM_BOOL
mem_simple_unaccess(MEM_HANDLE h)
{
	LockMutex (_MemoryLock);
	if (h > 0 && h <= MAX_EXTENTS && extents[h - 1].handle == h)
	{
		if (extents[h - 1].refcount)
			--extents[h - 1].refcount;
		UnlockMutex (_MemoryLock);
		return (1);
	}

	UnlockMutex (_MemoryLock);
	return (0);
}

static void *
_alloc_mem (int size)
{
	void *p;
	int h;

	h = mem_allocate (sizeof (MEM_HEADER) + size, 0, 0, 0);
	p = (void *)mem_simple_access (h);
	if (p)
	{
		((MEM_HEADER *) p)->handle = h;
		p = (char *)p + sizeof (MEM_HEADER);
	}

	return (p);
}

void *
HMalloc (int size)
{
    void *p;

	if (size == 0) return (0);

    if ((p = _alloc_mem(size)) == NULL) {
        fprintf(stderr, "Fatal Error: HMalloc(): out of memory.\n");
#ifdef DEBUG
        abort();
#else
        exit(1);
#endif  /* #ifdef DEBUG */
    }
    return (p);
}

void
HFree (void *p)
{
	if (p)
	{
		MEM_HEADER *hdr;
		MEM_HANDLE h;

		hdr = GET_MEM_HEADER(p);
		h = hdr->handle;
		mem_simple_unaccess (h);
		mem_release (h);
	}
}

void *
HCalloc (int size)
{
	void *p;

	p = HMalloc (size);
	if (p)
		memset (p, 0, size);

	return (p);
}

void *
HRealloc (void *p, int size)
{
	void *np;
	int osize;

	if ((np = _alloc_mem (size)) && p)
	{
		MEM_HEADER *hdr;
		MEM_HANDLE h;

		hdr = GET_MEM_HEADER (p);
		h = hdr->handle;
		osize = mem_get_size (h) - sizeof (MEM_HEADER);
		if (size > osize)
			size = osize;
		memcpy (np, p, size);
		HFree (p);
	}

	return (np);
}

