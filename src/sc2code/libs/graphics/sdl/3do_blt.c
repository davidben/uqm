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
		printf ("Non-existent image to blt()\n");
		return;
	}

	img = (TFB_Image *) ((BYTE *) SrcFramePtr + SrcFramePtr->DataOffs);
	
	if (SDL_mutexP(img->mutex))
		printf("blt(): couldn't lock img->mutex\n");

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
	{
		TFB_DrawCommand DC_on_stack;
		TFB_DrawCommand *DrawCommand = &DC_on_stack;

		// DrawCommand = HMalloc (sizeof (TFB_DrawCommand));

		DrawCommand->Type = TFB_DRAWCOMMANDTYPE_IMAGE;
		DrawCommand->x = pClipRect->corner.x -
			GetFrameHotX (_CurFramePtr);
		DrawCommand->y = pClipRect->corner.y -
			GetFrameHotY (_CurFramePtr);
		DrawCommand->w = img->SurfaceSDL->clip_rect.w;
		DrawCommand->h = img->SurfaceSDL->clip_rect.h;

		if (gscale != 0 && gscale != 256)
		{
			DrawCommand->x += (GetFrameHotX (SrcFramePtr) *
				((1 << 8) - gscale)) >> 8;
			DrawCommand->y += (GetFrameHotY (SrcFramePtr) *
				((1 << 8) - gscale)) >> 8;
			DrawCommand->w = (DrawCommand->w * gscale) >> 8;
			DrawCommand->h = (DrawCommand->h * gscale) >> 8;

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
				//       Also, we might want to change the way they are implemented, as
				//       currently the scaled img is like 1936x600x32bit so it takes
				//       much memory and is perhaps too slow to process on slower systems.
				//       -Mika

				SDL_Surface *new_surf;

				img->scale = gscale;
				new_surf = zoomSurface (img->DrawableImg, gscale / 256.0f,
					gscale / 256.0f, SMOOTHING_OFF);

				if (new_surf) 
				{
					img->ScaledImg = TFB_DisplayFormatAlpha (new_surf);

					if (img->ScaledImg)
					{
						SDL_FreeSurface(new_surf);
					}
					else
					{
						printf("blt(): TFB_DisplayFormatAlpha failed\n");
						img->ScaledImg = new_surf;
					}
				}
				else
				{
					printf("blt(): zoomSurface failed\n");
				}
			}
		}

		DrawCommand->image = (TFB_ImageStruct*) img; //TFB_Image

		if (GetPrimType (PrimPtr) == STAMPFILL_PRIM)
		{
			DWORD c32k;

			c32k = GetPrimColor (PrimPtr) >> 8;  // shift out color index
			DrawCommand->r = (c32k >> (10 - (8 - 5))) & 0xF8;
			DrawCommand->g = (c32k >> (5 - (8 - 5))) & 0xF8;
			DrawCommand->b = (c32k << (8 - 5));
		}

		DrawCommand->Qualifier = (TYPE_GET (GetFrameParentDrawable (
			SrcFramePtr)->FlagsAndIndex) >> FTYPE_SHIFT) &
			MAPPED_TO_DISPLAY;

		TFB_EnqueueDrawCommand(DrawCommand);
	}
	else
	{
		SDL_Rect SDL_SrcRect, SDL_DstRect;
		TFB_Image *dst_img;

		dst_img = ((TFB_Image *) ((BYTE *) _CurFramePtr +
			_CurFramePtr->DataOffs));
		
		if (SDL_mutexP(dst_img->mutex))
			printf("blt(): couldn't lock dst_img->mutex\n");

		SDL_SrcRect.x = (short) img->SurfaceSDL->clip_rect.x;
		SDL_SrcRect.y = (short) img->SurfaceSDL->clip_rect.y;
		SDL_SrcRect.w = (short) img->SurfaceSDL->clip_rect.w;
		SDL_SrcRect.h = (short) img->SurfaceSDL->clip_rect.h;

		SDL_DstRect.x = (short) pClipRect->corner.x -
						GetFrameHotX (_CurFramePtr) + SDL_SrcRect.x;
		SDL_DstRect.y = (short) pClipRect->corner.y -
						GetFrameHotY (_CurFramePtr) + SDL_SrcRect.y;

		SDL_BlitSurface (
			img->SurfaceSDL,  // src
			&SDL_SrcRect,     // src rect
			dst_img->SurfaceSDL,
			&SDL_DstRect      // dst rect
		);

		dst_img->dirty = TRUE;
		
		if (SDL_mutexV(dst_img->mutex))
			printf("blt(): couldn't unlock dst_img->mutex\n");
	}

	if (SDL_mutexV(img->mutex))
		printf("blt(): couldn't unlock img->mutex\n");
}

static void
fillrect_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
	BYTE r, g, b;
	DWORD c32k;

	c32k = GetPrimColor (PrimPtr) >> 8; //shift out color index
	r = (c32k >> (10 - (8 - 5))) & 0xF8;
	g = (c32k >> (5 - (8 - 5))) & 0xF8;
	b = (c32k << (8 - 5));

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
	{
		TFB_DrawCommand DC_on_stack;
		TFB_DrawCommand *DrawCommand = &DC_on_stack;

		if (DrawCommand)
		{
			DrawCommand->Type = TFB_DRAWCOMMANDTYPE_RECTANGLE;
			DrawCommand->x = pClipRect->corner.x - GetFrameHotX (_CurFramePtr);
			DrawCommand->y = pClipRect->corner.y - GetFrameHotY (_CurFramePtr);
			DrawCommand->w = pClipRect->extent.width;
			DrawCommand->h = pClipRect->extent.height;
			DrawCommand->r = r;
			DrawCommand->g = g;
			DrawCommand->b = b;

			if (gscale && GetPrimType (PrimPtr) != POINT_PRIM)
			{
				DrawCommand->w = (DrawCommand->w * gscale) >> 8;
				DrawCommand->h = (DrawCommand->h * gscale) >> 8;
				DrawCommand->x += (pClipRect->extent.width -
						DrawCommand->w) >> 1;
				DrawCommand->y += (pClipRect->extent.height -
						DrawCommand->h) >> 1;
			}

			DrawCommand->image = 0;

			TFB_EnqueueDrawCommand(DrawCommand);
		}
	}
	else
	{
		SDL_Rect  SDLRect;
		TFB_Image *img;

		img = ((TFB_Image *) ((BYTE *) _CurFramePtr +
				_CurFramePtr->DataOffs));

		if (SDL_mutexP(img->mutex))
			printf("fillrect_blt(): couldn't lock img->mutex\n");

		SDLRect.x = (short) (pClipRect->corner.x -
				GetFrameHotX (_CurFramePtr));
		SDLRect.y = (short) (pClipRect->corner.y -
				GetFrameHotY (_CurFramePtr));
		SDLRect.w = (short) pClipRect->extent.width;
		SDLRect.h = (short) pClipRect->extent.height;

		SDL_FillRect (img->SurfaceSDL, &SDLRect, SDL_MapRGB (img->SurfaceSDL->format, r, g, b));

		img->dirty = TRUE;

		if (SDL_mutexV(img->mutex))
			printf("fillrect_blt(): couldn't unlock img->mutex\n");
	}
}

static void
cmap_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
// printf ("Unimplemented function activated: cmap_blt()\n");
}

static void
line_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
	float x1,y1,x2,y2;
	BYTE r, g, b;
	DWORD c32k;

	c32k = GetPrimColor (PrimPtr) >> 8;  // shift out color index
	r = (c32k >> (10 - (8 - 5))) & 0xF8;
	g = (c32k >> (5 - (8 - 5))) & 0xF8;
	b = (c32k << (8 - 5));

	x1=PrimPtr->Object.Line.first.x - GetFrameHotX (_CurFramePtr);
	y1=PrimPtr->Object.Line.first.y - GetFrameHotY (_CurFramePtr);
	x2=PrimPtr->Object.Line.second.x - GetFrameHotX (_CurFramePtr);
	y2=PrimPtr->Object.Line.second.y - GetFrameHotY (_CurFramePtr);

	if (TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
	{
		TFB_DrawCommand DC;

		DC.Type = TFB_DRAWCOMMANDTYPE_LINE;
		DC.x = x1;
		DC.y = y1;
		DC.w = x2;
		DC.h = y2;
		DC.r = r;
		DC.g = g;
		DC.b = b;
		DC.image = 0;

		TFB_EnqueueDrawCommand(&DC);
	}
	else
	{
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
		printf("Unimplemented function activated: read_screen()\n");
	}
	else
	{
		TFB_DrawCommand DC_auto;
		TFB_DrawCommand* DC = &DC_auto;

		DC->Type = TFB_DRAWCOMMANDTYPE_COPYBACKBUFFERTOOTHERBUFFER;
		DC->x = lpRect->corner.x;
		DC->y = lpRect->corner.y;
		DC->w = lpRect->extent.width;
		DC->h = lpRect->extent.height;
		DC->image = (TFB_ImageStruct *) ((BYTE *) DstFramePtr +
				DstFramePtr->DataOffs);

		TFB_EnqueueDrawCommand (DC);
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
