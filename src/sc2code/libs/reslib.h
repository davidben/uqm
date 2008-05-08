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

//#include <stdio.h>
#include "compiler.h"
#include "port.h"
#include "misc.h"
#include "libs/uio.h"

typedef struct resource_index_desc RESOURCE_INDEX_DESC;
typedef RESOURCE_INDEX_DESC *RESOURCE_INDEX;

typedef const char *RESOURCE;

typedef enum
{
	UNKNOWNRES = 0,
	GFXRES,
	FONTRES,
	STRTAB,
	BINTAB,
	SNDRES,
	MUSICRES,
	CODE
} RES_TYPE;

#define NULL_RESOURCE NULL

extern const char *_cur_resfile_name;

typedef void *(ResourceLoadFun) (uio_Stream *fp, DWORD len);
typedef BOOLEAN (ResourceFreeFun) (void *handle);

extern uio_Stream *res_OpenResFile (uio_DirHandle *dir, const char *filename,
		const char *mode);
extern int ReadResFile (void *lpBuf, COUNT size, COUNT count, uio_Stream *fp);
extern int WriteResFile (const void *lpBuf, COUNT size, COUNT count,
		uio_Stream *fp);
extern int GetResFileChar (uio_Stream *fp);
extern int PutResFileChar (char ch, uio_Stream *fp);
extern int PutResFileNewline (uio_Stream *fp);
extern long SeekResFile (uio_Stream *fp, long offset, int whence);
extern long TellResFile (uio_Stream *fp);
extern long LengthResFile (uio_Stream *fp);
extern BOOLEAN res_CloseResFile (uio_Stream *fp);
extern BOOLEAN DeleteResFile (uio_DirHandle *dir, const char *filename);

extern RESOURCE_INDEX InitResourceSystem ();
extern void UninitResourceSystem (void);
extern BOOLEAN InstallResTypeVectors (RES_TYPE res_type,
		ResourceLoadFun *loadFun, ResourceFreeFun *freeFun);
extern void *res_GetResource (RESOURCE res);
extern void *res_DetachResource (RESOURCE res);
extern BOOLEAN FreeResource (RESOURCE res);
extern COUNT CountResourceTypes (void);

extern void LoadResourceIndex (uio_DirHandle *dir, const char *filename);

extern void *GetResourceData (uio_Stream *fp, DWORD length);

#define AllocResourceData HMalloc
extern BOOLEAN FreeResourceData (void *);

#include "strlib.h"

typedef STRING_TABLE DIRENTRY_REF;
typedef STRING DIRENTRY;
typedef STRINGPTR DIRENTRYPTR;

extern DIRENTRY_REF LoadDirEntryTable (uio_DirHandle *dirHandle,
		const char *path, const char *pattern, match_MatchType matchType);
#define CaptureDirEntryTable CaptureStringTable
#define ReleaseDirEntryTable ReleaseStringTable
#define DestroyDirEntryTable DestroyStringTable
#define GetDirEntryTableRef GetStringTable
#define GetDirEntryTableCount GetStringTableCount
#define GetDirEntryTableIndex GetStringTableIndex
#define SetAbsDirEntryTableIndex SetAbsStringTableIndex
#define SetRelDirEntryTableIndex SetRelStringTableIndex
#define GetDirEntryLength GetStringLengthBin
#define GetDirEntryAddress GetStringAddress
#define GetDirEntryContents GetStringContents

/* Key-Value resources */
void res_ClearTables (void);

void res_LoadFilename (uio_DirHandle *path, const char *fname);
void res_SaveFilename (uio_DirHandle *path, const char *fname, const char *root);

void res_LoadFile (uio_Stream *fname);
void res_SaveFile (uio_Stream *fname, const char *root);

BOOLEAN res_HasKey (const char *key);

const char *res_GetString (const char *key);
void res_PutString (const char *key, const char *value);

BOOLEAN res_IsInteger (const char *key);
int res_GetInteger (const char *key);
void res_PutInteger (const char *key, int value);

BOOLEAN res_IsBoolean (const char *key);
BOOLEAN res_GetBoolean (const char *key);
void res_PutBoolean (const char *key, BOOLEAN value);

BOOLEAN res_Remove (const char *key);

#endif /* _RESLIB_H */
