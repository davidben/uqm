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

#ifndef SDL_COMMON_H
#define SDL_COMMON_H

#include "SDL.h"
#include "SDL_image.h"

#include "libs/graphics/gfxintrn.h"
#include "libs/input/inpintrn.h"
#include "libs/graphics/gfx_common.h"
#include "libs/input/sdl/input.h"
#include "libs/threadlib.h"

extern SDL_Surface *SDL_Video;
extern SDL_Surface *SDL_Screen;
extern SDL_Surface *ExtraScreen;
extern SDL_Surface *TransitionScreen;

extern volatile int TransitionAmount;
extern SDL_Rect TransitionClipRect;

extern volatile int continuity_break;
extern int GfxFlags;

void ScreenOrigin (FRAME Display, COORD sx, COORD sy);
void LoadDisplay (PDISPLAY_INTERFACE *pDisplay);

// TFB_ImageStruct will be casted to this and vice versa, so sizes must be equal
typedef struct tfb_image
{
	SDL_Surface *NormalImg;
	SDL_Surface *ScaledImg;
	SDL_Color *Palette;
	int colormap_index;
	int scale;
	Mutex mutex;
	UBYTE pad[sizeof(TFB_ImageStruct)-sizeof(SDL_Surface*)-sizeof(SDL_Surface*)-
		sizeof(SDL_Color*)-sizeof(int)-sizeof(int)-sizeof(Mutex)];
} TFB_Image;

TFB_Image *TFB_LoadImage (SDL_Surface *img);
void TFB_FreeImage (TFB_Image *img);

void TFB_SwapBuffers ();
SDL_Surface* TFB_DisplayFormatAlpha (SDL_Surface *surface);
void TFB_BlitSurface (SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst,
					  SDL_Rect *dstrect, int blend_numer, int blend_denom);


#endif
