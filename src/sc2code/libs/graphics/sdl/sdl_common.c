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
#include "version.h"
#include "SDL_thread.h"
#include "libs/graphics/drawcmd.h"
#include "bbox.h"

SDL_Surface *SDL_Video;
SDL_Surface *SDL_Screen;
SDL_Surface *TransitionScreen;

SDL_Surface *SDL_Screens[TFB_GFX_NUMSCREENS];

volatile int TransitionAmount = 255;
SDL_Rect TransitionClipRect;

int GfxFlags = 0;

static TFB_Palette palette[256];

#define FPS_PERIOD 1000
int RenderedFrames = 0;

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

	sprintf (caption, "The Ur-Quan Masters v%d.%d%s", 
			UQM_MAJOR_VERSION, UQM_MINOR_VERSION, UQM_EXTRA_VERSION);
	SDL_WM_SetCaption (caption, NULL);

	//if (flags & TFB_GFXFLAGS_FULLSCREEN)
	//	SDL_ShowCursor (SDL_DISABLE);

	TFB_FlushPaletteCache ();

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
				TFB_SwapBuffers (1);
				break;
			default:
				break;
		}
	}
}

void
TFB_SwapBuffers (int force_full_redraw)
{
#ifdef HAVE_OPENGL
	if (GraphicsDriver == TFB_GFXDRIVER_SDL_OPENGL)
		TFB_GL_SwapBuffers (force_full_redraw);
	else
		TFB_Pure_SwapBuffers (force_full_redraw);
#else
	TFB_Pure_SwapBuffers (force_full_redraw);
#endif
}

/* Probably ought to clean this away at some point. */
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
	if (fps_counter > FPS_PERIOD)
	{
		fprintf (stderr, "fps %.2f, effective %.2f\n",
				1000.0 / delta_time,
				1000.0 * RenderedFrames / fps_counter);

		fps_counter = 0;
		RenderedFrames = 0;
	}
}

void
TFB_FlushGraphics () // Only call from main thread!!
{
	int commands_handled;
	BOOLEAN livelock_deterrence;
	BOOLEAN done;

	// This is technically a locking violation on DrawCommandQueue.Size,
	// but it is likely to not be very destructive.
	if (DrawCommandQueue.Size == 0)
	{
		static int last_fade = 255;
		static int last_transition = 255;
		int current_fade = FadeAmount;
		int current_transition = TransitionAmount;
		
		if ((current_fade != 255 && current_fade != last_fade) ||
			(current_transition != 255 && current_transition != last_transition) ||
			(current_fade == 255 && last_fade != 255) ||
			(current_transition == 255 && last_transition != 255))
		{
			TFB_SwapBuffers (1); // if fading, redraw every frame
		}
		else
		{
			TaskSwitch ();
		}
		
		last_fade = current_fade;
		last_transition = current_transition;
		BroadcastCondVar (RenderingCond);
		return;
	}

	if (GfxFlags & TFB_GFXFLAGS_SHOWFPS)
		TFB_ComputeFPS ();

	commands_handled = 0;
	livelock_deterrence = FALSE;

	if (DrawCommandQueue.FullSize > DCQ_FORCE_BREAK_SIZE)
	{
		TFB_BatchReset ();
	}

	if (DrawCommandQueue.Size > DCQ_FORCE_SLOWDOWN_SIZE)
	{
		Lock_DCQ (-1);
		livelock_deterrence = TRUE;
	}

	TFB_BBox_Reset ();
	TFB_BBox_GetClipRect (SDL_Screens[0]);

	done = FALSE;
	while (!done)
	{
		TFB_DrawCommand DC;

		if (!TFB_DrawCommandQueue_Pop (&DC))
		{
			// the Queue is now empty.
			break;
		}

		++commands_handled;
		if (!livelock_deterrence && commands_handled + DrawCommandQueue.Size > DCQ_LIVELOCK_MAX)
		{
			// fprintf (stderr, "Initiating livelock deterrence!\n");
			livelock_deterrence = TRUE;
			
			Lock_DCQ (-1);
		}

		switch (DC.Type)
		{
		case TFB_DRAWCOMMANDTYPE_SETPALETTE:
			{
				int index = DC.data.setpalette.index;
				if (index < 0 || index > 255)
				{
					fprintf(stderr, "DCQ panic: Tried to set palette #%i", index);
				}
				else
				{
					palette[index].r = DC.data.setpalette.r & 0xFF;
					palette[index].g = DC.data.setpalette.g & 0xFF;
					palette[index].b = DC.data.setpalette.b & 0xFF;
				}
				break;
			}
		case TFB_DRAWCOMMANDTYPE_IMAGE:
			{
				TFB_Image *DC_image = DC.data.image.image;
				TFB_Palette *pal;
				int x = DC.data.image.x;
				int y = DC.data.image.y;

				if (DC.data.image.UseScaling)
					TFB_BBox_RegisterCanvas (DC_image->ScaledImg, x, y);
				else
					TFB_BBox_RegisterCanvas (DC_image->NormalImg, x, y);

				if (DC.data.image.UsePalette)
				{
					pal = palette;
				}
				else
				{
					pal = DC_image->Palette;
				}

				TFB_DrawCanvas_Image (DC_image, x, y,
						DC.data.image.UseScaling, pal,
						SDL_Screens[DC.data.image.destBuffer]);

				break;
			}
		case TFB_DRAWCOMMANDTYPE_FILLEDIMAGE:
			{
				TFB_Image *DC_image = DC.data.filledimage.image;
				int x = DC.data.filledimage.x;
				int y = DC.data.filledimage.y;

				if (DC.data.filledimage.UseScaling)
					TFB_BBox_RegisterCanvas (DC_image->ScaledImg, x, y);
				else
					TFB_BBox_RegisterCanvas (DC_image->NormalImg, x, y);

				TFB_DrawCanvas_FilledImage (DC.data.filledimage.image, DC.data.filledimage.x, DC.data.filledimage.y,
						DC.data.filledimage.UseScaling, DC.data.filledimage.r, DC.data.filledimage.g,
						DC.data.filledimage.b, SDL_Screens[DC.data.filledimage.destBuffer]);
				break;
			}
		case TFB_DRAWCOMMANDTYPE_LINE:
			{
				TFB_BBox_RegisterPoint (DC.data.line.x1, DC.data.line.y1);
				TFB_BBox_RegisterPoint (DC.data.line.x2, DC.data.line.y2);
				TFB_DrawCanvas_Line (DC.data.line.x1, DC.data.line.y1, 
						DC.data.line.x2, DC.data.line.y2, 
						DC.data.line.r, DC.data.line.g, DC.data.line.b,
						SDL_Screens[DC.data.line.destBuffer]);
				break;
			}
		case TFB_DRAWCOMMANDTYPE_RECTANGLE:
			{
				TFB_BBox_RegisterRect (&DC.data.rect.rect);
				TFB_DrawCanvas_Rect (&DC.data.rect.rect, DC.data.rect.r, 
						DC.data.rect.g, DC.data.rect.b, 
						SDL_Screens[DC.data.rect.destBuffer]);

				break;
			}
		case TFB_DRAWCOMMANDTYPE_SCISSORENABLE:
			{
				SDL_Rect r;
				r.x = TFB_BBox.clip.corner.x = DC.data.scissor.x;
				r.y = TFB_BBox.clip.corner.y = DC.data.scissor.y;
				r.w = TFB_BBox.clip.extent.width = DC.data.scissor.w;
				r.h = TFB_BBox.clip.extent.height = DC.data.scissor.h;
				SDL_SetClipRect(SDL_Screens[0], &r);
				break;
			}
		case TFB_DRAWCOMMANDTYPE_SCISSORDISABLE:
			SDL_SetClipRect(SDL_Screens[0], NULL);
			TFB_BBox.clip.corner.x = 0;
			TFB_BBox.clip.corner.y = 0;
			TFB_BBox.clip.extent.width = ScreenWidth;
			TFB_BBox.clip.extent.height = ScreenHeight;
			break;
		case TFB_DRAWCOMMANDTYPE_COPYTOIMAGE:
			{
				SDL_Rect src, dest;
				TFB_Image *DC_image = DC.data.copytoimage.image;

				if (DC_image == 0)
				{
					fprintf (stderr, "DCQ ERROR: COPYTOIMAGE passed null image ptr\n");
					break;
				}
				LockMutex (DC_image->mutex);

				src.x = dest.x = DC.data.copytoimage.x;
				src.y = dest.y = DC.data.copytoimage.y;
				src.w = DC.data.copytoimage.w;
				src.h = DC.data.copytoimage.h;

				dest.x = 0;
				dest.y = 0;
				SDL_BlitSurface(SDL_Screens[DC.data.copytoimage.srcBuffer], &src, DC_image->NormalImg, &dest);
				UnlockMutex (DC_image->mutex);
				break;
			}
		case TFB_DRAWCOMMANDTYPE_COPY:
			{
				SDL_Rect src, dest;
				src.x = dest.x = DC.data.copy.x;
				src.y = dest.y = DC.data.copy.y;
				src.w = DC.data.copy.w;
				src.h = DC.data.copy.h;

				TFB_BBox_RegisterPoint (src.x, src.y);
				TFB_BBox_RegisterPoint (src.x + src.w, src.y + src.h);


				SDL_BlitSurface(SDL_Screens[DC.data.copy.srcBuffer], &src, SDL_Screens[DC.data.copy.destBuffer], &dest);
				break;
			}
		case TFB_DRAWCOMMANDTYPE_DELETEIMAGE:
			{
				TFB_Image *DC_image = (TFB_Image *)DC.data.deleteimage.image;
				TFB_DrawImage_Delete(DC_image);
				break;
			}
		case TFB_DRAWCOMMANDTYPE_SENDSIGNAL:
			SignalThread (DC.data.sendsignal.thread);
			break;
		}		
	}
	
	if (livelock_deterrence)
	{
		Unlock_DCQ ();
	}

	TFB_SwapBuffers (0);
	RenderedFrames++;
	BroadcastCondVar (RenderingCond);
}

#endif
