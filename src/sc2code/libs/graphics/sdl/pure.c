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

#include "pure.h"
#include "primitives.h"
#include "bbox.h"

static SDL_Surface *fade_black;
static SDL_Surface *fade_white;
static SDL_Surface *fade_temp;
static int gfx_flags;

static const Uint32 bilinear_table[4][4] = 
{
	{ 9, 3, 3, 1 },
	{ 3, 1, 9, 3 },
	{ 3, 9, 1, 3 },
	{ 1, 3, 3, 9 }
};

typedef union 
{
	struct
	{
		Uint8 c1, c2, c3;
	} bchan;
	Uint8 channels[3];
} PIXEL_24BIT;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
// big endian cpu
// We avoid potential pagefaults based on the assumption
// that all buffers are allocated on *at least* 32bit boundary
// So if the pointer (p) is 32bit-aligned, we can read at
// least 32 bits at once; or if it is not aligned then we
// are not at the beginning of the buffer and can read 1 byte
// before (p)
//
// WARNING!! Do not call these macros like so:
//		SET_PIX_24BIT (dst_p++, pixel)
// The pre-increment or post-increment will be done
// several times
//
#	define GET_PIX_24BIT(p) \
		( ((Uint32)(p)) & 3 ? \
			(*(Uint32 *)((Uint8 *)(p) - 1) & 0x00ffffff) : \
			(*(Uint32 *)(p) >> 8) \
		)

#	define SET_PIX_24BIT(p, c) \
		( ((Uint32)(p)) & 3 ? \
			( *(Uint32 *)((Uint8 *)(p) - 1) = \
				(*(Uint32 *)((Uint8 *)(p) - 1) & 0xff000000) | \
				((c) & 0x00ffffff) \
			) : \
			( *(Uint32 *)(p) = \
				(*(Uint32 *)(p) & 0x000000ff) | \
				((c) << 8) \
			) \
		)
#else
// little endian cpu
// Same page-safety assumption applies as for big-endian
//
#	define GET_PIX_24BIT(p) \
		( ((Uint32)(p)) & 3 ? \
			(*(Uint32 *)((Uint8 *)(p) - 1) >> 8) : \
			(*(Uint32 *)(p) & 0x00ffffff) \
		)

#	define SET_PIX_24BIT(p, c) \
		( ((Uint32)(p)) & 3 ? \
			( *(Uint32 *)((Uint8 *)(p) - 1) = \
				(*(Uint32 *)((Uint8 *)(p) - 1) & 0x000000ff) | \
				((c) << 8) \
			) : \
			( *(Uint32 *)(p) = \
				(*(Uint32 *)(p) & 0xff000000) | \
				((c) & 0x00ffffff) \
			) \
		)
#endif

static SDL_Surface *
Create_Screen (SDL_Surface *template)
{
	SDL_Surface *newsurf = SDL_DisplayFormat (template);
	if (newsurf == 0) {
		fprintf (stderr, "Couldn't create screen buffers: %s\n", SDL_GetError());
		exit(-1);
	}
	return newsurf;
}

int
TFB_Pure_InitGraphics (int driver, int flags, int width, int height, int bpp)
{
	char VideoName[256];
	int videomode_flags;
	SDL_Surface *test_extra;
	int i;

	GraphicsDriver = driver;
	gfx_flags = flags;

	fprintf (stderr, "Initializing SDL (pure).\n");

	if ((SDL_Init (SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) == -1))
	{
		fprintf (stderr, "Could not initialize SDL: %s.\n", SDL_GetError());
		exit(-1);
	}

	SDL_VideoDriverName (VideoName, sizeof (VideoName));
	fprintf (stderr, "SDL driver used: %s\n", VideoName);
			// Set the environment variable SDL_VIDEODRIVER to override
			// For Linux: x11 (default), dga, fbcon, directfb, svgalib,
			//            ggi, aalib
			// For Windows: directx (default), windib

	fprintf (stderr, "SDL initialized.\n");

	fprintf (stderr, "Initializing Screen.\n");

	ScreenWidth = 320;
	ScreenHeight = 240;

	// must use SDL_SWSURFACE, HWSURFACE doesn't work properly with fades/scaling
	if (width == 320 && height == 240)
	{
		videomode_flags = SDL_SWSURFACE;
		ScreenWidthActual = 320;
		ScreenHeightActual = 240;
	}
	else
	{
		videomode_flags = SDL_SWSURFACE;
		ScreenWidthActual = 640;
		ScreenHeightActual = 480;

		if (width != 640 || height != 480)
			fprintf (stderr, "Screen resolution of %dx%d not supported under pure SDL, using 640x480\n", width, height);
	}

	videomode_flags |= SDL_ANYFORMAT;
	if (flags & TFB_GFXFLAGS_FULLSCREEN)
		videomode_flags |= SDL_FULLSCREEN;

	SDL_Video = SDL_SetVideoMode (ScreenWidthActual, ScreenHeightActual, 
		bpp, videomode_flags);

	if (SDL_Video == NULL)
	{
		fprintf (stderr, "Couldn't set %ix%ix%i video mode: %s\n",
			ScreenWidthActual, ScreenHeightActual, bpp,
			SDL_GetError ());
		exit(-1);
	}
	else
	{
		fprintf (stderr, "Set the resolution to: %ix%ix%i\n",
			SDL_GetVideoSurface()->w, SDL_GetVideoSurface()->h,
			SDL_GetVideoSurface()->format->BitsPerPixel);
	}

	test_extra = SDL_CreateRGBSurface(SDL_SWSURFACE, ScreenWidth, ScreenHeight, 32,
		0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000);

	if (test_extra == NULL)
	{
		fprintf (stderr, "Couldn't create back buffer: %s\n", SDL_GetError());
		exit(-1);
	}

	for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
	{
		SDL_Screens[i] = Create_Screen (test_extra);
	}

	SDL_Screen = SDL_Screens[0];
	TransitionScreen = SDL_Screens[2];

	fade_white = Create_Screen(test_extra);
	SDL_FillRect (fade_white, NULL, SDL_MapRGB (fade_white->format, 255, 255, 255));
	fade_black = Create_Screen(test_extra);
	SDL_FillRect (fade_black, NULL, SDL_MapRGB (fade_black->format, 0, 0, 0));
	fade_temp = Create_Screen(test_extra);
	
	SDL_FreeSurface (test_extra);
	
	if (SDL_Video->format->BytesPerPixel != SDL_Screen->format->BytesPerPixel)
	{
		fprintf (stderr, "Fatal error: SDL_Video and SDL_Screen bpp doesn't match (%d vs. %d)\n",
			SDL_Video->format->BytesPerPixel,SDL_Screen->format->BytesPerPixel);
		exit(-1);
	}

	return 0;
}

// expands the given rectangle in all directions by 'expansion'
// guarded by 'limits'
void
Scale_ExpandRect (SDL_Rect* rect, int expansion, SDL_Rect* limits)
{
	if (rect->x - expansion >= limits->x)
	{
		rect->w += expansion;
		rect->x -= expansion;
	}
	else
	{
		rect->w += rect->x - limits->x;
		rect->x = limits->x;
	}

	if (rect->y - expansion >= limits->y)
	{
		rect->h += expansion;
		rect->y -= expansion;
	}
	else
	{
		rect->h += rect->y - limits->y;
		rect->y = limits->y;
	}

	if (rect->x + rect->w + expansion <= limits->w)
		rect->w += expansion;
	else
		rect->w = limits->w - rect->x;

	if (rect->y + rect->h + expansion <= limits->h)
		rect->h += expansion;
	else
		rect->h = limits->h - rect->y;
}

// blends two pixels with ratio 50% - 50%
__inline__ Uint32
Scale_BlendPixels (SDL_PixelFormat* fmt, Uint32 pix1, Uint32 pix2)
{
	return	((((pix1 & fmt->Rmask) +
			   (pix2 & fmt->Rmask)
			 ) >> 1) & fmt->Rmask) |
			((((pix1 & fmt->Gmask) +
			   (pix2 & fmt->Gmask)
			 ) >> 1) & fmt->Gmask) |
			((((pix1 & fmt->Bmask) +
			   (pix2 & fmt->Bmask)
			) >> 1) & fmt->Bmask);
}

// biadapt scaling to 2x
static void
Scale_BiAdaptFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r)
{
	int x, y;
	const int w = src->w, h = src->h, dw = dst->w;
	int xend, yend;
	int dsrc, ddst;
	SDL_Rect region = *r;
	SDL_Rect limits;
	SDL_PixelFormat *fmt = dst->format;

	// expand updated region if necessary
	// pixels neighbooring the updated region may
	// change as a result of updates
	limits.x = 0;
	limits.y = 0;
	limits.w = src->w;
	limits.h = src->h;
	Scale_ExpandRect (&region, 2, &limits);

	xend = region.x + region.w;
	yend = region.y + region.h;
	dsrc = w - region.w;
	ddst = (dw - region.w) * 2;

	switch (fmt->BytesPerPixel)
	{
	case 2: // 16bpp scaling
	#define BIADAPT_GETPIX(p)        ( *(Uint16 *)(p) )
	#define BIADAPT_SETPIX(p, c)     ( *(Uint16 *)(p) = (c) )
	#define BIADAPT_BUF              Uint16
	{
		BIADAPT_BUF *src_p = (BIADAPT_BUF *)src->pixels;
		BIADAPT_BUF *dst_p = (BIADAPT_BUF *)dst->pixels;
		Uint32 pixval_tl, pixval_tr, pixval_bl, pixval_br;
		
		// move ptrs to the first updated pixel
		src_p += w * region.y + region.x;
		dst_p += (dw * region.y + region.x) * 2;

		for (y = region.y; y < yend; ++y, dst_p += ddst, src_p += dsrc)
		{
			for (x = region.x; x < xend; ++x, ++src_p, ++dst_p)
			{
				pixval_tl = BIADAPT_GETPIX (src_p);
				
				BIADAPT_SETPIX (dst_p, pixval_tl);
				
				if (y + 1 < h)
				{
					// check pixel below the current one
					pixval_bl = BIADAPT_GETPIX (src_p + w);

					if (pixval_tl == pixval_bl)
						BIADAPT_SETPIX (dst_p + dw, pixval_tl);
					else
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, pixval_tl, pixval_bl)
								);
				}
				else
				{
					// last pixel in column - propagate
					BIADAPT_SETPIX (dst_p + dw, pixval_tl);
				}
				dst_p++;


				if (x + 1 >= w)
				{
					// last pixel in row - propagate
					BIADAPT_SETPIX (dst_p, pixval_tl);
					BIADAPT_SETPIX (dst_p + dw, pixval_tl);
					continue;
				}
				
				// check pixel to the right from the current one
				pixval_tr = BIADAPT_GETPIX (src_p + 1);

				if (pixval_tl == pixval_tr)
					BIADAPT_SETPIX (dst_p, pixval_tr);
				else
					BIADAPT_SETPIX (dst_p, Scale_BlendPixels (
							fmt, pixval_tl, pixval_tr)
							);

				if (y + 1 >= h)
				{
					// last pixel in column - propagate
					BIADAPT_SETPIX (dst_p + dw, pixval_tl);
					continue;
				}
				
				// check pixel to the bottom-right
				pixval_br = BIADAPT_GETPIX (src_p + 1 + w);

				if (pixval_tl == pixval_br && pixval_tr == pixval_bl)
				{
					int cl, cr;
					Uint32 clr;

					if (pixval_tl == pixval_tr)
					{
						// all 4 are equal - propagate
						BIADAPT_SETPIX (dst_p + dw, pixval_tl);
						continue;
					}

					// both pairs are equal, have to resolve the pixel
					// race; we try detecting which color is
					// the background by looking for a line or an edge
					// examine 8 pixels surrounding the current quad

					cl = cr = 1;

					if (x > 0)
					{
						clr = BIADAPT_GETPIX (src_p - 1);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;

						clr = BIADAPT_GETPIX (src_p - 1 + w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;
					}

					if (y > 0)
					{
						clr = BIADAPT_GETPIX (src_p - w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;

						clr = BIADAPT_GETPIX (src_p + 1 - w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;
					}

					if (x + 2 < w)
					{
						clr = BIADAPT_GETPIX (src_p + 2);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;

						clr = BIADAPT_GETPIX (src_p + 2 + w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;
					}

					if (y + 2 < h)
					{
						clr = BIADAPT_GETPIX (src_p + 2 * w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;

						clr = BIADAPT_GETPIX (src_p + 1 + 2 * w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;
					}
					
					// least count wins
					if (cl > cr)
						BIADAPT_SETPIX (dst_p + dw, pixval_tr);
					else if (cr > cl)
						BIADAPT_SETPIX (dst_p + dw, pixval_tl);
					else
						BIADAPT_SETPIX (dst_p + dw,
								Scale_BlendPixels (fmt, pixval_tl,
								pixval_tr));
				}
				else if (pixval_tl == pixval_br)
				{
					// main diagonal is same color
					// use its value
					BIADAPT_SETPIX (dst_p + dw, pixval_tl);
				}
				else if (pixval_tr == pixval_bl)
				{
					// 2nd diagonal is same color
					// use its value
					BIADAPT_SETPIX (dst_p + dw, pixval_tr);
				}
				else
				{
					// blend all 4
					BIADAPT_SETPIX (dst_p + dw,
							((((pixval_tl & fmt->Rmask) +
							   (pixval_bl & fmt->Rmask) +
							   (pixval_tr & fmt->Rmask) +
							   (pixval_br & fmt->Rmask)
							 ) >> 2) & fmt->Rmask) |
							((((pixval_tl & fmt->Gmask) +
							   (pixval_bl & fmt->Gmask) +
							   (pixval_tr & fmt->Gmask) +
							   (pixval_br & fmt->Gmask)
							 ) >> 2) & fmt->Gmask) |
							((((pixval_tl & fmt->Bmask) +
							   (pixval_bl & fmt->Bmask) +
							   (pixval_tr & fmt->Bmask) +
							   (pixval_br & fmt->Bmask)
							) >> 2) & fmt->Bmask));
				}
			}
		}
		break;
	}
	#undef BIADAPT_GETPIX
	#undef BIADAPT_SETPIX
	#undef BIADAPT_BUF

	case 3: // 24bpp scaling
	#define BIADAPT_GETPIX(p)        GET_PIX_24BIT(p)
	#define BIADAPT_SETPIX(p, c)     SET_PIX_24BIT(p, c)
	#define BIADAPT_BUF              PIXEL_24BIT
	{
		BIADAPT_BUF *src_p = (BIADAPT_BUF *)src->pixels;
		BIADAPT_BUF *dst_p = (BIADAPT_BUF *)dst->pixels;
		Uint32 pixval_tl, pixval_tr, pixval_bl, pixval_br;
		
		// move ptrs to the first updated pixel
		src_p += w * region.y + region.x;
		dst_p += (dw * region.y + region.x) * 2;

		for (y = region.y; y < yend; ++y, dst_p += ddst, src_p += dsrc)
		{
			for (x = region.x; x < xend; ++x, ++src_p, ++dst_p)
			{
				pixval_tl = BIADAPT_GETPIX (src_p);
				
				BIADAPT_SETPIX (dst_p, pixval_tl);
				
				if (y + 1 < h)
				{
					// check pixel below the current one
					pixval_bl = BIADAPT_GETPIX (src_p + w);

					if (pixval_tl == pixval_bl)
						BIADAPT_SETPIX (dst_p + dw, pixval_tl);
					else
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, pixval_tl, pixval_bl)
								);
				}
				else
				{
					// last pixel in column - propagate
					BIADAPT_SETPIX (dst_p + dw, pixval_tl);
				}
				dst_p++;


				if (x + 1 >= w)
				{
					// last pixel in row - propagate
					BIADAPT_SETPIX (dst_p, pixval_tl);
					BIADAPT_SETPIX (dst_p + dw, pixval_tl);
					continue;
				}
				
				// check pixel to the right from the current one
				pixval_tr = BIADAPT_GETPIX (src_p + 1);

				if (pixval_tl == pixval_tr)
					BIADAPT_SETPIX (dst_p, pixval_tr);
				else
					BIADAPT_SETPIX (dst_p, Scale_BlendPixels (
							fmt, pixval_tl, pixval_tr)
							);

				if (y + 1 >= h)
				{
					// last pixel in column - propagate
					BIADAPT_SETPIX (dst_p + dw, pixval_tl);
					continue;
				}
				
				// check pixel to the bottom-right
				pixval_br = BIADAPT_GETPIX (src_p + 1 + w);

				if (pixval_tl == pixval_br && pixval_tr == pixval_bl)
				{
					int cl, cr;
					Uint32 clr;

					if (pixval_tl == pixval_tr)
					{
						// all 4 are equal - propagate
						BIADAPT_SETPIX (dst_p + dw, pixval_tl);
						continue;
					}

					// both pairs are equal, have to resolve the pixel
					// race; we try detecting which color is
					// the background by looking for a line or an edge
					// examine 8 pixels surrounding the current quad

					cl = cr = 1;

					if (x > 0)
					{
						clr = BIADAPT_GETPIX (src_p - 1);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;

						clr = BIADAPT_GETPIX (src_p - 1 + w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;
					}

					if (y > 0)
					{
						clr = BIADAPT_GETPIX (src_p - w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;

						clr = BIADAPT_GETPIX (src_p + 1 - w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;
					}

					if (x + 2 < w)
					{
						clr = BIADAPT_GETPIX (src_p + 2);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;

						clr = BIADAPT_GETPIX (src_p + 2 + w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;
					}

					if (y + 2 < h)
					{
						clr = BIADAPT_GETPIX (src_p + 2 * w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;

						clr = BIADAPT_GETPIX (src_p + 1 + 2 * w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;
					}
					
					// least count wins
					if (cl > cr)
						BIADAPT_SETPIX (dst_p + dw, pixval_tr);
					else if (cr > cl)
						BIADAPT_SETPIX (dst_p + dw, pixval_tl);
					else
						BIADAPT_SETPIX (dst_p + dw,
								Scale_BlendPixels (fmt, pixval_tl,
								pixval_tr));
				}
				else if (pixval_tl == pixval_br)
				{
					// main diagonal is same color
					// use its value
					BIADAPT_SETPIX (dst_p + dw, pixval_tl);
				}
				else if (pixval_tr == pixval_bl)
				{
					// 2nd diagonal is same color
					// use its value
					BIADAPT_SETPIX (dst_p + dw, pixval_tr);
				}
				else
				{
					// blend all 4
					BIADAPT_SETPIX (dst_p + dw,
							((((pixval_tl & fmt->Rmask) +
							   (pixval_bl & fmt->Rmask) +
							   (pixval_tr & fmt->Rmask) +
							   (pixval_br & fmt->Rmask)
							 ) >> 2) & fmt->Rmask) |
							((((pixval_tl & fmt->Gmask) +
							   (pixval_bl & fmt->Gmask) +
							   (pixval_tr & fmt->Gmask) +
							   (pixval_br & fmt->Gmask)
							 ) >> 2) & fmt->Gmask) |
							((((pixval_tl & fmt->Bmask) +
							   (pixval_bl & fmt->Bmask) +
							   (pixval_tr & fmt->Bmask) +
							   (pixval_br & fmt->Bmask)
							) >> 2) & fmt->Bmask));
				}
			}
		}
		break;
	}
	#undef BIADAPT_GETPIX
	#undef BIADAPT_SETPIX
	#undef BIADAPT_BUF

	case 4: // 32bpp scaling
	#define BIADAPT_GETPIX(p)        ( *(Uint32 *)(p) )
	#define BIADAPT_SETPIX(p, c)     ( *(Uint32 *)(p) = (c) )
	#define BIADAPT_BUF              Uint32
	{
		BIADAPT_BUF *src_p = (BIADAPT_BUF *)src->pixels;
		BIADAPT_BUF *dst_p = (BIADAPT_BUF *)dst->pixels;
		Uint32 pixval_tl, pixval_tr, pixval_bl, pixval_br;
		
		// move ptrs to the first updated pixel
		src_p += w * region.y + region.x;
		dst_p += (dw * region.y + region.x) * 2;

		for (y = region.y; y < yend; ++y, dst_p += ddst, src_p += dsrc)
		{
			for (x = region.x; x < xend; ++x, ++src_p, ++dst_p)
			{
				pixval_tl = BIADAPT_GETPIX (src_p);
				
				BIADAPT_SETPIX (dst_p, pixval_tl);
				
				if (y + 1 < h)
				{
					// check pixel below the current one
					pixval_bl = BIADAPT_GETPIX (src_p + w);

					if (pixval_tl == pixval_bl)
						BIADAPT_SETPIX (dst_p + dw, pixval_tl);
					else
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, pixval_tl, pixval_bl)
								);
				}
				else
				{
					// last pixel in column - propagate
					BIADAPT_SETPIX (dst_p + dw, pixval_tl);
				}
				dst_p++;


				if (x + 1 >= w)
				{
					// last pixel in row - propagate
					BIADAPT_SETPIX (dst_p, pixval_tl);
					BIADAPT_SETPIX (dst_p + dw, pixval_tl);
					continue;
				}
				
				// check pixel to the right from the current one
				pixval_tr = BIADAPT_GETPIX (src_p + 1);

				if (pixval_tl == pixval_tr)
					BIADAPT_SETPIX (dst_p, pixval_tr);
				else
					BIADAPT_SETPIX (dst_p, Scale_BlendPixels (
							fmt, pixval_tl, pixval_tr)
							);

				if (y + 1 >= h)
				{
					// last pixel in column - propagate
					BIADAPT_SETPIX (dst_p + dw, pixval_tl);
					continue;
				}
				
				// check pixel to the bottom-right
				pixval_br = BIADAPT_GETPIX (src_p + 1 + w);

				if (pixval_tl == pixval_br && pixval_tr == pixval_bl)
				{
					int cl, cr;
					Uint32 clr;

					if (pixval_tl == pixval_tr)
					{
						// all 4 are equal - propagate
						BIADAPT_SETPIX (dst_p + dw, pixval_tl);
						continue;
					}

					// both pairs are equal, have to resolve the pixel
					// race; we try detecting which color is
					// the background by looking for a line or an edge
					// examine 8 pixels surrounding the current quad

					cl = cr = 1;

					if (x > 0)
					{
						clr = BIADAPT_GETPIX (src_p - 1);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;

						clr = BIADAPT_GETPIX (src_p - 1 + w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;
					}

					if (y > 0)
					{
						clr = BIADAPT_GETPIX (src_p - w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;

						clr = BIADAPT_GETPIX (src_p + 1 - w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;
					}

					if (x + 2 < w)
					{
						clr = BIADAPT_GETPIX (src_p + 2);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;

						clr = BIADAPT_GETPIX (src_p + 2 + w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;
					}

					if (y + 2 < h)
					{
						clr = BIADAPT_GETPIX (src_p + 2 * w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;

						clr = BIADAPT_GETPIX (src_p + 1 + 2 * w);
						if (clr == pixval_tl)
							cl++;
						else if (clr == pixval_tr)
							cr++;
					}
					
					// least count wins
					if (cl > cr)
						BIADAPT_SETPIX (dst_p + dw, pixval_tr);
					else if (cr > cl)
						BIADAPT_SETPIX (dst_p + dw, pixval_tl);
					else
						BIADAPT_SETPIX (dst_p + dw,
								Scale_BlendPixels (fmt, pixval_tl,
								pixval_tr));
				}
				else if (pixval_tl == pixval_br)
				{
					// main diagonal is same color
					// use its value
					BIADAPT_SETPIX (dst_p + dw, pixval_tl);
				}
				else if (pixval_tr == pixval_bl)
				{
					// 2nd diagonal is same color
					// use its value
					BIADAPT_SETPIX (dst_p + dw, pixval_tr);
				}
				else
				{
					// blend all 4
					BIADAPT_SETPIX (dst_p + dw,
							((((pixval_tl & fmt->Rmask) +
							   (pixval_bl & fmt->Rmask) +
							   (pixval_tr & fmt->Rmask) +
							   (pixval_br & fmt->Rmask)
							 ) >> 2) & fmt->Rmask) |
							((((pixval_tl & fmt->Gmask) +
							   (pixval_bl & fmt->Gmask) +
							   (pixval_tr & fmt->Gmask) +
							   (pixval_br & fmt->Gmask)
							 ) >> 2) & fmt->Gmask) |
							((((pixval_tl & fmt->Bmask) +
							   (pixval_bl & fmt->Bmask) +
							   (pixval_tr & fmt->Bmask) +
							   (pixval_br & fmt->Bmask)
							) >> 2) & fmt->Bmask));
				}
			}
		}
		break;
	}
	#undef BIADAPT_GETPIX
	#undef BIADAPT_SETPIX
	#undef BIADAPT_BUF
	#undef BIADAPT_CLR

	}
}

// nearest neighbor scaling to 2x
static void Scale_Nearest (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r)
{
	int x, y;
	const int w = src->w, h = src->h, dw = dst->w;
	const int x0 = r->x, y0 = r->y, rw = r->w, rh = r->h;
	const int ds = w-rw, dd = (dw-rw) * 2;

	switch (dst->format->BytesPerPixel)
	{
	case 2:
	{
		Uint16 *src_p = (Uint16 *)src->pixels, *dst_p = (Uint16 *)dst->pixels;
		Uint16 pixval_16;
		src_p += w*y0 + x0;
		dst_p += (dw*y0 + x0) * 2;
		for (y = 0; y < rh; ++y)
		{
			for (x = 0; x < rw; ++x)
			{
				pixval_16 = *src_p++;
				dst_p[dw] = pixval_16;
				*dst_p++ = pixval_16;
				dst_p[dw] = pixval_16;
				*dst_p++ = pixval_16;
			}
			dst_p += dd;
			src_p += ds;
		}
		break;
	}
	case 3:
	{
		Uint8 *src_p = (Uint8 *)src->pixels, *dst_p = (Uint8 *)dst->pixels, *src_p2;
		Uint32 pixval_32;
		for (y = 0; y < h; ++y)
		{
			src_p2 = src_p;			
			for (x = 0; x < w; ++x)
			{
				pixval_32 = *(Uint32*)src_p;
				src_p += 3;
				*(Uint32*)dst_p = pixval_32;
				dst_p += 3;
				*(Uint32*)dst_p = pixval_32;
				dst_p += 3;
			}
			src_p = src_p2;
			for (x = 0; x < w; ++x)
			{
				pixval_32 = *(Uint32*)src_p;
				src_p += 3;
				*(Uint32*)dst_p = pixval_32;
				dst_p += 3;
				*(Uint32*)dst_p = pixval_32;
				dst_p += 3;
			}
		}
		break;
	}
	case 4:
	{
		Uint32 *src_p = (Uint32 *)src->pixels, *dst_p = (Uint32 *)dst->pixels;
		Uint32 pixval_32;
		src_p += w*y0 + x0;
		dst_p += (dw*y0 + x0) * 2;
		for (y = 0; y < rh; ++y)
		{
			for (x = 0; x < rw; ++x)
			{
				pixval_32 = *src_p++;
				dst_p[dw] = pixval_32;
				*dst_p++ = pixval_32;
				dst_p[dw] = pixval_32;
				*dst_p++ = pixval_32;
			}
			dst_p += dd;
			src_p += ds;
		}
		break;
	}
	}
}

// bilinear scaling to 2x
static void Scale_BilinearFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r)
{
	int x, y, i = 0, j, fa, fb, fc, fd;
	const int w = dst->w, h = dst->h;
	SDL_PixelFormat *fmt = dst->format;

	switch (dst->format->BytesPerPixel)
	{
	case 2:
	{
		Uint16 *src_p = (Uint16 *) src->pixels, *src_p2;
		Uint16 *dst_p = (Uint16 *) dst->pixels;
		Uint32 p[4];

		for (y = 0; y < h; ++y)
		{
			src_p2 = src_p;
			j = i;

			p[0] = src_p[0];
			if (y < h - 2)
				p[2] = src_p[src->w];
			else
				p[2] = 0;

			for (x = 0; x < w; ++x)
			{
				if (j == i)
				{
					if (y < h - 2)
					{
						if (x < w - 2)
						{
							p[1] = src_p[1];
							p[3] = src_p[src->w + 1];
						}
						else
						{
							p[1] = 0;
							p[3] = 0;
						}
					}
					else
					{
						if (x < w - 2)
							p[1] = src_p[1];
						else
							p[1] = 0;
						p[3] = 0;
					}
				}

				fa = bilinear_table[j][0];
				fb = bilinear_table[j][1];
				fc = bilinear_table[j][2];
				fd = bilinear_table[j][3];

				*dst_p++ = 
					(((((p[0] & fmt->Rmask) * fa) +
					((p[1] & fmt->Rmask) * fb) +
					((p[2] & fmt->Rmask) * fc) +
					((p[3] & fmt->Rmask) * fd)) >> 4) & fmt->Rmask) |
					(((((p[0] & fmt->Gmask) * fa) +
					((p[1] & fmt->Gmask) * fb) +
					((p[2] & fmt->Gmask) * fc) +
					((p[3] & fmt->Gmask) * fd)) >> 4) & fmt->Gmask) |
					(((((p[0] & fmt->Bmask) * fa) +
					((p[1] & fmt->Bmask) * fb) +
					((p[2] & fmt->Bmask) * fc) +
					((p[3] & fmt->Bmask) * fd)) >> 4) & fmt->Bmask);

				if (j & 2)
				{
					j = i;
					src_p++;
					p[0] = p[1];
					p[2] = p[3];
				}
				else
				{
					j = i | 2;
				}
			}

			if (!i)
				src_p = src_p2;
			i = !i;
		}

		break;
	}
	case 3:
	{
		// 24bpp mode bilinear scaling isn't implemented currently
		// it would probably be too slow to be useful anyway
		Scale_Nearest (src, dst, r);
		break;
	}
	case 4:
	{
		Uint32 *src_p = (Uint32 *) src->pixels, *src_p2;
		Uint32 *dst_p = (Uint32 *) dst->pixels;
		Uint32 p[4];

		for (y = 0; y < h; ++y)
		{
			src_p2 = src_p;
			j = i;

			p[0] = src_p[0];
			if (y < h - 2)
				p[2] = src_p[src->w];
			else
				p[2] = 0;

			for (x = 0; x < w; ++x)
			{
				if (j == i)
				{
					if (y < h - 2)
					{
						if (x < w - 2)
						{
							p[1] = src_p[1];
							p[3] = src_p[src->w + 1];
						}
						else
						{
							p[1] = 0;
							p[3] = 0;
						}
					}
					else
					{
						if (x < w - 2)
							p[1] = src_p[1];
						else
							p[1] = 0;
						p[3] = 0;
					}
				}

				fa = bilinear_table[j][0];
				fb = bilinear_table[j][1];
				fc = bilinear_table[j][2];
				fd = bilinear_table[j][3];

				*dst_p++ = 
					(((((p[0] & fmt->Rmask) * fa) +
					((p[1] & fmt->Rmask) * fb) +
					((p[2] & fmt->Rmask) * fc) +
					((p[3] & fmt->Rmask) * fd)) >> 4) & fmt->Rmask) |
					(((((p[0] & fmt->Gmask) * fa) +
					((p[1] & fmt->Gmask) * fb) +
					((p[2] & fmt->Gmask) * fc) +
					((p[3] & fmt->Gmask) * fd)) >> 4) & fmt->Gmask) |
					(((((p[0] & fmt->Bmask) * fa) +
					((p[1] & fmt->Bmask) * fb) +
					((p[2] & fmt->Bmask) * fc) +
					((p[3] & fmt->Bmask) * fd)) >> 4) & fmt->Bmask);

				if (j & 2)
				{
					j = i;
					src_p++;
					p[0] = p[1];
					p[2] = p[3];
				}
				else
				{
					j = i | 2;
				}
			}

			if (!i)
				src_p = src_p2;
			i = !i;
		}

		break;
	}
	}
}

static void ScanLines (SDL_Surface *dst)
{
	Uint8 *pixels = (Uint8 *) dst->pixels;
	SDL_PixelFormat *fmt = dst->format;
	int x, y;

	switch (dst->format->BytesPerPixel)
	{
	case 2:
	{
		for (y = 0; y < dst->h; y += 2)
		{
			Uint16 *p = (Uint16 *) &pixels[y * dst->pitch];
			for (x = 0; x < dst->w; ++x)
			{
				*p++ = ((((*p & fmt->Rmask) * 3) >> 2) & fmt->Rmask) |
					((((*p & fmt->Gmask) * 3) >> 2) & fmt->Gmask) |
					((((*p & fmt->Bmask) * 3) >> 2) & fmt->Bmask);
			}
		}
		break;
	}
	case 3:
	{
		for (y = 0; y < dst->h; y += 2)
		{
			Uint8 *p = (Uint8 *) &pixels[y * dst->pitch];
			Uint32 pixval;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			for (x = 0; x < dst->w; ++x)
			{
				pixval = p[0] << 16 | p[1] << 8 | p[2];
				pixval = ((((pixval & fmt->Rmask) * 3) >> 2) & fmt->Rmask) |
					((((pixval & fmt->Gmask) * 3) >> 2) & fmt->Gmask) |
					((((pixval & fmt->Bmask) * 3) >> 2) & fmt->Bmask);
				p[0] = (pixval >> 16) & 0xff;
				p[1] = (pixval >> 8) & 0xff;
				p[2] = pixval & 0xff;
				p += 3;
			}
#else
			for (x = 0; x < dst->w; ++x)
			{
				pixval = p[0] | p[1] << 8 | p[2] << 16;
				pixval = ((((pixval & fmt->Rmask) * 3) >> 2) & fmt->Rmask) |
					((((pixval & fmt->Gmask) * 3) >> 2) & fmt->Gmask) |
					((((pixval & fmt->Bmask) * 3) >> 2) & fmt->Bmask);
				p[0] = pixval & 0xff;
				p[1] = (pixval >> 8) & 0xff;
				p[2] = (pixval >> 16) & 0xff;
				p += 3;
			}
#endif
		}
		break;
	}
	case 4:
	{
		for (y = 0; y < dst->h; y += 2)
		{
			Uint32 *p = (Uint32 *) &pixels[y * dst->pitch];
			for (x = 0; x < dst->w; ++x)
			{
				*p++ = ((((*p & fmt->Rmask) * 3) >> 2) & fmt->Rmask) |
					((((*p & fmt->Gmask) * 3) >> 2) & fmt->Gmask) |
					((((*p & fmt->Bmask) * 3) >> 2) & fmt->Bmask);
			}
		}
		break;
	}
	}
}

void
TFB_Pure_SwapBuffers ()
{
	int fade_amount = FadeAmount;
	int transition_amount = TransitionAmount;
	SDL_Rect updated;

	updated.x = TFB_BBox.region.corner.x;
	updated.y = TFB_BBox.region.corner.y;
	updated.w = TFB_BBox.region.extent.width;
	updated.h = TFB_BBox.region.extent.height;
		
	if ((transition_amount != 255) || (fade_amount != 255))
	{
		updated.x = updated.y = 0;
		updated.w = ScreenWidth;
		updated.h = ScreenHeight;
	}

	if (ScreenWidth == 320 && ScreenHeight == 240 &&
		ScreenWidthActual == 640 && ScreenHeightActual == 480)
	{
		// scales 320x240 backbuffer to 640x480

		SDL_Surface *backbuffer = SDL_Screen;
		
		if (transition_amount != 255)
		{
			backbuffer = fade_temp;
			SDL_BlitSurface (SDL_Screen, NULL, backbuffer, NULL);

			SDL_SetAlpha (TransitionScreen, SDL_SRCALPHA, 255 - transition_amount);
			SDL_BlitSurface (TransitionScreen, &TransitionClipRect, backbuffer, &TransitionClipRect);
		}

		if (fade_amount != 255)
		{
			if (transition_amount == 255)
			{
				backbuffer = fade_temp;
				SDL_BlitSurface (SDL_Screen, NULL, backbuffer, NULL);
			}

			if (fade_amount < 255)
			{
				SDL_SetAlpha (fade_black, SDL_SRCALPHA, 255 - fade_amount);
				SDL_BlitSurface (fade_black, NULL, backbuffer, NULL);
			}
			else
			{
				SDL_SetAlpha (fade_white, SDL_SRCALPHA, fade_amount - 255);
				SDL_BlitSurface (fade_white, NULL, backbuffer, NULL);
			}
		}

		SDL_LockSurface (SDL_Video);
		SDL_LockSurface (backbuffer);
		
		if (gfx_flags & TFB_GFXFLAGS_SCALE_BILINEAR)
			Scale_BilinearFilter (backbuffer, SDL_Video, &updated);
		else if (gfx_flags & TFB_GFXFLAGS_SCALE_BIADAPT)
			Scale_BiAdaptFilter (backbuffer, SDL_Video, &updated);
		else
			Scale_Nearest (backbuffer, SDL_Video, &updated);

		if (gfx_flags & TFB_GFXFLAGS_SCANLINES)
			ScanLines (SDL_Video);
		
		SDL_UnlockSurface (backbuffer);
		SDL_UnlockSurface (SDL_Video);
	}
	else
	{
		// resolution is 320x240 so we can blit directly

		SDL_BlitSurface (SDL_Screen, &updated, SDL_Video, &updated);

		if (transition_amount != 255)
		{
			SDL_SetAlpha (TransitionScreen, SDL_SRCALPHA, 255 - transition_amount);
			SDL_BlitSurface (TransitionScreen, &TransitionClipRect, SDL_Video, &TransitionClipRect);
		}

		if (fade_amount != 255)
		{
			if (fade_amount < 255)
			{
				SDL_SetAlpha (fade_black, SDL_SRCALPHA, 255 - fade_amount);
				SDL_BlitSurface (fade_black, NULL, SDL_Video, NULL);
			}
			else
			{
				SDL_SetAlpha (fade_white, SDL_SRCALPHA, fade_amount - 255);
				SDL_BlitSurface (fade_white, NULL, SDL_Video, NULL);
			}
		}
	}

	SDL_UpdateRect (SDL_Video, 0, 0, 0, 0);
}

#endif
