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
DrawPolygon (POLYGON Polygon)
{
	SetPrimType (&_locPrim, POLY_PRIM);
	_locPrim.Object.Polygon = Polygon;

	DrawBatch (&_locPrim, 0, BATCH_SINGLE);
}

void
DrawFilledPolygon (POLYGON Polygon)
{
	SetPrimType (&_locPrim, POLYFILL_PRIM);
	_locPrim.Object.Polygon = Polygon;

	DrawBatch (&_locPrim, 0, BATCH_SINGLE);
}

POLYREF
CreatePolygon (COUNT PtCount)
{
	POLYREF hPoly;

	hPoly = (POLYREF)NULL_PTR;
	if (PtCount > 0)
	{
		PPOLYGON_DESC lpPoly;

		if ((hPoly = AllocPolygon (PtCount)) != (POLYREF)NULL_PTR)
		{
			if ((lpPoly = LockPolygon (hPoly)) == (PPOLYGON_DESC)NULL_PTR)
			{
				FreePolygon (hPoly);
				hPoly = (POLYREF)NULL_PTR;
			}
			else
			{
				lpPoly->hPoly = hPoly;
				lpPoly->PtCount = PtCount;
				UnlockPolygon (hPoly);
			}
		}
	}

	return (hPoly);
}

BOOLEAN
DestroyPolygon (POLYREF PolyRef)
{
	return (FreePolygon (PolyRef));
}

POLYGON
CapturePolygon (POLYREF PolyRef)
{
	PPOLYGON_DESC lpPoly;

	if (PolyRef == (POLYREF)NULL_PTR)
		lpPoly = (PPOLYGON_DESC)NULL_PTR;
	else
	{
		lpPoly = LockPolygon (PolyRef);
		lpPoly->PtList = (PPOINT)&lpPoly[1];
	}

	return ((POLYGON)lpPoly);
}

POLYREF
ReleasePolygon (POLYGON Polygon)
{
	POLYREF PolyRef;

	PolyRef = (POLYREF)NULL_PTR;
	if (Polygon != (POLYGON)NULL_PTR)
	{
		PolyRef = ((PPOLYGON_DESC)Polygon)->hPoly;
		UnlockPolygon (PolyRef);
	}

	return (PolyRef);
}

BOOLEAN
SetPolygonPoint (POLYGON Polygon, COUNT PtIndex, COORD x, COORD y)
{
	BOOLEAN retval;

	retval = FALSE;
	if (Polygon != (POLYGON)NULL_PTR)
	{
		PPOLYGON_DESC lpPoly;
		PPOINT lpPt;

		lpPoly = (PPOLYGON_DESC)Polygon;
		if (lpPoly->BoundRect.extent.width == 0)
		{
			lpPoly->BoundRect.corner.x = x;
			lpPoly->BoundRect.corner.y = y;
			lpPoly->BoundRect.extent.width =
					lpPoly->BoundRect.extent.height = 1;
		}
		else
		{
			SIZE delta, width, height;

			width = height = 1;
			if ((delta = x - lpPoly->BoundRect.corner.x) >= 0)
				width += delta;
			else
			{
				lpPoly->BoundRect.extent.width -= delta;
				lpPoly->BoundRect.corner.x = x;
			}
			if ((delta = y - lpPoly->BoundRect.corner.y) >= 0)
				height += delta;
			else
			{
				lpPoly->BoundRect.extent.height -= delta;
				lpPoly->BoundRect.corner.y = y;
			}
			if ((delta = width - lpPoly->BoundRect.extent.width) > 0)
				lpPoly->BoundRect.extent.width += delta;
			if ((delta = height - lpPoly->BoundRect.extent.height) > 0)
				lpPoly->BoundRect.extent.height += delta;
		}

		lpPt = &lpPoly->PtList[PtIndex];
		lpPt->x = x;
		lpPt->y = y;

		retval = TRUE;
	}

	return (retval);
}

BOOLEAN
GetPolygonPoint (POLYGON Polygon, COUNT PtIndex, PCOORD px, PCOORD py)
{
	BOOLEAN retval;

	retval = FALSE;
	if (Polygon != (POLYGON)NULL_PTR)
	{
		PPOINT lpPt;

		lpPt = &((PPOLYGON_DESC)Polygon)->PtList[PtIndex];
		*px = lpPt->x;
		*py = lpPt->y;

		retval = TRUE;
	}

	return (retval);
}

