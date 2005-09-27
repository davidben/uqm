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
#include "gfx_common.h"
#include "tfb_draw.h"
#include "tfb_prim.h"
#include "gfxother.h"

HOT_SPOT
MAKE_HOT_SPOT (COORD x, COORD y)
{
	HOT_SPOT hs;
	hs.x = x;
	hs.y = y;
	return hs;
}

typedef union
{
	POINT Point;
	STAMP Stamp;
	BRESENHAM_LINE Line;
	TEXT Text;
	RECT Rect;
} INTERNAL_PRIM_DESC;
typedef INTERNAL_PRIM_DESC *PINTERNAL_PRIM_DESC;

typedef PINTERNAL_PRIM_DESC INTERNAL_PRIM_DESCPTR;

typedef struct
{
	PRIM_LINKS Links;
	GRAPHICS_PRIM Type;
	COLOR Color;
	INTERNAL_PRIM_DESC Object;
} INTERNAL_PRIMITIVE;
typedef INTERNAL_PRIMITIVE *PINTERNAL_PRIMITIVE;

STAMP _save_stamp;

static BOOLEAN
GetFrameValidRect (PRECT pValidRect, HOT_SPOT *pOldHot)
{
	COORD hx, hy;
	HOT_SPOT OldHot;

	OldHot = _CurFramePtr->HotSpot;
	hx = OldHot.x;
	hy = OldHot.y;
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
		_CurFramePtr->HotSpot = MAKE_HOT_SPOT (hx, hy);
	}

	*pOldHot = OldHot;
	return (TRUE);
}

void
ClearBackGround (PRECT pClipRect)
{
	TFB_Palette color;

	COLORtoPalette (_get_context_bg_color (), &color);
	TFB_Prim_FillRect (pClipRect, &color);
}

void
DrawBatch (PPRIMITIVE lpBasePrim, PRIM_LINKS PrimLinks, 
		BATCH_FLAGS BatchFlags)
{
	RECT ValidRect;
	HOT_SPOT OldHot;

	if (GraphicsSystemActive () && GetFrameValidRect (&ValidRect, &OldHot))
	{
		COUNT CurIndex;
		PRIM_LINKS OldLinks;
		PPRIMITIVE lpPrim;

		BatchFlags &= BATCH_SINGLE
				| BATCH_BUILD_PAGE
				| BATCH_XFORM;

		BatchFlags |= _get_context_flags () & BATCH_CLIP_GRAPHICS;

		BatchGraphics ();

		if (BatchFlags & BATCH_BUILD_PAGE)
		{
			ClearBackGround (&ValidRect);
		}

		CurIndex = GetPredLink (PrimLinks);

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

		for (; CurIndex != END_OF_LIST; CurIndex = GetSuccLink (GetPrimLinks (lpPrim)))
		{
			GRAPHICS_PRIM PrimType;
			PPRIMITIVE lpWorkPrim;
			RECT ClipRect;
			TFB_Palette color;

			lpPrim = &lpBasePrim[CurIndex];
			PrimType = GetPrimType (lpPrim);
			if (!ValidPrimType (PrimType))
				continue;

			lpWorkPrim = lpPrim;

			switch (PrimType)
			{
				case POINT_PRIM:
					COLORtoPalette (GetPrimColor (lpWorkPrim), &color);
					TFB_Prim_Point (&lpWorkPrim->Object.Point, &color);
					break;
				case STAMP_PRIM:
					TFB_Prim_Stamp (&lpWorkPrim->Object.Stamp);
					break;
				case STAMPFILL_PRIM:
					COLORtoPalette (GetPrimColor (lpWorkPrim), &color);
					TFB_Prim_StampFill (&lpWorkPrim->Object.Stamp, &color);
					break;
				case LINE_PRIM:
					COLORtoPalette (GetPrimColor (lpWorkPrim), &color);
					TFB_Prim_Line (&lpWorkPrim->Object.Line, &color);
					break;
				case TEXT_PRIM:
					if (!TextRect (&lpWorkPrim->Object.Text,
							&ClipRect, NULL_PTR))
						continue;

					_save_stamp.origin = ClipRect.corner;
					
					_text_blt (&ClipRect, lpWorkPrim);
					break;
				case RECT_PRIM:
					COLORtoPalette (GetPrimColor (lpWorkPrim), &color);
					TFB_Prim_Rect (&lpWorkPrim->Object.Rect, &color);
					break;
				case RECTFILL_PRIM:
					COLORtoPalette (GetPrimColor (lpWorkPrim), &color);
					TFB_Prim_FillRect (&lpWorkPrim->Object.Rect, &color);
					break;
			}
		}

		UnbatchGraphics ();

		_CurFramePtr->HotSpot = OldHot;

		if (BatchFlags & BATCH_SINGLE)
			SetPrimLinks (lpBasePrim,
					GetPredLink (OldLinks),
					GetSuccLink (OldLinks));

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

		_CurFramePtr->HotSpot = OldHot;
	}
}

void
DrawPoint (PPOINT lpPoint)
{
	RECT ValidRect;
	HOT_SPOT OldHot;

	if (GraphicsSystemActive () && GetFrameValidRect (&ValidRect, &OldHot))
	{
		TFB_Palette color;
		
		COLORtoPalette (GetPrimColor (&_locPrim), &color);
		TFB_Prim_Point (lpPoint, &color);
		_CurFramePtr->HotSpot = OldHot;
	}
}

void
DrawRectangle (PRECT lpRect)
{
	RECT ValidRect;
	HOT_SPOT OldHot;

	if (GraphicsSystemActive () && GetFrameValidRect (&ValidRect, &OldHot))
	{
		TFB_Palette color;
		
		COLORtoPalette (GetPrimColor (&_locPrim), &color);
		TFB_Prim_Rect (lpRect, &color);  
		_CurFramePtr->HotSpot = OldHot;
	}
}

void
DrawFilledRectangle (PRECT lpRect)
{
	RECT ValidRect;
	HOT_SPOT OldHot;

	if (GraphicsSystemActive () && GetFrameValidRect (&ValidRect, &OldHot))
	{
		TFB_Palette color;
		
		COLORtoPalette (GetPrimColor (&_locPrim), &color);
		TFB_Prim_FillRect (lpRect, &color);  
		_CurFramePtr->HotSpot = OldHot;
	}
}

void
DrawLine (PLINE lpLine)
{
	RECT ValidRect;
	HOT_SPOT OldHot;

	if (GraphicsSystemActive () && GetFrameValidRect (&ValidRect, &OldHot))
	{
		TFB_Palette color;

		COLORtoPalette (GetPrimColor (&_locPrim), &color);
		TFB_Prim_Line (lpLine, &color);
		_CurFramePtr->HotSpot = OldHot;
	} 
}

void
DrawStamp (PSTAMP stmp)
{
	RECT ValidRect;
	HOT_SPOT OldHot;

	if (GraphicsSystemActive () && GetFrameValidRect (&ValidRect, &OldHot))
	{
		TFB_Prim_Stamp (stmp);
		_CurFramePtr->HotSpot = OldHot;
	}
}

void
DrawFilledStamp (PSTAMP stmp)
{
	RECT ValidRect;
	HOT_SPOT OldHot;

	if (GraphicsSystemActive () && GetFrameValidRect (&ValidRect, &OldHot))
	{
		TFB_Palette color;
		
		COLORtoPalette (GetPrimColor (&_locPrim), &color);
		TFB_Prim_StampFill (stmp, &color);
		_CurFramePtr->HotSpot = OldHot;
	}
}
