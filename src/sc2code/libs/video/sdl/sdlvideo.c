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

#include "libs/graphics/sdl/sdl_common.h"
#include "libs/video/video.h"
#include "sdlvideo.h"


void
TFB_GetScreenFormat (TFB_VideoFormat *fmt)
{
	SDL_PixelFormat *sdl = SDL_Video->format;
	fmt->BitsPerPixel = sdl->BitsPerPixel;
	fmt->Rmask = sdl->Rmask;
	fmt->Gmask = sdl->Gmask;
	fmt->Bmask = sdl->Bmask;
	fmt->Amask = 0; // no alpha support for now
}

TFB_Image*
TFB_CreateVideoImage (int w, int h)
{
	TFB_Image* img;
	SDL_PixelFormat* fmt = SDL_Video->format;

	img = HMalloc (sizeof (TFB_Image));
	img->mutex = CreateMutex ("vid image lock", SYNC_CLASS_VIDEO);
	img->ScaledImg = NULL;
	img->MipmapImg = NULL;
	img->colormap_index = -1;
	img->last_scale_type = -1;
	img->Palette = NULL;
	
	img->NormalImg = SDL_CreateRGBSurface (SDL_SWSURFACE, w, h,
			fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, 0);

	return img;
}

void*
TFB_GetVideoLine (TFB_Image* img, uint32 line)
{
	SDL_Surface* surf = img->NormalImg;
	return (uint8*)surf->pixels + surf->pitch * line;
}
