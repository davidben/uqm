#include "SDL.h"
#include "sdl_common.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/sdl/primitives.h"
#include "libs/graphics/tfb_draw.h"

typedef SDL_Surface *NativeCanvas;

void
TFB_DrawCanvas_Line (int x1, int y1, int x2, int y2, int r, int g, int b, TFB_Canvas target)
{
	Uint32 color;
	PutPixelFn screen_plot;
	
	screen_plot = putpixel_for (target);
	color = SDL_MapRGB (((NativeCanvas) target)->format, r, g, b);

	SDL_LockSurface ((NativeCanvas) target);
	line (x1, y1, x2, y2, color, screen_plot, (NativeCanvas) target);
	SDL_UnlockSurface ((NativeCanvas) target);
}

void
TFB_DrawCanvas_Rect (PRECT rect, int r, int g, int b, TFB_Canvas target)
{
	SDL_Rect sr;
	sr.x = rect->corner.x;
	sr.y = rect->corner.y;
	sr.w = rect->extent.width;
	sr.h = rect->extent.height;
	
	SDL_FillRect((NativeCanvas) target, &sr, SDL_MapRGB(((NativeCanvas) target)->format, r, g, b));
}

void
TFB_DrawCanvas_Image (TFB_Image *img, int x, int y, int scale, TFB_Palette *palette, TFB_Canvas target)
{
	SDL_Rect srcRect, targetRect, *pSrcRect;
	SDL_Surface *surf;

	if (img == 0)
	{
		fprintf (stderr, "ERROR: TFB_DrawCanvas_Image passed null image ptr\n");
		return;
	}

	LockMutex (img->mutex);
	targetRect.x = x;
	targetRect.y = y;

	if (scale != 0 && scale != GSCALE_IDENTITY)
	{
		TFB_DrawImage_FixScaling (img, scale);
		surf = img->ScaledImg;
		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.w = img->extent.width;
		srcRect.h = img->extent.height;
		pSrcRect = &srcRect;
	}
	else
	{
		surf = img->NormalImg;
		pSrcRect = NULL;
	}
	
	if (surf->format->palette)
	{
		if (palette)
		{
			SDL_SetColors (surf, (SDL_Color*)palette, 0, 256);
		}
		else
		{
			SDL_SetColors (surf, (SDL_Color*)img->Palette, 0, 256);
		}
	}
	
	SDL_BlitSurface(surf, pSrcRect, (NativeCanvas) target, &targetRect);
	UnlockMutex (img->mutex);
}

void
TFB_DrawCanvas_FilledImage (TFB_Image *img, int x, int y, int scale, int r, int g, int b, TFB_Canvas target)
{
	SDL_Rect srcRect, targetRect, *pSrcRect;
	SDL_Surface *surf;
	int i;
	SDL_Color pal[256];

	if (img == 0)
	{
		fprintf (stderr, "ERROR: TFB_DrawCanvas_FilledImage passed null image ptr\n");
		return;
	}

	LockMutex (img->mutex);

	targetRect.x = x;
	targetRect.y = y;

	if (scale != 0 && scale != GSCALE_IDENTITY)
	{
		TFB_DrawImage_FixScaling (img, scale);
		surf = img->ScaledImg;
		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.w = img->extent.width;
		srcRect.h = img->extent.height;
		pSrcRect = &srcRect;
	}
	else
	{
		surf = img->NormalImg;
		pSrcRect = NULL;
	}
				
	for (i = 0; i < 256; i++)
	{
		pal[i].r = r;
		pal[i].g = g;
		pal[i].b = b;
	}
	SDL_SetColors (surf, pal, 0, 256);

	SDL_BlitSurface(surf, pSrcRect, (NativeCanvas) target, &targetRect);
	UnlockMutex (img->mutex);
}

TFB_Canvas TFB_DrawCanvas_New_TrueColor (int w, int h, BOOLEAN hasalpha)
{
	SDL_Surface *new_surf;
	new_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, w, h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, hasalpha ? 0xff000000 : 0);
	if (!new_surf) {
		fprintf(stderr, "INTERNAL PANIC: Failed to create TFB_Canvas: %s", SDL_GetError());
		exit(-1);
	}
	return new_surf;
}

TFB_Canvas
TFB_DrawCanvas_New_Paletted (int w, int h, TFB_Palette *palette, int transparent_index)
{
	SDL_Surface *new_surf;
	new_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
	if (!new_surf) {
		fprintf(stderr, "INTERNAL PANIC: Failed to create TFB_Canvas: %s\n", SDL_GetError());
		exit(-1);
	}
	if (palette != NULL)
	{
		SDL_SetColors(new_surf, (SDL_Color *)palette, 0, 256);
	}
	if (transparent_index >= 0)
	{
		SDL_SetColorKey (new_surf, SDL_SRCCOLORKEY, transparent_index);
	}
	else
	{
		SDL_SetColorKey (new_surf, 0, 0);
	}
	return new_surf;
}

TFB_Canvas
TFB_DrawCanvas_New_ScaleTarget (TFB_Canvas canvas)
{
	SDL_Surface *src = (SDL_Surface *)canvas;
	SDL_Surface *newsurf = SDL_CreateRGBSurface (SDL_SWSURFACE, src->w,
				src->h,
				src->format->BitsPerPixel,
				src->format->Rmask,
				src->format->Gmask,
				src->format->Bmask,
				src->format->Amask);
	if (src->format->palette)
	{
		TFB_DrawCanvas_SetTransparentIndex (newsurf, TFB_DrawCanvas_GetTransparentIndex (src));
	}
	return newsurf;
}

void
TFB_DrawCanvas_Delete (TFB_Canvas canvas)
{
	if (!canvas)
	{
		fprintf(stderr, "INTERNAL PANIC: Attempted to delete a NULL canvas!\n");
		/* Should we actually die here? */
	} 
	else
	{
		SDL_FreeSurface ((SDL_Surface *) canvas);
	}

}

TFB_Palette *
TFB_DrawCanvas_ExtractPalette (TFB_Canvas canvas)
{
	int i;		
	TFB_Palette *result = (TFB_Palette*) HMalloc (sizeof (TFB_Palette) * 256);
	SDL_Surface *surf = (SDL_Surface *)canvas;

	if (!surf->format->palette)
	{
		return NULL;
	}

	for (i = 0; i < 256; ++i)
	{
		result[i].r = surf->format->palette->colors[i].r;
		result[i].g = surf->format->palette->colors[i].g;
		result[i].b = surf->format->palette->colors[i].b;
	}
	return result;
}

TFB_Canvas
TFB_DrawCanvas_ToScreenFormat (TFB_Canvas canvas)
{
	SDL_Surface *result = TFB_DisplayFormatAlpha ((SDL_Surface *)canvas);
	if (result == NULL)
	{
		fprintf (stderr, "WARNING: Could not convert sprite-canvas to display format.\n");
		return canvas;
	}
	else
	{
		TFB_DrawCanvas_Delete (canvas);
		return result;
	}

	return canvas;
}

BOOLEAN
TFB_DrawCanvas_IsPaletted (TFB_Canvas canvas)
{
	return ((SDL_Surface *)canvas)->format->palette != NULL;
}

void
TFB_DrawCanvas_SetPalette (TFB_Canvas target, TFB_Palette *palette)
{
	SDL_SetColors ((SDL_Surface *)target, (SDL_Color *)palette, 0, 256);
}

int
TFB_DrawCanvas_GetTransparentIndex (TFB_Canvas canvas)
{
	if (((SDL_Surface *)canvas)->flags & SDL_SRCCOLORKEY)
		return ((SDL_Surface *)canvas)->format->colorkey;
	return -1;
}

void
TFB_DrawCanvas_SetTransparentIndex (TFB_Canvas canvas, int index)
{
	if (index >= 0)
	{
		SDL_SetColorKey ((SDL_Surface *)canvas, SDL_SRCCOLORKEY, index); 
	}
	else
	{
		SDL_SetColorKey ((SDL_Surface *)canvas, 0, 0); 
	}		
}

void
TFB_DrawCanvas_GetScaledExtent (TFB_Canvas src_canvas, int scale, PEXTENT size)
{
	SDL_Surface *src = (SDL_Surface *)src_canvas;
	size->width = (int) (src->w * scale / (float)GSCALE_IDENTITY);
	size->height = (int) (src->h * scale / (float)GSCALE_IDENTITY);
	if (!size->width && src->w)
		size->width = 1;
	if (!size->height && src->h)
		size->height = 1;
}

void
TFB_DrawCanvas_Rescale (TFB_Canvas src_canvas, TFB_Canvas dest_canvas, EXTENT size)
{
	SDL_Surface *src = (SDL_Surface *)src_canvas;
	SDL_Surface *dst = (SDL_Surface *)dest_canvas;
	int x, y, sx, sy, *csax, *csay, csx, csy;
	int sax[321], say[241];

	if (size.width > 320 || size.height > 240)
	{
		fprintf (stderr, "TFB_DrawCanvas_Scale: Tried to zoom an image to be larger than the screen!  Failing.\n");
		return;
	}
	if (size.width > dst->w || size.height > dst->h) 
	{
		fprintf (stderr, "TFB_DrawCanvas_Scale: Tried to scale image to size %d %d when dest_canvas has only dimensions of %d %d!  Failing.\n",
			size.width, size.height, dst->w, dst->h);
		return;
	}

	sx = (int) (65536.0 * (float) src->w / (float) size.width);
	sy = (int) (65536.0 * (float) src->h / (float) size.height);

	/*
	 * Precalculate row increments 
	 */
	csx = 0;
	csax = sax;
	for (x = 0; x <= size.width; x++) {
		*csax = csx >> 16;
		csax++;
		csx &= 0xffff;
		csx += sx;
	}
	csy = 0;
	csay = say;
	for (y = 0; y <= size.height; y++) {
		*csay = csy >> 16;
		csay++;
		csy &= 0xffff;
		csy += sy;
	}

	SDL_LockSurface (src);
	SDL_LockSurface (dst);

	if (src->format->BytesPerPixel == 1 && dst->format->BytesPerPixel == 1)
	{
		Uint8 *sp, *csp, *dp, *cdp;

		sp = csp = (Uint8 *) src->pixels;
		dp = cdp = (Uint8 *) dst->pixels;

		csay = say;
		for (y = 0; y < size.height; ++y) {
			sp = csp;
			dp = cdp;
			csax = sax;
			for (x = 0; x < size.width; ++x) {
				*dp = *sp;
				++csax;
				sp += *csax;
				++dp;
			}
			++csay;
			csp += (*csay) * src->pitch;
			cdp += dst->pitch;
		}
	}	
	else if (src->format->BytesPerPixel == 4 && dst->format->BytesPerPixel == 4)
	{
		Uint32 *sp, *csp, *dp, *cdp;
		int sgap, dgap;

		sgap = src->pitch >> 2;
		dgap = dst->pitch >> 2;
		
		sp = csp = (Uint32 *) src->pixels;
		dp = cdp = (Uint32 *) dst->pixels;

		csay = say;
		for (y = 0; y < size.height; ++y) {
			sp = csp;
			dp = cdp;
			csax = sax;
			for (x = 0; x < size.width; ++x) {
				*dp = *sp;
				++csax;
				sp += *csax;
				++dp;
			}
			++csay;
			csp += (*csay) * sgap;
			cdp += dgap;
		}
	}
	else
	{
		fprintf (stderr, "Tried to deal with unknown BPP: %d -> %d\n", src->format->BitsPerPixel, dst->format->BitsPerPixel);
	}
	SDL_UnlockSurface (dst);
	SDL_UnlockSurface (src);
}
