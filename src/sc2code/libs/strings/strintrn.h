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

#ifndef _STRINTRN_H
#define _STRINTRN_H

#include <stdio.h>
#include <string.h>
#include "strlib.h"
#include "memlib.h"
#include "reslib.h"

typedef struct string_table
{
	unsigned short StringCount;
	unsigned short flags;
	DWORD StringOffsets[1];
} STRING_TABLE_DESC;
typedef STRING_TABLE_DESC *PSTRING_TABLE_DESC;

#define HAS_SOUND_CLIPS (1 << 0)
#define HAS_TIMESTAMP (1 << 1)
#define STRING_TABLEPTR PSTRING_TABLE_DESC

#define AllocStringTable(s) AllocResourceData((s),MEM_SOUND)
#define LockStringTable(h,ps) LockResourceData((MEM_HANDLE)LOWORD(h),ps)
#define UnlockStringTable(h) UnlockResourceData ((MEM_HANDLE)LOWORD(h))
#define FreeStringTable(h) FreeResourceData ((MEM_HANDLE)LOWORD(h))

#define STRING_INDEX(S) ((COUNT)HIWORD (S))
#define BUILD_STRING(h,i) ((STRING_TABLE)MAKE_DWORD(h,i))
#define BUILD_STRING_TABLE(h) (STRING_TABLE)(h)

extern MEM_HANDLE _GetStringData (uio_Stream *fp, DWORD length);

#endif /* _STRINTRN_H */

