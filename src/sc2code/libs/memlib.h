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

#ifndef _MEMLIB_H
#define _MEMLIB_H

#include "compiler.h"

extern BOOLEAN mem_init (void);
extern BOOLEAN mem_uninit (void);

#ifdef LEGACY_HANDLE_ALLOCATOR

#define NULL_HANDLE (MEM_HANDLE)(0L)

typedef signed long MEM_SIZE;

typedef SWORD MEM_HANDLE;
typedef UWORD MEM_FLAGS;

#define DEFAULT_MEM_FLAGS (MEM_FLAGS)0
#define MEM_PRIMARY (MEM_FLAGS)(1 << 8)
#define MEM_ZEROINIT (MEM_FLAGS)(1 << 9)
#define MEM_GRAPHICS (MEM_FLAGS)(1 << 10)
#define MEM_SOUND (MEM_FLAGS)(1 << 11)

typedef struct mem_header {
	MEM_HANDLE handle;
} _ALIGNED_ANY MEM_HEADER;
// _ALIGNED_ANY adds padding to the end of MEM_HEADER, so that the following
// address is aligned for any object (if the address of the MEM_HEADER
// itself is aligned for any object).

#define GET_MEM_HEADER(addr) ((MEM_HEADER *) \
		(((char *) addr) - sizeof (MEM_HEADER)))

extern MEM_HANDLE mem_allocate (MEM_SIZE size, MEM_FLAGS flags);
extern BOOLEAN mem_release (MEM_HANDLE handle);

extern void* mem_lock (MEM_HANDLE handle);
extern BOOLEAN mem_unlock (MEM_HANDLE handle);

extern MEM_SIZE mem_get_size (MEM_HANDLE handle);

#endif /* LEGACY_HANDLE_ALLOCATOR */

#endif /* _MEMLIB_H */

