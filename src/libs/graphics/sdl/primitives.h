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

#ifndef PRIMITIVES_H
#define PRIMITIVES_H

/* Function types for the pixel functions */

typedef Uint32 (*GetPixelFn)(SDL_Surface *, int, int);
typedef void (*PutPixelFn)(SDL_Surface *, int, int, Uint32);

GetPixelFn getpixel_for(SDL_Surface *surface);
PutPixelFn putpixel_for(SDL_Surface *surface);

void line(int x1, int y1, int x2, int y2, Uint32 color, PutPixelFn plot, 
		  SDL_Surface *surface);
int clip_line(int *lx1, int *ly1, int *lx2, int *ly2, SDL_Rect *r);
#endif
