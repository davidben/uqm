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
	color = SDL_MapRGB (((NativeCanvas)target)->format, r, g, b);

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
	
	SDL_FillRect((NativeCanvas)target, &sr, SDL_MapRGB(((NativeCanvas)target)->format, r, g, b));
}

void
TFB_DrawCanvas_Image (TFB_Image *img, int x, int y, BOOLEAN scaled, TFB_Palette *palette, TFB_Canvas target, int bn, int bd)
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
	
	TFB_BlitSurface(surf, NULL, target, &targetRect, bn, bd);
	UnlockMutex (img->mutex);
}

void
TFB_DrawCanvas_FilledImage (TFB_Image *img, int x, int y, BOOLEAN scaled, int r, int g, int b, TFB_Canvas target, int bn, int bd)
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

	TFB_BlitSurface(surf, NULL, target, &targetRect, bn, bd);
	UnlockMutex (img->mutex);
}
