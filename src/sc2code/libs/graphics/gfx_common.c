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
	DC.x = x1;
	DC.y = y1;
	DC.w = x2;
	DC.h = y2;
	DC.r = r;
	DC.g = g;
	DC.b = b;
	DC.image = 0;
	DC.UsePalette = FALSE;
	DC.destBuffer = dest;

	DC.BlendNumerator = BlendNumerator;
	DC.BlendDenominator = BlendDenominator;

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
	DC.x = rect->corner.x;
	DC.y = rect->corner.y;
	DC.w = rect->extent.width;
	DC.h = rect->extent.height;
	DC.r = r;
	DC.g = g;
	DC.b = b;
	DC.image = 0;
	DC.UsePalette = FALSE;
	DC.destBuffer = dest;

	DC.BlendNumerator = BlendNumerator;
	DC.BlendDenominator = BlendDenominator;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_Draw_Image (TFB_ImageStruct *img, int x, int y, BOOLEAN scaled, TFB_Palette *palette, SCREEN dest)
{
	TFB_DrawCommand DC;
	
	DC.Type = TFB_DRAWCOMMANDTYPE_IMAGE;
	DC.image = img;
	DC.x = x;
	DC.y = y;
	DC.UseScaling = scaled;

	if (palette != NULL)
	{
		int i;
		for (i = 0; i < 256; i++)
		{
			DC.Palette[i] = palette[i];
		}
		DC.UsePalette = TRUE;
	} 
	else
	{
		DC.UsePalette = FALSE;
	}

	DC.destBuffer = dest;
	DC.BlendNumerator = BlendNumerator;
	DC.BlendDenominator = BlendDenominator;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_Draw_CopyToImage (TFB_ImageStruct *img, PRECT lpRect, SCREEN src)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_COPYTOIMAGE;
	DC.x = lpRect->corner.x;
	DC.y = lpRect->corner.y;
	DC.w = lpRect->extent.width;
	DC.h = lpRect->extent.height;
	DC.image = img;
	DC.srcBuffer = src;
	
	DC.BlendNumerator = BlendNumerator;
	DC.BlendDenominator = BlendDenominator;
	
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
	DC.x = r->corner.x;
	DC.y = r->corner.y;
	DC.w = r->extent.width;
	DC.h = r->extent.height;
	DC.image = 0;
	DC.srcBuffer = src;
	DC.destBuffer = dest;

	DC.BlendNumerator = BlendNumerator;
	DC.BlendDenominator = BlendDenominator;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_Draw_DeleteImage (TFB_ImageStruct *img)
{
	if (img)
	{
		TFB_DrawCommand DC;

		DC.Type = TFB_DRAWCOMMANDTYPE_DELETEIMAGE;
		DC.image = img;

		TFB_EnqueueDrawCommand (&DC);
	}
}

void
TFB_Draw_WaitForSignal (void)
{
	TFB_DrawCommand DrawCommand;
	DrawCommand.Type = TFB_DRAWCOMMANDTYPE_SENDSIGNAL;
	DrawCommand.image = 0;
	// We need to lock the mutex before enqueueing the DC to prevent races 
	LockSignalMutex ();
	Lock_DCQ (1);
	TFB_BatchReset ();
	TFB_EnqueueDrawCommand(&DrawCommand);
	Unlock_DCQ();
	WaitForSignal ();
}
