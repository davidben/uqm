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

#define MEM_NULL 0L
#define NULL_HANDLE (MEM_HANDLE)MEM_NULL

typedef signed long MEM_SIZE;
typedef MEM_SIZE *PMEM_SIZE;
typedef long MEM_OFFS;

typedef SWORD MEM_HANDLE;
typedef MEM_HANDLE *PMEM_HANDLE;

typedef BOOLEAN MEM_BOOL;
#define MEM_FAILURE FALSE
#define MEM_SUCCESS TRUE

enum
{
	SAME_MEM_USAGE = 0,
	MEM_SIMPLE,
	MEM_COMPLEX
};
typedef BYTE MEM_USAGE;

#define NO_MEM_USAGE (MEM_USAGE)SAME_MEM_USAGE
#define DEFAULT_MEM_USAGE (MEM_USAGE)MEM_SIMPLE

enum
{
	MEM_NO_ACCESS = 0,
	MEM_READ_ONLY,
	MEM_WRITE_ONLY,
	MEM_READ_WRITE
};
typedef BYTE MEM_ACCESS_MODE;

enum
{
	MEM_SEEK_REL,
	MEM_SEEK_ABS
};
typedef BYTE MEM_SEEK_MODE;

enum
{
	MEM_FORWARD,
	MEM_BACKWARD
};
typedef BYTE MEM_DIRECTION;

typedef struct mem_size_request
{
	MEM_SIZE min_size, size, max_size;
	MEM_DIRECTION direction;
} MEM_SIZE_REQUEST;

typedef MEM_SIZE_REQUEST *PMEM_SIZE_REQUEST;
typedef SBYTE MEM_PRIORITY;
typedef UWORD MEM_FLAGS;

#define DEFAULT_MEM_FLAGS (MEM_FLAGS)0
#define SAME_MEM_FLAGS (MEM_FLAGS)0
#define MEM_CONSTRAINED (MEM_FLAGS)(1 << 2)
#define MEM_DISCARDABLE (MEM_FLAGS)(1 << 3)
#define MEM_NODISCARD (MEM_FLAGS)(1 << 4)
#define MEM_DISCARDED (MEM_FLAGS)(1 << 5)
#define MEM_ACCESSED (MEM_FLAGS)(1 << 6)
#define MEM_STATIC (MEM_FLAGS)(1 << 7)
#define MEM_PRIMARY (MEM_FLAGS)(1 << 8)
#define MEM_ZEROINIT (MEM_FLAGS)(1 << 9)
#define MEM_GRAPHICS (MEM_FLAGS)(1 << 10)
#define MEM_SOUND (MEM_FLAGS)(1 << 11)

#define DEFAULT_MEM_PRIORITY (MEM_PRIORITY)0
#define SAME_MEM_PRIORITY (MEM_PRIORITY)0
#define HIGHEST_MEM_PRIORITY (MEM_PRIORITY)1
#define LOWEST_MEM_PRIORITY (MEM_PRIORITY)100

typedef struct mem_header {
	MEM_HANDLE handle;
} _ALIGNED_ANY MEM_HEADER;

#define GET_MEM_HEADER(addr) ((MEM_HEADER *) \
		(((char *) addr) - sizeof (MEM_HEADER)))

//Newer verion from w_memlib.c to follow...
/*
extern MEM_BOOL mem_init (MEM_SIZE core_size, PMEM_SIZE pmin_addressable,
		PSTR disk_name);*/

extern MEM_BOOL mem_init (void);

extern MEM_BOOL mem_uninit (void);

extern MEM_HANDLE mem_allocate (MEM_SIZE size, MEM_FLAGS flags,
		MEM_PRIORITY priority, MEM_USAGE usage);
#define mem_request(size) \
		mem_allocate((MEM_SIZE)(size), DEFAULT_MEM_FLAGS, DEFAULT_MEM_PRIORITY, DEFAULT_MEM_USAGE)
extern MEM_BOOL mem_release (MEM_HANDLE handle);
extern MEM_HANDLE mem_reallocate (MEM_HANDLE handle, MEM_SIZE size,
		MEM_FLAGS flags, MEM_PRIORITY priority, MEM_USAGE usage);
#define mem_resize(h, size) \
		mem_reallocate(h, (MEM_SIZE)size, SAME_MEM_FLAGS, SAME_MEM_PRIORITY, SAME_MEM_USAGE)
extern MEM_HANDLE mem_static (PVOID pVoid, MEM_SIZE size, MEM_USAGE
		usage);
extern MEM_HANDLE mem_dup (MEM_HANDLE handle, MEM_USAGE usage);

extern PVOID mem_simple_access (MEM_HANDLE handle);
extern MEM_BOOL mem_simple_unaccess (MEM_HANDLE handle);

#define mem_lock mem_simple_access
#define mem_unlock mem_simple_unaccess

extern PVOID mem_complex_access (MEM_HANDLE handle, MEM_SIZE pos,
		PMEM_SIZE_REQUEST pSizeRequest, MEM_ACCESS_MODE access_mode);
extern MEM_BOOL mem_complex_unaccess (MEM_HANDLE handle);
extern PVOID mem_complex_seek (MEM_HANDLE handle, MEM_OFFS offset,
		PMEM_SIZE_REQUEST pSizeRequest, MEM_SEEK_MODE seek_mode);

extern MEM_SIZE mem_get_memavailable (MEM_SIZE size, MEM_PRIORITY
		priority);

extern MEM_SIZE mem_get_size (MEM_HANDLE handle);
extern MEM_USAGE mem_get_usage (MEM_HANDLE handle);
extern MEM_BOOL mem_set_flags (MEM_HANDLE handle, MEM_FLAGS mem_flags);
extern MEM_FLAGS mem_get_flags (MEM_HANDLE handle);
extern MEM_BOOL mem_set_priority (MEM_HANDLE handle, MEM_PRIORITY
		mem_priority);
extern MEM_PRIORITY mem_get_priority (MEM_HANDLE handle);

#endif /* _MEMLIB_H */

