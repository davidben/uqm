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

static SDL_Surface *fade_black;
static SDL_Surface *fade_white;
static SDL_Surface *fade_temp;


int
TFB_Pure_InitGraphics (int driver, int flags, int width, int height, int bpp)
{
	char VideoName[256];
	int videomode_flags;
	SDL_Surface *test_extra;

	GraphicsDriver = driver;

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
	atexit (SDL_Quit);

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

	SDL_Screen = SDL_DisplayFormat (test_extra);
	ExtraScreen = SDL_DisplayFormat (test_extra);
	TransitionScreen = SDL_DisplayFormat (test_extra);

	fade_white = SDL_DisplayFormat (test_extra);
	SDL_FillRect (fade_white, NULL, SDL_MapRGB (fade_white->format, 255, 255, 255));
	fade_black = SDL_DisplayFormat (test_extra);
	SDL_FillRect (fade_black, NULL, SDL_MapRGB (fade_black->format, 0, 0, 0));
	fade_temp = SDL_DisplayFormat (test_extra);

	SDL_FreeSurface (test_extra);



	if (SDL_Video->format->BytesPerPixel != SDL_Screen->format->BytesPerPixel)
	{
		fprintf (stderr, "Fatal error: SDL_Video and SDL_Screen bpp doesn't match (%d vs. %d)\n",
			SDL_Video->format->BytesPerPixel,SDL_Screen->format->BytesPerPixel);
		exit(-1);
	}

	return 0;
}

void
TFB_Pure_SwapBuffers ()
{
	int fade_amount = FadeAmount;
	int transition_amount = TransitionAmount;

	if (ScreenWidth == 320 && ScreenHeight == 240 &&
		ScreenWidthActual == 640 && ScreenHeightActual == 480)
	{
		// scales 320x240 backbuffer to 640x480

		int x, y;
		Uint8 *src_p, *src_p2, *dst_p;
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

		src_p = backbuffer->pixels;
		dst_p = SDL_Video->pixels;

		switch (backbuffer->format->BytesPerPixel)
		{
		case 1:
		{
			Uint8 pixval_8;
			for (y = 0; y < 240; ++y)
			{
				src_p2 = dst_p;
				for (x = 0; x < 320; ++x)
				{
					pixval_8 = *src_p++;

					*dst_p++ = pixval_8;
					*dst_p++ = pixval_8;
				}
				for (x = 0; x < 160; ++x)
				{
					*(Uint32*)dst_p = *(Uint32*)src_p2;
					dst_p += 4;
					src_p2 += 4;
				}
			}
			break;
		}
		case 2:
		{
			Uint16 pixval_16;
			for (y = 0; y < 240; ++y)
			{
				src_p2 = dst_p;
				for (x = 0; x < 320; ++x)
				{
					pixval_16 = *(Uint16*)src_p;
					src_p += 2;

					*(Uint16*)dst_p = pixval_16;
					dst_p += 2;
					*(Uint16*)dst_p = pixval_16;
					dst_p += 2;
				}
				for (x = 0; x < 320; ++x)
				{
					*(Uint32*)dst_p = *(Uint32*)src_p2;
					dst_p += 4;
					src_p2 += 4;
				}
			}
			break;
		}
		case 3:
		{
			Uint32 pixval_32;
			for (y = 0; y < 240; ++y)
			{
				src_p2 = src_p;			
				for (x = 0; x < 320; ++x)
				{
					pixval_32 = *(Uint32*)src_p;
					src_p += 3;

					*(Uint32*)dst_p = pixval_32;
					dst_p += 3;
					*(Uint32*)dst_p = pixval_32;
					dst_p += 3;
				}
				src_p = src_p2;
				for (x = 0; x < 320; ++x)
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
			Uint32 pixval_32;
			for (y = 0; y < 240; ++y)
			{
				src_p2 = src_p;
				for (x = 0; x < 320; ++x)
				{
					pixval_32 = *(Uint32*)src_p;
					src_p += 4;

					*(Uint32*)dst_p = pixval_32;
					dst_p += 4;
					*(Uint32*)dst_p = pixval_32;
					dst_p += 4;
				}				
				src_p = src_p2;
				for (x = 0; x < 320; ++x)

				{
					pixval_32 = *(Uint32*)src_p;
					src_p += 4;

					*(Uint32*)dst_p = pixval_32;
					dst_p += 4;
					*(Uint32*)dst_p = pixval_32;
					dst_p += 4;
				}
			}
			break;
		}
		}

		SDL_UnlockSurface (backbuffer);
		SDL_UnlockSurface (SDL_Video);
	}
	else
	{
		// resolution is 320x240 so we can blit directly

		SDL_BlitSurface (SDL_Screen, NULL, SDL_Video, NULL);

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
