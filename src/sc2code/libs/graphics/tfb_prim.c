// Copyright Michael Martin, 2003

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

/* The original Primitive routines do various elaborate checks to
 * ensure we're within bounds for the clipping.  Since clipping is
 * handled by the underlying TFB_Canvas implementation, we need not
 * worry about this. */

#include "gfxintrn.h"
#include "tfb_draw.h"
#include "tfb_prim.h"

void
TFB_Prim_Point (PPOINT p, TFB_Palette *color)
{
	RECT r;

	r.corner.x = p->x - GetFrameHotX (_CurFramePtr);
	r.corner.y = p->y - GetFrameHotY (_CurFramePtr);
	r.extent.width = r.extent.height = 1;

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
		TFB_DrawScreen_Rect (&r, color->r, color->g, color->b, TFB_SCREEN_MAIN);
	else
		TFB_DrawImage_Rect (&r, color->r, color->g, color->b, _CurFramePtr->image);
}

void
TFB_Prim_Rect (PRECT r, TFB_Palette *color)
{
	RECT arm;
	int gscale;
	gscale = GetGraphicScale ();
	if (!gscale)
		gscale = 256;
	arm = *r;
	arm.extent.width = r->extent.width;
	arm.extent.height = 1;
	TFB_Prim_FillRect (&arm, color);
	arm.extent.height = r->extent.height;
	arm.extent.width = 1;
	TFB_Prim_FillRect (&arm, color);
	arm.corner.x += ((r->extent.width * gscale) >> 8) - 1;
	TFB_Prim_FillRect (&arm, color);
	arm.corner.x = r->corner.x;
	arm.corner.y += ((r->extent.height * gscale) >> 8) - 1;
	arm.extent.width = r->extent.width;
	arm.extent.height = 1;
	TFB_Prim_FillRect (&arm, color);	
}

void
TFB_Prim_FillRect (PRECT r, TFB_Palette *color)
{
	RECT rect;
	int gscale;

	rect.corner.x = r->corner.x - GetFrameHotX (_CurFramePtr);
	rect.corner.y = r->corner.y - GetFrameHotY (_CurFramePtr);
	rect.extent.width = r->extent.width;
	rect.extent.height = r->extent.height;

	gscale = GetGraphicScale ();
	if (gscale)
	{
		rect.extent.width = (rect.extent.width * gscale) >> 8;
		rect.extent.height = (rect.extent.height * gscale) >> 8;
		rect.corner.x += (r->extent.width -
				  rect.extent.width) >> 1;
		rect.corner.y += (r->extent.height -
				  rect.extent.height) >> 1;
	}

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
		TFB_DrawScreen_Rect (&rect, color->r, color->g, color->b, TFB_SCREEN_MAIN);
	else
		TFB_DrawImage_Rect (&rect, color->r, color->g, color->b, _CurFramePtr->image);
}

void
TFB_Prim_Line (PLINE line, TFB_Palette *color)
{
	int x1, y1, x2, y2;

	x1=line->first.x - GetFrameHotX (_CurFramePtr);
	y1=line->first.y - GetFrameHotY (_CurFramePtr);
	x2=line->second.x - GetFrameHotX (_CurFramePtr);
	y2=line->second.y - GetFrameHotY (_CurFramePtr);

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
		TFB_DrawScreen_Line (x1, y1, x2, y2, color->r, color->g, color->b, TFB_SCREEN_MAIN);
	else
		TFB_DrawImage_Line (x1, y1, x2, y2, color->r, color->g, color->b, _CurFramePtr->image);		
}

void
TFB_Prim_Stamp (PSTAMP stmp)
{
	int x, y;
	PFRAME_DESC SrcFramePtr;
	TFB_Image *img;
	BOOLEAN paletted;
	TFB_Palette palette[256];
	int gscale;

	SrcFramePtr = (PFRAME_DESC)stmp->frame;
	img = SrcFramePtr->image;
	gscale = GetGraphicScale ();
	
	if (!img)
	{
		fprintf (stderr, "Non-existent image to TFB_Prim_Stamp()\n");
		return;
	}

	LockMutex (img->mutex);

	x = stmp->origin.x - GetFrameHotX (_CurFramePtr) - GetFrameHotX (SrcFramePtr);
	y = stmp->origin.y - GetFrameHotY (_CurFramePtr) - GetFrameHotY (SrcFramePtr);
	paletted = FALSE;

	if (gscale != 0 && gscale != 256)
	{
		TFB_DrawImage_FixScaling (img, gscale);
		x += (GetFrameHotX (SrcFramePtr) *
		      ((1 << 8) - gscale)) >> 8;
		y += (GetFrameHotY (SrcFramePtr) *
		      ((1 << 8) - gscale)) >> 8;
	}

	if (TFB_DrawCanvas_IsPaletted(img->NormalImg) && img->colormap_index != -1)
	{
		TFB_ColorMapToRGB (palette, img->colormap_index);
		paletted = TRUE;
	}

	UnlockMutex (img->mutex);

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
	{
		TFB_DrawScreen_Image (img, x, y, gscale, (paletted ? palette : NULL),
				      TFB_SCREEN_MAIN);
	}
	else
	{
		TFB_DrawImage_Image (img, x, y, gscale, (paletted ? palette : NULL),
				     _CurFramePtr->image);
	}
}

void
TFB_Prim_StampFill (PSTAMP stmp, TFB_Palette *color)
{
	int x, y;
	PFRAME_DESC SrcFramePtr;
	TFB_Image *img;
	int r, g, b;
	int gscale;

	SrcFramePtr = (PFRAME_DESC)stmp->frame;
	img = SrcFramePtr->image;
	gscale = GetGraphicScale ();

	if (!img)
	{
		fprintf (stderr, "Non-existent image to TFB_Prim_StampFill()\n");
		return;
	}

	LockMutex (img->mutex);

	x = stmp->origin.x - GetFrameHotX (_CurFramePtr) - GetFrameHotX (SrcFramePtr);
	y = stmp->origin.y - GetFrameHotY (_CurFramePtr) - GetFrameHotY (SrcFramePtr);
	r = color->r;
	g = color->g;
	b = color->b;

	if (gscale != 0 && gscale != 256)
	{
		TFB_DrawImage_FixScaling (img, gscale);
		x += (GetFrameHotX (SrcFramePtr) *
		      ((1 << 8) - gscale)) >> 8;
		y += (GetFrameHotY (SrcFramePtr) *
		      ((1 << 8) - gscale)) >> 8;
	}

	UnlockMutex (img->mutex);

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
	{
		TFB_DrawScreen_FilledImage (img, x, y, gscale, r, g, b,
					    TFB_SCREEN_MAIN);
	}
	else
	{
		TFB_DrawImage_FilledImage (img, x, y, gscale, r, g, b,
					   _CurFramePtr->image);
	}
}

// Text rendering is in font.c, under the name _text_blt
