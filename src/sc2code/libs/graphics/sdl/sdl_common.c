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
#include "opengl.h"
#include "pure.h"
#include "rotozoom.h"
#include "primitives.h"
#include "dcqueue.h"
#include "options.h"
#include "SDL_thread.h"

SDL_Surface *SDL_Video;
SDL_Surface *SDL_Screen;
SDL_Surface *ExtraScreen;
SDL_Surface *TransitionScreen;


volatile int TransitionAmount = 255;
SDL_Rect TransitionClipRect;

int GfxFlags = 0;
volatile int continuity_break;


int
TFB_InitGraphics (int driver, int flags, int width, int height, int bpp)
{
	int result;
	char caption[200];

	if (bpp != 15 && bpp != 16 && bpp != 24 && bpp != 32)
	{
		fprintf(stderr, "Fatal error: bpp of %d not supported\n", bpp);
		exit(-1);
	}

	atexit (TFB_UninitGraphics);

	GfxFlags = flags;

	if (driver == TFB_GFXDRIVER_SDL_OPENGL)
	{
#ifdef HAVE_OPENGL
		result = TFB_GL_InitGraphics (driver, flags, width, height, bpp);
#else
		driver = TFB_GFXDRIVER_SDL_PURE;
		fprintf (stderr, "OpenGL support not compiled in, so using pure sdl driver\n");
		result = TFB_Pure_InitGraphics (driver, flags, width, height, bpp);
#endif
	}
	else
	{
		result = TFB_Pure_InitGraphics (driver, flags, width, height, bpp);
	}

	SDL_EnableUNICODE (1);
	sprintf (caption, "The Ur-Quan Masters v%d.%d", UQM_MAJOR_VERSION, UQM_MINOR_VERSION);
	SDL_WM_SetCaption (caption, NULL);

	//if (flags & TFB_GFXFLAGS_FULLSCREEN)
	//	SDL_ShowCursor (SDL_DISABLE);

	return 0;
}

void
TFB_UninitGraphics (void)
{
	SDL_Quit ();
}

void
TFB_ProcessEvents ()
{
	SDL_Event Event;

	while (SDL_PollEvent (&Event))
	{
		switch (Event.type) {
			case SDL_ACTIVEEVENT:    /* Loose/gain visibility */
				// TODO
				break;
			case SDL_KEYDOWN:        /* Key pressed */
			case SDL_KEYUP:          /* Key released */
				ProcessKeyboardEvent (&Event);
				break;
			case SDL_JOYAXISMOTION:  /* Joystick axis motion */
			case SDL_JOYBUTTONDOWN:  /* Joystick button pressed */
			case SDL_JOYBUTTONUP:    /* Joystick button released */
				ProcessJoystickEvent (&Event);
				break;
			case SDL_QUIT:
				exit (0);
				break;
			case SDL_VIDEORESIZE:    /* User resized video mode */
				// TODO
				break;
			case SDL_VIDEOEXPOSE:    /* Screen needs to be redrawn */
				TFB_SwapBuffers ();
				break;
			default:
				break;
		}

		//Still testing this... Should fix sticky input soon...
		/*
		{
			Uint8* Temp;

			SDL_PumpEvents ();
			Temp = SDL_GetKeyState (NULL);
			for(a = 0; a < 512; a++)
			{
				KeyboardDown[a] = Temp[a];
			}
		}
		*/
	}
}

TFB_Image* 
TFB_LoadImage (SDL_Surface *img)
{
	TFB_Image *myImage;

	myImage = (TFB_Image*) HMalloc (sizeof (TFB_Image));
	myImage->mutex = CreateMutex ();
	myImage->ScaledImg = NULL;
	myImage->colormap_index = -1;
	
	if (img->format->palette)
	{
		int i;		
		myImage->Palette = (SDL_Color*) HMalloc (sizeof (SDL_Color) * 256);
		for (i = 0; i < 256; ++i)
		{
			myImage->Palette[i].r = img->format->palette->colors[i].r;
			myImage->Palette[i].g = img->format->palette->colors[i].g;
			myImage->Palette[i].b = img->format->palette->colors[i].b;
		}
		myImage->NormalImg = img;
	}
	else
	{
		myImage->Palette = NULL;
		myImage->NormalImg = TFB_DisplayFormatAlpha (img);
		if (myImage->NormalImg)
		{
			SDL_FreeSurface (img);
		}
		else
		{
			fprintf (stderr, "TFB_LoadImage(): TFB_DisplayFormatAlpha failed\n");
			myImage->NormalImg = img;
		}
	}

	return(myImage);
}

void
TFB_FreeImage (TFB_Image *img)
{
	if (img)
	{
		TFB_DrawCommand DC;

		DC.Type = TFB_DRAWCOMMANDTYPE_DELETEIMAGE;
		DC.image = (TFB_ImageStruct*) img;

		TFB_EnqueueDrawCommand (&DC);
	}
}

void
TFB_SwapBuffers ()
{
#ifdef HAVE_OPENGL
	if (GraphicsDriver == TFB_GFXDRIVER_SDL_OPENGL)
		TFB_GL_SwapBuffers ();
	else
		TFB_Pure_SwapBuffers ();
#else
	TFB_Pure_SwapBuffers ();
#endif
}

SDL_Surface* 
TFB_DisplayFormatAlpha (SDL_Surface *surface)
{
#ifdef HAVE_OPENGL
	if (GraphicsDriver == TFB_GFXDRIVER_SDL_OPENGL)
	{
		return TFB_GL_DisplayFormatAlpha (surface);
	}
#endif

	return SDL_DisplayFormatAlpha (surface);
}

void TFB_BlitSurface (SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst,
					  SDL_Rect *dstrect, int blend_numer, int blend_denom)
{
	BOOLEAN has_colorkey;
	int x, y, x1, y1, x2, y2, dst_x2, dst_y2, colorkey, nr, ng, nb;
	int srcx, srcy, w, h;
	Uint8 sr, sg, sb, dr, dg, db;
	Uint32 src_pixval, dst_pixval;
	GetPixelFn src_getpix, dst_getpix;
	PutPixelFn putpix;
	SDL_Rect fulldst;

	if (blend_numer == blend_denom)
	{
		// normal blit: dst = src
		
		// fprintf(stderr, "normal blit\n");
		SDL_BlitSurface (src, srcrect, dst, dstrect);
		return;
	}
		
	// NOTE: following clipping code is copied from SDL-1.2.4 sources

	// If the destination rectangle is NULL, use the entire dest surface
	if (dstrect == NULL)
	{
		fulldst.x = fulldst.y = 0;
		dstrect = &fulldst;
	}

	// clip the source rectangle to the source surface
	if (srcrect) 
	{
		int maxw, maxh;

		srcx = srcrect->x;
		w = srcrect->w;
		if(srcx < 0) 
		{
			w += srcx;
			dstrect->x -= srcx;
			srcx = 0;
		}
		maxw = src->w - srcx;
		if(maxw < w)
			w = maxw;

		srcy = srcrect->y;
		h = srcrect->h;
		if(srcy < 0) 
		{
			h += srcy;
			dstrect->y -= srcy;
			srcy = 0;
		}
		maxh = src->h - srcy;
		if(maxh < h)
			h = maxh;

	} 
	else 
	{
		srcx = srcy = 0;
		w = src->w;
		h = src->h;
	}

	// clip the destination rectangle against the clip rectangle
	{
		SDL_Rect *clip = &dst->clip_rect;
		int dx, dy;

		dx = clip->x - dstrect->x;
		if(dx > 0) 
		{
			w -= dx;
			dstrect->x += dx;
			srcx += dx;
		}
		dx = dstrect->x + w - clip->x - clip->w;
		if(dx > 0)
			w -= dx;

		dy = clip->y - dstrect->y;
		if(dy > 0) 
		{
			h -= dy;
			dstrect->y += dy;
			srcy += dy;
		}
		dy = dstrect->y + h - clip->y - clip->h;
		if(dy > 0)
			h -= dy;
	}

	dstrect->w = w;
	dstrect->h = h;

	if (w <= 0 || h <= 0)
		return;

	x1 = srcx;
	y1 = srcy;
	x2 = srcx + w;
	y2 = srcy + h;

	if (src->flags & SDL_SRCCOLORKEY)
	{
		has_colorkey = TRUE;
		colorkey = src->format->colorkey;
	}
	else
	{
		has_colorkey = FALSE;
	}

	src_getpix = getpixel_for (src);
	dst_getpix = getpixel_for (dst);
	putpix = putpixel_for (dst);

	if (SDL_MUSTLOCK(src))
	{
		if (SDL_LockSurface (src) == -1)
		{
			printf("Couldn't lock src!\n");
		}
		else
		{
			printf("Locked src.\n");
		}
	}
	if (SDL_MUSTLOCK(dst))
	{
		if (SDL_LockSurface (dst) == -1)
		{
			printf("Couldn't lock dst!\n");
		}
		else
		{
			printf("Locked dst.\n");
		}
	}

	if (blend_denom < 0)
	{
		// additive blit: dst = src + dst

		// fprintf(stderr, "additive blit %d %d, src %d %d %d %d dst %d %d, srcbpp %d\n",blend_numer, blend_denom, x1, y1, x2, y2, dstrect->x, dstrect->y, src->format->BitsPerPixel);
		
		for (y = y1; y < y2; ++y)
		{
			dst_y2 = dstrect->y + (y - y1);
			for (x = x1; x < x2; ++x)
			{
				dst_x2 = dstrect->x + (x - x1);
				src_pixval = src_getpix (src, x, y);

				if (has_colorkey && src_pixval == colorkey)
					continue;

				dst_pixval = dst_getpix (dst, dst_x2, dst_y2);
				
				SDL_GetRGB (src_pixval, src->format, &sr, &sg, &sb);
				SDL_GetRGB (dst_pixval, dst->format, &dr, &dg, &db);

				nr = sr + dr;
				ng = sg + dg;
				nb = sb + db;

				if (nr > 255)
					nr = 255;
				if (ng > 255)
					ng = 255;
				if (nb > 255)
					nb = 255;

				putpix (dst, dst_x2, dst_y2, SDL_MapRGB (dst->format, nr, ng, nb));
			}
		}
	}
	else if (blend_numer < 0)
	{
		// subtractive blit: dst = src - dst

		// fprintf(stderr, "subtractive blit %d %d, src %d %d %d %d dst %d %d, srcbpp %d\n",blend_numer, blend_denom, x1, y1, x2, y2, dstrect->x, dstrect->y, src->format->BitsPerPixel);
		
		for (y = y1; y < y2; ++y)
		{
			dst_y2 = dstrect->y + (y - y1);
			for (x = x1; x < x2; ++x)
			{
				dst_x2 = dstrect->x + (x - x1);
				src_pixval = src_getpix (src, x, y);

				if (has_colorkey && src_pixval == colorkey)
					continue;

				dst_pixval = dst_getpix (dst, dst_x2, dst_y2);

				SDL_GetRGB (src_pixval, src->format, &sr, &sg, &sb);
				SDL_GetRGB (dst_pixval, dst->format, &dr, &dg, &db);

				nr = sr - dr;
				ng = sg - dg;
				nb = sb - db;

				if (nr < 0)
					nr = 0;
				if (ng < 0)
					ng = 0;
				if (nb < 0)
					nb = 0;

				putpix (dst, dst_x2, dst_y2, SDL_MapRGB (dst->format, nr, ng, nb));
			}
		}
	}
	else 
	{
		// modulated blit: dst = src * (blend_numer / blend_denom) 

		float f = blend_numer / (float)blend_denom;

		// fprintf(stderr, "modulated blit %d %d, f %f, src %d %d %d %d dst %d %d, srcbpp %d\n",blend_numer, blend_denom, f, x1, y1, x2, y2, dstrect->x, dstrect->y, src->format->BitsPerPixel);
		
		for (y = y1; y < y2; ++y)
		{
			dst_y2 = dstrect->y + (y - y1);
			for (x = x1; x < x2; ++x)
			{
				dst_x2 = dstrect->x + (x - x1);
				src_pixval = src_getpix (src, x, y);

				if (has_colorkey && src_pixval == colorkey)
					continue;
				
				SDL_GetRGB (src_pixval, src->format, &sr, &sg, &sb);

				nr = (int)(sr * f);
				ng = (int)(sg * f);
				nb = (int)(sb * f);

				if (nr > 255)
					nr = 255;
				if (ng > 255)
					ng = 255;
				if (nb > 255)
					nb = 255;

				putpix (dst, dst_x2, dst_y2, SDL_MapRGB (dst->format, nr, ng, nb));
			}
		}
	}
	if (SDL_MUSTLOCK(dst))
	{
		SDL_UnlockSurface (dst);
	}
	if (SDL_MUSTLOCK(src))
	{
		SDL_UnlockSurface (src);
	}
}

void
TFB_ComputeFPS ()
{
	static Uint32 last_time = 0, fps_counter = 0;
	Uint32 current_time, delta_time;

	current_time = SDL_GetTicks ();
	delta_time = current_time - last_time;
	last_time = current_time;
	
	fps_counter += delta_time;
	if (fps_counter > 1000)
	{
		fps_counter = 0;
		fprintf (stderr, "fps %.2f\n",1.0 / (delta_time / 1000.0));
	}
}

// Livelock deterrance constants.  Because the entire screen is rarely
// refreshed, we may not drop draw commands on the floor with abandon.
// Furthermore, if the main program is queuing commands at a speed
// comparable to our processing of the commands, we never finish and
// the game freezes.  Thus, if the queue starts out larger than
// DCQ_FORCE_SLOWDOWN_SIZE, or DCQ_LIVELOCK_MAX commands find
// themselves being processed in one go, livelock deterrence is
// enabled, and TFB_FlushGraphics locks the DCQ until it has processed
// all entries.  If batched but pending commands exceed DCQ_FORCE_BREAK_SIZE,
// a continuity break is performed.  This will effectively slow down the 
// game logic, a fate we seek to avoid - however, it seems to be unavoidable
// on slower machines.  Even there, it's seems nonexistent outside of
// communications screens.  --Michael

#define DCQ_FORCE_SLOWDOWN_SIZE 1024
#define DCQ_FORCE_BREAK_SIZE 4096
#define DCQ_LIVELOCK_MAX 2048

void
TFB_FlushGraphics () // Only call from main thread!!
{
	int semval;
	int commands_handled;
	BOOLEAN livelock_deterrence;
	BOOLEAN done;

	// This is technically a locking violation on DrawCommandQueue->Size,
	// but it is likely to not be very destructive.
	if (DrawCommandQueue == 0 || DrawCommandQueue->Size == 0)
	{
		static int last_fade = 255;
		static int last_transition = 255;
		int current_fade = FadeAmount;
		int current_transition = TransitionAmount;
		
		if ((current_fade != 255 && current_fade != last_fade) ||
			(current_transition != 255 && current_transition != last_transition))
		{
			TFB_SwapBuffers(); // if fading, redraw every frame
		}
		else
		{
			SDL_Delay(1);
		}
		
		last_fade = current_fade;
		last_transition = current_transition;
		BroadcastCondVar (RenderingCond);
		return;
	}

	if (!continuity_break) {
		// TODO: a more optimal way of getting the lock on GraphicsSem..
		//       cannot currently use SDL_SemWait because it would break
		//       the usage of continuity_break
		// Michael asks: Why are we locking, then releasing, the GraphicsSem?

		semval = TimeoutSetSemaphore (GraphicsSem, ONE_SECOND / 10);
		if (semval != 0)
		{
			BroadcastCondVar (RenderingCond);
			return;
		}
		else
			SDL_SemPost (GraphicsSem);
	}
	continuity_break = 0;

	if (GfxFlags & TFB_GFXFLAGS_SHOWFPS)
		TFB_ComputeFPS ();

	commands_handled = 0;
	livelock_deterrence = FALSE;

	if (DrawCommandQueue->FullSize > DCQ_FORCE_BREAK_SIZE)
	{
		TFB_BatchReset ();
	}

	if (DrawCommandQueue->Size > DCQ_FORCE_SLOWDOWN_SIZE)
	{
		Lock_DCQ ();
		livelock_deterrence = TRUE;
	}

	done = FALSE;
	while (!done)
	{
		TFB_DrawCommand DC;
		TFB_Image *DC_image;

		if (!TFB_DrawCommandQueue_Pop (DrawCommandQueue, &DC))
		{
			// the Queue is now empty.
			break;
		}

		++commands_handled;
		if (!livelock_deterrence && commands_handled + DrawCommandQueue->Size > DCQ_LIVELOCK_MAX)
		{
			// fprintf (stderr, "Initiating livelock deterrence!\n");
			livelock_deterrence = TRUE;
			
			Lock_DCQ ();
		}

		DC_image = (TFB_Image*) DC.image;
		if (DC_image)
			LockMutex (DC_image->mutex);
		
		switch (DC.Type)
		{
		case TFB_DRAWCOMMANDTYPE_IMAGE:
			{
				SDL_Rect targetRect;
				SDL_Surface *surf;


				if (DC_image == 0)
				{
					fprintf (stderr, "TFB_FlushGraphics(): error, DC_image == 0\n");
					break;
				}

				targetRect.x = DC.x;
				targetRect.y = DC.y;

				if ((DC.w != DC_image->NormalImg->w || DC.h != DC_image->NormalImg->h) && 
					DC_image->ScaledImg)
					surf = DC_image->ScaledImg;
				else
					surf = DC_image->NormalImg;

				if (surf->format->palette)
				{
					if (DC.UsePalette)
					{
						SDL_SetColors (surf, (SDL_Color*)DC.Palette, 0, 256);
					}
					else
					{
						SDL_SetColors (surf, DC_image->Palette, 0, 256);
					}
				}

				TFB_BlitSurface(surf, NULL, SDL_Screen, &targetRect, DC.BlendNumerator, DC.BlendDenominator);

				break;
			}
		case TFB_DRAWCOMMANDTYPE_LINE:
			{
				SDL_Rect r;
				int x1, y1, x2, y2;
				Uint32 color;
				PutPixelFn screen_plot;

				screen_plot = putpixel_for (SDL_Screen);
				color = SDL_MapRGB (SDL_Screen->format, DC.r, DC.g, DC.b);

				x1 = DC.x;
				x2 = DC.w;
				y1 = DC.y;
				y2 = DC.h;
				
				SDL_GetClipRect(SDL_Screen, &r);
				
				if (x1 < r.x)
					x1 = r.x;
				else if (x1 > r.x + r.w)
					x1 = r.x + r.w;

				if (x2 < r.x)
					x2 = r.x;
				else if (x2 > r.x + r.w)
					x2 = r.x + r.w;
				
				if (y1 < r.y)
					y1 = r.y;
				else if (y1 > r.y + r.h)
					y1 = r.y + r.h;

				if (y2 < r.y)
					y2 = r.y;
				else if (y2 > r.y + r.h)
					y2 = r.y + r.h;

				SDL_LockSurface (SDL_Screen);
				line (x1, y1, x2, y2, color, screen_plot, SDL_Screen);
				SDL_UnlockSurface (SDL_Screen);

				break;
			}
			case TFB_DRAWCOMMANDTYPE_RECTANGLE:
			{
				SDL_Rect r;
				r.x = DC.x;
				r.y = DC.y;
				r.w = DC.w;
				r.h = DC.h;

				SDL_FillRect(SDL_Screen, &r, SDL_MapRGB(SDL_Screen->format, DC.r, DC.g, DC.b));
				break;
			}
		case TFB_DRAWCOMMANDTYPE_SCISSORENABLE:
			{
				SDL_Rect r;
				r.x = DC.x;
				r.y = DC.y;
				r.w = DC.w;
				r.h = DC.h;
				
				SDL_SetClipRect(SDL_Screen, &r);
				break;
			}
		case TFB_DRAWCOMMANDTYPE_SCISSORDISABLE:
			SDL_SetClipRect(SDL_Screen, NULL);
			break;
		case TFB_DRAWCOMMANDTYPE_COPYBACKBUFFERTOOTHERBUFFER:
			{
				SDL_Rect src, dest;
				src.x = dest.x = DC.x;
				src.y = dest.y = DC.y;
				src.w = DC.w;
				src.h = DC.h;

				if (DC_image == 0) 
				{
					TFB_BlitSurface(SDL_Screen, &src, ExtraScreen, &dest, DC.BlendNumerator, DC.BlendDenominator);
				}
				else
				{
					dest.x = 0;
					dest.y = 0;
					TFB_BlitSurface(SDL_Screen, &src, DC_image->NormalImg, &dest, DC.BlendNumerator, DC.BlendDenominator);
				}
				break;
			}
		case TFB_DRAWCOMMANDTYPE_COPYFROMOTHERBUFFER:
			{
				SDL_Rect src, dest;
				src.x = dest.x = DC.x;
				src.y = dest.y = DC.y;
				src.w = DC.w;
				src.h = DC.h;

				TFB_BlitSurface(ExtraScreen, &src, SDL_Screen, &dest, DC.BlendNumerator, DC.BlendDenominator);
				
				break;
			}
		case TFB_DRAWCOMMANDTYPE_DELETEIMAGE:
			SDL_FreeSurface (DC_image->NormalImg);
			
			if (DC_image->ScaledImg) {
				//fprintf (stderr, "DELETEIMAGE to ScaledImg %x, size %d %d\n",DC_image->ScaledImg,DC_image->ScaledImg->w,DC_image->ScaledImg->h);
				SDL_FreeSurface (DC_image->ScaledImg);
			}

			if (DC_image->Palette)
				HFree (DC_image->Palette);

			UnlockMutex (DC_image->mutex);
			DestroyMutex (DC_image->mutex);
			
			HFree (DC_image);
			DC_image = 0;
			break;
		case TFB_DRAWCOMMANDTYPE_FLUSHGRAPHICS:
			done = TRUE;			
			break;
		case TFB_DRAWCOMMANDTYPE_SKIPGRAPHICS:
			TFB_DrawCommandQueue_Clear (DrawCommandQueue);
		}		
		if (DC_image)
			UnlockMutex (DC_image->mutex);
	}
	
	if (livelock_deterrence)
	{
		Unlock_DCQ ();
	}

	TFB_SwapBuffers();
        BroadcastCondVar (RenderingCond);
}

#endif
