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

#ifndef _DISPLAY_H
#define _DISPLAY_H

/*
#define WANT_MASK (CREATE_FLAGS)(1 << 0)
#define WANT_PIXMAP (CREATE_FLAGS)(1 << 1)
*/
#define WANT_COMPRESSED (CREATE_FLAGS)(1 << 2)
#define DOUBLE_RES (CREATE_FLAGS)(1 << 3)

typedef struct
{
	CREATE_FLAGS DisplayFlags;

	BYTE DisplayDepth;
	COUNT DisplayWidth, DisplayHeight;

	DRAWABLE (*alloc_image) (COUNT NumFrames, DRAWABLE_TYPE DrawableType,
			CREATE_FLAGS flags, SIZE width, SIZE height);

	void (*read_display) (PRECT pRect, FRAMEPTR DstFramePtr);

	void (**graphics_func_array) (PRECT pClipRect, PRIMITIVEPTR
			PrimPtr);
} DISPLAY_INTERFACE;
typedef DISPLAY_INTERFACE *PDISPLAY_INTERFACE;

extern PDISPLAY_INTERFACE _pCurDisplay;

extern void (* mask_func_array[]) (PRECT pClipRect,
		PRIMITIVEPTR PrimPtr);

#define AllocDrawableImage (*_pCurDisplay->alloc_image)
#define DrawGraphicsFunc(pRect,pPtr) \
		(*_pCurContext->func_array[GetPrimType(pPtr)]) (pRect, (PRIMITIVEPTR)(pPtr))
#define ReadDisplay (*_pCurDisplay->read_display)
extern void _bitplane_blt (PRECT pClipRect, FRAMEPTR
		DstFramePtr, COORD src_x, COORD src_y, FRAMEPTR
		SrcFramePtr, DRAW_MODE DrawMode);

#define GetDisplayFlags() (_pCurDisplay->DisplayFlags)
#define GetDisplayDepth() (_pCurDisplay->DisplayDepth)
#define GetDisplayWidth() (_pCurDisplay->DisplayWidth)
#define GetDisplayHeight() (_pCurDisplay->DisplayHeight)

#define SetContextGraphicsFunctions() \
do \
{ \
	if (_get_context_draw_dest () == DEST_MASK) \
		_pCurContext->func_array = mask_func_array; \
	else \
		_pCurContext->func_array = _pCurDisplay->graphics_func_array; \
} while (0)

#endif /* _DISPLAY_H */

