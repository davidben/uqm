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


/****************************************

  Star Control II: 3DO => SDL Port

  Copyright Toys for Bob, 2002

  Programmer: Chris Nelson
  
*****************************************/


#ifndef GFX_COMMON_H
#define GFX_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "starcon.h"
#include "libs/gfxlib.h"
#include "libs/vidlib.h"
#include "libs/misc.h"


int Main(int argc, char *argv[]);
int Starcon2Main (void *);

// Graphics Stuff

void LoadIntoExtraScreen (PRECT r);
void DrawFromExtraScreen (PRECT r);
void SetGraphicStrength (int numer, int denom);
void SetGraphicGrabOther (int grab_other);
void SetGraphicScale (int scale);
void SetGraphicUseOtherExtra (int other);
void *GetScreenBitmap ();
void ScreenTransition (int transition, PRECT pRect);
void ScreenOrigin (FRAME Display, COORD sx, COORD sy);

extern float FrameRate;
extern int FrameRateTickBase;

// Image + Drawing Stuff

#define TFB_IMAGESTRUCT_SIZE 32

// TFB_Image (which is module-specific) will be casted to this and vice versa
typedef struct tfb_imagestruct
{
	UBYTE data[TFB_IMAGESTRUCT_SIZE];
} TFB_ImageStruct;

enum
{
	TFB_DRAWCOMMANDTYPE_LINE,
	TFB_DRAWCOMMANDTYPE_RECTANGLE,
	TFB_DRAWCOMMANDTYPE_IMAGE,
	TFB_DRAWCOMMANDTYPE_COPYFROMOTHERBUFFER,

	TFB_DRAWCOMMANDTYPE_SCISSORENABLE,
	TFB_DRAWCOMMANDTYPE_SCISSORDISABLE,
	TFB_DRAWCOMMANDTYPE_COPYBACKBUFFERTOOTHERBUFFER,
	TFB_DRAWCOMMANDTYPE_DELETEIMAGE,
};

typedef struct tfb_drawcommand
{
	int Type;
	int x;
	int y;
	int w;
	int h;
	TFB_ImageStruct *image; // only used for image draw type
	int r; // RGB only used for rect draw types
	int g;
	int b;
	int Qualifier;
} TFB_DrawCommand;

// Queue Stuff

typedef struct tfb_drawcommandqueue
{
	int Front;
	int Back;
	int Size;
} TFB_DrawCommandQueue;

TFB_DrawCommandQueue *TFB_DrawCommandQueue_Create ();

void TFB_DrawCommandQueue_Push (TFB_DrawCommandQueue* myQueue,
		TFB_DrawCommand* Command);

int TFB_DrawCommandQueue_Pop (TFB_DrawCommandQueue* myQueue,
		TFB_DrawCommand* Command);

void TFB_DeallocateDrawCommand (TFB_DrawCommand* Command);

extern TFB_DrawCommandQueue *DrawCommandQueue;

// The TFB_Enqueue* functions are necessary, because only the 
// main thread can draw to the window.

void TFB_EnqueueDrawCommand (TFB_DrawCommand* DrawCommand);
void TFB_FlushGraphics (); // Only call from main thread!!

// CCB Stuff

typedef struct
{
	int ccb_Flags;
	int ccb_HDX;
	int ccb_HDY;
	int ccb_HDDX;
	int ccb_HDDY;
	int ccb_VDX;
	int ccb_VDY;
	int ccb_PIXC;
	void* ccb_SourcePtr;
	unsigned int* ccb_PLUTPtr;
	int ccb_Width;
	int ccb_Height;
	int ccb_PRE0;
	int ccb_PRE1;
	void* ccb_NextPtr;
	int ccb_XPos;
	int ccb_YPos;
} CCB;

// Geez, I hope these #defines work out...

#define CCB_SPABS    (1 << 0)
#define CCB_PPABS    (1 << 1)
#define CCB_YOXY     (1 << 2)
#define CCB_ACW      (1 << 3)
#define CCB_ACCW     (1 << 4)
#define CCB_LDSIZE   (1 << 5)
#define CCB_CCBPRE   (1 << 6)
#define CCB_LDPRS    (1 << 7)
#define CCB_LDPPMP   (1 << 8)
#define CCB_ACE      (1 << 9)
#define CCB_LCE      (1 << 10)
#define CCB_PLUTPOS  (1 << 11)
#define CCB_BGND     (1 << 12)
#define CCB_NPABS    (1 << 13)
#define CCB_PACKED   (1 << 14)
#define CCB_LDPLUT   (1 << 15)
#define CCB_LAST     (1 << 16)
#define CCB_USEAV    (1 << 17)

#define PPMP_0_SHIFT   0  // ?
#define PPMP_1_SHIFT   1  // ?

#define PPMPC_MS_PIN    (1 << 0)
#define PPMPC_MS_CCB    (1 << 1)
#define PPMPC_SF_8      (1 << 2)
#define PPMPC_MF_8      (1 << 3)
#define PPMPC_MF_SHIFT  3
#define PPMPC_1S_PDC    (1 << 4)
#define PPMPC_2S_CFBD   (1 << 5)
#define PPMPC_2D_1      (1 << 6)
#define PPMPC_MF_6      (1 << 7)
#define PPMPC_2S_0      (1 << 8)
#define PPMPC_1S_CFBD   (1 << 9)
#define PPMPC_MF_2      (1 << 10)
#define PPMPC_SF_2      (1 << 11)
#define PPMPC_2S_PDC    (1 << 12)
#define PPMPC_MF_1      (1 << 13)
#define PPMPC_SF_16     (1 << 14)

#define PRE0_LITERAL           (1 << 0)
#define PRE0_VCNT_PREFETCH     (1 << 1)
#define PRE0_VCNT_SHIFT        1
#define PRE0_VCNT_MASK         (1 << 1)
#define PRE0_BPP_8             (1 << 2)
#define PRE0_BPP_SHIFT         2
#define PRE0_BPP_16            (1 << 3)
#define PRE0_BGND              (1 << 4)
#define PRE0_LINEAR            (1 << 5)
#define PRE0_SKIPX_MASK        0
#define PRE0_SKIPX_SHIFT       0
#define PRE0_BPP_1             (1 << 6)
#define PRE1_TLLSB_PDC0        (1 << 7)
#define PRE1_TLHPCNT_PREFETCH  (1 << 7)
#define PRE1_TLHPCNT_SHIFT     7
#define PRE1_TLHPCNT_MASK      (1 << 7)
#define PRE1_WOFFSET_PREFETCH  (1 << 8)
#define PRE1_WOFFSET10_SHIFT   (1 << 9)
#define PRE1_WOFFSET8_SHIFT    (1 << 10)

// Cel Stuff

void add_cel(CCB* cel);
void add_cels(CCB* first, CCB* last);
void myMapCel(CCB* myCCB, POINT Points[4]);
int _ThreeDO_batch_cels(int unknown);
CCB *ParseCel(CCB*,unsigned int);

#define CelData void

// Unknown Stuff

extern int ScreenWidth;
extern int ScreenHeight;
extern int ScreenWidthActual;
extern int ScreenHeightActual;
extern int ScreenBPPActual;

#endif
