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
#include "declib.h"

typedef union
{
	POINT Point;
	STAMP Stamp;
	BRESENHAM_LINE Line;
	TEXT Text;
	STAMP_CMAP StampCMap;
	RECT Rect;
	POLYGON_DESC Polygon;
	struct
	{
		POINT origin;
		FRAME frame;
		COUNT NumFrames;
		PFRAME FrameList;
	} Composite;
} INTERNAL_PRIM_DESC;
typedef INTERNAL_PRIM_DESC *PINTERNAL_PRIM_DESC;

typedef PINTERNAL_PRIM_DESC INTERNAL_PRIM_DESCPTR;

typedef struct
{
	PRIM_LINKS Links;
	DWORD TypeAndColor;
	INTERNAL_PRIM_DESC Object;
} INTERNAL_PRIMITIVE;
typedef INTERNAL_PRIMITIVE *PINTERNAL_PRIMITIVE;

STAMP _save_stamp;

static BOOLEAN
GetFrameValidRect (PRECT pValidRect, HOT_SPOT *pOldHot)
{
	COORD hx, hy;
	HOT_SPOT OldHot;

	OldHot = GetFrameHotSpot (_CurFramePtr);
	hx = GET_HOT_X (OldHot);
	hy = GET_HOT_Y (OldHot);
	pValidRect->corner.x = hx;
	pValidRect->corner.y = hy;
	pValidRect->extent.width = GetFrameWidth (_CurFramePtr);
	pValidRect->extent.height = GetFrameHeight (_CurFramePtr);
	if (_pCurContext->ClipRect.extent.width)
	{
		if (!BoxIntersect (&_pCurContext->ClipRect,
				pValidRect, pValidRect))
			return (FALSE);

		hx -= _pCurContext->ClipRect.corner.x;
		hy -= _pCurContext->ClipRect.corner.y;
		pValidRect->corner.x += hx;
		pValidRect->corner.y += hy;
		SetFrameHotSpot (_CurFramePtr, MAKE_HOT_SPOT (hx, hy));
	}

	*pOldHot = OldHot;
	return (TRUE);
}

void
ClearBackGround (PRECT pClipRect)
{
	if (_get_context_bg_func ())
	{
		(*_get_context_bg_func ()) (pClipRect);
	}
	else
	{
		RECT locRect;
		PRIMITIVE locPrim;

		locPrim.Object.Rect = *pClipRect;
		if (BGFrame == 0)
		{
			SetPrimColor (&locPrim, _get_context_bg_color ());
			SetPrimType (&locPrim, RECTFILL_PRIM);
		}
		else
		{
			locRect.corner.x = -GetFrameHotX (BGFrame);
			locRect.corner.y = -GetFrameHotY (BGFrame);
			locRect.extent.width = GetFrameWidth (BGFrame);
			locRect.extent.height = GetFrameHeight (BGFrame);
			_save_stamp.origin = locRect.corner;

			if (!BoxIntersect (&locRect, &locPrim.Object.Rect, &locRect))
				return;

			pClipRect = &locRect;
			SetPrimType (&locPrim, STAMP_PRIM);
			SetPrimColor (&locPrim, _get_context_bg_color ());
			locPrim.Object.Stamp = _save_stamp;
		}

		DrawGraphicsFunc (pClipRect, &locPrim);
	}
}

void
DrawBatch (PPRIMITIVE lpBasePrim, PRIM_LINKS PrimLinks, register
		BATCH_FLAGS BatchFlags)
{
	RECT ValidRect;
	HOT_SPOT OldHot;

	if (GraphicsSystemActive () && GetFrameValidRect (&ValidRect, &OldHot))
	{
		int frame_dir;
		COUNT CurIndex;
		PRIM_LINKS OldLinks;
		register PPRIMITIVE lpPrim;
#ifdef MAPPING
		MAP_TYPE MapType;

		_init_context_map (&MapType);
		if (MapType != MAP_NOXFORM)
			BatchFlags |= BATCH_XFORM;
#endif /* MAPPING */

		BatchFlags &= BATCH_SINGLE
				| BATCH_BUILD_PAGE
				| BATCH_XFORM;

		BatchFlags |= _get_context_flags () & BATCH_CLIP_GRAPHICS;

		BatchGraphics ();

		frame_dir = ((GetDisplayFlags () & WANT_MASK)
				&& _get_context_draw_mode () == DRAW_SUBTRACTIVE) ? -1 : 1;
		if (BatchFlags & BATCH_BUILD_PAGE)
		{
			if (frame_dir == 1)
				ClearBackGround (&ValidRect);
		}
		else if (_get_context_draw_mode () == DRAW_ADDITIVE
					&& _get_context_bg_func ())
			BatchFlags |= BATCH_DIRTY_TILES;

		CurIndex = frame_dir < 0
				? GetSuccLink (PrimLinks)
				: GetPredLink (PrimLinks);

		if (BatchFlags & BATCH_SINGLE)
		{
			if (CurIndex == END_OF_LIST)
				BatchFlags &= ~BATCH_SINGLE;
			else
			{
				lpBasePrim += CurIndex;
				OldLinks = GetPrimLinks (lpBasePrim);
				SetPrimLinks (lpBasePrim, END_OF_LIST, END_OF_LIST);
				CurIndex = 0;
			}
		}

		for (; CurIndex != END_OF_LIST;
				CurIndex = frame_dir < 0
						? GetPredLink (GetPrimLinks (lpPrim))
						: GetSuccLink (GetPrimLinks (lpPrim)))
		{
			register GRAPHICS_PRIM PrimType;
			PPRIMITIVE lpWorkPrim;
			RECT ClipRect;
			INTERNAL_PRIMITIVE Prim;
			FRAMEPTR SrcFramePtr;

			lpPrim = &lpBasePrim[CurIndex];
			PrimType = GetPrimType (lpPrim);
			if (!ValidPrimType (PrimType))
				continue;

			lpWorkPrim = lpPrim;
#ifdef MAPPING
			if (BatchFlags & BATCH_XFORM)
			{
				COORD x, y;

				Prim = *(PINTERNAL_PRIMITIVE)lpWorkPrim;
				lpWorkPrim = (PPRIMITIVE)&Prim;

				x = LXtoDX (Prim.Object.Point.x);
				y = LYtoDY (Prim.Object.Point.y);
				if (PrimType == RECT_PRIM
						|| PrimType == RECTFILL_PRIM)
				{
					Prim.Object.Line.second.x =
							LXtoDX (Prim.Object.Rect.corner.x +
							Prim.Object.Rect.extent.width - 1);
					Prim.Object.Line.second.y =
							LYtoDY (Prim.Object.Rect.corner.y +
							Prim.Object.Rect.extent.height - 1);
					Prim.Object.Rect.extent.width = Prim.Object.Line.second.x - x + 1;
					Prim.Object.Rect.extent.height = Prim.Object.Line.second.y - y + 1;
				}

				Prim.Object.Point.x = x;
				Prim.Object.Point.y = y;
			}
#endif /* MAPPING */

			switch (PrimType)
			{
				case POINT_PRIM:
					ClipRect.corner = lpWorkPrim->Object.Point;
					if ((BatchFlags & BATCH_CLIP_GRAPHICS)
							&& (ClipRect.corner.x < ValidRect.corner.x
							|| ClipRect.corner.y < ValidRect.corner.y
							|| ClipRect.corner.x >= ValidRect.corner.x
									+ ValidRect.extent.width
							|| ClipRect.corner.y >= ValidRect.corner.y
									+ ValidRect.extent.height))
						continue;
					ClipRect.extent.width = ClipRect.extent.height = 1;
					goto DoGraphics;
				case STAMPCMAP_PRIM:
					BatchColorMap (lpWorkPrim->Object.StampCMap.CMap);
					goto ProcessStamp;
				case COMPOSITE_PRIM:
				case COMPOSITEFILL_PRIM:
					Prim = *(PINTERNAL_PRIMITIVE)lpWorkPrim;
					if ((Prim.Object.Composite.FrameList =
							(PFRAME)Prim.Object.Composite.frame) == 0
							|| Prim.Object.Composite.NumFrames == 0)
						continue;
					if (frame_dir < 0)
						Prim.Object.Composite.FrameList +=
								Prim.Object.Composite.NumFrames - 1;
					lpWorkPrim = (PPRIMITIVE)&Prim;
					SetPrimType (lpWorkPrim,
							STAMP_PRIM + (PrimType - COMPOSITE_PRIM));
ProcessComposite:
					Prim.Object.Composite.frame =
							*Prim.Object.Composite.FrameList;
					Prim.Object.Composite.FrameList += frame_dir;
					if (--Prim.Object.Composite.NumFrames == 0)
						PrimType = GetPrimType (&Prim);
				case STAMP_PRIM:
				case STAMPFILL_PRIM:
ProcessStamp:
					SrcFramePtr = (FRAMEPTR)(
							lpWorkPrim->Object.Stamp.frame
							);
					if (SrcFramePtr == 0)
					{
						if (PrimType >= COMPOSITE_PRIM)
							goto ProcessComposite;
						else
							continue;
					}

					ClipRect.corner = lpWorkPrim->Object.Stamp.origin;
					ClipRect.corner.x -= GetFrameHotX (SrcFramePtr);
					ClipRect.corner.y -= GetFrameHotY (SrcFramePtr);
					ClipRect.extent.width = GetFrameWidth (SrcFramePtr);
					ClipRect.extent.height = GetFrameHeight (SrcFramePtr);

					_save_stamp.origin = ClipRect.corner;
					break;
				case LINE_PRIM:
#ifdef MAPPING
					if (BatchFlags & BATCH_XFORM)
					{
						Prim.Object.Line.second.x = LXtoDX (Prim.Object.Line.second.x);
						Prim.Object.Line.second.y = LYtoDY (Prim.Object.Line.second.y);
					}
					else
#endif /* MAPPING */
					{
						Prim = *(PINTERNAL_PRIMITIVE)lpWorkPrim;
						lpWorkPrim = (PPRIMITIVE)&Prim;
					}

					if (Prim.Object.Line.second.x >= Prim.Object.Line.first.x)
					{
						ClipRect.corner.x = Prim.Object.Line.first.x;
						ClipRect.extent.width =
								Prim.Object.Line.second.x
								- Prim.Object.Line.first.x + 1;
					}
					else
					{
						ClipRect.corner.x = Prim.Object.Line.second.x;
						ClipRect.extent.width =
								Prim.Object.Line.first.x
								- Prim.Object.Line.second.x + 1;
					}
					if (Prim.Object.Line.second.y >= Prim.Object.Line.first.y)
					{
						ClipRect.corner.y = Prim.Object.Line.first.y;
						ClipRect.extent.height =
								Prim.Object.Line.second.y
								- Prim.Object.Line.first.y + 1;
					}
					else
					{
						ClipRect.corner.y = Prim.Object.Line.second.y;
						ClipRect.extent.height =
								Prim.Object.Line.first.y
								- Prim.Object.Line.second.y + 1;
					}
					break;
				case TEXT_PRIM:
					if (!TextRect (&lpWorkPrim->Object.Text,
							&ClipRect, NULL_PTR))
						continue;

					_save_stamp.origin = ClipRect.corner;
					break;
				case RECT_PRIM:
				case RECTFILL_PRIM:
					ClipRect = lpWorkPrim->Object.Rect;
					break;
				case POLY_PRIM:
				case POLYFILL_PRIM:
					ClipRect = ((PPOLYGON_DESC)lpWorkPrim->Object.Polygon)->BoundRect;
					break;
			}

#if 0
			if ((BatchFlags & BATCH_CLIP_GRAPHICS)
					&& BoxIntersect (&ClipRect,
					&ValidRect, &ClipRect) == 0)
			{
				if (PrimType >= COMPOSITE_PRIM)
					goto ProcessComposite;
				else
					continue;
			}
#endif

DoGraphics:
			if (BatchFlags & BATCH_DIRTY_TILES)
				(*_get_context_bg_func ()) (&ClipRect);

			DrawGraphicsFunc (&ClipRect, lpWorkPrim);
			if (PrimType >= COMPOSITE_PRIM)
				goto ProcessComposite;
		}

		if (BatchFlags & BATCH_BUILD_PAGE)
		{
			if (frame_dir < 0)
				ClearBackGround (&ValidRect);
		}

		UnbatchGraphics ();

		SetFrameHotSpot (_CurFramePtr, OldHot);

		if (BatchFlags & BATCH_SINGLE)
			SetPrimLinks (lpBasePrim,
					GetPredLink (OldLinks),
					GetSuccLink (OldLinks));

#ifdef MAPPING
		_uninit_context_map ();
#endif /* MAPPING */
	}
}

void
ClearDrawable (void)
{
	RECT ValidRect;
	HOT_SPOT OldHot;

	if (GraphicsSystemActive () && GetFrameValidRect (&ValidRect, &OldHot))
	{
		BatchGraphics ();
		ClearBackGround (&ValidRect);
		UnbatchGraphics ();

		SetFrameHotSpot (_CurFramePtr, OldHot);
	}
}

