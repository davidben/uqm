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

#ifndef _CODERES_H
#define _CODERES_H

#include "reslib.h"

extern MEM_HANDLE LoadCodeResFile (PSTR pStr);
extern BOOLEAN InstallCodeResType (COUNT code_type);
extern MEM_HANDLE LoadCodeResInstance (RESOURCE res);
extern PVOID CaptureCodeRes (MEM_HANDLE hCode, PVOID pData, PVOID *ppLocData);
extern MEM_HANDLE ReleaseCodeRes (PVOID CodeRef);
extern BOOLEAN DestroyCodeRes (MEM_HANDLE hCode);

typedef struct
{
	MEM_HANDLE hCode;
	UWORD size;
} CODE_REF;
typedef CODE_REF *PCODE_REF;

#endif /* _CODERES_H */

