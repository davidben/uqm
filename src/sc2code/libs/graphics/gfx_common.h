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

#ifndef GFX_COMMON_H
#define GFX_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "starcon.h"
#include "libs/gfxlib.h"
#include "libs/vidlib.h"
#include "libs/misc.h"

// driver for TFB_InitGraphics
enum
{
	TFB_GFXDRIVER_SDL_OPENGL,
	TFB_GFXDRIVER_SDL_PURE,
};

// flags for TFB_InitGraphics
#define TFB_GFXFLAGS_FULLSCREEN         (1<<0)
#define TFB_GFXFLAGS_SHOWFPS            (1<<1)
#define TFB_GFXFLAGS_SCANLINES          (1<<2)
#define TFB_GFXFLAGS_SCALE_BILINEAR     (1<<3)
#define TFB_GFXFLAGS_SCALE_BIADAPT      (1<<4)

#define TFB_GFX_NUMSCREENS              3

typedef enum {
	TFB_SCREEN_MAIN,
	TFB_SCREEN_EXTRA,
	TFB_SCREEN_TRANSITION
} SCREEN;

int TFB_InitGraphics (int driver, int flags, int width, int height, int bpp);
void TFB_UninitGraphics (void);
void TFB_ProcessEvents (void);

// 3DO Graphics Stuff

void LoadIntoExtraScreen (PRECT r);
void DrawFromExtraScreen (PRECT r);
void SetGraphicStrength (int numer, int denom);
void SetGraphicGrabOther (int grab_other);
void SetGraphicScale (int scale);
void SetGraphicUseOtherExtra (int other);
void *GetScreenBitmap (void);
void ScreenTransition (int transition, PRECT pRect);

extern float FrameRate;
extern int FrameRateTickBase;

// Image + Drawing Stuff

#define TFB_IMAGESTRUCT_SIZE 32

// TFB_Image (which is module-specific) will be casted to this and vice versa
typedef struct tfb_imagestruct
{
	UBYTE data[TFB_IMAGESTRUCT_SIZE];
} TFB_ImageStruct;


typedef struct tfb_palette
{
	UBYTE r;
	UBYTE g;
	UBYTE b;
	UBYTE unused;
} TFB_Palette;

// Drawing commands

void TFB_Draw_Line (int x1, int y1, int x2, int y2, int r, int g, int b, SCREEN dest);
void TFB_Draw_Rect (PRECT rect, int r, int g, int b, SCREEN dest);
void TFB_Draw_Image (TFB_ImageStruct *img, int x, int y, BOOLEAN scaled, TFB_Palette *palette, SCREEN dest);
void TFB_Draw_Copy (PRECT r, SCREEN src, SCREEN dest);
void TFB_Draw_CopyToImage (TFB_ImageStruct *img, PRECT lpRect, SCREEN src);
void TFB_Draw_DeleteImage (TFB_ImageStruct *img);
void TFB_Draw_WaitForSignal (void);

void TFB_FlushGraphics (void); // Only call from main thread!!

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
extern int GraphicsDriver;
extern int BlendNumerator;
extern int BlendDenominator;

int Starcon2Main (void *);

#include "cmap.h"

#endif
