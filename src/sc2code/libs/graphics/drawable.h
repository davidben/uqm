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

typedef UWORD DRAWABLE_TYPE;
#define ROM_DRAWABLE 0
#define RAM_DRAWABLE 1
#define SCREEN_DRAWABLE 2

typedef struct frame_desc
{
	DRAWABLE_TYPE Type;
	UWORD Index;
	HOT_SPOT HotSpot;
	EXTENT Bounds;
	TFB_Image *image;
	struct drawable_desc *parent;
} FRAME_DESC;
typedef FRAME_DESC *PFRAME_DESC;

typedef struct drawable_desc
{
	MEM_HANDLE hDrawable;

	CREATE_FLAGS Flags;
	UWORD MaxIndex;
	FRAME_DESC *Frame;
} DRAWABLE_DESC;
typedef DRAWABLE_DESC *PDRAWABLE_DESC;

#define GetFrameWidth(f) (((PFRAME_DESC)(f))->Bounds.width)
#define GetFrameHeight(f) (((PFRAME_DESC)(f))->Bounds.height)
#define SetFrameBounds(f,w,h) \
		(((PFRAME_DESC)(f))->Bounds.width=(w), \
		((PFRAME_DESC)(f))->Bounds.height=(h))

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

