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

#include "port.h"
#include "libs/gfxlib.h"
#include "libs/graphics/tfb_draw.h"
#include "sdl_common.h"
#include "primitives.h"


// stretch_frame
// create a new frame of size neww x newh, and blit a scaled version FramePtr
// into it.
// destroy the old frame if 'destroy' is 1
FRAME stretch_frame (FRAME FramePtr, int neww, int newh, int destroy)
{
	FRAME NewFrame;
	CREATE_FLAGS flags;
	TFB_Image *tfbImg;
	TFB_Canvas src, dst;

	flags = GetFrameParentDrawable (FramePtr)->Flags;
	NewFrame = CaptureDrawable (
				CreateDrawable (flags, (SIZE)neww, (SIZE)newh, 1));
	tfbImg = FramePtr->image;
	LockMutex (tfbImg->mutex);
	src = tfbImg->NormalImg;
	dst = NewFrame->image->NormalImg;
	TFB_DrawCanvas_Rescale_Nearest (src, dst, -1, NULL, NULL, NULL);
	UnlockMutex (tfbImg->mutex);
	if (destroy)
		DestroyDrawable (ReleaseDrawable (FramePtr));
	return (NewFrame);
}

void process_rgb_bmp (FRAME FramePtr, Uint32 *rgba, int maxx, int maxy)
{
	int x, y;
	TFB_Image *tfbImg;
	SDL_Surface *img;
	PutPixelFn putpix;

	tfbImg = FramePtr->image;
	LockMutex (tfbImg->mutex);
	img = (SDL_Surface *)tfbImg->NormalImg;
	SDL_LockSurface (img);

	putpix = putpixel_for (img);

	for (y = 0; y < maxy; ++y)
		for (x = 0; x < maxx; ++x)
			putpix (img, x, y, *rgba++);
	SDL_UnlockSurface (img);
	UnlockMutex (tfbImg->mutex);
}

void fill_frame_rgb (FRAME FramePtr, Uint32 color, int x0, int y0,
		int x, int y)
{
	SDL_Surface *img;
	TFB_Image *tfbImg;
	SDL_Rect rect;

	tfbImg = FramePtr->image;
	LockMutex (tfbImg->mutex);
	img = (SDL_Surface *)tfbImg->NormalImg;
	SDL_LockSurface (img);
	if (x0 == 0 && y0 == 0 && x == 0 && y == 0)
		SDL_FillRect(img, NULL, color);
	else
	{
		rect.x = x0;
		rect.y = y0;
		rect.w = x - x0;
		rect.h = y - y0;
		SDL_FillRect(img, &rect, color);
	}
	SDL_UnlockSurface (img);
	UnlockMutex (tfbImg->mutex);
}

void arith_frame_blit (FRAME srcFrame, const RECT *rsrc, FRAME dstFrame,
		const RECT *rdst, int num, int denom)
{
	TFB_Image *srcImg, *dstImg;
	SDL_Surface *src, *dst;
	SDL_Rect srcRect, dstRect, *srp = NULL, *drp = NULL;
	srcImg = srcFrame->image;
	dstImg = dstFrame->image;
	LockMutex (srcImg->mutex);
	LockMutex (dstImg->mutex);
	src = (SDL_Surface *)srcImg->NormalImg;
	dst = (SDL_Surface *)dstImg->NormalImg;
	if (rdst)
	{
		dstRect.x = rdst->corner.x;
		dstRect.y = rdst->corner.y;
		dstRect.w = rdst->extent.width;
		dstRect.h = rdst->extent.height;
		drp = &dstRect;
	}
	if (rsrc)
	{
		srcRect.x = rsrc->corner.x;
		srcRect.y = rsrc->corner.y;
		srcRect.w = rsrc->extent.width;
		srcRect.h = rsrc->extent.height;
		srp = &srcRect;
	}
	else if (srcFrame->HotSpot.x || srcFrame->HotSpot.y)
	{
		if (rdst)
		{
			dstRect.x -= srcFrame->HotSpot.x;
			dstRect.y -= srcFrame->HotSpot.y;
		}
		else
		{
			dstRect.x = -srcFrame->HotSpot.x;
			dstRect.y = -srcFrame->HotSpot.y;
			dstRect.w = GetFrameWidth (srcFrame);
			dstRect.h = GetFrameHeight (srcFrame);
			drp = &dstRect;
		}

	}
	TFB_BlitSurface (src, srp, dst, drp, num, denom);
	UnlockMutex (srcImg->mutex);
	UnlockMutex (dstImg->mutex);
}

// Generate an array of all pixels in FramePtr
// The 32bpp pixel format is :
//  bits 24-31 : red
//  bits 16-23 : green
//  bits 8-15  : blue
//  bits 0-7   : alpha
// The 8bpp pixel format is 1 index per pixel
void getpixelarray (void *map, int Bpp, FRAME FramePtr,
		int width, int height)
{
	Uint8 r,g,b,a;
	Uint32 p, pos, row;
	TFB_Image *tfbImg;
	SDL_Surface *img;
	GetPixelFn getpix;
	int x, y, w, h;

	tfbImg = FramePtr->image;
	LockMutex (tfbImg->mutex);
	img = (SDL_Surface *)tfbImg->NormalImg;
	getpix = getpixel_for (img);

	w = width < img->w ? width : img->w;
	h = height < img->h ? height : img->h;
	
	SDL_LockSurface (img);

	if (Bpp == 4)
	{
		Uint32 *dp = (Uint32 *)map;

		for (y = 0, row = 0; y < h; y++, row += width)
		{
			for (x = 0, pos = row; x < w; x++, pos++)
			{
				p = getpix (img, x, y);
				SDL_GetRGBA (p, img->format, &r, &g, &b, &a);
				dp[pos] = r << 24 | g << 16 | b << 8 | a;
			}
		}
	}
	else if (Bpp == 1)
	{
		Uint8 *dp = (Uint8 *)map;

		for (y = 0, row = 0; y < h; y++, row += width)
		{
			for (x = 0, pos = row; x < w; x++, pos++)
			{
				p = getpix (img, x, y);
				dp[pos] = p;
			}
		}
	}

	SDL_UnlockSurface (img);
	UnlockMutex (tfbImg->mutex);
}


// Generate a pixel (in the correct format to be applied to FramePtr) from the
// r,g,b,a values supplied
Uint32 frame_mapRGBA (FRAME FramePtr,Uint8 r, Uint8 g,  Uint8 b, Uint8 a)
{
	SDL_Surface *img= (SDL_Surface *)FramePtr->image->NormalImg;
	return (SDL_MapRGBA (img->format, r, g, b, a));
}

#endif
