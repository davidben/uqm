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

#include "libs/graphics/sdl/pure.h"


int
TFB_Pure_InitGraphics (int driver, int flags, int width, int height, int bpp)
{
	char VideoName[256];
	int videomode_flags;
	SDL_Surface *test_extra;

	GraphicsDriver = driver;

	printf("Initializing SDL (pure).\n");

	if ((SDL_Init (SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) == -1))
	{
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		exit(-1);
	}

	SDL_VideoDriverName (VideoName, sizeof (VideoName));
	printf("SDL driver used: %s\n", VideoName);
			// Set the environment variable SDL_VIDEODRIVER to override
			// For Linux: x11 (default), dga, fbcon, directfb, svgalib,
			//            ggi, aalib
			// For Windows: directx (default), windib
	atexit (SDL_Quit);

	printf ("SDL initialized.\n");

	printf ("Initializing Screen.\n");

	ScreenWidth = 320;
	ScreenHeight = 240;
	ScreenWidthActual = width;
	ScreenHeightActual = height;

	videomode_flags = SDL_HWSURFACE | SDL_ANYFORMAT;
	if (flags & TFB_GFXFLAGS_FULLSCREEN)
		videomode_flags |= SDL_FULLSCREEN;

	SDL_Video = SDL_SetVideoMode (ScreenWidthActual, ScreenHeightActual, 
		bpp, videomode_flags);

	if (SDL_Video == NULL)
	{
		printf ("Couldn't set %ix%ix%i video mode: %s\n",
			ScreenWidthActual, ScreenHeightActual, bpp,
			SDL_GetError ());
		exit(-1);
	}
	else
	{
		printf ("Set the resolution to: %ix%ix%i\n",
			SDL_GetVideoSurface()->w, SDL_GetVideoSurface()->h,
			SDL_GetVideoSurface()->format->BitsPerPixel);

	}

	test_extra = SDL_CreateRGBSurface(SDL_SWSURFACE, ScreenWidthActual, ScreenHeightActual, 32,
		0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000);

	if (test_extra == NULL)
	{
		printf("Couldn't create back buffer: %s\n",
			SDL_GetError());
		exit(-1);
	}

	SDL_Screen = SDL_DisplayFormat(test_extra);
	SDL_FreeSurface(test_extra);

	test_extra = SDL_CreateRGBSurface(SDL_SWSURFACE, ScreenWidthActual, ScreenHeightActual, 32,
		0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000);

	if (test_extra == NULL)
	{
		printf("Couldn't create workspace buffer: %s\n",
			SDL_GetError());
		exit(-1);
	}

	ExtraScreen = SDL_DisplayFormat(test_extra);
	SDL_FreeSurface(test_extra);

	return 0;
}

void
TFB_Pure_SwapBuffers ()
{
	SDL_BlitSurface(SDL_Screen, NULL, SDL_Video, NULL);
	SDL_UpdateRect(SDL_Video, 0, 0, 0, 0);
}

#endif
