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

#include "pure.h"
#include "bbox.h"
#include "2xscalers.h"

static SDL_Surface *fade_black;
static SDL_Surface *fade_white;
static SDL_Surface *fade_temp;

static SDL_Surface *
Create_Screen (SDL_Surface *template)
{
	SDL_Surface *newsurf = SDL_DisplayFormat (template);
	if (newsurf == 0) {
		fprintf (stderr, "Couldn't create screen buffers: %s\n", SDL_GetError());
		exit(-1);
	}
	return newsurf;
}

int
TFB_Pure_InitGraphics (int driver, int flags, int width, int height, int bpp)
{
	char VideoName[256];
	int videomode_flags;
	SDL_Surface *test_extra;
	int i;

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

	for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
	{
		SDL_Screens[i] = Create_Screen (test_extra);
	}

	SDL_Screen = SDL_Screens[0];
	TransitionScreen = SDL_Screens[2];

	fade_white = Create_Screen(test_extra);
	SDL_FillRect (fade_white, NULL, SDL_MapRGB (fade_white->format, 255, 255, 255));
	fade_black = Create_Screen(test_extra);
	SDL_FillRect (fade_black, NULL, SDL_MapRGB (fade_black->format, 0, 0, 0));
	fade_temp = Create_Screen(test_extra);
	
	SDL_FreeSurface (test_extra);
	
	if (SDL_Video->format->BytesPerPixel != SDL_Screen->format->BytesPerPixel)
	{
		fprintf (stderr, "Fatal error: SDL_Video and SDL_Screen bpp doesn't match (%d vs. %d)\n",
			SDL_Video->format->BytesPerPixel,SDL_Screen->format->BytesPerPixel);
		exit(-1);
	}

	// pre-compute the RGB->YUV transformations
	Scale_PrepYUV();

	return 0;
}

static void ScanLines (SDL_Surface *dst, SDL_Rect *r)
{
	const int rx = r->x * 2;
	const int rw = r->w * 2;
	const int ry1 = r->y * 2;
	const int ry2 = ry1 + r->h * 2;
	Uint8 *pixels = (Uint8 *) dst->pixels;
	SDL_PixelFormat *fmt = dst->format;
	int x, y;

	switch (dst->format->BytesPerPixel)
	{
	case 2:
	{
		for (y = ry1; y < ry2; y += 2)
		{
			Uint16 *p = (Uint16 *) &pixels[y * dst->pitch + (rx << 1)];
			for (x = 0; x < rw; ++x)
			{
				*p = ((((*p & fmt->Rmask) * 3) >> 2) & fmt->Rmask) |
				      ((((*p & fmt->Gmask) * 3) >> 2) & fmt->Gmask) |
				      ((((*p & fmt->Bmask) * 3) >> 2) & fmt->Bmask);
				++p;
			}
		}
		break;
	}
	case 3:
	{
		for (y = ry1; y < ry2; y += 2)
		{
			Uint8 *p = (Uint8 *) &pixels[y * dst->pitch + rx * 3];
			Uint32 pixval;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			for (x = 0; x < rw; ++x)
			{
				pixval = p[0] << 16 | p[1] << 8 | p[2];
				pixval = ((((pixval & fmt->Rmask) * 3) >> 2) & fmt->Rmask) |
					((((pixval & fmt->Gmask) * 3) >> 2) & fmt->Gmask) |
					((((pixval & fmt->Bmask) * 3) >> 2) & fmt->Bmask);
				p[0] = (pixval >> 16) & 0xff;
				p[1] = (pixval >> 8) & 0xff;
				p[2] = pixval & 0xff;
				p += 3;
			}
#else
			for (x = 0; x < rw; ++x)
			{
				pixval = p[0] | p[1] << 8 | p[2] << 16;
				pixval = ((((pixval & fmt->Rmask) * 3) >> 2) & fmt->Rmask) |
					((((pixval & fmt->Gmask) * 3) >> 2) & fmt->Gmask) |
					((((pixval & fmt->Bmask) * 3) >> 2) & fmt->Bmask);
				p[0] = pixval & 0xff;
				p[1] = (pixval >> 8) & 0xff;
				p[2] = (pixval >> 16) & 0xff;
				p += 3;
			}
#endif
		}
		break;
	}
	case 4:
	{
		for (y = ry1; y < ry2; y += 2)
		{
			Uint32 *p = (Uint32 *) &pixels[y * dst->pitch + (rx << 2)];
			for (x = 0; x < rw; ++x)
			{
				*p = ((((*p & fmt->Rmask) * 3) >> 2) & fmt->Rmask) |
				      ((((*p & fmt->Gmask) * 3) >> 2) & fmt->Gmask) |
				      ((((*p & fmt->Bmask) * 3) >> 2) & fmt->Bmask);
				++p;
			}
		}
		break;
	}
	}
}

void
TFB_Pure_SwapBuffers (int force_full_redraw)
{
	static int last_fade_amount = 255, last_transition_amount = 255;
	int fade_amount = FadeAmount;
	int transition_amount = TransitionAmount;
	SDL_Rect updated;

	if (force_full_redraw ||
		fade_amount != 255 || transition_amount != 255 ||
		last_fade_amount != 255 || last_transition_amount != 255)
	{
		updated.x = updated.y = 0;
		updated.w = ScreenWidth;
		updated.h = ScreenHeight;	
	}
	else
	{
		if (!TFB_BBox.valid)
			return;
		updated.x = TFB_BBox.region.corner.x;
		updated.y = TFB_BBox.region.corner.y;
		updated.w = TFB_BBox.region.extent.width;
		updated.h = TFB_BBox.region.extent.height;
	}

	last_fade_amount = fade_amount;
	last_transition_amount = transition_amount;	
	
	if (ScreenWidth == 320 && ScreenHeight == 240 &&
		ScreenWidthActual == 640 && ScreenHeightActual == 480)
	{
		// scales 320x240 backbuffer to 640x480

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

		if (GfxFlags & TFB_GFXFLAGS_SCALE_BILINEAR)
			Scale_BilinearFilter (backbuffer, SDL_Video, &updated);
		else if (GfxFlags & TFB_GFXFLAGS_SCALE_BIADAPT)
			Scale_BiAdaptFilter (backbuffer, SDL_Video, &updated);
		else if (GfxFlags & TFB_GFXFLAGS_SCALE_BIADAPTADV)
			Scale_BiAdaptAdvFilter (backbuffer, SDL_Video, &updated);
		else
			Scale_Nearest (backbuffer, SDL_Video, &updated);

		if (GfxFlags & TFB_GFXFLAGS_SCANLINES)
			ScanLines (SDL_Video, &updated);
		
		SDL_UnlockSurface (backbuffer);
		SDL_UnlockSurface (SDL_Video);

		SDL_UpdateRect (SDL_Video, updated.x * 2, updated.y * 2, updated.w * 2, updated.h * 2);
	}
	else
	{
		// resolution is 320x240 so we can blit directly

		SDL_BlitSurface (SDL_Screen, &updated, SDL_Video, &updated);

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

		SDL_UpdateRect (SDL_Video, updated.x, updated.y, updated.w, updated.h);
	}
}

#endif
