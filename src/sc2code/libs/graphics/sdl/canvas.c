#include "SDL.h"
#include "sdl_common.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/sdl/primitives.h"
#include "libs/graphics/tfb_draw.h"
#include "options.h"

typedef SDL_Surface *NativeCanvas;

static Uint8 btable[256][256];

void
TFB_DrawCanvas_Initialize (void)
{
	int i, j;
	for (i = 0; i < 256; ++i)
		for (j = 0; j < 256; ++j)
			btable[j][i] = (j * i) >> 8;
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
		int type;
		if (optMeleeScale == TFB_SCALE_TRILINEAR && img->MipmapImg)
		{
			type = TFB_SCALE_TRILINEAR;
			if (palette)
			{
				if (((SDL_Surface *)img->NormalImg)->format->palette)
					SDL_SetColors (img->NormalImg, (SDL_Color*)palette, 0, 256);
				if (((SDL_Surface *)img->MipmapImg)->format->palette)
					SDL_SetColors (img->MipmapImg, (SDL_Color*)palette, 0, 256);
			}
		}
		else
		{
			type = TFB_SCALE_NEAREST;
		}

		TFB_DrawImage_FixScaling (img, scale, type);
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
		TFB_DrawImage_FixScaling (img, scale, TFB_SCALE_NEAREST);
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
				TFB_DrawCanvas_SetTransparentIndex (newsurf, TFB_DrawCanvas_GetTransparentIndex (src));
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
			newsurf = SDL_CreateRGBSurface (SDL_SWSURFACE, src->w, src->h, 32,
				0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
		}
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
TFB_DrawCanvas_GetScaledExtent (TFB_Canvas src_canvas, TFB_Canvas src_mipmap, int scale, PEXTENT size)
{
	SDL_Surface *src = (SDL_Surface *)src_canvas;
	
	if (!src_mipmap)
	{
		size->width = (int) (src->w * scale / (float)GSCALE_IDENTITY);
		size->height = (int) (src->h * scale / (float)GSCALE_IDENTITY);
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

		size->width = (int)((src->w - mipmap->w) * ratio + mipmap->w);
		size->height = (int)((src->h - mipmap->h) * ratio + mipmap->h);
	}
		
	if (!size->width && src->w)
		size->width = 1;
	if (!size->height && src->h)
		size->height = 1;
}

void
TFB_DrawCanvas_Rescale_Nearest (TFB_Canvas src_canvas, TFB_Canvas dest_canvas, EXTENT size)
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

void
TFB_DrawCanvas_Rescale_Trilinear (TFB_Canvas src_canvas, TFB_Canvas dest_canvas, TFB_Canvas src_mipmap, EXTENT size)
{
	SDL_Surface *src = (SDL_Surface *)src_canvas;
	SDL_Surface *dst = (SDL_Surface *)dest_canvas;
	SDL_Surface *mipmap = (SDL_Surface *)src_mipmap;
	SDL_PixelFormat *dstfmt = dst->format;
	const int dstopaque = 0xff << dstfmt->Ashift;
	const int w = size.width, h = size.height;
	const int ALPHA_THRESHOLD = 128;
	
	int ratio = (int)(256.0f * (((float)w / src->w) * 2.0f - 1.0f));
	if (ratio < 0)
		ratio = 0;
	if (ratio > 256)
		ratio = 256;

	if (size.width > dst->w || size.height > dst->h) 
	{
		fprintf (stderr, "TFB_DrawCanvas_Rescale_Trilinear: Tried to scale image to size %d %d when dest_canvas has only dimensions of %d %d!  Failing.\n",
			size.width, size.height, dst->w, dst->h);
		return;
	}

	if (src->format->BytesPerPixel == 1 && dst->format->BytesPerPixel == 4 &&
		mipmap->format->BytesPerPixel == 1)
	{
		SDL_Color *srcpal = src->format->palette->colors;
		SDL_Color *mmpal = mipmap->format->palette->colors;
		Uint32 fsx0 = (int)(65536.0f * (float)src->w / w);
		Uint32 fsy0 = (int)(65536.0f * (float)src->h / h);
		Uint32 fsx1 = (int)(65536.0f * (float)mipmap->w / w);
		Uint32 fsy1 = (int)(65536.0f * (float)mipmap->h / h);
		Uint32 ck0 = src->format->colorkey;
		Uint32 ck1 = mipmap->format->colorkey;
		int sx0 = 0, sy0 = 0, sx1 = 0, sy1 = 0;
		int x, y;

		SDL_LockSurface(src);
		SDL_LockSurface(dst);
		SDL_LockSurface(mipmap);
		
		for (y = 0; y < h; ++y)
		{
			Uint32 *dst_p = (Uint32 *)dst->pixels + y * dst->w;
			Uint8 *src_a0 = (Uint8 *)src->pixels + (sy0 >> 16) * src->pitch;
			Uint8 *src_a1 = (Uint8 *)mipmap->pixels + (sy1 >> 16) * mipmap->pitch;

			for (x = 0; x < w; ++x)
			{
				Uint8 u0 = (sx0 >> 8) & 0xff;
				Uint8 u1 = (sx1 >> 8) & 0xff;
				Uint8 v0 = (sy0 >> 8) & 0xff;
				Uint8 v1 = (sy1 >> 8) & 0xff;
				Uint8 ul0 = btable[255 - u0][255 - v0];
				Uint8 ul1 = btable[255 - u1][255 - v1];
				Uint8 ur0 = btable[u0][255 - v0];
				Uint8 ur1 = btable[u1][255 - v1];
				Uint8 ll0 = btable[255 - u0][v0];
				Uint8 ll1 = btable[255 - u1][v1];
				Uint8 lr0 = btable[u0][v0];
				Uint8 lr1 = btable[u1][v1];

				Uint8 r0[5], r1[5], g0[5], g1[5], b0[5], b1[5], a0[5], a1[5];
				Uint8 c;
				
				Uint8 *src_p0 = src_a0 + (sx0 >> 16);
				Uint8 *src_p1 = src_a1 + (sx1 >> 16);

				if ((sx0 >> 16) <= src->w - 2)
				{						
					a0[0] = ((c = *src_p0) == ck0) ? 0 : 255;
					r0[0] = srcpal[c].r;
					g0[0] = srcpal[c].g;
					b0[0] = srcpal[c].b;
					a0[1] = ((c = *(src_p0 + 1)) == ck0) ? 0 : 255;
					r0[1] = srcpal[c].r;
					g0[1] = srcpal[c].g;
					b0[1] = srcpal[c].b;

					if ((sy0 >> 16) <= src->h - 2)
					{
						a0[2] = ((c = *(src_p0 + src->pitch)) == ck0) ? 0 : 255;
						r0[2] = srcpal[c].r;
						g0[2] = srcpal[c].g;
						b0[2] = srcpal[c].b;
						a0[3] = ((c = *(src_p0 + src->pitch + 1)) == ck0) ? 0 : 255;
						r0[3] = srcpal[c].r;
						g0[3] = srcpal[c].g;
						b0[3] = srcpal[c].b;
					}
					else
					{
						a0[2] = a0[0];
						r0[2] = r0[0];
						g0[2] = g0[0];
						b0[2] = b0[0];
						a0[3] = a0[1];
						r0[3] = r0[1];
						g0[3] = g0[1];
						b0[3] = b0[1];
					}
				}
				else
				{
					a0[0] = a0[1] = ((c = *src_p0) == ck0) ? 0 : 255;
					r0[0] = r0[1] = srcpal[c].r;
					g0[0] = g0[1] = srcpal[c].g;
					b0[0] = b0[1] = srcpal[c].b;
					if ((sy0 >> 16) <= src->h - 2)
					{
						a0[2] = a0[3] = ((c = *(src_p0 + src->pitch)) == ck0) ? 0 : 255;
						r0[2] = r0[3] = srcpal[c].r;
						g0[2] = g0[3] = srcpal[c].g;
						b0[2] = b0[3] = srcpal[c].b;
					}
					else
					{
						a0[2] = a0[3] = a0[0];
						r0[2] = r0[3] = r0[0];
						g0[2] = g0[3] = g0[0];
						b0[2] = b0[3] = b0[0];
					}
				}

				if ((sx1 >> 16) <= mipmap->w - 2)
				{						
					a1[0] = ((c = *src_p1) == ck1) ? 0 : 255;
					r1[0] = mmpal[c].r;
					g1[0] = mmpal[c].g;
					b1[0] = mmpal[c].b;
					a1[1] = ((c = *(src_p1 + 1)) == ck1) ? 0 : 255;
					r1[1] = mmpal[c].r;
					g1[1] = mmpal[c].g;
					b1[1] = mmpal[c].b;

					if ((sy1 >> 16) <= mipmap->h - 2)
					{
						a1[2] = ((c = *(src_p1 + mipmap->pitch)) == ck1) ? 0 : 255;
						r1[2] = mmpal[c].r;
						g1[2] = mmpal[c].g;
						b1[2] = mmpal[c].b;
						a1[3] = ((c = *(src_p1 + mipmap->pitch + 1)) == ck1) ? 0 : 255;
						r1[3] = mmpal[c].r;
						g1[3] = mmpal[c].g;
						b1[3] = mmpal[c].b;
					}
					else
					{
						a1[2] = a1[0];
						r1[2] = r1[0];
						g1[2] = g1[0];
						b1[2] = b1[0];
						a1[3] = a1[1];
						r1[3] = r1[1];
						g1[3] = g1[1];
						b1[3] = b1[1];
					}
				}
				else
				{
					a1[0] = a1[1] = ((c = *src_p1) == ck1) ? 0 : 255;
					r1[0] = r1[1] = mmpal[c].r;
					g1[0] = g1[1] = mmpal[c].g;
					b1[0] = b1[1] = mmpal[c].b;
					if ((sy1 >> 16) <= mipmap->h - 2)
					{
						a1[2] = a1[3] = ((c = *(src_p1 + mipmap->pitch)) == ck1) ? 0 : 255;
						r1[2] = r1[3] = mmpal[c].r;
						g1[2] = g1[3] = mmpal[c].g;
						b1[2] = b1[3] = mmpal[c].b;
					}
					else
					{
						a1[2] = a1[3] = a1[0];
						r1[2] = r1[3] = r1[0];
						g1[2] = g1[3] = g1[0];
						b1[2] = b1[3] = b1[0];
					}
				}

				a0[4] = ((ul0 * a0[0]) >> 8) + ((ur0 * a0[1]) >> 8) + ((ll0 * a0[2]) >> 8) + ((lr0 * a0[3]) >> 8);
				a1[4] = ((ul1 * a1[0]) >> 8) + ((ur1 * a1[1]) >> 8) + ((ll1 * a1[2]) >> 8) + ((lr1 * a1[3]) >> 8);
				
				if ((((a0[4] - a1[4]) * ratio) >> 8) + a1[4] > ALPHA_THRESHOLD)
				{
					r0[4] = ((ul0 * r0[0]) >> 8) + ((ur0 * r0[1]) >> 8) + ((ll0 * r0[2]) >> 8) + ((lr0 * r0[3]) >> 8);
					g0[4] = ((ul0 * g0[0]) >> 8) + ((ur0 * g0[1]) >> 8) + ((ll0 * g0[2]) >> 8) + ((lr0 * g0[3]) >> 8);
					b0[4] = ((ul0 * b0[0]) >> 8) + ((ur0 * b0[1]) >> 8) + ((ll0 * b0[2]) >> 8) + ((lr0 * b0[3]) >> 8);

					r1[4] = ((ul1 * r1[0]) >> 8) + ((ur1 * r1[1]) >> 8) + ((ll1 * r1[2]) >> 8) + ((lr1 * r1[3]) >> 8);
					g1[4] = ((ul1 * g1[0]) >> 8) + ((ur1 * g1[1]) >> 8) + ((ll1 * g1[2]) >> 8) + ((lr1 * g1[3]) >> 8);
					b1[4] = ((ul1 * b1[0]) >> 8) + ((ur1 * b1[1]) >> 8) + ((ll1 * b1[2]) >> 8) + ((lr1 * b1[3]) >> 8);

					*dst_p++ =
						(((((r0[4] - r1[4]) * ratio) >> 8) + r1[4]) << dstfmt->Rshift) |
						(((((g0[4] - g1[4]) * ratio) >> 8) + g1[4]) << dstfmt->Gshift) |
						(((((b0[4] - b1[4]) * ratio) >> 8) + b1[4]) << dstfmt->Bshift) |
						dstopaque;
				}
				else
				{
					*dst_p++ = 0;
				}

				sx0 += fsx0;
				sx1 += fsx1;
			}
			sx0 = sx1 = 0;
			sy0 += fsy0;
			sy1 += fsy1;
		}

		SDL_UnlockSurface(mipmap);
		SDL_UnlockSurface(dst);
		SDL_UnlockSurface(src);
	}
	else if (src->format->BytesPerPixel == 4 && dst->format->BytesPerPixel == 4 &&
		mipmap->format->BytesPerPixel == 4)
	{
		Uint32 fsx0 = (int)(65536.0f * (float)src->w / w);
		Uint32 fsy0 = (int)(65536.0f * (float)src->h / h);
		Uint32 fsx1 = (int)(65536.0f * (float)mipmap->w / w);
		Uint32 fsy1 = (int)(65536.0f * (float)mipmap->h / h);
		int sx0 = 0, sy0 = 0, sx1 = 0, sy1 = 0;
		int x, y;

		SDL_LockSurface(src);
		SDL_LockSurface(dst);
		SDL_LockSurface(mipmap);
		
		for (y = 0; y < h; ++y)
		{
			Uint32 *dst_p = (Uint32 *)dst->pixels + y * dst->w;
			Uint32 *src_a0 = (Uint32 *)src->pixels + (sy0 >> 16) * src->w;
			Uint32 *src_a1 = (Uint32 *)mipmap->pixels + (sy1 >> 16) * mipmap->w;

			for (x = 0; x < w; ++x)
			{
				Uint8 u0 = (sx0 >> 8) & 0xff;
				Uint8 u1 = (sx1 >> 8) & 0xff;
				Uint8 v0 = (sy0 >> 8) & 0xff;
				Uint8 v1 = (sy1 >> 8) & 0xff;
				Uint8 ul0 = btable[255 - u0][255 - v0];
				Uint8 ul1 = btable[255 - u1][255 - v1];
				Uint8 ur0 = btable[u0][255 - v0];
				Uint8 ur1 = btable[u1][255 - v1];
				Uint8 ll0 = btable[255 - u0][v0];
				Uint8 ll1 = btable[255 - u1][v1];
				Uint8 lr0 = btable[u0][v0];
				Uint8 lr1 = btable[u1][v1];

				Uint8 r0[5], r1[5], g0[5], g1[5], b0[5], b1[5], a0[5], a1[5];
				
				Uint32 *src_p0 = src_a0 + (sx0 >> 16);
				Uint32 *src_p1 = src_a1 + (sx1 >> 16);

				if ((sx0 >> 16) <= src->w - 2)
				{						
					SDL_GetRGBA (*src_p0, src->format, &r0[0], &g0[0], &b0[0], &a0[0]);
					SDL_GetRGBA (*(src_p0 + 1), src->format, &r0[1], &g0[1], &b0[1], &a0[1]);
					if ((sy0 >> 16) <= src->h - 2)
					{
						SDL_GetRGBA (*(src_p0 + src->w), src->format, &r0[2], &g0[2], &b0[2], &a0[2]);
						SDL_GetRGBA (*(src_p0 + src->w + 1), src->format, &r0[3], &g0[3], &b0[3], &a0[3]);
					}
					else
					{
						r0[2] = r0[0];
						g0[2] = g0[0];
						b0[2] = b0[0];
						a0[2] = a0[0];
						r0[3] = r0[1];
						g0[3] = g0[1];
						b0[3] = b0[1];
						a0[3] = a0[1];
					}
				}
				else
				{
					SDL_GetRGBA (*src_p0, src->format, &r0[0], &g0[0], &b0[0], &a0[0]);
					r0[1] = r0[0];
					g0[1] = g0[0];
					b0[1] = b0[0];
					a0[1] = a0[0];
					if ((sy0 >> 16) <= src->h - 2)
					{
						SDL_GetRGBA (*(src_p0 + src->w), src->format, &r0[2], &g0[2], &b0[2], &a0[2]);
						r0[3] = r0[2];
						g0[3] = g0[2];
						b0[3] = b0[2];
						a0[3] = a0[2];
					}
					else
					{
						r0[2] = r0[3] = r0[0];
						g0[2] = g0[3] = g0[0];
						b0[2] = b0[3] = b0[0];
						a0[2] = a0[3] = a0[0];
					}
				}

				if ((sx1 >> 16) <= mipmap->w - 2)
				{						
					SDL_GetRGBA (*src_p1, mipmap->format, &r1[0], &g1[0], &b1[0], &a1[0]);
					SDL_GetRGBA (*(src_p1 + 1), mipmap->format, &r1[1], &g1[1], &b1[1], &a1[1]);
					if ((sy1 >> 16) <= mipmap->h - 2)
					{
						SDL_GetRGBA (*(src_p1 + mipmap->w) , mipmap->format, &r1[2], &g1[2], &b1[2], &a1[2]);
						SDL_GetRGBA (*(src_p1 + mipmap->w + 1), mipmap->format, &r1[3], &g1[3], &b1[3], &a1[3]);
					}
					else
					{
						r1[2] = r1[0];
						g1[2] = g1[0];
						b1[2] = b1[0];
						a1[2] = a1[0];
						r1[3] = r1[1];
						g1[3] = g1[1];
						b1[3] = b1[1];
						a1[3] = a1[1];
					}
				}
				else
				{
					SDL_GetRGBA (*src_p1, mipmap->format, &r1[0], &g1[0], &b1[0], &a1[0]);
					r1[1] = r1[0];
					g1[1] = g1[0];
					b1[1] = b1[0];
					a1[1] = a1[0];
					if ((sy1 >> 16) <= mipmap->h - 2)
					{
						SDL_GetRGBA (*(src_p1 + mipmap->w), mipmap->format, &r1[2], &g1[2], &b1[2], &a1[2]);
						r1[3] = r1[2];
						g1[3] = g1[2];
						b1[3] = b1[2];
						a1[3] = a1[2];
					}
					else
					{
						r1[2] = r1[3] = r1[0];
						g1[2] = g1[3] = g1[0];
						b1[2] = b1[3] = b1[0];
						a1[2] = a1[3] = a1[0];
					}
				}

				a0[4] = ((ul0 * a0[0]) >> 8) + ((ur0 * a0[1]) >> 8) + ((ll0 * a0[2]) >> 8) + ((lr0 * a0[3]) >> 8);
				a1[4] = ((ul1 * a1[0]) >> 8) + ((ur1 * a1[1]) >> 8) + ((ll1 * a1[2]) >> 8) + ((lr1 * a1[3]) >> 8);
				
				if ((((a0[4] - a1[4]) * ratio) >> 8) + a1[4] > ALPHA_THRESHOLD)
				{
					r0[4] = ((ul0 * r0[0]) >> 8) + ((ur0 * r0[1]) >> 8) + ((ll0 * r0[2]) >> 8) + ((lr0 * r0[3]) >> 8);
					g0[4] = ((ul0 * g0[0]) >> 8) + ((ur0 * g0[1]) >> 8) + ((ll0 * g0[2]) >> 8) + ((lr0 * g0[3]) >> 8);
					b0[4] = ((ul0 * b0[0]) >> 8) + ((ur0 * b0[1]) >> 8) + ((ll0 * b0[2]) >> 8) + ((lr0 * b0[3]) >> 8);

					r1[4] = ((ul1 * r1[0]) >> 8) + ((ur1 * r1[1]) >> 8) + ((ll1 * r1[2]) >> 8) + ((lr1 * r1[3]) >> 8);
					g1[4] = ((ul1 * g1[0]) >> 8) + ((ur1 * g1[1]) >> 8) + ((ll1 * g1[2]) >> 8) + ((lr1 * g1[3]) >> 8);
					b1[4] = ((ul1 * b1[0]) >> 8) + ((ur1 * b1[1]) >> 8) + ((ll1 * b1[2]) >> 8) + ((lr1 * b1[3]) >> 8);

					*dst_p++ =
						(((((r0[4] - r1[4]) * ratio) >> 8) + r1[4]) << dstfmt->Rshift) |
						(((((g0[4] - g1[4]) * ratio) >> 8) + g1[4]) << dstfmt->Gshift) |
						(((((b0[4] - b1[4]) * ratio) >> 8) + b1[4]) << dstfmt->Bshift) |
						dstopaque;
				}
				else
				{
					*dst_p++ = 0;
				}

				sx0 += fsx0;
				sx1 += fsx1;
			}
			sx0 = sx1 = 0;
			sy0 += fsy0;
			sy1 += fsy1;
		}

		SDL_UnlockSurface(mipmap);
		SDL_UnlockSurface(dst);
		SDL_UnlockSurface(src);
	}
	else
	{
		fprintf (stderr, "Tried to deal with unknown BPP: %d -> %d, mipmap %d\n",
			src->format->BitsPerPixel, dst->format->BitsPerPixel, mipmap->format->BitsPerPixel);
	}
}
