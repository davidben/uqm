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

#ifndef _RESLIB_H
#define _RESLIB_H

#include <stdio.h>
#include "memlib.h"

typedef DWORD RESOURCE;
typedef RESOURCE *PRESOURCE;

typedef BYTE RES_TYPE;
typedef COUNT RES_INSTANCE;
typedef COUNT RES_PACKAGE;

#define TYPE_BITS 8
#define INSTANCE_BITS 13
#define PACKAGE_BITS 11

#define MAX_TYPES ((1 << TYPE_BITS) - 1) /* zero is invalid */
#define MAX_INSTANCES (1 << INSTANCE_BITS)
#define MAX_PACKAGES ((1 << PACKAGE_BITS) - 1) /* zero is invalid */

#define GET_TYPE(res) \
		((RES_TYPE)(res) & (RES_TYPE)((1 << TYPE_BITS) - 1))
#define GET_INSTANCE(res) \
		((RES_INSTANCE)((res) >> TYPE_BITS) & ((1 << INSTANCE_BITS) - 1))
#define GET_PACKAGE(res) \
		((RES_PACKAGE)((res) >> (TYPE_BITS + INSTANCE_BITS)) & \
		((1 << PACKAGE_BITS) - 1))
#define MAKE_RESOURCE(p,t,i) \
		(((RESOURCE)(p) << (TYPE_BITS + INSTANCE_BITS)) | \
		((RESOURCE)(i) << TYPE_BITS) | \
		((RESOURCE)(t)))

extern FILE *res_OpenResFile (PVOID filename, const char *mode);
extern int ReadResFile (PVOID lpBuf, COUNT size, COUNT count, FILE *fp);
extern int WriteResFile (PVOID lpBuf, COUNT size, COUNT count, FILE
		*fp);
extern int GetResFileChar (FILE *fp);
extern int PutResFileChar (char ch, FILE *fp);
extern long SeekResFile (FILE *fp, long offset, int whence);
extern long TellResFile (FILE *fp);
extern long LengthResFile (FILE *fp);
extern BOOLEAN res_CloseResFile (FILE *fp);
extern BOOLEAN DeleteResFile (PVOID filename);

extern MEM_HANDLE InitResourceSystem (PVOID resfile, COUNT resindex_type,
		BOOLEAN (*FileErrorFunc) (PVOID filename));
extern BOOLEAN UninitResourceSystem (void);
extern BOOLEAN InstallResTypeVectors (
		COUNT res_type, 
		MEM_HANDLE (*load_func) (FILE *fp, DWORD len),
		BOOLEAN (*free_func) (MEM_HANDLE handle));
extern MEM_HANDLE res_GetResource (RESOURCE res);
extern MEM_HANDLE res_DetachResource (RESOURCE res);
extern BOOLEAN FreeResource (RESOURCE res);
extern COUNT CountResourceTypes (void);

extern MEM_HANDLE OpenResourceIndexFile (PVOID
		resfile);
extern MEM_HANDLE OpenResourceIndexInstance (DWORD
		res);
extern MEM_HANDLE SetResourceIndex (MEM_HANDLE hRH);
extern BOOLEAN CloseResourceIndex (MEM_HANDLE hRH);

extern MEM_HANDLE GetResourceData (FILE *fp, DWORD
		length, MEM_FLAGS mem_flags);

#define RESOURCE_PRIORITY DEFAULT_MEM_PRIORITY
#define RESOURCE_DATAPTR PBYTE

#define AllocResourceData(s,mf) \
	mem_allocate ((MEM_SIZE)(s), (mf), RESOURCE_PRIORITY, MEM_SIMPLE)
#define LockResourceData(h,lp) \
do \
{ \
	*(lp) = mem_lock ((MEM_HANDLE)(h)); \
} while (0)
#define UnlockResourceData mem_unlock
#define FreeResourceData mem_release

#include "strlib.h"

typedef STRING_TABLE DIRENTRY_REF;
typedef STRING DIRENTRY;
typedef STRINGPTR DIRENTRYPTR;

extern DIRENTRY_REF LoadDirEntryTable (PSTR pattern,
		PCOUNT pnum_entries);
#define CaptureDirEntryTable CaptureStringTable
#define ReleaseDirEntryTable ReleaseStringTable
#define DestroyDirEntryTable DestroyStringTable
#define GetDirEntryTableRef GetStringTable
#define GetDirEntryTableCount GetStringTableCount
#define GetDirEntryTableIndex GetStringTableIndex
#define SetAbsDirEntryTableIndex SetAbsStringTableIndex
#define SetRelDirEntryTableIndex SetRelStringTableIndex
#define GetDirEntryLength GetStringLength
#define GetDirEntryAddress GetStringAddress
#define GetDirEntryContents GetStringContents

#endif /* _RESLIB_H */

