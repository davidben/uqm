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

SDL_Surface *SDL_Video;
SDL_Surface *SDL_Screen;
SDL_Surface *ExtraScreen;

volatile int continuity_break;


int
TFB_InitGraphics (int driver, int flags, int width, int height, int bpp)
{
	int result;

	if (driver == TFB_GFXDRIVER_SDL_OPENGL)
	{
#ifdef HAVE_OPENGL
		result = TFB_GL_InitGraphics (driver, flags, width, height, bpp);
#else
		driver = TFB_GFXDRIVER_SDL_PURE;
		printf("OpenGL support not compiled in, so using pure sdl driver\n");
		result = TFB_Pure_InitGraphics (driver, flags, width, height, bpp);
#endif
	}
	else
	{
		result = TFB_Pure_InitGraphics (driver, flags, width, height, bpp);
	}

	SDL_EnableUNICODE (1);
	SDL_WM_SetCaption (TFB_WINDOW_CAPTION, NULL);

	//if (flags & TFB_GFXFLAGS_FULLSCREEN)
	//	SDL_ShowCursor (SDL_DISABLE);

	return 0;
}

int
TFB_CreateGamePlayThread ()
{
	TFB_DEBUG_HALT = 0;

	GraphicsSem = SDL_CreateSemaphore (1);
	_MemorySem = SDL_CreateSemaphore (1);

	SDL_CreateThread (Starcon2Main, NULL);

	return 0;
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
	SDL_Surface *new_surf;

    float x_scale = (float)ScreenWidthActual / ScreenWidth;
	float y_scale = (float)ScreenHeightActual / ScreenHeight;	
	
	myImage = (TFB_Image*) HMalloc (sizeof (TFB_Image));
	myImage->dirty = FALSE;
	myImage->ScaledImg = NULL;
	
	if (img->format->BytesPerPixel < 4) {
		// Because image doesn't have alpha channel, blit it to new surface, using
		// transparent color (if SDL_SetColorKey was called before this)

		SDL_Surface *full_surf;
		SDL_Rect SDL_DstRect;

		full_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, img->clip_rect.w, img->clip_rect.h, 
			32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

		SDL_DstRect.x = SDL_DstRect.y = 0;

		SDL_BlitSurface (img, &img->clip_rect, full_surf, &SDL_DstRect);

		full_surf->clip_rect.x = full_surf->clip_rect.y = 0;
		full_surf->clip_rect.w = img->clip_rect.w;
		full_surf->clip_rect.h = img->clip_rect.h;

		SDL_FreeSurface (img);
		img = full_surf;
	}
	
	if ((x_scale==1.0f && y_scale==1.0f) || (img->w==0 && img->h==0))
	{
		new_surf = img;
	}
	else 
	{
		// Note: SMOOTHING_OFF and SMOOTHING_ON controls bilinear filtering
		new_surf = zoomSurface (img, x_scale, y_scale, SMOOTHING_OFF);
	}

	// Now convert to the native display format
	
	myImage->DrawableImg = TFB_DisplayFormatAlpha (new_surf);
	if (new_surf != img)
		SDL_FreeSurface(new_surf);

	myImage->SurfaceSDL = TFB_DisplayFormatAlpha (img);
	SDL_FreeSurface(img);

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
TFB_UpdateDrawableImage (TFB_Image *img)
{
	SDL_Surface *new_surf;

    float x_scale = (float)ScreenWidthActual / ScreenWidth;
	float y_scale = (float)ScreenHeightActual / ScreenHeight;

	SDL_LockSurface (img->SurfaceSDL);
	new_surf = zoomSurface (img->SurfaceSDL, x_scale, y_scale, SMOOTHING_OFF);
	SDL_UnlockSurface (img->SurfaceSDL);
	
	// Now convert to the native display format

	SDL_FreeSurface (img->DrawableImg);
	img->DrawableImg = TFB_DisplayFormatAlpha (new_surf);
	SDL_FreeSurface (new_surf);
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

void
TFB_FlushGraphics () // Only call from main thread!!
{
	int semval;
	Uint32 this_flush;
	static Uint32 last_flush = 0;

	this_flush = SDL_GetTicks ();
	if (this_flush - last_flush < 1000 / 100)
		return;

	semval = SDL_SemTryWait (GraphicsSem);
	if (semval != 0 && !continuity_break)
		return;

	continuity_break = 0;
	if (DrawCommandQueue == 0 || DrawCommandQueue->Size == 0)
	{
		if (semval == 0)
			SDL_SemPost (GraphicsSem);
		return;
	}

	last_flush = this_flush;

	if (semval == 0)
		SDL_SemPost (GraphicsSem);

	while (DrawCommandQueue->Size > 0)
	{
		TFB_DrawCommand DC_real;
		TFB_Image *DC_image;

		TFB_DrawCommand* DC = &DC_real;

		if (!TFB_DrawCommandQueue_Pop (DrawCommandQueue, DC))
		{
			printf ("Woah there coyboy! Trouble with TFB_Queues...\n");
			continue;
		}

		DC_image = (TFB_Image*) DC->image;
		
		switch (DC->Type)
		{
		case TFB_DRAWCOMMANDTYPE_IMAGE:
			if (DC_image==0)
			{
				printf ("Error!!! Bad Image!\n");
			}
			else
			{
				SDL_Rect targetRect;

				targetRect.x = DC->x * ScreenWidthActual / ScreenWidth;
				targetRect.y = DC->y * ScreenHeightActual / ScreenHeight;

				if (DC->Qualifier != MAPPED_TO_DISPLAY)
				{
					if ((DC->w != DC_image->SurfaceSDL->w || DC->h != DC_image->SurfaceSDL->h) && 
						DC_image->ScaledImg)
					{
						SDL_BlitSurface(DC_image->ScaledImg, NULL, SDL_Screen, &targetRect);
					}
					else
					{
						if (DC_image->dirty)
						{
							TFB_UpdateDrawableImage (DC_image);
							DC_image->dirty = FALSE;
						}

						SDL_BlitSurface(DC_image->DrawableImg, NULL, SDL_Screen, &targetRect);
					}
				}
				else
				{
					SDL_BlitSurface(DC_image->SurfaceSDL, NULL, SDL_Screen, &targetRect);
				}
			}
			break;
		case TFB_DRAWCOMMANDTYPE_LINE:
			{
				int x_scale = ScreenWidthActual / ScreenWidth;
				int y_scale = ScreenHeightActual / ScreenHeight;
				int x1, y1, x2, y2;
				Uint32 color;
				PutPixelFn screen_plot;
				int xi, yi;

				screen_plot = putpixel_for (SDL_Screen);

				x1 = DC->x * x_scale;
				x2 = DC->w * x_scale;
				y1 = DC->y * y_scale;
				y2 = DC->h * y_scale;
				color = SDL_MapRGB (SDL_Screen->format, DC->r, DC->g, DC->b);
				
				SDL_LockSurface (SDL_Screen);
				for (xi = 0; xi < x_scale; xi++)
				{
					for (yi = 0; yi < y_scale; yi++)
					{
						line (x1 + xi, y1 + yi, x2 + xi, y2 + yi,
							  color, screen_plot, SDL_Screen);
					}
				}
				SDL_UnlockSurface (SDL_Screen);
				break;
			}
			case TFB_DRAWCOMMANDTYPE_RECTANGLE:
			{
				SDL_Rect r;
				r.x = DC->x * ScreenWidthActual / ScreenWidth;
				r.y = DC->y * ScreenHeightActual / ScreenHeight;
				r.w = DC->w * ScreenWidthActual / ScreenWidth;
				r.h = DC->h * ScreenHeightActual / ScreenHeight;

				SDL_FillRect(SDL_Screen, &r, SDL_MapRGB(SDL_Screen->format, DC->r, DC->g, DC->b));
				break;
			}
		case TFB_DRAWCOMMANDTYPE_SCISSORENABLE:
			{
				SDL_Rect r;
				r.x = DC->x * ScreenWidthActual / ScreenWidth;
				r.y = DC->y * ScreenHeightActual / ScreenHeight;
				r.w = DC->w * ScreenWidthActual / ScreenWidth;
				r.h = DC->h * ScreenHeightActual / ScreenHeight;
				
				SDL_SetClipRect(SDL_Screen, &r);
				break;
			}
		case TFB_DRAWCOMMANDTYPE_SCISSORDISABLE:
			SDL_SetClipRect(SDL_Screen, NULL);
			break;
		case TFB_DRAWCOMMANDTYPE_COPYBACKBUFFERTOOTHERBUFFER:
			{
				SDL_Rect src, dest;
				src.x = dest.x = DC->x * ScreenWidthActual / ScreenWidth;
				src.y = dest.y = DC->y * ScreenHeightActual / ScreenHeight;
				src.w = DC->w * ScreenWidthActual / ScreenWidth;
				src.h = DC->h * ScreenHeightActual / ScreenHeight;

				if (DC_image == 0) 
				{
					SDL_BlitSurface(SDL_Screen, &src, ExtraScreen, &dest);
				}
				else
				{
					dest.x = 0;
					dest.y = 0;
					SDL_BlitSurface(SDL_Screen, &src, DC_image->SurfaceSDL, &dest);
					DC_image->dirty = TRUE;
				}
				break;
			}
		case TFB_DRAWCOMMANDTYPE_COPYFROMOTHERBUFFER:
			{
				SDL_Rect src, dest;
				src.x = dest.x = DC->x * ScreenWidthActual / ScreenWidth;
				src.y = dest.y = DC->y * ScreenHeightActual / ScreenHeight;
				src.w = DC->w * ScreenWidthActual / ScreenWidth;
				src.h = DC->h * ScreenHeightActual / ScreenHeight;
				SDL_BlitSurface(ExtraScreen, &src, SDL_Screen, &dest);
				break;
			}
		case TFB_DRAWCOMMANDTYPE_DELETEIMAGE:
			SDL_FreeSurface (DC_image->SurfaceSDL);
			SDL_FreeSurface (DC_image->DrawableImg);
			
			if (DC_image->ScaledImg) {
				//printf("DELETEIMAGE to ScaledImg %x, size %d %d\n",DC_image->ScaledImg,DC_image->ScaledImg->w,DC_image->ScaledImg->h);
				SDL_FreeSurface (DC_image->ScaledImg);
			}

			HFree (DC_image);
			break;
		}

		TFB_DeallocateDrawCommand (DC);
	}
	
	TFB_SwapBuffers();
}

#endif
