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

/* By Mika Kolehmainen, 2002-12-08
 */


#include <assert.h>
#include "sdl_common.h"
#include "primitives.h"
#include "libs/sound/trackplayer.h"

static FRAMEPTR scope_frame;
static int scope_init = 0;
static SDL_Surface *scope_bg = NULL; // oscilloscope background (lbm/activity.9.png)
static SDL_Surface *scope_surf = NULL;
static UBYTE scope_data[RADAR_WIDTH];


void
InitOscilloscope (DWORD x, DWORD y, DWORD width, DWORD height, FRAME_DESC *f)
{
	TFB_Image *img = (TFB_Image *)((BYTE *)f + f->DataOffs);

	assert (img->NormalImg->format->BytesPerPixel == 1);
	assert (img->NormalImg->w == RADAR_WIDTH);
	assert (img->NormalImg->h == RADAR_HEIGHT);

	if (scope_init == 0)
	{
		int i;

		LockMutex (img->mutex);
		scope_bg = SDL_CreateRGBSurface (SDL_SWSURFACE, img->NormalImg->w, 
			img->NormalImg->h, 8, 
			0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000);
		scope_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, img->NormalImg->w, 
			img->NormalImg->h, 8, 
			0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000);
		
		for (i = 0; i < 256; ++i)
		{
			scope_bg->format->palette->colors[i].r = scope_surf->format->palette->colors[i].r = 
				img->NormalImg->format->palette->colors[i].r;
			scope_bg->format->palette->colors[i].g = scope_surf->format->palette->colors[i].g = 
				img->NormalImg->format->palette->colors[i].g;
			scope_bg->format->palette->colors[i].b = scope_surf->format->palette->colors[i].b = 
				img->NormalImg->format->palette->colors[i].b;
		}

		SDL_BlitSurface (img->NormalImg, NULL, scope_bg, NULL);
		UnlockMutex (img->mutex);
		scope_init = 1;
	}

	scope_frame = f;
	
}

void
UninitOscilloscope (void)
{
	if (scope_bg)
	{
		SDL_FreeSurface (scope_bg);
		scope_bg = NULL;
	}
	if (scope_surf)
	{
		SDL_FreeSurface (scope_surf);
		scope_surf = NULL;
	}
}

// draws the oscilloscope
void
Oscilloscope (DWORD grab_data)
{
	int i;
	TFB_Image *img;
	STAMP s;
	
	if (!grab_data)
		return;

	SDL_BlitSurface (scope_bg, NULL, scope_surf, NULL);
	if (GetSoundData (scope_data))
	{
		PutPixelFn scope_plot = putpixel_for (scope_surf);

		SDL_LockSurface (scope_surf);
		for (i = 0; i < RADAR_WIDTH - 1; ++i)
		{
			line (i, scope_data[i], i + 1, scope_data[i + 1], 238, scope_plot, scope_surf);
		}
		SDL_UnlockSurface (scope_surf);
	}

	img = (TFB_Image *)((BYTE *)scope_frame + scope_frame->DataOffs);	
	LockMutex (img->mutex);
	SDL_BlitSurface (scope_surf, NULL, img->NormalImg, NULL);
	UnlockMutex (img->mutex);

	s.frame = scope_frame;
	s.origin.x = s.origin.y = 0;
	DrawStamp (&s);
}
