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

#ifndef _VIDLIB_H
#define _VIDLIB_H

#include "compiler.h"

typedef enum
{
	NO_FMV = 0,
	HARDWARE_FMV,
	SOFTWARE_FMV
} VIDEO_TYPE;

#define MODES_UNDEFINED ((BYTE)~0)

typedef DWORD VIDEO_REF;

extern BOOLEAN InitVideo (BOOLEAN UseCDROM);
extern void UninitVideo (void);

extern BYTE SetVideoMode (BYTE new_mode);
extern BYTE GetVideoMode (void);

extern VIDEO_REF LoadVideoFile (const char *pStr);
extern BOOLEAN DestroyVideo (VIDEO_REF VideoRef);
extern VIDEO_TYPE VidPlay (VIDEO_REF VidRef, const char *loopname,
		BOOLEAN uninit);
extern void VidStop (void);
extern VIDEO_REF VidPlaying (void);
extern void VidDoInput (void);

#endif /* _VIDLIB_H */
