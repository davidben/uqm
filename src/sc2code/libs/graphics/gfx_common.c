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

#include "libs/graphics/gfxintrn.h"
#include "libs/input/inpintrn.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawcmd.h"

PDISPLAY_INTERFACE _pCurDisplay; //Not a function. Probably has to be initialized...

void (*mask_func_array[])
		(PRECT pClipRect, PRIMITIVEPTR PrimPtr)
		= { 0 };

int ScreenWidth;
int ScreenHeight;
int ScreenWidthActual;
int ScreenHeightActual;
int GraphicsDriver;
int TFB_DEBUG_HALT = 0;
int BlendNumerator = 4;
int BlendDenominator = 4;


void
SetGraphicStrength (int numerator, int denominator)
{ 
	//fprintf (stderr, "SetGraphicsStrength %d %d\n",numerator, denominator);
	BlendNumerator = numerator;
	BlendDenominator = denominator;
}

void
TFB_Draw_Line (int x1, int y1, int x2, int y2, int r, int g, int b, SCREEN dest)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_LINE;
	DC.data.line.x1 = x1;
	DC.data.line.y1 = y1;
	DC.data.line.x2 = x2;
	DC.data.line.y2 = y2;
	DC.data.line.r = r;
	DC.data.line.g = g;
	DC.data.line.b = b;
	DC.data.line.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_Draw_Rect (PRECT rect, int r, int g, int b, SCREEN dest)
{
	RECT locRect;
	TFB_DrawCommand DC;

	if (!rect)
	{
		locRect.corner.x = locRect.corner.y = 0;
		locRect.extent.width = SCREEN_WIDTH;
		locRect.extent.height = SCREEN_HEIGHT;
		rect = &locRect;
	}

	DC.Type = TFB_DRAWCOMMANDTYPE_RECTANGLE;
	DC.data.rect.x = rect->corner.x;
	DC.data.rect.y = rect->corner.y;
	DC.data.rect.w = rect->extent.width;
	DC.data.rect.h = rect->extent.height;
	DC.data.rect.r = r;
	DC.data.rect.g = g;
	DC.data.rect.b = b;
	DC.data.rect.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_Draw_SetPalette (int index, int r, int g, int b)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_SETPALETTE;
	DC.data.setpalette.r = r;
	DC.data.setpalette.g = g;
	DC.data.setpalette.b = b;
	DC.data.setpalette.index = index;

	TFB_EnqueueDrawCommand (&DC);
}

/* This value is protected by the DCQ's lock. */
static int _localpal[256][3];

void
TFB_FlushPaletteCache ()
{
	int i;
	Lock_DCQ (-1);
	for (i = 0; i < 256; i++)
	{
		_localpal[i][0] = -1;
		_localpal[i][1] = -1;
		_localpal[i][2] = -1;
	}
	Unlock_DCQ ();
}

void
TFB_Draw_Image (TFB_ImageStruct *img, int x, int y, BOOLEAN scaled, TFB_Palette *palette, SCREEN dest)
{
	TFB_DrawCommand DC;
	
	DC.Type = TFB_DRAWCOMMANDTYPE_IMAGE;
	DC.data.image.image = img;
	DC.data.image.x = x;
	DC.data.image.y = y;
	DC.data.image.UseScaling = scaled;

	if (palette != NULL)
	{
		int i, changed;
		Lock_DCQ(257);
		changed = 0;
		for (i = 0; i < 256; i++)
		{
			if ((_localpal[i][0] != palette[i].r) ||
			    (_localpal[i][1] != palette[i].g) ||
			    (_localpal[i][2] != palette[i].b))
			{
				changed++;
				_localpal[i][0] = palette[i].r;
				_localpal[i][1] = palette[i].g;
				_localpal[i][2] = palette[i].b;
				TFB_Draw_SetPalette (i, palette[i].r, palette[i].g, 
						palette[i].b);
			}
		}
		// if (changed) { fprintf (stderr, "Actually changing palette! "); }
		DC.data.image.UsePalette = TRUE;
	} 
	else
	{
		Lock_DCQ (1);
		DC.data.image.UsePalette = FALSE;
	}

	DC.data.image.destBuffer = dest;
	DC.data.image.BlendNumerator = BlendNumerator;
	DC.data.image.BlendDenominator = BlendDenominator;

	TFB_EnqueueDrawCommand (&DC);
	Unlock_DCQ ();
}

void
TFB_Draw_FilledImage (TFB_ImageStruct *img, int x, int y, BOOLEAN scaled, int r, int g, int b, SCREEN dest)
{
	TFB_DrawCommand DC;
	
	DC.Type = TFB_DRAWCOMMANDTYPE_FILLEDIMAGE;
	DC.data.filledimage.image = img;
	DC.data.filledimage.x = x;
	DC.data.filledimage.y = y;
	DC.data.filledimage.UseScaling = scaled;
	DC.data.filledimage.r = r;
	DC.data.filledimage.g = g;
	DC.data.filledimage.b = b;
	DC.data.filledimage.destBuffer = dest;
	DC.data.filledimage.BlendNumerator = BlendNumerator;
	DC.data.filledimage.BlendDenominator = BlendDenominator;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_Draw_CopyToImage (TFB_ImageStruct *img, PRECT lpRect, SCREEN src)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_COPYTOIMAGE;
	DC.data.copytoimage.x = lpRect->corner.x;
	DC.data.copytoimage.y = lpRect->corner.y;
	DC.data.copytoimage.w = lpRect->extent.width;
	DC.data.copytoimage.h = lpRect->extent.height;
	DC.data.copytoimage.image = img;
	DC.data.copytoimage.srcBuffer = src;
	
	DC.data.copytoimage.BlendNumerator = BlendNumerator;
	DC.data.copytoimage.BlendDenominator = BlendDenominator;
	
	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_Draw_Copy (PRECT r, SCREEN src, SCREEN dest)
{
	RECT locRect;
	TFB_DrawCommand DC;

	if (!r)
	{
		locRect.corner.x = locRect.corner.y = 0;
		locRect.extent.width = SCREEN_WIDTH;
		locRect.extent.height = SCREEN_HEIGHT;
		r = &locRect;
	}

	DC.Type = TFB_DRAWCOMMANDTYPE_COPY;
	DC.data.copy.x = r->corner.x;
	DC.data.copy.y = r->corner.y;
	DC.data.copy.w = r->extent.width;
	DC.data.copy.h = r->extent.height;
	DC.data.copy.srcBuffer = src;
	DC.data.copy.destBuffer = dest;

	DC.data.copy.BlendNumerator = BlendNumerator;
	DC.data.copy.BlendDenominator = BlendDenominator;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_Draw_DeleteImage (TFB_ImageStruct *img)
{
	if (img)
	{
		TFB_DrawCommand DC;

		DC.Type = TFB_DRAWCOMMANDTYPE_DELETEIMAGE;
		DC.data.deleteimage.image = img;

		TFB_EnqueueDrawCommand (&DC);
	}
}

void
TFB_Draw_WaitForSignal (void)
{
	TFB_DrawCommand DrawCommand;
	DrawCommand.Type = TFB_DRAWCOMMANDTYPE_SENDSIGNAL;
	// We need to lock the mutex before enqueueing the DC to prevent races 
	LockSignalMutex ();
	Lock_DCQ (1);
	TFB_BatchReset ();
	TFB_EnqueueDrawCommand(&DrawCommand);
	Unlock_DCQ();
	WaitForSignal ();
}
