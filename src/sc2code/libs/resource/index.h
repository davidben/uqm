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

#ifndef _INDEX_H
#define _INDEX_H

#include <stdio.h>

typedef struct
{
	MEM_HANDLE (*load_func) (uio_Stream *fp, DWORD len);
	BOOLEAN (*free_func) (MEM_HANDLE handle);
} RES_VECTORS;

typedef struct
{
	RES_INSTANCE instance_count;
	RES_VECTORS func_vectors;
} TYPE_DESC;
typedef TYPE_DESC *PTYPE_DESC;

typedef struct
{
	RESOURCE packmem_info;
	DWORD flags_and_data_loc;
			/* data_loc doubles as 3 byte offset into package file or 2 byte
			 * MEM_HANDLE to package member array with the offset stored in
			 * the MEM_HANDLE (cuts down on use of near memory).
			 */
} PACKAGE_DESC;
typedef PACKAGE_DESC *PPACKAGE_DESC;

typedef BYTE RES_FLAGS;
#define IS_PACKAGED ((RES_FLAGS)(1 << 0))

typedef struct
{
	uio_Stream *res_fp;

	DWORD packmem_list_offs;
	DWORD path_list_offs;
	DWORD file_list_offs;

	RES_PACKAGE num_packages;
	RES_TYPE num_types;
	RES_FLAGS res_flags;

	union
	{
		DWORD header_len;
		struct
		{
			PPACKAGE_DESC package_list;
			PTYPE_DESC type_list;
		} lists;
	} index_info;

#ifndef PACKAGING
	char index_file_name[36];
	DWORD data_offs;
	MEM_HANDLE hPredHeader, hSuccHeader;
#endif /* PACKAGING */
} INDEX_HEADER;
typedef INDEX_HEADER *PINDEX_HEADER;

typedef PINDEX_HEADER INDEX_HEADERPTR;

void forAllResourceIndices(
		void (*callback)(INDEX_HEADERPTR ResHeaderPtr, void *extra),
		void *extra);

#define PackageList index_info.lists.package_list
#define TypeList index_info.lists.type_list

#define INDEX_HEADER_PRIORITY DEFAULT_MEM_PRIORITY
#define AllocResourceHeader(s) \
		mem_allocate ((s), MEM_ZEROINIT | MEM_PRIMARY, \
		INDEX_HEADER_PRIORITY, MEM_SIMPLE)
#define LockResourceHeader (INDEX_HEADERPTR)mem_lock
#define UnlockResourceHeader mem_unlock
#define FreeResourceHeader mem_release

typedef struct
{
	BYTE path_offset[2];
	char filename[8];
	char extension[3];
} FILE_INFO;

#define FILE_NAME_SIZE 8
#define EXTENSION_SIZE 3
#define NO_PATH 0xFFFF

#define FILE_LIST_SIZE \
		((sizeof (BYTE) * 2) + FILE_NAME_SIZE + EXTENSION_SIZE)
#define PACKMEM_LIST_SIZE \
		(sizeof (DWORD))
#define INSTANCE_LIST_SIZE \
		(sizeof (BYTE) * 2)

#endif /* _INDEX_H */

