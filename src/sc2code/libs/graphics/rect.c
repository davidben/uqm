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

#include "gfxintrn.h"

void
DrawPoint (PPOINT lpPoint)
{
	SetPrimType (&_locPrim, POINT_PRIM);
	_locPrim.Object.Point = *lpPoint;

	DrawBatch (&_locPrim, 0, BATCH_SINGLE);
}

void
DrawRectangle (PRECT lpRect)
{
	SetPrimType (&_locPrim, RECT_PRIM);
	_locPrim.Object.Rect = *lpRect;

	DrawBatch (&_locPrim, 0, BATCH_SINGLE);
}

void
DrawFilledRectangle (PRECT lpRect)
{
	SetPrimType (&_locPrim, RECTFILL_PRIM);
	_locPrim.Object.Rect = *lpRect;

	DrawBatch (&_locPrim, 0, BATCH_SINGLE);
}

void
_rect_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
	COORD x, y;
	RECTPTR RectPtr;
	PRIMITIVE locPrim;

	locPrim = *PrimPtr;
	SetPrimType (&locPrim, RECTFILL_PRIM);

	RectPtr = &PrimPtr->Object.Rect;
	locPrim.Object.Rect = *pClipRect;
	if (locPrim.Object.Rect.corner.x == RectPtr->corner.x)
	{
		locPrim.Object.Rect.extent.width = 1;
		DrawGraphicsFunc (&locPrim.Object.Rect, &locPrim);
		locPrim.Object.Rect.extent.width = pClipRect->extent.width;
	}
	if (locPrim.Object.Rect.corner.y == RectPtr->corner.y)
	{
		locPrim.Object.Rect.extent.height = 1;
		DrawGraphicsFunc (&locPrim.Object.Rect, &locPrim);
		locPrim.Object.Rect.extent.height = pClipRect->extent.height;
	}
	if ((x = locPrim.Object.Rect.corner.x
			+ locPrim.Object.Rect.extent.width) ==
			RectPtr->corner.x + RectPtr->extent.width)
	{
		locPrim.Object.Rect.corner.x = x - 1;
		locPrim.Object.Rect.extent.width = 1;
		DrawGraphicsFunc (&locPrim.Object.Rect, &locPrim);
		locPrim.Object.Rect.corner.x = pClipRect->corner.x;
		locPrim.Object.Rect.extent.width = pClipRect->extent.width;
	}
	if ((y = locPrim.Object.Rect.corner.y
			+ locPrim.Object.Rect.extent.height) ==
			RectPtr->corner.y + RectPtr->extent.height)
	{
		locPrim.Object.Rect.corner.y = y - 1;
		locPrim.Object.Rect.extent.height = 1;
		DrawGraphicsFunc (&locPrim.Object.Rect, &locPrim);
	}
}

