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

#include "libs/graphics/sdl/sdl_common.h"
#include "2xscalers.h"

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

// RGB -> YUV transformation
// the RGB vector is multiplied by the transformation matrix
// to get the YUV vector
static const double YUV_matrix[3][3] =
{
	/*         Y        U        V    */
	/* R */ {0.2989, -0.1687,  0.5000},
	/* G */ {0.5867, -0.3312, -0.4183},
	/* B */ {0.1144,  0.5000, -0.0816}
};

typedef enum
{
	YUV_XFORM_R = 0,
	YUV_XFORM_G = 1,
	YUV_XFORM_B = 2,
	YUV_XFORM_Y = 0,
	YUV_XFORM_U = 1,
	YUV_XFORM_V = 2
} RGB_YUV_INDEX;

// pre-computed transformations for 8 bits per channel
static int RGB_to_YUV[/*RGB*/ 3][/*YUV*/ 3][ /*mult-res*/ 256];
// double to int rounding error correction
#define YUV_ERR_CORR 0.4999

// pre-compute the RGB->YUV transformations
void
Scale_PrepYUV()
{
	int i1;

	for (i1 = 0; i1 < 3; i1++) // enum R,G,B
	{
		int i2;
		for (i2 = 0; i2 < 3; i2++) // enum Y,U,V
		{
			int i3;
			for (i3 = 0; i3 < 256; i3++) // enum possible channel vals
				RGB_to_YUV[i1][i2][i3] = (int)
						(YUV_matrix[i1][i2] * i3 + YUV_ERR_CORR);
		}
	}
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

// blends two pixels with ratio 50% - 50% (safe for 32bpp surfaces)
__inline__ Uint32
Scale_BlendPixels_32bpp_safe (SDL_PixelFormat* fmt, Uint32 pix1, Uint32 pix2)
{
	return ((((((pix1 & fmt->Rmask) >> fmt->Rshift) +
			((pix2 & fmt->Rmask) >> fmt->Rshift)) >> 1) << fmt->Rshift) & fmt->Rmask) |
			((((((pix1 & fmt->Gmask) >> fmt->Gshift) +
			((pix2 & fmt->Gmask) >> fmt->Gshift)) >> 1) << fmt->Gshift) & fmt->Gmask) |
			((((((pix1 & fmt->Bmask) >> fmt->Bshift) +
			((pix2 & fmt->Bmask) >> fmt->Bshift)) >> 1) << fmt->Bshift) & fmt->Bmask);
}

// compare pixels by their RGB
__inline__ int
Scale_GetRGBDelta (SDL_PixelFormat* fmt, Uint32 pix1, Uint32 pix2)
{
	Uint32 c;
	int delta;

	c = (((pix1 & fmt->Rmask) >> fmt->Rshift) << fmt->Rloss)
			- (((pix2 & fmt->Rmask) >> fmt->Rshift) << fmt->Rloss);
	delta = c * c;

	c = (((pix1 & fmt->Gmask) >> fmt->Gshift) << fmt->Gloss)
			- (((pix2 & fmt->Gmask) >> fmt->Gshift) << fmt->Gloss);
	delta += c * c;

	c = (((pix1 & fmt->Bmask) >> fmt->Bshift) << fmt->Bloss)
			- (((pix2 & fmt->Bmask) >> fmt->Bshift) << fmt->Bloss);
	delta += c * c;

	return delta;
}

// compare pixels by their YUVs
__inline__ int
Scale_GetYUVDelta (SDL_PixelFormat* fmt, Uint32 pix1, Uint32 pix2)
{
	Uint32 r1, g1, b1, r2, g2, b2;
	int compd, delta;

	r1 = ((pix1 & fmt->Rmask) >> fmt->Rshift) << fmt->Rloss;
	g1 = ((pix1 & fmt->Gmask) >> fmt->Gshift) << fmt->Gloss;
	b1 = ((pix1 & fmt->Bmask) >> fmt->Bshift) << fmt->Bloss;
	r2 = ((pix2 & fmt->Rmask) >> fmt->Rshift) << fmt->Rloss;
	g2 = ((pix2 & fmt->Gmask) >> fmt->Gshift) << fmt->Gloss;
	b2 = ((pix2 & fmt->Bmask) >> fmt->Bshift) << fmt->Bloss;

	// compare Ys
	compd = RGB_to_YUV [YUV_XFORM_R][YUV_XFORM_Y][r1]
			+ RGB_to_YUV [YUV_XFORM_G][YUV_XFORM_Y][g1]
			+ RGB_to_YUV [YUV_XFORM_B][YUV_XFORM_Y][b1]
			- RGB_to_YUV [YUV_XFORM_R][YUV_XFORM_Y][r2]
			- RGB_to_YUV [YUV_XFORM_G][YUV_XFORM_Y][g2]
			- RGB_to_YUV [YUV_XFORM_B][YUV_XFORM_Y][b2];
	if (compd < 0)
		compd = -compd;
	// Y is 2-4 times more important than U or V
	delta = compd << 1;

	// compare Us
	compd = RGB_to_YUV [YUV_XFORM_R][YUV_XFORM_U][r1]
			+ RGB_to_YUV [YUV_XFORM_G][YUV_XFORM_U][g1]
			+ RGB_to_YUV [YUV_XFORM_B][YUV_XFORM_U][b1]
			- RGB_to_YUV [YUV_XFORM_R][YUV_XFORM_U][r2]
			- RGB_to_YUV [YUV_XFORM_G][YUV_XFORM_U][g2]
			- RGB_to_YUV [YUV_XFORM_B][YUV_XFORM_U][b2];
	if (compd < 0)
		compd = -compd;
	delta += compd;
	
	// compare Vs
	compd = RGB_to_YUV [YUV_XFORM_R][YUV_XFORM_V][r1]
			+ RGB_to_YUV [YUV_XFORM_G][YUV_XFORM_V][g1]
			+ RGB_to_YUV [YUV_XFORM_B][YUV_XFORM_V][b1]
			- RGB_to_YUV [YUV_XFORM_R][YUV_XFORM_V][r2]
			- RGB_to_YUV [YUV_XFORM_G][YUV_XFORM_V][g2]
			- RGB_to_YUV [YUV_XFORM_B][YUV_XFORM_V][b2];
	if (compd < 0)
		compd = -compd;
	delta += compd;

	return delta;
}

// retrieve the Y (intensity) component of pixel's YUV
__inline__ int
Scale_GetPixY (SDL_PixelFormat* fmt, Uint32 pix)
{
	Uint32 r, g, b;
	
	r = ((pix & fmt->Rmask) >> fmt->Rshift) << fmt->Rloss;
	g = ((pix & fmt->Gmask) >> fmt->Gshift) << fmt->Gloss;
	b = ((pix & fmt->Bmask) >> fmt->Bshift) << fmt->Bloss;

	// compare Ys
	return RGB_to_YUV [YUV_XFORM_R][YUV_XFORM_Y][r]
			+ RGB_to_YUV [YUV_XFORM_G][YUV_XFORM_Y][g]
			+ RGB_to_YUV [YUV_XFORM_B][YUV_XFORM_Y][b];
}

// biadapt scaling to 2x
void
Scale_BiAdaptFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r)
{
	int x, y;
	const int w = src->w, h = src->h, dw = dst->w;
	int xend, yend;
	int dsrc, ddst;
	SDL_Rect *region = r;
	SDL_Rect limits;
	SDL_PixelFormat *fmt = dst->format;

	// expand updated region if necessary
	// pixels neighbooring the updated region may
	// change as a result of updates
	limits.x = 0;
	limits.y = 0;
	limits.w = src->w;
	limits.h = src->h;
	Scale_ExpandRect (region, 2, &limits);

	xend = region->x + region->w;
	yend = region->y + region->h;
	dsrc = w - region->w;
	ddst = (dw - region->w) * 2;

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
		src_p += w * region->y + region->x;
		dst_p += (dw * region->y + region->x) * 2;

		for (y = region->y; y < yend; ++y, dst_p += ddst, src_p += dsrc)
		{
			for (x = region->x; x < xend; ++x, ++src_p, ++dst_p)
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
		src_p += w * region->y + region->x;
		dst_p += (dw * region->y + region->x) * 2;

		for (y = region->y; y < yend; ++y, dst_p += ddst, src_p += dsrc)
		{
			for (x = region->x; x < xend; ++x, ++src_p, ++dst_p)
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
	#define Scale_BlendPixels Scale_BlendPixels_32bpp_safe
	{
		BIADAPT_BUF *src_p = (BIADAPT_BUF *)src->pixels;
		BIADAPT_BUF *dst_p = (BIADAPT_BUF *)dst->pixels;
		Uint32 pixval_tl, pixval_tr, pixval_bl, pixval_br;
		
		// move ptrs to the first updated pixel
		src_p += w * region->y + region->x;
		dst_p += (dw * region->y + region->x) * 2;

		for (y = region->y; y < yend; ++y, dst_p += ddst, src_p += dsrc)
		{
			for (x = region->x; x < xend; ++x, ++src_p, ++dst_p)
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
	#undef Scale_BlendPixels

	}
}

// advanced biadapt scaling to 2x
void
Scale_BiAdaptAdvFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r)
{
	int x, y;
	const int w = src->w, h = src->h, dw = dst->w;
	int xend, yend;
	int dsrc, ddst;
	SDL_Rect *region = r;
	SDL_Rect limits;
	SDL_PixelFormat *fmt = dst->format;
	// for clarity purposes, the 'pixels' array here is transposed
	Uint32 pixels[4][4];
	static int resolve_coord[][2] = 
	{
		{0, -1}, {1, -1}, { 2, 0}, { 2, 1},
		{1,  2}, {0,  2}, {-1, 1}, {-1, 0},
		{100, 100}	// term
	};

	// these macros are for clarity; they make the current pixel (0,0)
	// and allow to access pixels in all directions
	#define PIX(x, y)   (pixels[1 + (x)][1 + (y)])
	#define SRC(x, y)   (src_p + (x) + ((y) * w))
	// commonly used operations, for clarity also
	// others are defined at their respective bpp levels
	#define BIADAPT_RGBHIGH   8000
	#define BIADAPT_YUVLOW      30
	#define BIADAPT_YUVMED      70
	#define BIADAPT_YUVHIGH    130

	#define BIADAPT_CMPRGB(p1, p2) \
			Scale_GetRGBDelta (fmt, p1, p2)
	// high tolerance pixel comparison
	#define BIADAPT_CMPRGB_HIGH(p1, p2) \
			(p1 == p2 || BIADAPT_CMPRGB (p1, p2) <= BIADAPT_RGBHIGH)

	#define BIADAPT_CMPYUV(p1, p2) \
			Scale_GetYUVDelta (fmt, p1, p2)
	// low tolerance pixel comparison
	#define BIADAPT_CMPYUV_LOW(p1, p2) \
			(p1 == p2 || BIADAPT_CMPYUV (p1, p2) <= BIADAPT_YUVLOW)
	// medium tolerance pixel comparison
	#define BIADAPT_CMPYUV_MED(p1, p2) \
			(p1 == p2 || BIADAPT_CMPYUV (p1, p2) <= BIADAPT_YUVMED)
	// high tolerance pixel comparison
	#define BIADAPT_CMPYUV_HIGH(p1, p2) \
			(p1 == p2 || BIADAPT_CMPYUV (p1, p2) <= BIADAPT_YUVHIGH)

	// expand updated region if necessary
	// pixels neighbooring the updated region may
	// change as a result of updates
	limits.x = 0;
	limits.y = 0;
	limits.w = src->w;
	limits.h = src->h;
	Scale_ExpandRect (region, 2, &limits);

	xend = region->x + region->w;
	yend = region->y + region->h;
	dsrc = w - region->w;
	ddst = (dw - region->w) * 2;

	switch (fmt->BytesPerPixel)
	{
	case 2: // 16bpp scaling
	#define BIADAPT_GETPIX(p)        ( *(Uint16 *)(p) )
	#define BIADAPT_SETPIX(p, c)     ( *(Uint16 *)(p) = (c) )
	#define BIADAPT_BUF              Uint16
	{
		BIADAPT_BUF *src_p = (BIADAPT_BUF *)src->pixels;
		BIADAPT_BUF *dst_p = (BIADAPT_BUF *)dst->pixels;

		src_p += w * region->y + region->x;
		dst_p += (dw * region->y + region->x) * 2;

		for (y = region->y; y < yend; ++y, dst_p += ddst, src_p += dsrc)
		{
			for (x = region->x; x < xend; ++x, ++src_p, ++dst_p)
			{
				// pixel eqaulity counter
				int cmatch;

				// most pixels will fall into 'all 4 equal'
				// pattern, so we check it first
				cmatch = 0;

				PIX (0, 0) = BIADAPT_GETPIX (SRC (0, 0));
				
				BIADAPT_SETPIX (dst_p, PIX (0, 0));

				if (y + 1 < h)
				{
					// check pixel below the current one
					PIX (0, 1) = BIADAPT_GETPIX (SRC (0, 1));

					if (PIX (0, 0) == PIX (0, 1))
					{
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
						cmatch |= 1;
					}
				}
				else
				{
					// last pixel in column - propagate
					PIX (0, 1) = PIX (0, 0);
					BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
					cmatch |= 1;
					
				}

				if (x + 1 < w)
				{
					// check pixel to the right from the current one
					PIX (1, 0) = BIADAPT_GETPIX (SRC (1, 0));

					if (PIX (0, 0) == PIX (1, 0))
					{
						BIADAPT_SETPIX (dst_p + 1, PIX (0, 0));
						cmatch |= 2;
					}
				}
				else
				{
					// last pixel in row - propagate
					PIX (1, 0) = PIX (0, 0);
					BIADAPT_SETPIX (dst_p + 1, PIX (0, 0));
					cmatch |= 2;
				}

				if (cmatch == 3)
				{
					if (y + 1 < h || x + 1 < w)
					{
						// last pixel in row/column and nearest
						// neighboor is identical
						dst_p++;
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
						continue;
					}

					// check pixel to the bottom-right
					PIX (1, 1) = BIADAPT_GETPIX (SRC (1, 1));

					if (PIX (0, 0) == PIX (1, 1))
					{
						// all 4 are equal - propagate
						dst_p++;
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
						continue;
					}
				}

				// some neighnoors are different, lets check them

				if (x > 0)
					PIX (-1, 0) = BIADAPT_GETPIX (SRC (-1, 0));
				else
					PIX (-1, 0) = PIX (0, 0);

				if (x + 2 < w)
					PIX (2, 0) = BIADAPT_GETPIX (SRC (2, 0));
				else
					PIX (2, 0) = PIX (1, 0);
				
				if (y + 1 < h)
				{
					if (x > 0)
						PIX (-1, 1) = BIADAPT_GETPIX (SRC (-1, 1));
					else
						PIX (-1, 1) = PIX (0, 1);

					if (x + 2 < w)
					{
						PIX (1, 1) = BIADAPT_GETPIX (SRC (1, 1));
						PIX (2, 1) = BIADAPT_GETPIX (SRC (2, 1));
					}
					else if (x + 1 < w)
					{
						PIX (1, 1) = BIADAPT_GETPIX (SRC (1, 1));
						PIX (2, 1) = PIX (1, 1);
					}
					else
					{
						PIX (1, 1) = PIX (0, 1);
						PIX (2, 1) = PIX (0, 1);
					}
				}
				else
				{
					// last pixel in column
					PIX (-1, 1) = PIX (-1, 0);
					PIX (1, 1) = PIX (1, 0);
					PIX (2, 1) = PIX (2, 0);
				}

				if (y + 2 < h)
				{
					PIX (0, 2) = BIADAPT_GETPIX (SRC (0, 2));

					if (x > 0)
						PIX (-1, 2) = BIADAPT_GETPIX (SRC (-1, 2));
					else
						PIX (-1, 2) = PIX (0, 2);

					if (x + 2 < w)
					{
						PIX (1, 2) = BIADAPT_GETPIX (SRC (1, 2));
						PIX (2, 2) = BIADAPT_GETPIX (SRC (2, 2));
					}
					else if (x + 1 < w)
					{
						PIX (1, 2) = BIADAPT_GETPIX (SRC (1, 2));
						PIX (2, 2) = PIX (1, 2);
					}
					else
					{
						PIX (1, 2) = PIX (0, 2);
						PIX (2, 2) = PIX (0, 2);
					}
				}
				else
				{
					// last pixel in column
					PIX (-1, 2) = PIX (-1, 1);
					PIX (0, 2) = PIX (0, 1);
					PIX (1, 2) = PIX (1, 1);
					PIX (2, 2) = PIX (2, 1);
				}

				if (y > 0)
				{
					PIX (0, -1) = BIADAPT_GETPIX (SRC (0, -1));

					if (x > 0)
						PIX (-1, -1) = BIADAPT_GETPIX (SRC (-1, -1));
					else
						PIX (-1, -1) = PIX (0, -1);

					if (x + 2 < w)
					{
						PIX (1, -1) = BIADAPT_GETPIX (SRC (1, -1));
						PIX (2, -1) = BIADAPT_GETPIX (SRC (2, -1));
					}
					else if (x + 1 < w)
					{
						PIX (1, -1) = BIADAPT_GETPIX (SRC (1, -1));
						PIX (2, -1) = PIX (1, -1);
					}
					else
					{
						PIX (1, -1) = PIX (0, -1);
						PIX (2, -1) = PIX (0, -1);
					}
				}
				else
				{
					PIX (-1, -1) = PIX (-1, 0);
					PIX (0, -1) = PIX (0, 0);
					PIX (1, -1) = PIX (1, 0);
					PIX (2, -1) = PIX (2, 0);
				}

				// check pixel below the current one
				if (!(cmatch & 1))
				{
					if (BIADAPT_CMPYUV (PIX (0, 0), PIX (0, 1)) <= BIADAPT_YUVLOW)
					{
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (0, 1))
								);
						cmatch |= 1;
					}
					// detect a 2:1 line going across the current pixel
					else if ( (PIX (0, 0) == PIX (-1, 0)
							&& PIX (0, 0) == PIX (1, 1)
							&& PIX (0, 0) == PIX (2, 1) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 0))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 2))))) ||

							(PIX (0, 0) == PIX (1, 0)
							&& PIX (0, 0) == PIX (-1, 1)
							&& PIX (0, 0) == PIX (2, -1) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, -1))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 0))))) )
					{
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
					}
					// detect a 2:1 line going across the pixel below current
					else if ( (PIX (0, 1) == PIX (-1, 0)
							&& PIX (0, 1) == PIX (1, 1)
							&& PIX (0, 1) == PIX (2, 2) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (2, 1))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (0, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, 2))))) ||

							(PIX (0, 1) == PIX (1, 0)
							&& PIX (0, 1) == PIX (-1, 1)
							&& PIX (0, 1) == PIX (2, 0) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (2, -1))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (0, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (2, 1))))) )
					{
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 1));
					}
					else
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (0, 1))
								);
				}

				dst_p++;

				// check pixel to the right from the current one
				if (!(cmatch & 2))
				{
					if (BIADAPT_CMPYUV (PIX (0, 0), PIX (1, 0)) <= BIADAPT_YUVLOW)
					{
						BIADAPT_SETPIX (dst_p, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (1, 0))
								);
						cmatch |= 2;
					}
					// detect a 1:2 line going across the current pixel
					else if ( (PIX (0, 0) == PIX (1, -1)
							&& PIX (0, 0) == PIX (0, 1)
							&& PIX (0, 0) == PIX (-1, 2) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 1))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, 2))))) ||

							(PIX (0, 0) == PIX (0, -1)
							&& PIX (0, 0) == PIX (1, 1)
							&& PIX (0, 0) == PIX (1, 2) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, 2))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 2))))) )

					{
						BIADAPT_SETPIX (dst_p, PIX (0, 0));
					}
					// detect a 1:2 line going across the pixel to the right
					else if ( (PIX (1, 0) == PIX (1, -1)
							&& PIX (1, 0) == PIX (0, 1)
							&& PIX (1, 0) == PIX (0, 2) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (0, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (-1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (-1, 2))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, 2))))) ||
							
							(PIX (1, 0) == PIX (0, -1)
							&& PIX (1, 0) == PIX (1, 1)
							&& PIX (1, 0) == PIX (2, 2) &&
							
							((!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (-1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (0, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, 2))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, 1))))) )
					{
						BIADAPT_SETPIX (dst_p, PIX (1, 0));
					}
					else
						BIADAPT_SETPIX (dst_p, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (1, 0))
								);
				}

				if (PIX (0, 0) == PIX (1, 1) && PIX (1, 0) == PIX (0, 1))
				{
					// diagonals are equal
					int *coord;
					int cl, cr;
					Uint32 clr;

					// both pairs are equal, have to resolve the pixel
					// race; we try detecting which color is
					// the background by looking for a line or an edge
					// examine 8 pixels surrounding the current quad

					cl = cr = 2;
					for (coord = resolve_coord[0]; *coord < 100; coord += 2)
					{
						clr = PIX (coord[0], coord[1]);

						if (BIADAPT_CMPYUV_MED (clr, PIX (0, 0)))
							cl++;
						else if (BIADAPT_CMPYUV_MED (clr, PIX (1, 0)))
							cr++;
					}

					// least count wins
					if (cl > cr)
						clr = PIX (1, 0);
					else if (cr > cl)
						clr = PIX (0, 0);
					else
						clr = Scale_BlendPixels (fmt,
								PIX (0, 0), PIX (1, 0));

					BIADAPT_SETPIX (dst_p + dw, clr);
					continue;
				}

				if (cmatch == 3
						|| (BIADAPT_CMPYUV_LOW (PIX (1, 0), PIX (0, 1))
						&& BIADAPT_CMPYUV_LOW (PIX (1, 0), PIX (1, 1))))
				{
					BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
							fmt, PIX (0, 1), PIX (1, 0))
							);
					continue;
				}
				else if (cmatch && BIADAPT_CMPYUV_LOW (PIX (0, 0), PIX (1, 1)))
				{
					BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
							fmt, PIX (0, 0), PIX (1, 1))
							);
					continue;
				}

				// check pixel to the bottom-right
				if (BIADAPT_CMPYUV_HIGH (PIX (0, 0), PIX (1, 1))
						&& BIADAPT_CMPYUV_HIGH (PIX (1, 0), PIX (0, 1)))
				{
					if (Scale_GetPixY (fmt, PIX (0, 0))
							> Scale_GetPixY (fmt, PIX (1, 0)))
					{
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (1, 1))
								);
					}
					else
					{
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, PIX (1, 0), PIX (0, 1))
								);
					}
				}
				else if (BIADAPT_CMPYUV_HIGH (PIX (0, 0), PIX (1, 1)))
				{
					// main diagonal is same color
					// use its value
					BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
							fmt, PIX (0, 0), PIX (1, 1))
							);
				}
				else if (BIADAPT_CMPYUV_HIGH (PIX (1, 0), PIX (0, 1)))
				{
					// 2nd diagonal is same color
					// use its value
					BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
							fmt, PIX (1, 0), PIX (0, 1))
							);
				}
				else
				{
					// blend all 4
					BIADAPT_SETPIX (dst_p + dw,
							((((PIX (0, 0) & fmt->Rmask) +
							   (PIX (0, 1) & fmt->Rmask) +
							   (PIX (1, 0) & fmt->Rmask) +
							   (PIX (1, 1) & fmt->Rmask)
							 ) >> 2) & fmt->Rmask) |
							((((PIX (0, 0) & fmt->Gmask) +
							   (PIX (0, 1) & fmt->Gmask) +
							   (PIX (1, 0) & fmt->Gmask) +
							   (PIX (1, 1) & fmt->Gmask)
							 ) >> 2) & fmt->Gmask) |
							((((PIX (0, 0) & fmt->Bmask) +
							   (PIX (0, 1) & fmt->Bmask) +
							   (PIX (1, 0) & fmt->Bmask) +
							   (PIX (1, 1) & fmt->Bmask)
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

		src_p += w * region->y + region->x;
		dst_p += (dw * region->y + region->x) * 2;

		for (y = region->y; y < yend; ++y, dst_p += ddst, src_p += dsrc)
		{
			for (x = region->x; x < xend; ++x, ++src_p, ++dst_p)
			{
				// pixel eqaulity counter
				int cmatch;

				// most pixels will fall into 'all 4 equal'
				// pattern, so we check it first
				cmatch = 0;

				PIX (0, 0) = BIADAPT_GETPIX (SRC (0, 0));
				
				BIADAPT_SETPIX (dst_p, PIX (0, 0));

				if (y + 1 < h)
				{
					// check pixel below the current one
					PIX (0, 1) = BIADAPT_GETPIX (SRC (0, 1));

					if (PIX (0, 0) == PIX (0, 1))
					{
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
						cmatch |= 1;
					}
				}
				else
				{
					// last pixel in column - propagate
					PIX (0, 1) = PIX (0, 0);
					BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
					cmatch |= 1;
					
				}

				if (x + 1 < w)
				{
					// check pixel to the right from the current one
					PIX (1, 0) = BIADAPT_GETPIX (SRC (1, 0));

					if (PIX (0, 0) == PIX (1, 0))
					{
						BIADAPT_SETPIX (dst_p + 1, PIX (0, 0));
						cmatch |= 2;
					}
				}
				else
				{
					// last pixel in row - propagate
					PIX (1, 0) = PIX (0, 0);
					BIADAPT_SETPIX (dst_p + 1, PIX (0, 0));
					cmatch |= 2;
				}

				if (cmatch == 3)
				{
					if (y + 1 < h || x + 1 < w)
					{
						// last pixel in row/column and nearest
						// neighboor is identical
						dst_p++;
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
						continue;
					}

					// check pixel to the bottom-right
					PIX (1, 1) = BIADAPT_GETPIX (SRC (1, 1));

					if (PIX (0, 0) == PIX (1, 1))
					{
						// all 4 are equal - propagate
						dst_p++;
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
						continue;
					}
				}

				// some neighnoors are different, lets check them

				if (x > 0)
					PIX (-1, 0) = BIADAPT_GETPIX (SRC (-1, 0));
				else
					PIX (-1, 0) = PIX (0, 0);

				if (x + 2 < w)
					PIX (2, 0) = BIADAPT_GETPIX (SRC (2, 0));
				else
					PIX (2, 0) = PIX (1, 0);
				
				if (y + 1 < h)
				{
					if (x > 0)
						PIX (-1, 1) = BIADAPT_GETPIX (SRC (-1, 1));
					else
						PIX (-1, 1) = PIX (0, 1);

					if (x + 2 < w)
					{
						PIX (1, 1) = BIADAPT_GETPIX (SRC (1, 1));
						PIX (2, 1) = BIADAPT_GETPIX (SRC (2, 1));
					}
					else if (x + 1 < w)
					{
						PIX (1, 1) = BIADAPT_GETPIX (SRC (1, 1));
						PIX (2, 1) = PIX (1, 1);
					}
					else
					{
						PIX (1, 1) = PIX (0, 1);
						PIX (2, 1) = PIX (0, 1);
					}
				}
				else
				{
					// last pixel in column
					PIX (-1, 1) = PIX (-1, 0);
					PIX (1, 1) = PIX (1, 0);
					PIX (2, 1) = PIX (2, 0);
				}

				if (y + 2 < h)
				{
					PIX (0, 2) = BIADAPT_GETPIX (SRC (0, 2));

					if (x > 0)
						PIX (-1, 2) = BIADAPT_GETPIX (SRC (-1, 2));
					else
						PIX (-1, 2) = PIX (0, 2);

					if (x + 2 < w)
					{
						PIX (1, 2) = BIADAPT_GETPIX (SRC (1, 2));
						PIX (2, 2) = BIADAPT_GETPIX (SRC (2, 2));
					}
					else if (x + 1 < w)
					{
						PIX (1, 2) = BIADAPT_GETPIX (SRC (1, 2));
						PIX (2, 2) = PIX (1, 2);
					}
					else
					{
						PIX (1, 2) = PIX (0, 2);
						PIX (2, 2) = PIX (0, 2);
					}
				}
				else
				{
					// last pixel in column
					PIX (-1, 2) = PIX (-1, 1);
					PIX (0, 2) = PIX (0, 1);
					PIX (1, 2) = PIX (1, 1);
					PIX (2, 2) = PIX (2, 1);
				}

				if (y > 0)
				{
					PIX (0, -1) = BIADAPT_GETPIX (SRC (0, -1));

					if (x > 0)
						PIX (-1, -1) = BIADAPT_GETPIX (SRC (-1, -1));
					else
						PIX (-1, -1) = PIX (0, -1);

					if (x + 2 < w)
					{
						PIX (1, -1) = BIADAPT_GETPIX (SRC (1, -1));
						PIX (2, -1) = BIADAPT_GETPIX (SRC (2, -1));
					}
					else if (x + 1 < w)
					{
						PIX (1, -1) = BIADAPT_GETPIX (SRC (1, -1));
						PIX (2, -1) = PIX (1, -1);
					}
					else
					{
						PIX (1, -1) = PIX (0, -1);
						PIX (2, -1) = PIX (0, -1);
					}
				}
				else
				{
					PIX (-1, -1) = PIX (-1, 0);
					PIX (0, -1) = PIX (0, 0);
					PIX (1, -1) = PIX (1, 0);
					PIX (2, -1) = PIX (2, 0);
				}

				// check pixel below the current one
				if (!(cmatch & 1))
				{
					if (BIADAPT_CMPYUV (PIX (0, 0), PIX (0, 1)) <= BIADAPT_YUVLOW)
					{
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (0, 1))
								);
						cmatch |= 1;
					}
					// detect a 2:1 line going across the current pixel
					else if ( (PIX (0, 0) == PIX (-1, 0)
							&& PIX (0, 0) == PIX (1, 1)
							&& PIX (0, 0) == PIX (2, 1) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 0))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 2))))) ||

							(PIX (0, 0) == PIX (1, 0)
							&& PIX (0, 0) == PIX (-1, 1)
							&& PIX (0, 0) == PIX (2, -1) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, -1))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 0))))) )
					{
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
					}
					// detect a 2:1 line going across the pixel below current
					else if ( (PIX (0, 1) == PIX (-1, 0)
							&& PIX (0, 1) == PIX (1, 1)
							&& PIX (0, 1) == PIX (2, 2) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (2, 1))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (0, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, 2))))) ||

							(PIX (0, 1) == PIX (1, 0)
							&& PIX (0, 1) == PIX (-1, 1)
							&& PIX (0, 1) == PIX (2, 0) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (2, -1))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (0, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (2, 1))))) )
					{
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 1));
					}
					else
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (0, 1))
								);
				}

				dst_p++;

				// check pixel to the right from the current one
				if (!(cmatch & 2))
				{
					if (BIADAPT_CMPYUV (PIX (0, 0), PIX (1, 0)) <= BIADAPT_YUVLOW)
					{
						BIADAPT_SETPIX (dst_p, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (1, 0))
								);
						cmatch |= 2;
					}
					// detect a 1:2 line going across the current pixel
					else if ( (PIX (0, 0) == PIX (1, -1)
							&& PIX (0, 0) == PIX (0, 1)
							&& PIX (0, 0) == PIX (-1, 2) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 1))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, 2))))) ||

							(PIX (0, 0) == PIX (0, -1)
							&& PIX (0, 0) == PIX (1, 1)
							&& PIX (0, 0) == PIX (1, 2) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, 2))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 2))))) )

					{
						BIADAPT_SETPIX (dst_p, PIX (0, 0));
					}
					// detect a 1:2 line going across the pixel to the right
					else if ( (PIX (1, 0) == PIX (1, -1)
							&& PIX (1, 0) == PIX (0, 1)
							&& PIX (1, 0) == PIX (0, 2) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (0, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (-1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (-1, 2))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, 2))))) ||
							
							(PIX (1, 0) == PIX (0, -1)
							&& PIX (1, 0) == PIX (1, 1)
							&& PIX (1, 0) == PIX (2, 2) &&
							
							((!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (-1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (0, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, 2))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, 1))))) )
					{
						BIADAPT_SETPIX (dst_p, PIX (1, 0));
					}
					else
						BIADAPT_SETPIX (dst_p, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (1, 0))
								);
				}

				if (PIX (0, 0) == PIX (1, 1) && PIX (1, 0) == PIX (0, 1))
				{
					// diagonals are equal
					int *coord;
					int cl, cr;
					Uint32 clr;

					// both pairs are equal, have to resolve the pixel
					// race; we try detecting which color is
					// the background by looking for a line or an edge
					// examine 8 pixels surrounding the current quad

					cl = cr = 2;
					for (coord = resolve_coord[0]; *coord < 100; coord += 2)
					{
						clr = PIX (coord[0], coord[1]);

						if (BIADAPT_CMPYUV_MED (clr, PIX (0, 0)))
							cl++;
						else if (BIADAPT_CMPYUV_MED (clr, PIX (1, 0)))
							cr++;
					}

					// least count wins
					if (cl > cr)
						clr = PIX (1, 0);
					else if (cr > cl)
						clr = PIX (0, 0);
					else
						clr = Scale_BlendPixels (fmt,
								PIX (0, 0), PIX (1, 0));

					BIADAPT_SETPIX (dst_p + dw, clr);
					continue;
				}

				if (cmatch == 3
						|| (BIADAPT_CMPYUV_LOW (PIX (1, 0), PIX (0, 1))
						&& BIADAPT_CMPYUV_LOW (PIX (1, 0), PIX (1, 1))))
				{
					BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
							fmt, PIX (0, 1), PIX (1, 0))
							);
					continue;
				}
				else if (cmatch && BIADAPT_CMPYUV_LOW (PIX (0, 0), PIX (1, 1)))
				{
					BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
							fmt, PIX (0, 0), PIX (1, 1))
							);
					continue;
				}

				// check pixel to the bottom-right
				if (BIADAPT_CMPYUV_HIGH (PIX (0, 0), PIX (1, 1))
						&& BIADAPT_CMPYUV_HIGH (PIX (1, 0), PIX (0, 1)))
				{
					if (Scale_GetPixY (fmt, PIX (0, 0))
							> Scale_GetPixY (fmt, PIX (1, 0)))
					{
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (1, 1))
								);
					}
					else
					{
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, PIX (1, 0), PIX (0, 1))
								);
					}
				}
				else if (BIADAPT_CMPYUV_HIGH (PIX (0, 0), PIX (1, 1)))
				{
					// main diagonal is same color
					// use its value
					BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
							fmt, PIX (0, 0), PIX (1, 1))
							);
				}
				else if (BIADAPT_CMPYUV_HIGH (PIX (1, 0), PIX (0, 1)))
				{
					// 2nd diagonal is same color
					// use its value
					BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
							fmt, PIX (1, 0), PIX (0, 1))
							);
				}
				else
				{
					// blend all 4
					BIADAPT_SETPIX (dst_p + dw,
							((((PIX (0, 0) & fmt->Rmask) +
							   (PIX (0, 1) & fmt->Rmask) +
							   (PIX (1, 0) & fmt->Rmask) +
							   (PIX (1, 1) & fmt->Rmask)
							 ) >> 2) & fmt->Rmask) |
							((((PIX (0, 0) & fmt->Gmask) +
							   (PIX (0, 1) & fmt->Gmask) +
							   (PIX (1, 0) & fmt->Gmask) +
							   (PIX (1, 1) & fmt->Gmask)
							 ) >> 2) & fmt->Gmask) |
							((((PIX (0, 0) & fmt->Bmask) +
							   (PIX (0, 1) & fmt->Bmask) +
							   (PIX (1, 0) & fmt->Bmask) +
							   (PIX (1, 1) & fmt->Bmask)
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
	#define Scale_BlendPixels Scale_BlendPixels_32bpp_safe
	{
		BIADAPT_BUF *src_p = (BIADAPT_BUF *)src->pixels;
		BIADAPT_BUF *dst_p = (BIADAPT_BUF *)dst->pixels;

		src_p += w * region->y + region->x;
		dst_p += (dw * region->y + region->x) * 2;

		for (y = region->y; y < yend; ++y, dst_p += ddst, src_p += dsrc)
		{
			for (x = region->x; x < xend; ++x, ++src_p, ++dst_p)
			{
				// pixel eqaulity counter
				int cmatch;

				// most pixels will fall into 'all 4 equal'
				// pattern, so we check it first
				cmatch = 0;

				PIX (0, 0) = BIADAPT_GETPIX (SRC (0, 0));
				
				BIADAPT_SETPIX (dst_p, PIX (0, 0));

				if (y + 1 < h)
				{
					// check pixel below the current one
					PIX (0, 1) = BIADAPT_GETPIX (SRC (0, 1));

					if (PIX (0, 0) == PIX (0, 1))
					{
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
						cmatch |= 1;
					}
				}
				else
				{
					// last pixel in column - propagate
					PIX (0, 1) = PIX (0, 0);
					BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
					cmatch |= 1;
					
				}

				if (x + 1 < w)
				{
					// check pixel to the right from the current one
					PIX (1, 0) = BIADAPT_GETPIX (SRC (1, 0));

					if (PIX (0, 0) == PIX (1, 0))
					{
						BIADAPT_SETPIX (dst_p + 1, PIX (0, 0));
						cmatch |= 2;
					}
				}
				else
				{
					// last pixel in row - propagate
					PIX (1, 0) = PIX (0, 0);
					BIADAPT_SETPIX (dst_p + 1, PIX (0, 0));
					cmatch |= 2;
				}

				if (cmatch == 3)
				{
					if (y + 1 < h || x + 1 < w)
					{
						// last pixel in row/column and nearest
						// neighboor is identical
						dst_p++;
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
						continue;
					}

					// check pixel to the bottom-right
					PIX (1, 1) = BIADAPT_GETPIX (SRC (1, 1));

					if (PIX (0, 0) == PIX (1, 1))
					{
						// all 4 are equal - propagate
						dst_p++;
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
						continue;
					}
				}

				// some neighnoors are different, lets check them

				if (x > 0)
					PIX (-1, 0) = BIADAPT_GETPIX (SRC (-1, 0));
				else
					PIX (-1, 0) = PIX (0, 0);

				if (x + 2 < w)
					PIX (2, 0) = BIADAPT_GETPIX (SRC (2, 0));
				else
					PIX (2, 0) = PIX (1, 0);
				
				if (y + 1 < h)
				{
					if (x > 0)
						PIX (-1, 1) = BIADAPT_GETPIX (SRC (-1, 1));
					else
						PIX (-1, 1) = PIX (0, 1);

					if (x + 2 < w)
					{
						PIX (1, 1) = BIADAPT_GETPIX (SRC (1, 1));
						PIX (2, 1) = BIADAPT_GETPIX (SRC (2, 1));
					}
					else if (x + 1 < w)
					{
						PIX (1, 1) = BIADAPT_GETPIX (SRC (1, 1));
						PIX (2, 1) = PIX (1, 1);
					}
					else
					{
						PIX (1, 1) = PIX (0, 1);
						PIX (2, 1) = PIX (0, 1);
					}
				}
				else
				{
					// last pixel in column
					PIX (-1, 1) = PIX (-1, 0);
					PIX (1, 1) = PIX (1, 0);
					PIX (2, 1) = PIX (2, 0);
				}

				if (y + 2 < h)
				{
					PIX (0, 2) = BIADAPT_GETPIX (SRC (0, 2));

					if (x > 0)
						PIX (-1, 2) = BIADAPT_GETPIX (SRC (-1, 2));
					else
						PIX (-1, 2) = PIX (0, 2);

					if (x + 2 < w)
					{
						PIX (1, 2) = BIADAPT_GETPIX (SRC (1, 2));
						PIX (2, 2) = BIADAPT_GETPIX (SRC (2, 2));
					}
					else if (x + 1 < w)
					{
						PIX (1, 2) = BIADAPT_GETPIX (SRC (1, 2));
						PIX (2, 2) = PIX (1, 2);
					}
					else
					{
						PIX (1, 2) = PIX (0, 2);
						PIX (2, 2) = PIX (0, 2);
					}
				}
				else
				{
					// last pixel in column
					PIX (-1, 2) = PIX (-1, 1);
					PIX (0, 2) = PIX (0, 1);
					PIX (1, 2) = PIX (1, 1);
					PIX (2, 2) = PIX (2, 1);
				}

				if (y > 0)
				{
					PIX (0, -1) = BIADAPT_GETPIX (SRC (0, -1));

					if (x > 0)
						PIX (-1, -1) = BIADAPT_GETPIX (SRC (-1, -1));
					else
						PIX (-1, -1) = PIX (0, -1);

					if (x + 2 < w)
					{
						PIX (1, -1) = BIADAPT_GETPIX (SRC (1, -1));
						PIX (2, -1) = BIADAPT_GETPIX (SRC (2, -1));
					}
					else if (x + 1 < w)
					{
						PIX (1, -1) = BIADAPT_GETPIX (SRC (1, -1));
						PIX (2, -1) = PIX (1, -1);
					}
					else
					{
						PIX (1, -1) = PIX (0, -1);
						PIX (2, -1) = PIX (0, -1);
					}
				}
				else
				{
					PIX (-1, -1) = PIX (-1, 0);
					PIX (0, -1) = PIX (0, 0);
					PIX (1, -1) = PIX (1, 0);
					PIX (2, -1) = PIX (2, 0);
				}

				// check pixel below the current one
				if (!(cmatch & 1))
				{
					if (BIADAPT_CMPYUV (PIX (0, 0), PIX (0, 1)) <= BIADAPT_YUVLOW)
					{
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (0, 1))
								);
						cmatch |= 1;
					}
					// detect a 2:1 line going across the current pixel
					else if ( (PIX (0, 0) == PIX (-1, 0)
							&& PIX (0, 0) == PIX (1, 1)
							&& PIX (0, 0) == PIX (2, 1) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 0))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 2))))) ||

							(PIX (0, 0) == PIX (1, 0)
							&& PIX (0, 0) == PIX (-1, 1)
							&& PIX (0, 0) == PIX (2, -1) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, -1))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 0))))) )
					{
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 0));
					}
					// detect a 2:1 line going across the pixel below current
					else if ( (PIX (0, 1) == PIX (-1, 0)
							&& PIX (0, 1) == PIX (1, 1)
							&& PIX (0, 1) == PIX (2, 2) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (2, 1))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (0, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, 2))))) ||

							(PIX (0, 1) == PIX (1, 0)
							&& PIX (0, 1) == PIX (-1, 1)
							&& PIX (0, 1) == PIX (2, 0) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (2, -1))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (0, 2))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (2, 1))))) )
					{
						BIADAPT_SETPIX (dst_p + dw, PIX (0, 1));
					}
					else
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (0, 1))
								);
				}

				dst_p++;

				// check pixel to the right from the current one
				if (!(cmatch & 2))
				{
					if (BIADAPT_CMPYUV (PIX (0, 0), PIX (1, 0)) <= BIADAPT_YUVLOW)
					{
						BIADAPT_SETPIX (dst_p, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (1, 0))
								);
						cmatch |= 2;
					}
					// detect a 1:2 line going across the current pixel
					else if ( (PIX (0, 0) == PIX (1, -1)
							&& PIX (0, 0) == PIX (0, 1)
							&& PIX (0, 0) == PIX (-1, 2) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 1))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, 2))))) ||

							(PIX (0, 0) == PIX (0, -1)
							&& PIX (0, 0) == PIX (1, 1)
							&& PIX (0, 0) == PIX (1, 2) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, 2))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 2))))) )

					{
						BIADAPT_SETPIX (dst_p, PIX (0, 0));
					}
					// detect a 1:2 line going across the pixel to the right
					else if ( (PIX (1, 0) == PIX (1, -1)
							&& PIX (1, 0) == PIX (0, 1)
							&& PIX (1, 0) == PIX (0, 2) &&

							((!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (0, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (-1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (-1, 2))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, 2))))) ||
							
							(PIX (1, 0) == PIX (0, -1)
							&& PIX (1, 0) == PIX (1, 1)
							&& PIX (1, 0) == PIX (2, 2) &&
							
							((!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (-1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (0, 1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, 2))) ||
							(!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, -1))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, 0))
							&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, 1))))) )
					{
						BIADAPT_SETPIX (dst_p, PIX (1, 0));
					}
					else
						BIADAPT_SETPIX (dst_p, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (1, 0))
								);
				}

				if (PIX (0, 0) == PIX (1, 1) && PIX (1, 0) == PIX (0, 1))
				{
					// diagonals are equal
					int *coord;
					int cl, cr;
					Uint32 clr;

					// both pairs are equal, have to resolve the pixel
					// race; we try detecting which color is
					// the background by looking for a line or an edge
					// examine 8 pixels surrounding the current quad

					cl = cr = 2;
					for (coord = resolve_coord[0]; *coord < 100; coord += 2)
					{
						clr = PIX (coord[0], coord[1]);

						if (BIADAPT_CMPYUV_MED (clr, PIX (0, 0)))
							cl++;
						else if (BIADAPT_CMPYUV_MED (clr, PIX (1, 0)))
							cr++;
					}

					// least count wins
					if (cl > cr)
						clr = PIX (1, 0);
					else if (cr > cl)
						clr = PIX (0, 0);
					else
						clr = Scale_BlendPixels (fmt,
								PIX (0, 0), PIX (1, 0));

					BIADAPT_SETPIX (dst_p + dw, clr);
					continue;
				}

				if (cmatch == 3
						|| (BIADAPT_CMPYUV_LOW (PIX (1, 0), PIX (0, 1))
						&& BIADAPT_CMPYUV_LOW (PIX (1, 0), PIX (1, 1))))
				{
					BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
							fmt, PIX (0, 1), PIX (1, 0))
							);
					continue;
				}
				else if (cmatch && BIADAPT_CMPYUV_LOW (PIX (0, 0), PIX (1, 1)))
				{
					BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
							fmt, PIX (0, 0), PIX (1, 1))
							);
					continue;
				}

				// check pixel to the bottom-right
				if (BIADAPT_CMPYUV_HIGH (PIX (0, 0), PIX (1, 1))
						&& BIADAPT_CMPYUV_HIGH (PIX (1, 0), PIX (0, 1)))
				{
					if (Scale_GetPixY (fmt, PIX (0, 0))
							> Scale_GetPixY (fmt, PIX (1, 0)))
					{
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, PIX (0, 0), PIX (1, 1))
								);
					}
					else
					{
						BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
								fmt, PIX (1, 0), PIX (0, 1))
								);
					}
				}
				else if (BIADAPT_CMPYUV_HIGH (PIX (0, 0), PIX (1, 1)))
				{
					// main diagonal is same color
					// use its value
					BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
							fmt, PIX (0, 0), PIX (1, 1))
							);
				}
				else if (BIADAPT_CMPYUV_HIGH (PIX (1, 0), PIX (0, 1)))
				{
					// 2nd diagonal is same color
					// use its value
					BIADAPT_SETPIX (dst_p + dw, Scale_BlendPixels (
							fmt, PIX (1, 0), PIX (0, 1))
							);
				}
				else
				{
					// blend all 4
					BIADAPT_SETPIX (dst_p + dw,
							((((PIX (0, 0) & fmt->Rmask) +
							   (PIX (0, 1) & fmt->Rmask) +
							   (PIX (1, 0) & fmt->Rmask) +
							   (PIX (1, 1) & fmt->Rmask)
							 ) >> 2) & fmt->Rmask) |
							((((PIX (0, 0) & fmt->Gmask) +
							   (PIX (0, 1) & fmt->Gmask) +
							   (PIX (1, 0) & fmt->Gmask) +
							   (PIX (1, 1) & fmt->Gmask)
							 ) >> 2) & fmt->Gmask) |
							((((PIX (0, 0) & fmt->Bmask) +
							   (PIX (0, 1) & fmt->Bmask) +
							   (PIX (1, 0) & fmt->Bmask) +
							   (PIX (1, 1) & fmt->Bmask)
							) >> 2) & fmt->Bmask));
				}
			}
		}
		break;
	}
	#undef BIADAPT_GETPIX
	#undef BIADAPT_SETPIX
	#undef BIADAPT_BUF
	#undef Scale_BlendPixels
	}

	#undef BIADAPT_CMPRGB
	#undef BIADAPT_CMPRGB_HIGH
	#undef BIADAPT_CMPYUV
	#undef BIADAPT_CMPYUV_LOW
	#undef BIADAPT_CMPYUV_MED
	#undef BIADAPT_CMPYUV_HIGH

	#undef PIX
	#undef SRC
}


// nearest neighbor scaling to 2x
void Scale_Nearest (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r)
{
	int x, y;
	const int w = src->w, dw = dst->w;
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
		PIXEL_24BIT *src_p = (PIXEL_24BIT *)src->pixels;
		PIXEL_24BIT *dst_p = (PIXEL_24BIT *)dst->pixels;
		PIXEL_24BIT *src_p2;
		const int dd = dw - rw * 2; // override
		Uint32 pixval_32;
		src_p += w*y0 + x0;
		dst_p += (dw*y0 + x0) * 2;
		for (y = 0; y < rh; ++y)
		{
			src_p2 = src_p;			
			for (x = 0; x < rw; ++x, ++src_p)
			{
				pixval_32 = GET_PIX_24BIT (src_p);
				SET_PIX_24BIT (dst_p, pixval_32);
				dst_p++;
				SET_PIX_24BIT (dst_p, pixval_32);
				dst_p++;
			}
			dst_p += dd;
			src_p = src_p2;
			for (x = 0; x < rw; ++x, ++src_p)
			{
				pixval_32 = GET_PIX_24BIT (src_p);
				SET_PIX_24BIT (dst_p, pixval_32);
				dst_p++;
				SET_PIX_24BIT (dst_p, pixval_32);
				dst_p++;
			}
			dst_p += dd;
			src_p += ds;
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
void Scale_BilinearFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r)
{
	int x, y, i = 0, j, fa, fb, fc, fd;
	const int w = dst->w, h = dst->h;
	SDL_PixelFormat *fmt = dst->format;

	// TODO: support for partial updates, expand weight matrix in the code
	//       so that compiler can optimize the mults?
	
	r->x = 0;
	r->y = 0;
	r->w = src->w;
	r->h = src->h;

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
					(((((((p[0] & fmt->Rmask) >> fmt->Rshift) * fa) +
					(((p[1] & fmt->Rmask) >> fmt->Rshift) * fb) +
					(((p[2] & fmt->Rmask) >> fmt->Rshift) * fc) +
					(((p[3] & fmt->Rmask) >> fmt->Rshift) * fd)) >> 4) << fmt->Rshift) & fmt->Rmask) |
					(((((((p[0] & fmt->Gmask) >> fmt->Gshift) * fa) +
					(((p[1] & fmt->Gmask) >> fmt->Gshift) * fb) +
					(((p[2] & fmt->Gmask) >> fmt->Gshift) * fc) +
					(((p[3] & fmt->Gmask) >> fmt->Gshift) * fd)) >> 4) << fmt->Gshift) & fmt->Gmask) |
					(((((((p[0] & fmt->Bmask) >> fmt->Bshift) * fa) +
					(((p[1] & fmt->Bmask) >> fmt->Bshift) * fb) +
					(((p[2] & fmt->Bmask) >> fmt->Bshift) * fc) +
					(((p[3] & fmt->Bmask) >> fmt->Bshift) * fd)) >> 4) << fmt->Bshift) & fmt->Bmask);

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

#endif
