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

#ifndef DRAWCMD_H
#define DRAWCMD_H

enum
{
	TFB_DRAWCOMMANDTYPE_LINE,
	TFB_DRAWCOMMANDTYPE_RECTANGLE,
	TFB_DRAWCOMMANDTYPE_IMAGE,

	TFB_DRAWCOMMANDTYPE_COPY,
	TFB_DRAWCOMMANDTYPE_COPYTOIMAGE,

	TFB_DRAWCOMMANDTYPE_SCISSORENABLE,
	TFB_DRAWCOMMANDTYPE_SCISSORDISABLE,

	TFB_DRAWCOMMANDTYPE_DELETEIMAGE,
	TFB_DRAWCOMMANDTYPE_SENDSIGNAL,
};

typedef struct tfb_drawcommand
{
	int Type;
	int x;
	int y;
	int w;
	int h;
	TFB_ImageStruct *image;
	int r;
	int g;
	int b;
	TFB_Palette Palette[256];
	BOOLEAN UsePalette;
	BOOLEAN UseScaling;
	int BlendNumerator;
	int BlendDenominator;
	DWORD thread;
	SCREEN srcBuffer;
	SCREEN destBuffer;
} TFB_DrawCommand;


// Queue Stuff

typedef struct tfb_drawcommandqueue
{
	int Front;
	int Back;
	int InsertionPoint;
	int Batching;
	volatile int FullSize;
	volatile int Size;
} TFB_DrawCommandQueue;

void TFB_DrawCommandQueue_Create (void);

void TFB_BatchGraphics (void);

void TFB_UnbatchGraphics (void);

void TFB_BatchReset (void);

void TFB_DrawCommandQueue_Push (TFB_DrawCommand* Command);

int TFB_DrawCommandQueue_Pop (TFB_DrawCommand* Command);

void TFB_DrawCommandQueue_Clear (void);

extern TFB_DrawCommandQueue DrawCommandQueue;

void TFB_EnqueueDrawCommand (TFB_DrawCommand* DrawCommand);

#endif
