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

#ifndef _COMPILER_H
#define _COMPILER_H

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

typedef unsigned char     BYTE;
typedef BYTE            *PBYTE;
typedef unsigned char     UBYTE;
typedef UBYTE           *PUBYTE;
typedef signed char       SBYTE;
typedef SBYTE           *PSBYTE;
typedef unsigned short    UWORD;
typedef UWORD           *PUWORD;
typedef signed short      SWORD;
typedef SWORD           *PSWORD;
typedef unsigned long     DWORD;
typedef DWORD           *PDWORD;
typedef signed long       SDWORD;
typedef SDWORD          *PSDWORD;

typedef SBYTE           *PSTR;

typedef void            *PVOID;

typedef UWORD             COUNT;
typedef COUNT           *PCOUNT;
typedef SWORD              SIZE;
typedef SIZE             *PSIZE;

typedef unsigned char   UNICODE;

#define NULL_PTR 0L

typedef enum
{
	FALSE = 0,
	TRUE
} BOOLEAN;
typedef BOOLEAN *PBOOLEAN;

typedef void     (*PVOIDFUNC) (void);
typedef BOOLEAN  (*PBOOLFUNC) (void);
typedef BYTE     (*PBYTEFUNC) (void);
typedef UWORD    (*PUWORDFUNC) (void);
typedef SWORD    (*PSWORDFUNC) (void);
typedef DWORD    (*PDWORDFUNC) (void);

#define MAKE_BYTE(lo, hi)   ((BYTE) (((BYTE) (hi) << (BYTE) 4) | (BYTE) (lo)))
#define LONIBBLE(x)  ((BYTE) ((BYTE) (x) & (BYTE) 0x0F))
#define HINIBBLE(x)  ((BYTE) ((BYTE) (x) >> (BYTE) 4))
#define MAKE_WORD(lo, hi)   ((UWORD) ((BYTE) (hi) << 8) | (BYTE) (lo))
#define LOBYTE(x)    ((BYTE) ((UWORD) (x)))
#define HIBYTE(x)    ((BYTE) ((UWORD) (x) >> 8))
#define MAKE_DWORD(lo, hi)  (((DWORD) (hi) << 16) | (UWORD) (lo))
#define LOWORD(x)    ((UWORD) ((DWORD) (x)))
#define HIWORD(x)    ((UWORD) ((DWORD) (x) >> 16))

#endif /* _COMPILER_H */

