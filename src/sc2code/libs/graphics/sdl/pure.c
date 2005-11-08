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
#include "scalers.h"

static SDL_Surface *fade_black = NULL;
static SDL_Surface *fade_white = NULL;
static SDL_Surface *fade_temp = NULL;
static SDL_Surface *scaled_display = NULL;

static TFB_ScaleFunc scaler = NULL;

static SDL_Surface *
Create_Screen (SDL_Surface *template, int w, int h)
{
	SDL_Surface *newsurf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h,
			template->format->BitsPerPixel,
			template->format->Rmask, template->format->Gmask,
			template->format->Bmask, 0);
	if (newsurf == 0) {
		fprintf (stderr, "Couldn't create screen buffers: %s\n", SDL_GetError());
	}
	return newsurf;
}

static int
ReInit_Screen (SDL_Surface **screen, SDL_Surface *template, int w, int h)
{
	if (*screen)
		SDL_FreeSurface (*screen);
	*screen = Create_Screen (template, w, h);
	
	return *screen == 0 ? -1 : 0;
}

int
TFB_Pure_ConfigureVideo (int driver, int flags, int width, int height, int bpp)
{
	int i, videomode_flags;
	SDL_Surface *temp_surf;
	
	GraphicsDriver = driver;
	ScreenColorDepth = bpp;

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
		return -1;
	}
	else
	{
		fprintf (stderr, "Set the resolution to: %ix%ix%i\n",
			SDL_GetVideoSurface()->w, SDL_GetVideoSurface()->h,
			SDL_GetVideoSurface()->format->BitsPerPixel);
	}

	// Create a 32bpp surface in a compatible format which will supply
	// the format information to all other surfaces used in the game
	if (format_conv_surf)
	{
		SDL_FreeSurface (format_conv_surf);
		format_conv_surf = NULL;
	}
	temp_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, 0, 0, 32,
			0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000);
	if (temp_surf)
	{	// acquire a fast compatible format from SDL
		format_conv_surf = SDL_DisplayFormatAlpha (temp_surf);
		if (!format_conv_surf || format_conv_surf->format->BitsPerPixel != 32)
		{	// absolute fallback case
			format_conv_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, 0, 0, 32,
					0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
		}
		SDL_FreeSurface (temp_surf);
	}
	if (!format_conv_surf)
	{
		fprintf (stderr, "Couldn't create format_conv_surf: %s\n", SDL_GetError());
		return -1;
	}
	
	
	for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
	{
		if (0 != ReInit_Screen (&SDL_Screens[i], format_conv_surf,
				ScreenWidth, ScreenHeight))
			return -1;
	}

	SDL_Screen = SDL_Screens[0];
	TransitionScreen = SDL_Screens[2];

	if (0 != ReInit_Screen (&fade_white, format_conv_surf,
			ScreenWidth, ScreenHeight))
		return -1;
	SDL_FillRect (fade_white, NULL, SDL_MapRGB (fade_white->format, 255, 255, 255));
	
	if (0 != ReInit_Screen (&fade_black, format_conv_surf,
			ScreenWidth, ScreenHeight))
		return -1;
	SDL_FillRect (fade_black, NULL, SDL_MapRGB (fade_black->format, 0, 0, 0));
	
	if (0 != ReInit_Screen (&fade_temp, format_conv_surf,
			ScreenWidth, ScreenHeight))
		return -1;

	if (ScreenWidthActual > ScreenWidth || ScreenHeightActual > ScreenHeight)
	{
		if (0 != ReInit_Screen (&scaled_display, format_conv_surf,
				ScreenWidthActual, ScreenHeightActual))
			return -1;

		scaler = Scale_PrepPlatform (flags, SDL_Screen->format);
	}
	else
	{	// no need to scale
		scaler = NULL;
	}

	return 0;
}

int
TFB_Pure_InitGraphics (int driver, int flags, int width, int height, int bpp)
{
	char VideoName[256];

	fprintf (stderr, "Initializing Pure-SDL graphics.\n");

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

	if (TFB_Pure_ConfigureVideo (driver, flags, width, height, bpp))
	{
		fprintf (stderr, "Could not initialize video: no fallback at start of program!\n");
		exit (-1);
	}

	// Initialize scalers (let them precompute whatever)
	Scale_Init ();

	return 0;
}

static void
ScanLines (SDL_Surface *dst, SDL_Rect *r)
{
	const int rw = r->w * 2;
	const int rh = r->h * 2;
	SDL_PixelFormat *fmt = dst->format;
	const int pitch = dst->pitch;
	const int len = pitch / fmt->BytesPerPixel;
	int ddst;
	Uint32 *p = (Uint32 *) dst->pixels;
	int x, y;

	p += len * (r->y * 2) + (r->x * 2);
	ddst = len + len - rw;

	for (y = rh; y; y -= 2, p += ddst)
	{
		for (x = rw; x; --x, ++p)
		{
			// we ignore the lower bits as the difference
			// of 1 in 255 is negligible
			*p = ((*p >> 1) & 0x7f7f7f7f) + ((*p >> 2) & 0x3f3f3f3f);
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
	
	if (ScreenWidthActual == ScreenWidth * 2 &&
			ScreenHeightActual == ScreenHeight * 2)
	{
		// scales 320x240 backbuffer to 640x480

		SDL_Surface *backbuffer = SDL_Screen;
		SDL_Surface *scalebuffer = scaled_display;

		// we can scale directly onto SDL_Video if video is compatible
		if (SDL_Video->format->BitsPerPixel == SDL_Screen->format->BitsPerPixel)
			scalebuffer = SDL_Video;
		
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

		SDL_LockSurface (scalebuffer);
		SDL_LockSurface (backbuffer);

		if (scaler)
			scaler (backbuffer, scalebuffer, &updated);

		if (GfxFlags & TFB_GFXFLAGS_SCANLINES)
			ScanLines (scalebuffer, &updated);
		
		SDL_UnlockSurface (backbuffer);
		SDL_UnlockSurface (scalebuffer);

		updated.x *= 2;
		updated.y *= 2;
		updated.w *= 2;
		updated.h *= 2;
		if (scalebuffer != SDL_Video)
			SDL_BlitSurface (scalebuffer, &updated, SDL_Video, &updated);

		SDL_UpdateRects (SDL_Video, 1, &updated);
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

void
Scale_PerfTest (void)
{
	DWORD TimeStart, TimeIn, Now;
	SDL_Rect updated = {0, 0, ScreenWidth, ScreenHeight};
	int i;

	if (!scaler)
	{
		fprintf (stderr, "No scaler configured! Run with larger resolution, please\n");
		return;
	}
	if (!scaled_display)
	{
		fprintf (stderr, "Run scaler performance tests in Pure mode, please\n");
		return;
	}

	SDL_LockSurface (SDL_Screen);
	SDL_LockSurface (scaled_display);

	TimeStart = TimeIn = SDL_GetTicks ();

	for (i = 1; i < 1001; ++i) // run for 1000 frames
	{
		scaler (SDL_Screen, scaled_display, &updated);
		
		if (GfxFlags & TFB_GFXFLAGS_SCANLINES)
			ScanLines (scaled_display, &updated);

		if (i % 100 == 0)
		{
			Now = SDL_GetTicks ();
			fprintf(stderr, "%03ld(%04ld) ", 100*1000 / (Now - TimeIn),
					Now - TimeIn);
			TimeIn = Now;
		}
	}

	fprintf (stderr, "Full frames scaled: %d; over %ld ms; %ld fps\n",
			(i - 1), Now - TimeStart, i * 1000 / (Now - TimeStart));

	SDL_UnlockSurface (scaled_display);
	SDL_UnlockSurface (SDL_Screen);
}

#endif
