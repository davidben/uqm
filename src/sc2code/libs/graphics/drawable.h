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

#ifndef _DRAWABLE_H
#define _DRAWABLE_H

#include <stdio.h>
#include "tfb_draw.h"

#define ValidPrimType(pt) ((pt)<NUM_PRIMS)

typedef struct bresenham_line
{
	POINT first, second;
	SIZE abs_delta_x, abs_delta_y;
	SIZE error_term;
	BOOLEAN end_points_exchanged;
	INTERSECT_CODE intersect_code;
} BRESENHAM_LINE;
typedef BRESENHAM_LINE *PBRESENHAM_LINE;

#define BYTE_WIDTH(w) (((w) + 7) >> 3)
#define BYTE_OFFS(x) ((BYTE)(x) & (BYTE)0x7)
#define BYTE_POS(x) ((x) >> 3)
#define WORD_WIDTH(w) (((w) + 15) >> 4)
#define WORD_OFFS(x) ((UWORD)(x) & (UWORD)0xF)
#define WORD_POS(x) ((x) >> 4)
#ifdef MSDOS
#define SCAN_WIDTH BYTE_WIDTH
#define PAD_WIDTH(w) ((w)+((sizeof (BYTE) << 3) - 1))
#else /* !MSDOS */
#define SCAN_WIDTH(w) ((BYTE_WIDTH(w)+1)&~1)
#define PAD_WIDTH(w) (w)
#endif /* MSDOS */

#define FTYPE_SHIFT 12
#define FINDEX_MASK ((1 << FTYPE_SHIFT) - 1)
#define FTYPE_MASK (0xFFFF & ~FINDEX_MASK)

#define ROM_DRAWABLE (0 << FTYPE_SHIFT)
#define RAM_DRAWABLE (1 << FTYPE_SHIFT)
#define SCREEN_DRAWABLE (2 << FTYPE_SHIFT)

typedef UWORD DRAWABLE_TYPE;

#define DATA_HARDWARE (1 << 12)
#define DATA_COPY (1 << 13)
#define DATA_PACKED (1 << 14)
#define X_FLIP (1 << 15)

typedef struct
{
	DWORD TypeIndexAndFlags;
	HOT_SPOT HotSpot;
	DWORD Bounds;
	TFB_Image *image;
	struct _drawable_desc *parent;
} FRAME_DESC;
typedef FRAME_DESC *PFRAME_DESC;

typedef struct _drawable_desc
{
	MEM_HANDLE hDrawable;

	UWORD FlagsAndIndex;
	FRAME_DESC *Frame;
} DRAWABLE_DESC;
typedef DRAWABLE_DESC *PDRAWABLE_DESC;

#define GetFrameWidth(f) LOWORD (((FRAMEPTR)(f))->Bounds)
#define GetFrameHeight(f) HIWORD (((FRAMEPTR)(f))->Bounds)
#define SetFrameBounds(f,w,h) (((FRAMEPTR)(f))->Bounds=MAKE_DWORD(w,h))
#define GetFrameFlags(f) HIWORD(((FRAMEPTR)(f))->TypeIndexAndFlags)
#define AddFrameFlags(f,v) (((FRAMEPTR)(f))->TypeIndexAndFlags|=((DWORD)(v)<<16))
#define SubFrameFlags(f,v) (((FRAMEPTR)(f))->TypeIndexAndFlags&=~((DWORD)(v)<<16))

#define DRAWABLE_PRIORITY DEFAULT_MEM_PRIORITY

#define DRAWABLEPTR PDRAWABLE_DESC
#define FRAMEPTR PFRAME_DESC
#define COUNTPTR PCOUNT

extern DRAWABLE AllocDrawable (COUNT num_frames);
#define LockDrawable(D) ((DRAWABLEPTR)mem_lock (GetDrawableHandle (D)))
#define UnlockDrawable(D) mem_unlock (GetDrawableHandle (D))
#define FreeDrawable(D) _ReleaseCelData (GetDrawableHandle (D))
#define GetDrawableHandle(D) ((MEM_HANDLE)LOWORD (D))
#define GetDrawableIndex(D) ((COUNT)HIWORD (D))
#define GetFrameParentDrawable(F) (F)->parent

#define NULL_DRAWABLE (DRAWABLE)NULL_PTR

#define TYPE_GET(f) ((f) & FTYPE_MASK)
#define INDEX_GET(f) ((f) & FINDEX_MASK)
#define TYPE_SET(f,t) ((f)|=t)
#define INDEX_SET(f,i) ((f)|=i)

typedef struct
{
	RECT Box;
	FRAMEPTR FramePtr;
} IMAGE_BOX;
typedef IMAGE_BOX *PIMAGE_BOX;

extern DRAWABLE _request_drawable (COUNT NumFrames, DRAWABLE_TYPE
		DrawableType, CREATE_FLAGS flags, SIZE width, SIZE height);
extern INTERSECT_CODE _clip_line (PRECT pClipRect, PBRESENHAM_LINE
		pLine);

extern MEM_HANDLE _GetCelData (uio_Stream *fp, DWORD length);
extern BOOLEAN _ReleaseCelData (MEM_HANDLE handle);

typedef PPRIMITIVE PRIMITIVEPTR;
typedef PPOINT POINTPTR;
typedef PRECT RECTPTR;
typedef PSTAMP STAMPPTR;
typedef PTEXT TEXTPTR;

extern STAMP _save_stamp;
extern FRAMEPTR _CurFramePtr;

extern void _rect_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr);
extern void _text_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr);

#endif /* _DRAWABLE_H */

