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

// replaces all non-alpha pixels by color
void replace_color (Uint32 color, SDL_Surface *surface)
{
	int x,y,w,h;
	Uint32 p;
	Uint8 r,g,b,a;
	GetPixelFn getpix;
	PutPixelFn putpix;

	getpix = getpixel_for (surface);
	putpix = putpixel_for (surface);

	SDL_LockSurface(surface);

	w = surface->w;
	h = surface->h;

	for (y=0;y<h;++y)
	{
		for (x=0;x<w;++x)
		{
			p = getpix (surface,x,y);
			SDL_GetRGBA (p, surface->format, &r, &g, &b, &a);
			
			if (a)
				putpix (surface,x,y,color);
		}
	}

	SDL_UnlockSurface(surface);
}

#endif
