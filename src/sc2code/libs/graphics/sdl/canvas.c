#include "SDL.h"
#include "sdl_common.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/sdl/primitives.h"

typedef SDL_Surface *NativeCanvas;

void
TFB_DrawCanvas_Line (int x1, int y1, int x2, int y2, int r, int g, int b, TFB_Canvas target)
{
	Uint32 color;
	PutPixelFn screen_plot;
	SDL_Rect rect;
	
	screen_plot = putpixel_for (target);
	color = SDL_MapRGB (((NativeCanvas) target)->format, r, g, b);

	SDL_GetClipRect((NativeCanvas) target, &rect);
				
	/* Danger, Will Robinson!  This code
	   looks VERY suspicious.  It will end
	   up changing the slope of the
	   line! */

	if (x1 < rect.x)
		x1 = rect.x;
	else if (x1 > rect.x + rect.w)
		x1 = rect.x + rect.w;
	
	if (x2 < rect.x)
		x2 = rect.x;
	else if (x2 > rect.x + rect.w)
		x2 = rect.x + rect.w;
	
	if (y1 < rect.y)
		y1 = rect.y;
	else if (y1 > rect.y + rect.h)
		y1 = rect.y + rect.h;
	
	if (y2 < rect.y)
		y2 = rect.y;
	else if (y2 > rect.y + rect.h)
		y2 = rect.y + rect.h;
	
	SDL_LockSurface ((NativeCanvas) target);
	line (x1, y1, x2, y2, color, screen_plot, (NativeCanvas) target);
	SDL_UnlockSurface ((NativeCanvas) target);
}

void
TFB_DrawCanvas_Rect (PRECT rect, int r, int g, int b, TFB_Canvas target)
{
	SDL_Rect sr;
	sr.x = rect->corner.x;
	sr.y = rect->corner.y;
	sr.w = rect->extent.width;
	sr.h = rect->extent.height;
	
	SDL_FillRect((NativeCanvas) target, &sr, SDL_MapRGB(((NativeCanvas) target)->format, r, g, b));
}

void
TFB_DrawCanvas_Image (TFB_Image *img, int x, int y, BOOLEAN scaled, TFB_Palette *palette, TFB_Canvas target)
{
	SDL_Rect targetRect;
	SDL_Surface *surf;

	if (img == 0)
	{
		fprintf (stderr, "ERROR: TFB_DrawCanvas_Image passed null image ptr\n");
		return;
	}

	LockMutex (img->mutex);
	targetRect.x = x;
	targetRect.y = y;

	if (scaled)
		surf = img->ScaledImg;
	else
		surf = img->NormalImg;
	
	if (surf->format->palette)
	{
		if (palette)
		{
			SDL_SetColors (surf, (SDL_Color*)palette, 0, 256);
		}
		else
		{
			SDL_SetColors (surf, (SDL_Color*)img->Palette, 0, 256);
		}
	}
	
	SDL_BlitSurface(surf, NULL, (NativeCanvas) target, &targetRect);
	UnlockMutex (img->mutex);
}

void
TFB_DrawCanvas_FilledImage (TFB_Image *img, int x, int y, BOOLEAN scaled, int r, int g, int b, TFB_Canvas target)
{
	SDL_Rect targetRect;
	SDL_Surface *surf;
	int i;
	SDL_Color pal[256];

	if (img == 0)
	{
		fprintf (stderr, "ERROR: TFB_DrawCanvas_FilledImage passed null image ptr\n");
		return;
	}

	LockMutex (img->mutex);

	targetRect.x = x;
	targetRect.y = y;

	if (scaled)
		surf = img->ScaledImg;
	else
		surf = img->NormalImg;
				
	for (i = 0; i < 256; i++)
	{
		pal[i].r = r;
		pal[i].g = g;
		pal[i].b = b;
	}
	SDL_SetColors (surf, pal, 0, 256);

	SDL_BlitSurface(surf, NULL, (NativeCanvas) target, &targetRect);
	UnlockMutex (img->mutex);
}

TFB_Canvas TFB_DrawCanvas_New_TrueColor (int w, int h, BOOLEAN hasalpha)
{
	SDL_Surface *new_surf;
	new_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, w, h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, hasalpha ? 0xff000000 : 0);
	if (!new_surf) {
		fprintf(stderr, "INTERNAL PANIC: Failed to create TFB_Canvas: %s", SDL_GetError());
		exit(-1);
	}
	return new_surf;
}

TFB_Canvas
TFB_DrawCanvas_New_Paletted (int w, int h, TFB_Palette *palette, int transparent_index)
{
	SDL_Surface *new_surf;
	new_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
	if (!new_surf) {
		fprintf(stderr, "INTERNAL PANIC: Failed to create TFB_Canvas: %s\n", SDL_GetError());
		exit(-1);
	}
	if (palette != NULL)
	{
		SDL_SetColors(new_surf, (SDL_Color *)palette, 0, 256);
	}
	if (transparent_index >= 0)
	{
		SDL_SetColorKey (new_surf, SDL_SRCCOLORKEY, transparent_index);
	}
	else
	{
		SDL_SetColorKey (new_surf, 0, 0);
	}
	return new_surf;
}

void
TFB_DrawCanvas_Delete (TFB_Canvas canvas)
{
	if (!canvas)
	{
		fprintf(stderr, "INTERNAL PANIC: Attempted to delete a NULL canvas!\n");
		/* Should we actually die here? */
	} 
	else
	{
		SDL_FreeSurface ((SDL_Surface *) canvas);
	}

}

TFB_Palette *
TFB_DrawCanvas_ExtractPalette (TFB_Canvas canvas)
{
	int i;		
	TFB_Palette *result = (TFB_Palette*) HMalloc (sizeof (TFB_Palette) * 256);
	SDL_Surface *surf = (SDL_Surface *)canvas;

	if (!surf->format->palette)
	{
		return NULL;
	}

	for (i = 0; i < 256; ++i)
	{
		result[i].r = surf->format->palette->colors[i].r;
		result[i].g = surf->format->palette->colors[i].g;
		result[i].b = surf->format->palette->colors[i].b;
	}
	return result;
}

TFB_Canvas
TFB_DrawCanvas_ToScreenFormat (TFB_Canvas canvas)
{
	SDL_Surface *result = TFB_DisplayFormatAlpha (canvas);
	if (result == NULL)
	{
		fprintf (stderr, "WARNING: Could not convert sprite-canvas to display format.  Expect performance penalties.\n");
		return canvas;
	}
	else
	{
		TFB_DrawCanvas_Delete(canvas);
		return result;
	}
}
