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
#include "pure.h"
#include "opengl.h"
#include "primitives.h"
#include "graphics/drawcmd.h"
#include "graphics/tfb_draw.h"

int batch_depth = 0;


//Status: Not entirely unimplemented!
BOOLEAN
InitGraphics (int argc, char* argv[], COUNT KbytesRequired)
		// Kbytes should only matter if we wanted to port Starcon2 to the
		// hand-helds...
{
	BOOLEAN ret;

	LoadDisplay (&_pCurDisplay);
	ActivateDisplay ();

	(void) argc;             /* lint */
	(void) argv;             /* lint */
	(void) KbytesRequired;   /* lint */

	ret = TRUE;
	return (ret);
}

// Status: Unimplemented
void
UninitGraphics ()  // Also probably empty
{
	// HFree (DrawCommandQueue);  This is static now!

	//mem_uninit ();
}

/* Batching and Unbatching functions.  A "Batch" is a collection of
   DrawCommands that will never be flipped to the screen half-rendered.
   BatchGraphics and UnbatchGraphics function vaguely like a non-blocking
   recursive lock to do this respect. */
void
BatchGraphics (void)
{
	TFB_BatchGraphics ();
}

void
UnbatchGraphics (void)
{
	TFB_UnbatchGraphics ();
}

/* Sleeps this thread until all Draw Commands queued by that thread have
   been processed. */

void
FlushGraphics ()
{
	TFB_DrawScreen_WaitForSignal ();
}

void
SetTransitionSource (PRECT pRect)
{
	TFB_DrawScreen_Copy(pRect, TFB_SCREEN_MAIN, TFB_SCREEN_TRANSITION);
}

// Status: Implemented
void
ScreenTransition (int TransType, PRECT pRect)
{
	const float DURATION = (31.0f / 60.0f); // in seconds
	Uint32 last_time = 0, current_time, delta_time, add_amount;
	(void) TransType;  /* dodge compiler warning */

	if (pRect)
	{
		TransitionClipRect.x = pRect->corner.x;
		TransitionClipRect.y = pRect->corner.y;
		TransitionClipRect.w = pRect->extent.width;
		TransitionClipRect.h = pRect->extent.height;
	}
	else
	{
		TransitionClipRect.x = TransitionClipRect.y = 0;
		TransitionClipRect.w = ScreenWidth;
		TransitionClipRect.h = ScreenHeight;
	}

#ifdef HAVE_OPENGL
	if (GraphicsDriver == TFB_GFXDRIVER_SDL_OPENGL)
	{
		TFB_GL_UploadTransitionScreen ();
	}
#endif
	
	TransitionAmount = 0;
	FlushGraphics ();
	last_time = SDL_GetTicks ();	
	for (;;)
	{
		SDL_Delay (10);

		current_time = SDL_GetTicks ();
		delta_time = current_time - last_time;
		last_time = current_time;
		
		add_amount = (Uint32) ((delta_time / 1000.0f / DURATION) * 255);
		if (TransitionAmount + add_amount >= 255)
			break;

		TransitionAmount += add_amount;
	}
	TransitionAmount = 255;
}

// Status: Ignored
BOOLEAN
InitVideo (BOOLEAN useCDROM) //useCDROM doesn't really apply to us...
{
	BOOLEAN ret;

	(void)useCDROM;  /* dodge compiler warning */

// fprintf (stderr, "Unimplemented function activated: InitVideo()\n");
	ret = TRUE;
	return (ret);
}

// Status: Ignored
void
UninitVideo ()  //empty
{
	// fprintf (stderr, "Unimplemented function activated: UninitVideo()\n");
}

//Status: Unimplemented
void
VidStop() // At least this one makes sense.
{
		// fprintf (stderr, "Unimplemented function activated: VidStop()\n");
}

//Status: Unimplemented
MEM_HANDLE
VidPlaying()  // Easy enough.
{
		MEM_HANDLE ret;

		//fprintf (stderr, "Unimplemented function activated: VidPlaying()\n");
		ret = 0;
		return (ret);
}

//Status: Unimplemented
VIDEO_TYPE
VidPlay (VIDEO_REF VidRef, char *loopname, BOOLEAN uninit)
		// uninit? when done, I guess.
{
	VIDEO_TYPE ret;
	ret = 0;

	/* dodge compiler warnings */
	(void) VidRef;
	(void) loopname;
	(void) uninit;
// fprintf (stderr, "Unimplemented function activated: VidPlay()\n");
	return (ret);
}

VIDEO_REF
_init_video_file(PVOID pStr)
{
	VIDEO_REF ret;

	fprintf (stderr, "Unimplemented function activated: _init_video_file()\n");

        /* dodge compiler warning */
	(void) pStr;
	ret = 0;
	return (ret);
}

BOOLEAN
_image_intersect (PIMAGE_BOX box1, PIMAGE_BOX box2, PRECT rect)
{
	BOOLEAN ret;
	SDL_Surface *img1, *img2;
	int img1w, img1xpos, img1ypos, img2w, img2xpos, img2ypos;
	int x,y;
	Uint32 img1colourkey, img2colourkey;
	GetPixelFn getpixel1, getpixel2;

	img1 = (SDL_Surface *)box1->FramePtr->image->NormalImg;
	img2 = (SDL_Surface *)box2->FramePtr->image->NormalImg;
	
	getpixel1 = getpixel_for(img1);
	getpixel2 = getpixel_for(img2);

	img1w = img1->w;
	img1xpos = box1->Box.corner.x;
	img1ypos = box1->Box.corner.y;
	img1colourkey = img1->format->colorkey;

	img2w = img2->w;
	img2xpos = box2->Box.corner.x;
	img2ypos = box2->Box.corner.y;
	img2colourkey = img2->format->colorkey;

	for (y = rect->corner.y; y < rect->corner.y + rect->extent.height; ++y)
	{
		for (x = rect->corner.x; x < rect->corner.x + rect->extent.width; ++x)
		{
			if ((getpixel1(img1, x - img1xpos, y - img1ypos) != img1colourkey) &&
					(getpixel2(img2, x - img2xpos, y - img2ypos) != img2colourkey))
			{
				return TRUE;
			}
		}
	}
	ret = FALSE;

	return (ret);
}


#endif
