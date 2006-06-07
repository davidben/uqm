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

/* 
   The adjustments are as follows:
   BLUR_TYPE:
		0: No blur
		1: A fast but very blurry convolution
		2: A fast implementation of gaussian_blur with R0 = 0.8
		3: Full 7x7 gaussian blur.  use R0 to control how blurry to get
			larger R0 numbers == more blurry.
	RANDOM_METHOD:
		0: Use standard RotoZoom without interpolation
		1: Use standard RotoZoom with  interpolationn
		2: Choose at random from nearest neighors
		3: Choose from nearest-neighbors with probability based on
			distance from each pixel
		4: Use the 'smart' random method, which gaurantees a path of
			constant color from a given pixel to one of the nearest
			neighbors.  Randomization is based on a weighted average of
			distance to each nearest neighbor.
*/
#define BLUR_TYPE 2
#define R0 0.8
#define RANDOM_METHOD 3

#ifdef GFXMODULE_SDL

#ifdef WIN32
#include <io.h>
#endif
#include <fcntl.h>

#include "sdl_common.h"
#include "primitives.h"
#include "libs/log.h"

#ifndef MAX
#define MAX(x,y) ((x) < (y) ? (y) : (x))
#endif
#ifndef MIN
#define MIN(x,y) ((x) > (y) ? (y) : (x))
#endif
#if 0
int MAX(int x, int y)
{
	return ((x) < (y) ? (y) : (x));
}

int MIN (int x, int y)
{
	return ((x) > (y) ? (y) : (x));
}
#endif

void blurSurface32 (SDL_Surface *src)
{
	typedef struct 
	{
		Uint32 r, g, b;
	} color;
	GetPixelFn getpix;
	Uint32 *ptr;
	int y, x, yy, xx;
	Uint32 i, offset, ypos, yof;
#if BLUR_TYPE == 1
	#define MWID 3
	#define ARRAY_SIZE 4
	#define BLUR_NORM(f) MIN (255, (f) >> 2)
	#define BLUR_SHIFT(x,f) ((x) << shift[f])
	#define NEED_ZERO_SHIFT
	unsigned char idx[MWID * MWID] = 
	{
		0, 1, 0,
		1, 1, 1,
		0, 1, 0
	};
	int shift[ARRAY_SIZE] = {0, 0, 1, 3};
	color *blur_array[ARRAY_SIZE];
#elif BLUR_TYPE == 2
	#define MWID 3
	#define ARRAY_SIZE 3
	#define BLUR_NORM(f) MIN (255, (f) >> 9)
	#define BLUR_SHIFT(x,f) ((x) * shift[f])
	unsigned char idx[MWID * MWID] = 
	{
		0, 1, 0,
		1, 2, 1,
		0, 1, 0
	};
	int shift[ARRAY_SIZE] = {21, 62, 182};
	color *blur_array[ARRAY_SIZE];
#elif BLUR_TYPE == 3
	#define MWID 7
	#define BLUR_NORM(f) MIN (255, (int)((f) / sum))
	#define BLUR_SHIFT(x,f) ((Uint32)((x) * shift[f]))
	// array-size is the triangular number of (MWID+1)/2
	#define ARRAY_SIZE ((((MWID + 1) >> 1) * (((MWID + 1) >> 1) + 1)) >> 1)
	float sum = 0.0;
	float shift[ARRAY_SIZE];
	color *blur_array[ARRAY_SIZE];
	unsigned char idx[MWID * MWID];
	{
		// Build th eidx and shift arrays using the gaussian function
		unsigned int i, j, k = 0;
		unsigned int r_2;
		for (i = 0;  i <= (MWID >> 1); i++)
		{
			for (j = 0; j <= i; j++)
			{
				r_2 = i  * i + j * j;
				shift[k] =  1000 * (float)exp (- log (2) * r_2 / (R0 * R0));
				if (i == 0 && j == 0)
					sum += shift[k];
				else if (j == 0 || i == j)
					sum += 4 * shift[k];
				else
					sum += 8 * shift[k];
				y = (MWID >> 1) - i;
				x = (MWID >> 1) - j;
				idx[y * MWID + x] = k;
				idx[y * MWID + MWID - x - 1] = k;
				idx[(MWID - y - 1) * MWID + x] = k;
				idx[(MWID - y - 1) * MWID + MWID - x - 1] = k;
				idx[x * MWID + y] = k;
				idx[x * MWID + MWID - y - 1] = k;
				idx[(MWID - x - 1) * MWID + y] = k;
				idx[(MWID - x - 1) * MWID + MWID - y - 1] = k;
				k++;
			}
		}
//		printf ("sum is: %f\n", sum);
//		i = 0;
//		for(y =  0; y < MWID; y++) 
//		{
//			for (x = 0; x< MWID; x++)
//				printf(" %f", shift[idx[i++]]);
//			printf("\n");
//		}
	}
#else
	#define MWID 3
	#define ARRAY_SIZE 1
	#define BLUR_NORM(f) MIN (255, (f) >> 2)
	#define BLUR_SHIFT(x,f) ((x) << shift[f])
	unsigned char idx[1];
	int shift[1];
	color *blur_array[1];
	return;
#endif

	if (src->format->BitsPerPixel != 32)
	{
		log_add (log_Debug, "blurSurface32 requires a 32bit Surface, but surface is %d bits!\n",
			src->format->BitsPerPixel);
		return;
	}
	SDL_LockSurface (src);
	getpix = getpixel_for (src);

	// Make ARRAY_SIZE copies of the surface, each multiplied by a constant in 'shift'
	for(i = 0; i < ARRAY_SIZE; i++)
		blur_array[i] = (color *)HMalloc (sizeof(color) * 
			(src->w + MWID - 1) * (src->h + MWID - 1));
	offset = 0;
	ptr = src->pixels;
	// the blur_array matrices are larger than the target surface by MWID - 1
	// this is to allow convolving to work at the image edges.
	// note that the convolution 'wraps' around on the x coordinate, butnot the y
	for (y = -(MWID >> 1); y < src->h + (MWID >> 1); y++)
	{
		for (x = - (MWID >> 1); x < src->w + (MWID >> 1); x++)
		{
			unsigned char r, g, b;
			Uint32 pixel;
			i = 0;
			if (x < 0)
				pixel = getpix (src, x + src->w, MIN (src->h - 1, MAX (0, y)));
			else if (x >= src->w)
				pixel = getpix (src, x - src->w, MIN (src->h - 1, MAX (0, y)));
			else if (y < 0 || y >= src->h)
				pixel = getpix (src, x, MIN (src->h - 1, MAX (0, y)));
			else
				pixel = *ptr++;
			SDL_GetRGB (pixel, src->format, &r, &g, &b);	
#ifdef NEED_ZERO_SHIFT
			blur_array[i][offset].r = 0;
			blur_array[i][offset].g = 0;
			blur_array[i][offset].b = 0;
			i++;
#endif
			for (; i < ARRAY_SIZE; i++) {
				blur_array[i][offset].r = BLUR_SHIFT((Uint32)r,i);
				blur_array[i][offset].g = BLUR_SHIFT((Uint32)g,i);
				blur_array[i][offset].b = BLUR_SHIFT((Uint32)b,i);
			}
			offset++;
		}
	}
	ptr = src->pixels;
	ypos =0;
	// actually do theconvolving here.
	for (y = 0; y < src->h; y++, ypos += src->w + (MWID - 1))
	{
		for (x = 0; x < src->w; x++)
		{
			Uint32 pixel;
			Uint32 bluroff, idxoff;
			color p;
			p.r =0;
			p.g =0;
			p.b =0;
			idxoff = 0;
			yof = ypos;
			for (yy = - (MWID >> 1); yy <= (MWID >> 1); yy++)
			{
				bluroff = yof + x;
				for (xx = - (MWID >> 1); xx <= (MWID >> 1); xx++)
				{
					p.r+=blur_array[idx[idxoff]][bluroff].r;
					p.g+=blur_array[idx[idxoff]][bluroff].g;
					p.b+=blur_array[idx[idxoff]][bluroff].b;
					idxoff++;
					bluroff++;
				}
				yof += src->w + (MWID - 1);
			}
			p.r = BLUR_NORM(p.r); 
			p.g = BLUR_NORM(p.g); 
			p.b = BLUR_NORM(p.b); 
			pixel = SDL_MapRGB (src->format, (Uint8)p.r, (Uint8)p.g, (Uint8)p.b);
			*ptr++ = pixel;
		}
	}
	SDL_UnlockSurface (src);
	for(i = 0; i < ARRAY_SIZE; i++)
		HFree (blur_array[i]);
}

/* the rZF_nearestneighbor method fills the 'dst' image with the 'src' image
   (performing a 16x zoom).  The interior points are generated by randomly choosing
   the color of one of the 4 nearest neighbors.  The randomization method is either
   an even probability i.e 1/4, or a weighted probabliliity based on the distance
   from each of the neighbors
*/
void rZF_nearestneighbor (SDL_Surface *src, SDL_Surface *dst, int  use_weight)
{
	int x, y;
	Uint32 p00, p01, p10, p11;
	Uint32 *yptr, *y1ptr, *dstptr;
	UBYTE rand2;
	UWORD prob_even[] = {64, 128, 192};
	UWORD prob_weight[] = {
		256, 256, 256,   144, 178, 127,   88, 127, 215,   48, 76, 220,
		144, 192, 226,   114, 165, 216,   79, 127, 206,   51, 89, 203,
		 88, 176, 215,    79, 158, 206,   64, 128, 192,   48, 96, 175,
		 48, 192, 220,    51, 165, 203,   48, 127, 175,   38, 89, 140
	};
	UWORD *pry = prob_weight, *pr;
	if (!use_weight)
		pr = prob_even;
	yptr = src->pixels;
	y1ptr= yptr + src->w;
	dstptr = dst->pixels;
	for (y = 0; y < dst->h; y++)
	{
		if (y)
		{
			if (!(y & 0x03))
			{
				yptr += src->w;
				if (y >> 2 < src->h - 1)
					y1ptr += src->w;
				if (use_weight)
					pry = prob_weight;
			}
			else if (use_weight)
				pry += 12;
		}
		for (x = 0; x < dst->w; x++)
		{
			Uint32 p;
			if (! (x & 0x03))
			{
				int x0 = x >> 2;
				p00 = *(yptr + x0);
				p01 = *(y1ptr + x0);
				if(x0 < src->w - 1)
				{
					p10 = *(yptr + x0 + 1);
					p11 = *(y1ptr + x0 + 1);
				}
				else
				{
					p10 = *yptr;
					p11 = *y1ptr;
				}
				if (use_weight)
					pr = pry;
			}
			else
			{
				if (use_weight)
					pr += 3;
			}
			rand2 = rand() & 0xff;
			if (rand2 < pr[0]) 
				p =p00;
			else if (rand2 < pr[1])
				p = p01;
			else if (rand2 < pr[2])
				p = p10;
			else
				p = p11;
			*dstptr++ = p;
		}
	}
}

/* the rZF_continuous method fills the 'dst' image with the 'src' image
   (performing a 16x zoom).  The interior points are generated by randomly choosing
   the color of one of the 4 nearest neighbors.  However, it is gauranteed that there
   will bbe a path of constant color from the current pixel to the nearest-neighbor
   that was chosen.  The randomization function is biased by the pixel's distance
   fromit's neighbors
*/
void rZF_continuous (SDL_Surface *src, SDL_Surface *dst)
{
	int rnd[] = {192, 171, 128, 0};
	GetPixelFn getpix;
	PutPixelFn putpix;
	int rand2, dx, dy;
	Uint32 pnew;
	int x, y, x1, y1;
	Uint32 p01, p10, p11;
	int rand1;
	getpix = getpixel_for (src);
	putpix = putpixel_for (dst);
	putpix (dst, 0, 0, getpix (src, 0, 0));
	for (y = 0; y < src->h; y++)
	{
		p10 = getpix (src, 0, y);
		if (y == src->h - 1)
			p11 = getpix (src, 0, y);
		else
			p11 = getpix (src, 0, y + 1);
		for (y1 = 0; y1 < 4; y1++)
		{
			dy = (y << 2) + y1;
			rand1 = rand () & 0xff;
			if (rand1 < rnd[y1])
				pnew = p10;
			else
			{
				pnew = p11;
				p10 = p11;
			}	
			putpix (dst, 0, dy, pnew);
		}
	}
	for (x = 0; x < src->w; x++)
	{
		p10 = getpix (src, x, 0);
		if (x == src->w - 1)
			p11 = getpix (src, 0, 0);
		else
			p11 = getpix (src, x + 1, 0);
		for (x1 = 0; x1 < 4; x1++)
		{
			dx = (x << 2) + x1;
			rand1 = rand () & 0xff;
			if (rand1 < rnd[x1])
				pnew = p10;
			else
			{
				pnew = p11;
				p10 = p11;
			}
			putpix (dst, dx, 0, pnew);
		}
	}
	
	for (y = 1; y <= src->h; y++)
	{
		int fromy, fromy4;
		if (y == src->h)
			fromy = y - 1;
		else
			fromy = y;
		fromy4 = fromy << 2;
		for(x = 1; x <= src->w; x++)
		{
			int fromx, fromx4;
			int dy1;
			Uint32 p00;
			char cpl[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
			if (x == src->w)
				fromx = 0;
			else
				fromx = x;
			fromx4 = fromx << 2;
			dx = x << 2;
			p00 = getpix (src, x - 1, y - 1);
			p10 = getpix (src, fromx, y - 1);
			p01 = getpix (src, x - 1, fromy);
			p11 = getpix (src, fromx, fromy);
			if (x != src->w) {
				for (y1 = 0; y1 < 4; y1++)
				{
					if (y == src->h && y1 == 3)
						continue;
					dy = ((y - 1) << 2) + y1 + 1;
					rand1 = rand () & 0xff;
					if (rand1 < rnd[y1])
						pnew = p01;
					else
					{
						pnew = p11;
						p01 = p11;
					}
					putpix (dst, dx, dy, pnew);
				}
			}
			if (y != src->h)
			{
				dy = y << 2;
				for (x1 = 0; x1 < 3; x1++)
				{
					dx = ((x - 1) << 2) + x1 + 1;
					rand1 = rand () & 0xff;
					if (rand1 < rnd[x1])
						pnew = p10;
					else
					{
						pnew = p11;
						p10 = p11;
					}
					putpix (dst, dx, dy, pnew);
				}
			}
			for (y1 = 0; y1 < 3; y1++)
			{
				for (x1 = 0; x1 < 3; x1++)
				{
					int dxx, dyy;
					if (cpl[x1][y1])
						continue;
					dyy=dy = ((y - 1) << 2) + y1 + 1;
					dxx=dx = ((x - 1) << 2) + x1 + 1;
					rand1 = rand () & 0xff;
					rand2 = rand () & 0xff;
					if (rand1 < 128)
					{
						if (rand2 < rnd[x1])
						{
							putpix (dst, dx, dy, getpix (dst, dx - 1, dy));
						}
						else
						{
							p00 = getpix (dst, fromx4, dy);
							while (dx < x <<2)
							{
								putpix (dst, dx, dy, p00);
								dx++;
								x1++;
							}
						}
					}
					else
					{
						if (rand2 < rnd[y1])
						{
							putpix (dst, dx, dy, getpix (dst, dx, dy - 1));
						}
						else
						{
							p00 = getpix (dst, dx, fromy4);
							dy1 = y1;
							while (dy < y << 2)
							{
								putpix (dst, dx, dy, p00);
								dy++;
								cpl[x1][dy1] = 1;
								dy1++;
							}
						}
					}
					if(getpix(dst,dxx,dyy) == 0)
						printf ("wtf\n");
				}
			}
		}
	}
}

/* perform a 16x zoom, and then apply a blur filter to the result */
void random16xZoomSurfaceRGBA (SDL_Surface *src, SDL_Surface *dst)
{
    /*
     * Alloc space to completely contain the zoomed surface 
     */
	SDL_LockSurface (src);
	SDL_LockSurface (dst);
#if RANDOM_METHOD == 0
	zoomSurfaceRGBA (src, dst, 0);
#elif RANDOM_METHOD == 1
	zoomSurfaceRGBA (src, dst, 1);
#elif RANDOM_METHOD == 2
	rZF_nearestneighbor (src, dst, 0);
#elif RANDOM_METHOD == 3
	rZF_nearestneighbor (src, dst, 1);
#else
	rZF_continuous(src, dst);
#endif
	SDL_UnlockSurface (src);
	SDL_UnlockSurface (dst);
	SDL_SetAlpha(dst, SDL_SRCALPHA, 255);
	blurSurface32 (dst);
}

FRAMEPTR scale16xRandomizeFrame (FRAMEPTR NewFrame, FRAMEPTR FramePtr)
{
	TFB_Image *origImg, *newImg;
	CREATE_FLAGS flags;

	if (NewFrame == NULL)
	{
		flags = GetFrameParentDrawable (FramePtr)->Flags;
		NewFrame = CaptureDrawable (
				CreateDrawable (flags, 
					GetFrameWidth (FramePtr) << 2, 
					GetFrameHeight (FramePtr) << 2, 1));
	}

	newImg = NewFrame->image;
	origImg = FramePtr->image;
	LockMutex (origImg->mutex);
	random16xZoomSurfaceRGBA (origImg->NormalImg, newImg->NormalImg);
	UnlockMutex (origImg->mutex);
	return (NewFrame);
}
#endif
