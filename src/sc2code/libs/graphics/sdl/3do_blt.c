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

#ifdef GFXMODULE_SDL

#include "sdl_common.h"
#include "rotozoom.h"

static int gscale;


static void
blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
	TFB_Image *img;
	PFRAME_DESC SrcFramePtr;

	SrcFramePtr = (PFRAME_DESC)PrimPtr->Object.Stamp.frame;
	if (SrcFramePtr->DataOffs == 0)
	{
		fprintf (stderr, "Non-existent image to blt()\n");
		return;
	}

	img = (TFB_Image *) ((BYTE *) SrcFramePtr + SrcFramePtr->DataOffs);
	
	LockMutex (img->mutex);

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
	{
		int x, y;
		BOOLEAN scaled, paletted, filled;
		TFB_Palette palette[256];

		x = pClipRect->corner.x - GetFrameHotX (_CurFramePtr);
		y = pClipRect->corner.y - GetFrameHotY (_CurFramePtr);
		scaled = filled = FALSE;

		if (gscale != 0 && gscale != 256)
		{
			x += (GetFrameHotX (SrcFramePtr) *
				((1 << 8) - gscale)) >> 8;
			y += (GetFrameHotY (SrcFramePtr) *
				((1 << 8) - gscale)) >> 8;
			scaled = TRUE;

			if (img->ScaledImg)
			{
				if (img->scale != gscale)
				{
					SDL_FreeSurface(img->ScaledImg);
					img->ScaledImg = NULL;
				}
			}

			if (!img->ScaledImg)
			{
				// Atleast melee zooming and planet surfaces will use this

				// NOTE: Planet surfaces maybe causes memory leaks,
				//       I haven't currently seen DELETEIMAGE executed to them at all.

				SDL_Surface *new_surf;

				img->scale = gscale;
				new_surf = zoomSurface (img->NormalImg, gscale / 256.0f,
					gscale / 256.0f, SMOOTHING_OFF);

				if (new_surf) 
				{
					if (!new_surf->format->palette)
					{
						img->ScaledImg = TFB_DisplayFormatAlpha (new_surf);
						if (img->ScaledImg)
						{
							SDL_FreeSurface(new_surf);
						}
						else
						{
							fprintf (stderr, "blt(): TFB_DisplayFormatAlpha failed\n");
							img->ScaledImg = new_surf;
						}
					}
					else
					{
						img->ScaledImg = new_surf;
						SDL_SetColors (img->ScaledImg, img->Palette, 0, 256);
						if (img->NormalImg->flags & SDL_SRCCOLORKEY)
						{
							SDL_SetColorKey (img->ScaledImg, SDL_SRCCOLORKEY, 
								img->NormalImg->format->colorkey);
						}
					}
				}
				else
				{
					fprintf (stderr, "blt(): zoomSurface failed\n");
				}
			}
		}

		paletted = FALSE;

		if (GetPrimType (PrimPtr) == STAMPFILL_PRIM)
		{
			int r, g, b;
			DWORD c32k;

			c32k = GetPrimColor (PrimPtr) >> 8;  // shift out color index
		        r = (c32k >> (10 - (8 - 5))) & 0xF8;
			g = (c32k >> (5 - (8 - 5))) & 0xF8;
			b = (c32k << (8 - 5)) & 0xF8;

			palette[0].r = r;
			palette[0].g = g;
			palette[0].b = b;
		
			filled = TRUE;
		}
		else
		{
			if (img->NormalImg->format->palette && img->colormap_index != -1)
			{
				TFB_ColorMapToRGB (palette, img->colormap_index);
				paletted = TRUE;
			}
		}

		UnlockMutex (img->mutex);
		if (filled)
		{
			TFB_Draw_FilledImage ((TFB_ImageStruct *)img, x, y,
					      scaled, palette[0].r, 
					      palette[0].g, palette[0].b,
					      TFB_SCREEN_MAIN);
		}
		else
		{
			TFB_Draw_Image ((TFB_ImageStruct *)img, x, y, scaled,
					(paletted ? palette : NULL),
					TFB_SCREEN_MAIN);
		}
	}
	else
	{
		SDL_Rect SDL_DstRect;
		TFB_Image *dst_img;

		dst_img = ((TFB_Image *) ((BYTE *) _CurFramePtr +
			_CurFramePtr->DataOffs));
		
		LockMutex (dst_img->mutex);

		SDL_DstRect.x = (short) pClipRect->corner.x -
						GetFrameHotX (_CurFramePtr);
		SDL_DstRect.y = (short) pClipRect->corner.y -
						GetFrameHotY (_CurFramePtr);

		SDL_BlitSurface (
			img->NormalImg,
			NULL,
			dst_img->NormalImg,
			&SDL_DstRect
		);
		
		UnlockMutex (dst_img->mutex);
		UnlockMutex (img->mutex);
	}
}

static void
fillrect_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
	int r, g, b;
	DWORD c32k;

	c32k = GetPrimColor (PrimPtr) >> 8; //shift out color index
	r = (c32k >> (10 - (8 - 5))) & 0xF8;
	g = (c32k >> (5 - (8 - 5))) & 0xF8;
	b = (c32k << (8 - 5)) & 0xF8;

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
	{
		RECT rect;
		rect.corner.x = pClipRect->corner.x - GetFrameHotX (_CurFramePtr);
		rect.corner.y = pClipRect->corner.y - GetFrameHotY (_CurFramePtr);
		rect.extent.width = pClipRect->extent.width;
		rect.extent.height = pClipRect->extent.height;

		if (gscale && GetPrimType (PrimPtr) != POINT_PRIM)
		{
			rect.extent.width = (rect.extent.width * gscale) >> 8;
			rect.extent.height = (rect.extent.height * gscale) >> 8;
			rect.corner.x += (pClipRect->extent.width -
					rect.extent.width) >> 1;
			rect.corner.y += (pClipRect->extent.height -
					rect.extent.height) >> 1;
		}

		TFB_Draw_Rect (&rect, r, g, b, TFB_SCREEN_MAIN);
	}
	else
	{
		SDL_Rect  SDLRect;
		TFB_Image *img;

		img = ((TFB_Image *) ((BYTE *) _CurFramePtr +
				_CurFramePtr->DataOffs));

		LockMutex (img->mutex);

		SDLRect.x = (short) (pClipRect->corner.x -
				GetFrameHotX (_CurFramePtr));
		SDLRect.y = (short) (pClipRect->corner.y -
				GetFrameHotY (_CurFramePtr));
		SDLRect.w = (short) pClipRect->extent.width;
		SDLRect.h = (short) pClipRect->extent.height;

		SDL_FillRect (img->NormalImg, &SDLRect, SDL_MapRGB (img->NormalImg->format, r, g, b));

		UnlockMutex (img->mutex);
	}
}

static void
cmap_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
	fprintf (stderr, "Unimplemented function activated: cmap_blt()\n");
}

static void
line_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
	int x1,y1,x2,y2;
	int r, g, b;
	DWORD c32k;

	c32k = GetPrimColor (PrimPtr) >> 8;  // shift out color index
	r = (c32k >> (10 - (8 - 5))) & 0xF8;
	g = (c32k >> (5 - (8 - 5))) & 0xF8;
	b = (c32k << (8 - 5)) & 0xF8;

	x1=PrimPtr->Object.Line.first.x - GetFrameHotX (_CurFramePtr);
	y1=PrimPtr->Object.Line.first.y - GetFrameHotY (_CurFramePtr);
	x2=PrimPtr->Object.Line.second.x - GetFrameHotX (_CurFramePtr);
	y2=PrimPtr->Object.Line.second.y - GetFrameHotY (_CurFramePtr);

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
	{
		TFB_Draw_Line (x1, y1, x2, y2, r, g, b, TFB_SCREEN_MAIN);
	}
	else
	{
		// Maybe we should do something about this?
	}
}

static void
read_screen (PRECT lpRect, FRAMEPTR DstFramePtr)
{
	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) != SCREEN_DRAWABLE
			|| TYPE_GET (DstFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE
			|| !(TYPE_GET (GetFrameParentDrawable (DstFramePtr)
			->FlagsAndIndex)
			& ((DWORD) MAPPED_TO_DISPLAY << FTYPE_SHIFT)))
	{
		fprintf (stderr, "Unimplemented function activated: read_screen()\n");
	}
	else
	{
		TFB_ImageStruct *img = (TFB_ImageStruct *) ((BYTE *) DstFramePtr +
				DstFramePtr->DataOffs);
		TFB_Draw_CopyToImage (img, lpRect, TFB_SCREEN_MAIN);
	}
}

static DRAWABLE
alloc_image (COUNT NumFrames, DRAWABLE_TYPE DrawableType, CREATE_FLAGS
		flags, SIZE width, SIZE height)
{
	DWORD data_byte_count;
	DRAWABLE Drawable;

	data_byte_count = 0;
	if (flags & WANT_MASK)
		data_byte_count += (DWORD) SCAN_WIDTH (width) * height;
	if ((flags & WANT_PIXMAP) && DrawableType == RAM_DRAWABLE)
	{
		width = ((width << 1) + 3) & ~3;
		data_byte_count += (DWORD) width * height;
	}

	Drawable = AllocDrawable (NumFrames, data_byte_count * NumFrames);
	if (Drawable)
	{
		if (DrawableType == RAM_DRAWABLE)
		{
			COUNT i;
			DWORD data_offs;
			DRAWABLEPTR DrawablePtr;
			FRAMEPTR F;

			data_offs = sizeof (*F) * NumFrames;
			DrawablePtr = LockDrawable (Drawable);
			for (i = 0, F = &DrawablePtr->Frame[0]; i < NumFrames; ++i, ++F)
			{
				F->DataOffs = data_offs;
				data_offs += data_byte_count - sizeof (*F);
			}
			UnlockDrawable (Drawable);
		}
	}

	return (Drawable);
}

void (*func_array[]) (PRECT pClipRect, PRIMITIVEPTR PrimPtr) =
{
	fillrect_blt,
	blt,
	blt,
	line_blt,
	_text_blt,
	cmap_blt,
	_rect_blt,
	fillrect_blt,
	_fillpoly_blt,
	_fillpoly_blt,
};

static DISPLAY_INTERFACE DisplayInterface =
{
	WANT_MASK,

	16, // SCREEN_DEPTH,
	320,
	240,

	alloc_image,
	read_screen,

	func_array,
};

void
LoadDisplay (PDISPLAY_INTERFACE *pDisplay)
{
	*pDisplay = &DisplayInterface;
}

// Status: Unimplemented
void
SetGraphicScale (int scale)
		//Calibration...
{
	gscale = scale;
}

#endif
