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
#include "primitives.h"


// Pixel drawing routines

Uint32 getpixel_8(SDL_Surface *surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x;
	return *p;
}

void putpixel_8(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 1;
	*p = pixel;
}

Uint32 getpixel_16(SDL_Surface *surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 2;
	return *(Uint16 *)p;
}

void putpixel_16(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 2;
	*(Uint16 *)p = pixel;
}

Uint32 getpixel_24_be(SDL_Surface *surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 3;
	return p[0] << 16 | p[1] << 8 | p[2];
}

void putpixel_24_be(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 3;
	p[0] = (pixel >> 16) & 0xff;
	p[1] = (pixel >> 8) & 0xff;
	p[2] = pixel & 0xff;
}

Uint32 getpixel_24_le(SDL_Surface *surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 3;
	return p[0] | p[1] << 8 | p[2] << 16;
}

void putpixel_24_le(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 3;
	p[0] = pixel & 0xff;
	p[1] = (pixel >> 8) & 0xff;
	p[2] = (pixel >> 16) & 0xff;
}

Uint32 getpixel_32(SDL_Surface *surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 4;
	return *(Uint32 *)p;
}

void putpixel_32(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 4;
	*(Uint32 *)p = pixel;
}

GetPixelFn getpixel_for(SDL_Surface *surface)
{
	int bpp = surface->format->BytesPerPixel;
	switch (bpp) {
	case 1:
		return &getpixel_8;
		break;
	case 2:
		return &getpixel_16;
		break;
	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			return &getpixel_24_be;
		} else {
			return &getpixel_24_le;
		}
		break;
	case 4:
		return &getpixel_32;
		break;
	}
	return NULL;
}

PutPixelFn putpixel_for(SDL_Surface *surface)
{
	int bpp = surface->format->BytesPerPixel;
	switch (bpp) {
	case 1:
		return &putpixel_8;
		break;
	case 2:
		return &putpixel_16;
		break;
	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			return &putpixel_24_be;
		} else {
			return &putpixel_24_le;
		}
		break;
	case 4:
		return &putpixel_32;
		break;
	}
	return NULL;
}

/* Line drawing routine
 * Adapted from Paul Heckbert's implementation of Bresenham's algorithm,
 * 3 Sep 85; taken from Graphics Gems I */

void line(int x1, int y1, int x2, int y2, Uint32 color, PutPixelFn plot, SDL_Surface *surface)
{
	int d, x, y, ax, ay, sx, sy, dx, dy;
	SDL_Rect r;

	SDL_GetClipRect (surface, &r);
	if (!clip_line (&x1, &y1, &x2, &y2, &r))
		return; // line is completely outside clipping rectangle

	dx = x2-x1;
	ax = ((dx < 0) ? -dx : dx) << 1;
	sx = (dx < 0) ? -1 : 1;
	dy = y2-y1;
	ay = ((dy < 0) ? -dy : dy) << 1;
	sy = (dy < 0) ? -1 : 1;

	x = x1;
	y = y1;
	if (ax > ay) {
		d = ay - (ax >> 1);
		for (;;) {
			(*plot)(surface, x, y, color);
			if (x == x2)
				return;
			if (d >= 0) {
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	} else {
		d = ax - (ay >> 1);
		for (;;) {
			(*plot)(surface, x, y, color);
			if (y == y2)
				return;
			if (d >= 0) {
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}


// Clips line against rectangle using Cohen-Sutherland algorithm

enum {C_TOP = 0x1, C_BOTTOM = 0x2, C_RIGHT = 0x4, C_LEFT = 0x8};

static int
compute_code (float x, float y, float xmin, float ymin, float xmax, float ymax)
{
	int c = 0;
	if (y > ymax)
		c |= C_TOP;
	else if (y < ymin)
		c |= C_BOTTOM;
	if (x > xmax)
		c |= C_RIGHT;
	else if (x < xmin)
		c |= C_LEFT;
	return c;
}

int
clip_line (int *lx1, int *ly1, int *lx2, int *ly2, SDL_Rect *r)
{
	int C0, C1, C;
	float x, y, x0, y0, x1, y1, xmin, ymin, xmax, ymax;

	x0 = (float)*lx1;
	y0 = (float)*ly1;
	x1 = (float)*lx2;
	y1 = (float)*ly2;

	xmin = (float)r->x;
	ymin = (float)r->y;
	xmax = (float)r->x + r->w - 1;
	ymax = (float)r->y + r->h - 1;

	C0 = compute_code (x0, y0, xmin, ymin, xmax, ymax);
	C1 = compute_code (x1, y1, xmin, ymin, xmax, ymax);

	for (;;) {
		/* trivial accept: both ends in rectangle */
		if ((C0 | C1) == 0)
		{
			*lx1 = (int)x0;
			*ly1 = (int)y0;
			*lx2 = (int)x1;
			*ly2 = (int)y1;
			return 1;
		}

		/* trivial reject: both ends on the external side of the rectangle */
		if ((C0 & C1) != 0)
			return 0;

		/* normal case: clip end outside rectangle */
		C = C0 ? C0 : C1;
		if (C & C_TOP)
		{
			x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
			y = ymax;
		}
		else if (C & C_BOTTOM)
		{
			x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
			y = ymin;
		}
		else if (C & C_RIGHT)
		{
			x = xmax;
			y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
		}
		else
		{
			x = xmin;
			y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
		}

		/* set new end point and iterate */
		if (C == C0)
		{
			x0 = x; y0 = y;
			C0 = compute_code (x0, y0, xmin, ymin, xmax, ymax);
		} 
		else
		{
			x1 = x; y1 = y;
			C1 = compute_code (x1, y1, xmin, ymin, xmax, ymax);
		}
	}
}

#endif
