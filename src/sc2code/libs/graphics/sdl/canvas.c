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

#include "port.h"
#include SDL_INCLUDE(SDL.h)
#include "sdl_common.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/sdl/primitives.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/log.h"
#include "rotozoom.h"
#include "options.h"
#include "types.h"

typedef SDL_Surface *NativeCanvas;

// BYTE x BYTE weight (mult >> 8) table
static Uint8 btable[256][256];

void
TFB_DrawCanvas_Initialize (void)
{
	int i, j;
	for (i = 0; i < 256; ++i)
		for (j = 0; j < 256; ++j)
			btable[j][i] = (j * i + 0x80) >> 8;
				// need error correction here
}

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
	SDL_Surface *dst = target;
	SDL_PixelFormat *fmt = dst->format;
	Uint32 color;
	SDL_Rect sr;
	sr.x = rect->corner.x;
	sr.y = rect->corner.y;
	sr.w = rect->extent.width;
	sr.h = rect->extent.height;

	color = SDL_MapRGB (fmt, r, g, b);
	if (fmt->Amask && (dst->flags & SDL_SRCCOLORKEY))
	{	// special case -- alpha surface with colorkey
		// colorkey rects are transparent
		if ((color & ~fmt->Amask) == (fmt->colorkey & ~fmt->Amask))
			color &= ~fmt->Amask; // make transparent
	}
	
	SDL_FillRect (dst, &sr, color);
}

void
TFB_DrawCanvas_Image (TFB_Image *img, int x, int y, int scale,
		TFB_ColorMap *cmap, TFB_Canvas target)
{
	SDL_Rect srcRect, targetRect, *pSrcRect;
	SDL_Surface *surf;
	TFB_Palette* palette;

	if (img == 0)
	{
		log_add (log_Warning, "ERROR: TFB_DrawCanvas_Image passed null image ptr");
		return;
	}

	LockMutex (img->mutex);

	if (cmap)
		palette = cmap->colors;
	else
		palette = img->Palette;

	// only set the new palette if it changed
	if (((SDL_Surface *)img->NormalImg)->format->palette
			&& cmap && img->colormap_version != cmap->version)
		SDL_SetColors (img->NormalImg, (SDL_Color*)palette, 0, 256);

	if (scale != 0 && scale != GSCALE_IDENTITY)
	{
		int type;
		if (optMeleeScale == TFB_SCALE_TRILINEAR && img->MipmapImg)
		{
			type = TFB_SCALE_TRILINEAR;
			// only set the new palette if it changed
			if (((SDL_Surface *)img->MipmapImg)->format->palette
					&& cmap && img->colormap_version != cmap->version)
				SDL_SetColors (img->MipmapImg, (SDL_Color*)palette, 0, 256);
		}
		else
		{
			type = TFB_SCALE_NEAREST;
		}

		TFB_DrawImage_FixScaling (img, scale, type);
		surf = img->ScaledImg;
		if (surf->format->palette)
			SDL_SetColors (surf, (SDL_Color*)palette, 0, 256);

		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.w = img->extent.width;
		srcRect.h = img->extent.height;
		pSrcRect = &srcRect;

		targetRect.x = x - img->last_scale_hs.x;
		targetRect.y = y - img->last_scale_hs.y;
	}
	else
	{
		surf = img->NormalImg;
		pSrcRect = NULL;

		targetRect.x = x - img->NormalHs.x;
		targetRect.y = y - img->NormalHs.y;
	}
	
	if (cmap)
	{
		img->colormap_version = cmap->version;
		TFB_ReturnColorMap (cmap);
	}
	
	SDL_BlitSurface (surf, pSrcRect, (NativeCanvas) target, &targetRect);
	UnlockMutex (img->mutex);
}

void
TFB_DrawCanvas_Fill (TFB_Canvas source, int width, int height,
					Uint32 fillcolor, TFB_Canvas target)
{
	SDL_Surface *src = (SDL_Surface *)source;
	SDL_Surface *dst = (SDL_Surface *)target;
	const SDL_PixelFormat *srcfmt = src->format;
	SDL_PixelFormat *dstfmt = dst->format;
	const int bpp = dstfmt->BytesPerPixel;
	const int sp = src->pitch, dp = dst->pitch;
	const int slen = sp / bpp, dlen = dp / bpp;
	const int dsrc = slen - width, ddst = dlen - width;
	Uint32 *src_p;
	Uint32 *dst_p;
	int x, y;
	Uint32 dstkey = 0; // 0 means alpha=0 too

	if (srcfmt->BytesPerPixel != 4 || dstfmt->BytesPerPixel != 4)
	{
		log_add (log_Warning, "TFB_DrawCanvas_Fill: Unsupported surface "
				"formats: %d bytes/pixel source, %d bytes/pixel destination",
				(int)srcfmt->BytesPerPixel, (int)dstfmt->BytesPerPixel);
		return;
	}

	SDL_LockSurface(src);
	SDL_LockSurface(dst);

	src_p = (Uint32 *)src->pixels;
	dst_p = (Uint32 *)dst->pixels;

	if (dstkey == fillcolor)
	{	// color collision, must switch colorkey
		// new colorkey is grey (1/2,1/2,1/2)
		dstkey = SDL_MapRGBA (dstfmt, 127, 127, 127, 0);
	}

	if (srcfmt->Amask)
	{	// alpha-based fill
		Uint32 amask = srcfmt->Amask;

		for (y = 0; y < height; ++y)
		{
			for (x = 0; x < width; ++x, ++src_p, ++dst_p)
			{
				Uint32 p = *src_p & amask;
				
				*dst_p = (p == 0) ? dstkey : (p | fillcolor);
			}
			dst_p += ddst;
			src_p += dsrc;
		}
	}
	else if (src->flags & SDL_SRCCOLORKEY)
	{	// colorkey-based fill
		Uint32 srckey = srcfmt->colorkey;
		Uint32 notmask = ~srcfmt->Amask;

		for (y = 0; y < height; ++y)
		{
			for (x = 0; x < width; ++x, ++src_p, ++dst_p)
			{
				Uint32 p = *src_p & notmask;

				*dst_p = (p == srckey) ? dstkey : fillcolor;
			}
			dst_p += ddst;
			src_p += dsrc;
		}
	}
	else
	{
		log_add (log_Warning, "TFB_DrawCanvas_Fill: Unsupported source"
				"surface format\n");
	}

	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);

	// save the colorkey (dynamic image -- not using RLE coding here)
	SDL_SetColorKey (dst, SDL_SRCCOLORKEY, dstkey);
			// if the filled surface is RGBA, colorkey will only be used
			// when SDL_SRCALPHA flag is cleared. this allows us to blit
			// the surface in different ways to diff targets
}

void
TFB_DrawCanvas_FilledImage (TFB_Image *img, int x, int y, int scale, int r, int g, int b, TFB_Canvas target)
{
	SDL_Rect srcRect, targetRect, *pSrcRect;
	SDL_Surface *surf;
	int i;
	SDL_Color pal[256];
	EXTENT prevextent = img->extent;

	if (img == 0)
	{
		log_add (log_Warning, "ERROR: TFB_DrawCanvas_FilledImage passed null image ptr");
		return;
	}

	LockMutex (img->mutex);

	if (scale != 0 && scale != GSCALE_IDENTITY)
	{
		TFB_DrawImage_FixScaling (img, scale, TFB_SCALE_NEAREST);
		surf = img->ScaledImg;
		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.w = img->extent.width;
		srcRect.h = img->extent.height;
		pSrcRect = &srcRect;

		targetRect.x = x - img->last_scale_hs.x;
		targetRect.y = y - img->last_scale_hs.y;
	}
	else
	{
		surf = img->NormalImg;
		pSrcRect = NULL;
		
		targetRect.x = x - img->NormalHs.x;
		targetRect.y = y - img->NormalHs.y;
	}

	if (surf->format->palette)
	{	// set palette for fill-stamp
		for (i = 0; i < 256; i++)
		{
			pal[i].r = r;
			pal[i].g = g;
			pal[i].b = b;
		}
		SDL_SetColors (surf, pal, 0, 256);
		// reflect the change in *actual* image palette
		img->colormap_version--;
	}
	else
	{	// fill the non-transparent parts of the image with fillcolor
		SDL_Surface *src = img->NormalImg;
		SDL_Surface *newfill = img->FilledImg;
		BOOLEAN force = FALSE;

		// prepare the filled image
		if (!newfill)
		{
			newfill = SDL_CreateRGBSurface (SDL_SWSURFACE,
						src->w, src->h,
						surf->format->BitsPerPixel,
						surf->format->Rmask,
						surf->format->Gmask,
						surf->format->Bmask,
						surf->format->Amask);
			force = TRUE;
		}

		if (force ||
				prevextent.height != img->extent.height ||
				prevextent.width != img->extent.width ||
				img->last_fill.r != r ||
				img->last_fill.g != g ||
				img->last_fill.b != b)
		{	// image or fillcolor changed - regenerate
			TFB_DrawCanvas_Fill (surf, img->extent.width, img->extent.height,
					SDL_MapRGBA (newfill->format, r, g, b, 0), newfill);
					// important to keep alpha=0 in fillcolor
					// -- we process alpha ourselves
		}

		// cache filled image if possible
		img->last_fill.r = r;
		img->last_fill.g = g;
		img->last_fill.b = b;
		img->FilledImg = newfill;

		surf = newfill;
	}

	SDL_BlitSurface (surf, pSrcRect, (NativeCanvas) target, &targetRect);
	UnlockMutex (img->mutex);
}

void
TFB_DrawCanvas_FontChar (TFB_Char *fontChar, TFB_Image *backing,
		int x, int y, TFB_Canvas target)
{
	SDL_Rect srcRect, targetRect;
	SDL_Surface *surf;
	int w, h;

	if (fontChar == 0)
	{
		log_add (log_Warning, "ERROR: "
				"TFB_DrawCanvas_FontChar passed null char ptr");
		return;
	}
	if (backing == 0)
	{
		log_add (log_Warning, "ERROR: "
				"TFB_DrawCanvas_FontChar passed null backing ptr");
		return;
	}

	w = fontChar->extent.width;
	h = fontChar->extent.height;

	LockMutex (backing->mutex);

	surf = backing->NormalImg;
	if (surf->format->BytesPerPixel != 4
			|| surf->w < w || surf->h < h)
	{
		log_add (log_Warning, "ERROR: "
				"TFB_DrawCanvas_FontChar bad backing surface: %dx%dx%d; "
				"char: %dx%d",
				surf->w, surf->h, (int)surf->format->BytesPerPixel, w, h);
		UnlockMutex (backing->mutex);
		return;
	}

	srcRect.x = 0;
	srcRect.y = 0;
	srcRect.w = fontChar->extent.width;
	srcRect.h = fontChar->extent.height;

	targetRect.x = x - fontChar->HotSpot.x;
	targetRect.y = y - fontChar->HotSpot.y;

	// transfer the alpha channel to the backing surface
	SDL_LockSurface (surf);
	{
		int x, y;
		const int sskip = fontChar->pitch - w;
		const int dskip = (surf->pitch / 4) - w;
		const Uint32 dmask = ~surf->format->Amask;
		const int ashift = surf->format->Ashift;
		Uint8 *src_p = fontChar->data;
		Uint32 *dst_p = (Uint32 *)surf->pixels;

		for (y = 0; y < h; ++y, src_p += sskip, dst_p += dskip)
		{
			for (x = 0; x < w; ++x, ++src_p, ++dst_p)
			{
				*dst_p = (*dst_p & dmask) | (((Uint32)*src_p) << ashift);
			}
		}
	}
	SDL_UnlockSurface (surf);

	SDL_BlitSurface (surf, &srcRect, (NativeCanvas) target, &targetRect);
	UnlockMutex (backing->mutex);
}

TFB_Canvas
TFB_DrawCanvas_New_TrueColor (int w, int h, BOOLEAN hasalpha)
{
	SDL_Surface *new_surf;
	SDL_PixelFormat* fmt = format_conv_surf->format;

	new_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, w, h,
			fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask,
			hasalpha ? fmt->Amask : 0);
	if (!new_surf)
	{
		log_add (log_Fatal, "INTERNAL PANIC: Failed to create TFB_Canvas: %s",
				SDL_GetError());
		exit (EXIT_FAILURE);
	}
	return new_surf;
}

TFB_Canvas
TFB_DrawCanvas_New_ForScreen (int w, int h, BOOLEAN withalpha)
{
	SDL_Surface *new_surf;
	SDL_PixelFormat* fmt = SDL_Screen->format;

	if (fmt->palette)
	{
		log_add (log_Warning, "TFB_DrawCanvas_New_ForScreen() WARNING:"
				"Paletted display format will be slow");

		new_surf = TFB_DrawCanvas_New_TrueColor (w, h, withalpha);
	}
	else
	{
		if (withalpha && fmt->Amask == 0)
			fmt = format_conv_surf->format; // Screen has no alpha and we need it

		new_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, w, h,
				fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask,
				withalpha ? fmt->Amask : 0);
	}

	if (!new_surf)
	{
		log_add (log_Fatal, "TFB_DrawCanvas_New_ForScreen() INTERNAL PANIC:"
				"Failed to create TFB_Canvas: %s", SDL_GetError());
		exit (EXIT_FAILURE);
	}
	return new_surf;
}

TFB_Canvas
TFB_DrawCanvas_New_Paletted (int w, int h, TFB_Palette *palette, int transparent_index)
{
	SDL_Surface *new_surf;
	new_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
	if (!new_surf)
	{
		log_add (log_Fatal, "INTERNAL PANIC: Failed to create TFB_Canvas: %s",
				SDL_GetError());
		exit (EXIT_FAILURE);
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
TFB_DrawCanvas_New_ScaleTarget (TFB_Canvas canvas, TFB_Canvas oldcanvas, int type, int last_type)
{
	SDL_Surface *src = (SDL_Surface *)canvas;
	SDL_Surface *old = (SDL_Surface *)oldcanvas;
	SDL_Surface *newsurf = old;

	if (type == TFB_SCALE_NEAREST)
	{
		if (old && type != last_type)
		{
			TFB_DrawCanvas_Delete (old);
			old = NULL;
		}
		if (!old)
		{
			newsurf = SDL_CreateRGBSurface (SDL_SWSURFACE, src->w,
						src->h,
						src->format->BitsPerPixel,
						src->format->Rmask,
						src->format->Gmask,
						src->format->Bmask,
						src->format->Amask);
			if (src->format->palette)
				TFB_DrawCanvas_SetTransparentIndex (newsurf, TFB_DrawCanvas_GetTransparentIndex (src), FALSE);
			else
			{
				int r, g, b;
				if (TFB_DrawCanvas_GetTransparentColor (src, &r, &g, &b))
					TFB_DrawCanvas_SetTransparentColor (newsurf, r, g, b, FALSE);
			}
		}
	}
	else
	{
		if (old && type != last_type)
		{
			TFB_DrawCanvas_Delete (old);
			old = NULL;
		}
		if (!old)
		{
			if (SDL_Screen->format->BitsPerPixel == 32)
				newsurf = TFB_DrawCanvas_New_ForScreen (src->w, src->h, TRUE);
			else
				newsurf = TFB_DrawCanvas_New_TrueColor (src->w, src->h, TRUE);
		}
	}
		
	return newsurf;
}

TFB_Canvas
TFB_DrawCanvas_New_RotationTarget (TFB_Canvas src_canvas, int angle)
{
	SDL_Surface *src = (SDL_Surface *)src_canvas;
	SDL_Surface *newsurf;
	EXTENT size;

	TFB_DrawCanvas_GetRotatedExtent (src_canvas, angle, &size);
	
	newsurf = SDL_CreateRGBSurface (SDL_SWSURFACE,
				size.width, size.height,
				src->format->BitsPerPixel,
				src->format->Rmask,
				src->format->Gmask,
				src->format->Bmask,
				src->format->Amask);
	if (!newsurf)
	{
		log_add (log_Fatal, "TFB_DrawCanvas_New_RotationTarget()"
				" INTERNAL PANIC: Failed to create TFB_Canvas: %s",
				SDL_GetError());
		exit (EXIT_FAILURE);
	}
	if (src->format->palette)
		TFB_DrawCanvas_SetTransparentIndex (newsurf, TFB_DrawCanvas_GetTransparentIndex (src), FALSE);
	else
	{
		int r, g, b;
		if (TFB_DrawCanvas_GetTransparentColor (src, &r, &g, &b))
			TFB_DrawCanvas_SetTransparentColor (newsurf, r, g, b, FALSE);
	}
	
	return newsurf;
}

void
TFB_DrawCanvas_Delete (TFB_Canvas canvas)
{
	if (!canvas)
	{
		log_add (log_Warning, "INTERNAL PANIC: Attempted"
				" to delete a NULL canvas!");
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
	TFB_Palette *result;

	SDL_Surface *surf = (SDL_Surface *)canvas;

	if (!surf->format->palette)
	{
		return NULL;
	}

	result = (TFB_Palette*) HMalloc (sizeof (TFB_Palette) * 256);
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
		log_add (log_Debug, "WARNING: Could not convert"
				" sprite-canvas to display format.");
		return canvas;
	}
	else if (result == canvas)
	{	// no conversion was necessary
		return canvas;
	}
	else
	{	// converted
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
TFB_DrawCanvas_SetTransparentIndex (TFB_Canvas canvas, int index, BOOLEAN rleaccel)
{
	if (index >= 0)
	{
		int flags = SDL_SRCCOLORKEY;
		if (rleaccel)
			flags |= SDL_RLEACCEL;
		SDL_SetColorKey ((SDL_Surface *)canvas, flags, index);
		
		if (!TFB_DrawCanvas_IsPaletted (canvas))
		{
			// disables alpha channel so color key transparency actually works
			SDL_SetAlpha ((SDL_Surface *)canvas, 0, 255); 
		}
	}
	else
	{
		SDL_SetColorKey ((SDL_Surface *)canvas, 0, 0); 
	}		
}

BOOLEAN
TFB_DrawCanvas_GetTransparentColor (TFB_Canvas canvas, int *r, int *g, int *b)
{
	if (!TFB_DrawCanvas_IsPaletted (canvas)
			&& (((SDL_Surface *)canvas)->flags & SDL_SRCCOLORKEY) )
	{
		Uint8 ur, ug, ub;
		int colorkey = ((SDL_Surface *)canvas)->format->colorkey;
		SDL_GetRGB (colorkey, ((SDL_Surface *)canvas)->format, &ur, &ug, &ub);
		*r = ur;
		*g = ug;
		*b = ub;
		return TRUE;
	}
	return FALSE;
}

void
TFB_DrawCanvas_SetTransparentColor (TFB_Canvas canvas, int r, int g, int b, BOOLEAN rleaccel)
{
	Uint32 color;
	int flags = SDL_SRCCOLORKEY;
	if (rleaccel)
		flags |= SDL_RLEACCEL;
	color = SDL_MapRGBA (((SDL_Surface *)canvas)->format, r, g, b, 0);
	SDL_SetColorKey ((SDL_Surface *)canvas, flags, color);
	
	if (!TFB_DrawCanvas_IsPaletted (canvas))
	{
		// disables alpha channel so color key transparency actually works
		SDL_SetAlpha ((SDL_Surface *)canvas, 0, 255); 
	}
}

void
TFB_DrawCanvas_GetScaledExtent (TFB_Canvas src_canvas, HOT_SPOT src_hs,
		TFB_Canvas src_mipmap, HOT_SPOT mm_hs,
		int scale, PEXTENT size, HOT_SPOT *hs)
{
	SDL_Surface *src = (SDL_Surface *)src_canvas;
	
	if (!src_mipmap)
	{
		size->width  = src->w * scale / GSCALE_IDENTITY;
		size->height = src->h * scale / GSCALE_IDENTITY;

		hs->x = src_hs.x * scale / GSCALE_IDENTITY;
		hs->y = src_hs.y * scale / GSCALE_IDENTITY;
	}
	else
	{
		// interpolates extents between src and mipmap to get smoother
		// transition when surface changes
		SDL_Surface *mipmap = (SDL_Surface *)src_mipmap;

		float ratio = (scale / (float)GSCALE_IDENTITY) * 2.0f - 1.0f;
		if (ratio < 0.0f)
			ratio = 0.0f;
		else if (ratio > 1.0f)
			ratio = 1.0f;

		size->width = (int)((src->w - mipmap->w) * ratio + mipmap->w + 0.5);
		size->height = (int)((src->h - mipmap->h) * ratio + mipmap->h + 0.5);

		hs->x = (int)((src_hs.x - mm_hs.x) * ratio + mm_hs.x + 0.5);
		hs->y = (int)((src_hs.y - mm_hs.y) * ratio + mm_hs.y + 0.5);
	}
		
	if (!size->width && src->w)
		size->width = 1;
	if (!size->height && src->h)
		size->height = 1;
}

void
TFB_DrawCanvas_GetExtent (TFB_Canvas canvas, PEXTENT size)
{
	SDL_Surface *src = (SDL_Surface *)canvas;

	size->width = src->w;
	size->height = src->h;
}

void
TFB_DrawCanvas_Rescale_Nearest (TFB_Canvas src_canvas, TFB_Canvas dest_canvas, EXTENT size)
{
#define NNS_MAX_DIMS 600	
	SDL_Surface *src = (SDL_Surface *)src_canvas;
	SDL_Surface *dst = (SDL_Surface *)dest_canvas;
	int x, y, sx, sy, *csax, *csay, csx, csy;
	int saspace[NNS_MAX_DIMS];
	int *sax, *say;

	if (size.width + size.height > NNS_MAX_DIMS)
	{
		log_add (log_Warning, "TFB_DrawCanvas_Scale: Tried to zoom"
				" an image to unreasonable size! Failing.");
		return;
	}
	if (size.width > dst->w || size.height > dst->h) 
	{
		log_add (log_Warning, "TFB_DrawCanvas_Scale: Tried to scale"
				" image to size %d %d when dest_canvas has only"
				" dimensions of %d %d! Failing.",
				size.width, size.height, dst->w, dst->h);
		return;
	}

	sx = sy = 0;
	if (size.width  > 1)
		sx = ((src->w - 1) << 16) / (size.width  - 1);
	if (size.height > 1)
		sy = ((src->h - 1) << 16) / (size.height - 1);

	sax = saspace;
	say = saspace + size.width;
	/*
	 * Precalculate row increments 
	 * We start with a value in 0..0.5 range to shift the bigger
	 * jumps towards the center of the image
	 */
	csax = sax;
	for (x = 0, csx = 0x6000; x < size.width; x++) {
		*csax = csx >> 16;
		csax++;
		csx &= 0xffff;
		csx += sx;
	}
	csay = say;
	for (y = 0, csy = 0x6000; y < size.height; y++) {
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
			csp += (*csay) * src->pitch;
			sp = csp;
			dp = cdp;
			csax = sax;
			for (x = 0; x < size.width; ++x) {
				sp += *csax;
				*dp = *sp;
				++csax;
				++dp;
			}
			++csay;
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
			csp += (*csay) * sgap;
			sp = csp;
			dp = cdp;
			csax = sax;
			for (x = 0; x < size.width; ++x) {
				sp += *csax;
				*dp = *sp;
				++csax;
				++dp;
			}
			++csay;
			cdp += dgap;
		}
	}
	else
	{
		log_add (log_Warning, "Tried to deal with unknown BPP: %d -> %d",
				src->format->BitsPerPixel, dst->format->BitsPerPixel);
	}
	SDL_UnlockSurface (dst);
	SDL_UnlockSurface (src);
}

typedef union
{
	Uint32 value;
	Uint8 chan[4];
	struct
	{
		Uint8 r, g, b, a;
	} c;
} pixel_t;

static inline Uint8
dot_product_8_4 (pixel_t* p, int c, Uint8* v)
{	// math expanded for speed
#if 0	
	return (
			(Uint32)p[0].chan[c] * v[0] + (Uint32)p[1].chan[c] * v[1] +
			(Uint32)p[2].chan[c] * v[2] + (Uint32)p[3].chan[c] * v[3]
		) >> 8;
#else
	// mult-table driven version
	return
			btable[p[0].chan[c]][v[0]] + btable[p[1].chan[c]][v[1]] +
			btable[p[2].chan[c]][v[2]] + btable[p[3].chan[c]][v[3]];
#endif
}

static inline Uint8
weight_product_8_4 (pixel_t* p, int c, Uint8* w)
{	// math expanded for speed
	return (
			(Uint32)p[0].chan[c] * w[0] + (Uint32)p[1].chan[c] * w[1] +
			(Uint32)p[2].chan[c] * w[2] + (Uint32)p[3].chan[c] * w[3]
		) / (w[0] + w[1] + w[2] + w[3]);
}

static inline Uint8
blend_ratio_2 (Uint8 c1, Uint8 c2, int ratio)
{	// blend 2 color values according to ratio (0..256)
	// identical to proper alpha blending
	return (((c1 - c2) * ratio) >> 8) + c2;
}

static inline Uint32
tri_get_pixel (void* ppix, SDL_PixelFormat *fmt, SDL_Color *pal,
				Uint32 mask, Uint32 key)
{
	pixel_t p;

	// start off with non-present pixel
	p.value = 0;

	if (pal)
	{	// paletted pixel; mask not used
		Uint32 c = *(Uint8 *)ppix;
		
		if (c != key)
		{
			p.c.r = pal[c].r;
			p.c.g = pal[c].g;
			p.c.b = pal[c].b;
			p.c.a = SDL_ALPHA_OPAQUE;
		}
	}
	else
	{	// RGB(A) pixel; (pix & mask) != key
		Uint32 c = *(Uint32 *)ppix;

		if ((c & mask) != key)
		{
#if 0
			SDL_GetRGBA (c, fmt, &p.c.r, &p.c.g, &p.c.b, &p.c.a);
#else
			p.c.r = (c >> fmt->Rshift) & 0xff;
			p.c.g = (c >> fmt->Gshift) & 0xff;
			p.c.b = (c >> fmt->Bshift) & 0xff;
			if (fmt->Amask)
				p.c.a = (c >> fmt->Ashift) & 0xff;
			else
				p.c.a = SDL_ALPHA_OPAQUE;
		}
#endif
	}

	return p.value;
}

void
TFB_DrawCanvas_Rescale_Trilinear (TFB_Canvas src_canvas,
		TFB_Canvas dest_canvas, TFB_Canvas src_mipmap, EXTENT size)
{
	SDL_Surface *src = (SDL_Surface *)src_canvas;
	SDL_Surface *dst = (SDL_Surface *)dest_canvas;
	SDL_Surface *mipmap = (SDL_Surface *)src_mipmap;
	SDL_PixelFormat *srcfmt = src->format;
	SDL_PixelFormat *mmfmt = mipmap->format;
	SDL_PixelFormat *dstfmt = dst->format;
	SDL_Color *srcpal = srcfmt->palette? srcfmt->palette->colors : 0;
	SDL_Color *mmpal = mmfmt->palette ? mmfmt->palette->colors : 0;
	const int sbpp = srcfmt->BytesPerPixel;
	const int mmbpp = mmfmt->BytesPerPixel;
	const int slen = src->pitch;
	const int mmlen = mipmap->pitch;
	const int dst_has_alpha = (dstfmt->Amask != 0);
	const int transparent = (dst->flags & SDL_SRCCOLORKEY) ?
			dstfmt->colorkey : 0;
	const int w = size.width, h = size.height;
	const int alpha_threshold = dst_has_alpha ? 0 : 127;
	// src v. mipmap importance factor
	int ratio;
	// source masks and keys
	Uint32 mk0 = 0, ck0 = ~0, mk1 = 0, ck1 = ~0;
	// source fractional x and y positions
	int sx0, sy0, sx1, sy1;
	// source fractional dx and dy increments
	int fsx0 = 0, fsy0 = 0, fsx1 = 0, fsy1 = 0;
	// source fractional x and y starting points
	int ssx0 = 0, ssy0 = 0, ssx1 = 0, ssy1 = 0;
	int x, y;

	fsx0 = (src->w << 16) / w;
	fsy0 = (src->h << 16) / h;

	if (w > 1)
		fsx1 = ((mipmap->w - 1) << 16) / (w - 1);
	if (h > 1)
		fsy1 = ((mipmap->h - 1) << 16) / (h - 1);

	// give equal importance to both edges
	ssx0 = (((src->w - 1) << 16) - fsx0 * (w - 1)) >> 1;
	ssy0 = (((src->h - 1) << 16) - fsy0 * (h - 1)) >> 1;

	ssx1 = (((mipmap->w - 1) << 16) - fsx1 * (w - 1)) >> 1;
	ssy1 = (((mipmap->h - 1) << 16) - fsy1 * (h - 1)) >> 1;

	// src v. mipmap importance factor
	ratio = (w << 9) / src->w - 256;
	if (ratio < 0)
		ratio = 0;
	if (ratio > 256)
		ratio = 256;

	if (size.width > dst->w || size.height > dst->h) 
	{
		log_add (log_Warning, "TFB_DrawCanvas_Rescale_Trilinear: "
				"Tried to scale image to size %d %d when dest_canvas"
				" has only dimensions of %d %d! Failing.",
				size.width, size.height, dst->w, dst->h);
		return;
	}

	if ((srcfmt->BytesPerPixel != 1 && srcfmt->BytesPerPixel != 4) ||
		(mmfmt->BytesPerPixel != 1 && mmfmt->BytesPerPixel != 4) ||
		(dst->format->BytesPerPixel != 4))
	{
		log_add (log_Warning, "TFB_DrawCanvas_Rescale_Trilinear: "
				"Tried to deal with unknown BPP: %d -> %d, mipmap %d",
				srcfmt->BitsPerPixel, dst->format->BitsPerPixel,
				mmfmt->BitsPerPixel);
		return;
	}

	// use colorkeys where appropriate
	if (srcfmt->Amask)
	{	// alpha transparency
		mk0 = srcfmt->Amask;
		ck0 = 0;
	}
	else if (src->flags & SDL_SRCCOLORKEY)
	{	// colorkey transparency
		mk0 = ~srcfmt->Amask;
		ck0 = srcfmt->colorkey & mk0;
	}

	if (mmfmt->Amask)
	{	// alpha transparency
		mk1 = mmfmt->Amask;
		ck1 = 0;
	}
	else if (mipmap->flags & SDL_SRCCOLORKEY)
	{	// colorkey transparency
		mk1 = ~mmfmt->Amask;
		ck1 = mmfmt->colorkey & mk1;
	}

	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	SDL_LockSurface(mipmap);
	
	for (y = 0, sy0 = ssy0, sy1 = ssy1;
			y < h;
			++y, sy0 += fsy0, sy1 += fsy1)
	{
		Uint32 *dst_p = (Uint32 *) ((Uint8*)dst->pixels + y * dst->pitch);
		Uint8 *src_a0 = (Uint8*)src->pixels + (sy0 >> 16) * slen;
		Uint8 *src_a1 = (Uint8*)mipmap->pixels + (sy1 >> 16) * mmlen;
		// retrieve the fractional portions of y
		const Uint8 v0 = (sy0 >> 8) & 0xff;
		const Uint8 v1 = (sy1 >> 8) & 0xff;
		Uint8 w0[4], w1[4]; // pixel weight vectors

		for (x = 0, sx0 = ssx0, sx1 = ssx1;
				x < w;
				++x, ++dst_p, sx0 += fsx0, sx1 += fsx1)
		{
			Uint8 *src_p0 = src_a0 + (sx0 >> 16) * sbpp;
			Uint8 *src_p1 = src_a1 + (sx1 >> 16) * mmbpp;
			// retrieve the fractional portions of x
			const Uint8 u0 = (sx0 >> 8) & 0xff;
			const Uint8 u1 = (sx1 >> 8) & 0xff;
			// pixels are examined and numbered in pattern
			//  0  1
			//  2  3
			// the ideal pixel (4) is somewhere between these four
			// and is calculated from these using weight vector (w)
			// with a dot product
			pixel_t p0[5], p1[5];
			Uint8 res_a;
			
			w0[0] = btable[255 - u0][255 - v0];
			w0[1] = btable[u0][255 - v0];
			w0[2] = btable[255 - u0][v0];
			w0[3] = btable[u0][v0];
			w1[0] = btable[255 - u1][255 - v1];
			w1[1] = btable[u1][255 - v1];
			w1[2] = btable[255 - u1][v1];
			w1[3] = btable[u1][v1];

			// collect interesting pixels from src image
			p0[0].value = tri_get_pixel (src_p0, srcfmt, srcpal, mk0, ck0);

			if ((sx0 >> 16) <= src->w - 2)
			{						
				p0[1].value = tri_get_pixel (src_p0 + sbpp,
						srcfmt, srcpal, mk0, ck0);
				
				if ((sy0 >> 16) <= src->h - 2)
				{
					p0[2].value = tri_get_pixel (src_p0 + slen,
							srcfmt, srcpal, mk0, ck0);
					p0[3].value = tri_get_pixel (src_p0 + slen + sbpp,
							srcfmt, srcpal, mk0, ck0);
				}
				else
				{
					p0[2].value = p0[0].value;
					p0[3].value = p0[1].value;
				}
			}
			else
			{
				p0[1].value = p0[0].value;
				if ((sy0 >> 16) <= src->h - 2)
				{
					p0[2].value = tri_get_pixel (src_p0 + slen,
							srcfmt, srcpal, mk0, ck0);
				}
				else
				{
					p0[2].value = p0[0].value;
				}
				p0[3].value = p0[2].value;
			}

			// collect interesting pixels from mipmap image
			p1[0].value = tri_get_pixel (src_p1, mmfmt, mmpal, mk1, ck1);

			if ((sx1 >> 16) <= mipmap->w - 2)
			{						
				p1[1].value = tri_get_pixel (src_p1 + mmbpp,
						mmfmt, mmpal, mk1, ck1);

				if ((sy1 >> 16) <= mipmap->h - 2)
				{
					p1[2].value = tri_get_pixel (src_p1 + mmlen,
							mmfmt, mmpal, mk1, ck1);
					p1[3].value = tri_get_pixel (src_p1 + mmlen + mmbpp,
							mmfmt, mmpal, mk1, ck1);
				}
				else
				{
					p1[2].value = p1[0].value;
					p1[3].value = p1[1].value;
				}
			}
			else
			{
				p1[1].value = p1[0].value;
				if ((sy1 >> 16) <= mipmap->h - 2)
				{
					p1[2].value = tri_get_pixel (src_p1 + mmlen,
							mmfmt, mmpal, mk1, ck1);
				}
				else
				{
					p1[2].value = p1[0].value;
				}
				p1[3].value = p1[2].value;
			}

			p0[4].c.a = dot_product_8_4 (p0, 3, w0);
			p1[4].c.a = dot_product_8_4 (p1, 3, w1);
			
			res_a = blend_ratio_2 (p0[4].c.a, p1[4].c.a, ratio);

			if (res_a <= alpha_threshold)
			{
				*dst_p = transparent;
			}
			else if (!dst_has_alpha)
			{	// RGB surface handling
				p0[4].c.r = dot_product_8_4 (p0, 0, w0);
				p0[4].c.g = dot_product_8_4 (p0, 1, w0);
				p0[4].c.b = dot_product_8_4 (p0, 2, w0);

				p1[4].c.r = dot_product_8_4 (p1, 0, w1);
				p1[4].c.g = dot_product_8_4 (p1, 1, w1);
				p1[4].c.b = dot_product_8_4 (p1, 2, w1);

				p0[4].c.r = blend_ratio_2 (p0[4].c.r, p1[4].c.r, ratio);
				p0[4].c.g = blend_ratio_2 (p0[4].c.g, p1[4].c.g, ratio);
				p0[4].c.b = blend_ratio_2 (p0[4].c.b, p1[4].c.b, ratio);

				*dst_p =
					(p0[4].c.r << dstfmt->Rshift) |
					(p0[4].c.g << dstfmt->Gshift) |
					(p0[4].c.b << dstfmt->Bshift);
			}
			else
			{	// RGBA surface handling

				// we do not want to blend with non-present pixels
				// (pixels that have alpha == 0) as these will
				// skew the result and make resulting alpha useless
				if (p0[4].c.a != 0)
				{
					int i;
					for (i = 0; i < 4; ++i)
						if (p0[i].c.a == 0)
							w0[i] = 0;

					p0[4].c.r = weight_product_8_4 (p0, 0, w0);
					p0[4].c.g = weight_product_8_4 (p0, 1, w0);
					p0[4].c.b = weight_product_8_4 (p0, 2, w0);
				}
				if (p1[4].c.a != 0)
				{
					int i;
					for (i = 0; i < 4; ++i)
						if (p1[i].c.a == 0)
							w1[i] = 0;

					p1[4].c.r = weight_product_8_4 (p1, 0, w1);
					p1[4].c.g = weight_product_8_4 (p1, 1, w1);
					p1[4].c.b = weight_product_8_4 (p1, 2, w1);
				}

				if (p0[4].c.a != 0 && p1[4].c.a != 0)
				{	// blend if both present
					p0[4].c.r = blend_ratio_2 (p0[4].c.r, p1[4].c.r, ratio);
					p0[4].c.g = blend_ratio_2 (p0[4].c.g, p1[4].c.g, ratio);
					p0[4].c.b = blend_ratio_2 (p0[4].c.b, p1[4].c.b, ratio);
				}
				else if (p1[4].c.a != 0)
				{	// other pixel is present
					p0[4].value = p1[4].value;
				}

				// error-correct alpha to fully opaque to remove
				// the often unwanted and unnecessary blending
				if (res_a > 0xf8)
					res_a = 0xff;

				*dst_p =
					(p0[4].c.r << dstfmt->Rshift) |
					(p0[4].c.g << dstfmt->Gshift) |
					(p0[4].c.b << dstfmt->Bshift) |
					(res_a << dstfmt->Ashift);
			}
		}
	}

	SDL_UnlockSurface(mipmap);
	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);
}

void
TFB_DrawCanvas_GetScreenFormat (TFB_PixelFormat *fmt)
{
	SDL_PixelFormat *sdl = SDL_Screen->format;
	
	if (sdl->palette)
	{
		log_add (log_Warning, "TFB_DrawCanvas_GetScreenFormat() WARNING:"
				"Paletted display format will be slow");

		fmt->BitsPerPixel = 32;
		fmt->Rmask = 0x000000ff;
		fmt->Gmask = 0x0000ff00;
		fmt->Bmask = 0x00ff0000;
		fmt->Amask = 0xff000000;
	}
	else
	{
		fmt->BitsPerPixel = sdl->BitsPerPixel;
		fmt->Rmask = sdl->Rmask;
		fmt->Gmask = sdl->Gmask;
		fmt->Bmask = sdl->Bmask;
		fmt->Amask = sdl->Amask;
	}
}

void*
TFB_DrawCanvas_GetLine (TFB_Canvas canvas, int line)
{
	SDL_Surface* surf = (SDL_Surface *)canvas;
	return (uint8*)surf->pixels + surf->pitch * line;
}

void
TFB_DrawCanvas_GetPixel (TFB_Canvas canvas, int x, int y, int *r, int *g, int *b)
{
	SDL_Surface* surf = (SDL_Surface *)canvas;
	Uint8 ur, ug, ub;
	Uint32 pixel;
	GetPixelFn getpixel;

	getpixel = getpixel_for(surf);
	pixel = (*getpixel)(surf, x, y);
	SDL_GetRGB (pixel, surf->format, &ur, &ug, &ub);
	*r = ur;
	*g = ug;
	*b = ub;
}

void
TFB_DrawCanvas_Rotate (TFB_Canvas src_canvas, TFB_Canvas dst_canvas, int angle, EXTENT size)
{
	SDL_Surface *src = (SDL_Surface *)src_canvas;
	SDL_Surface *dst = (SDL_Surface *)dst_canvas;
	int ret;
	int r, g, b;

	if (size.width > dst->w || size.height > dst->h) 
	{
		log_add (log_Warning, "TFB_DrawCanvas_Rotate: Tried to rotate"
				" image to size %d %d when dst_canvas has only dimensions"
				" of %d %d! Failing.",
				size.width, size.height, dst->w, dst->h);
		return;
	}

	if (TFB_DrawCanvas_GetTransparentColor (src, &r, &g, &b))
	{
		TFB_DrawCanvas_SetTransparentColor (dst, r, g, b, FALSE);
		/* fill destination with transparent color before rotating */
		SDL_FillRect(dst, NULL, SDL_MapRGBA (dst->format, r, g, b, 0));
	}

	ret = rotateSurface (src, dst, angle, 0);
	if (ret != 0)
	{
		log_add (log_Warning, "TFB_DrawCanvas_Rotate: WARNING:"
				" actual rotation func returned failure\n");
	}
}

void
TFB_DrawCanvas_GetRotatedExtent (TFB_Canvas src_canvas, int angle, PEXTENT size)
{
	int dstw, dsth;
	SDL_Surface *src = (SDL_Surface *)src_canvas;
	
	rotozoomSurfaceSize (src->w, src->h, angle, 1, &dstw, &dsth);
	size->height = dsth;
	size->width = dstw;
}
