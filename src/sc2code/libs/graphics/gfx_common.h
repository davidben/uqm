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
#define TFB_GFXFLAGS_SCALE_BIADAPTADV   (1<<5)
#define TFB_GFXFLAGS_SCALE_TRISCAN      (1<<6)
#define TFB_GFXFLAGS_SCALE_HQXX         (1<<7)
#define TFB_GFXFLAGS_SCALE_ANY \
		( TFB_GFXFLAGS_SCALE_BILINEAR   | \
		  TFB_GFXFLAGS_SCALE_BIADAPT    | \
		  TFB_GFXFLAGS_SCALE_BIADAPTADV | \
		  TFB_GFXFLAGS_SCALE_TRISCAN    | \
		  TFB_GFXFLAGS_SCALE_HQXX )
#define TFB_GFXFLAGS_SCALE_SOFT_ONLY \
		( TFB_GFXFLAGS_SCALE_ANY & ~TFB_GFXFLAGS_SCALE_BILINEAR )

// The flag variable itself
extern int GfxFlags;

void TFB_PreInit (void);
int TFB_InitGraphics (int driver, int flags, int width, int height);
void TFB_UninitGraphics (void);
void TFB_ProcessEvents (void);

// 3DO Graphics Stuff

#define GSCALE_IDENTITY 256

void LoadIntoExtraScreen (PRECT r);
void DrawFromExtraScreen (PRECT r);
void SetGraphicGrabOther (int grab_other);
void SetGraphicScale (int scale);
int  GetGraphicScale (void);
void SetGraphicUseOtherExtra (int other);
void SetTransitionSource (PRECT pRect);
void ScreenTransition (int transition, PRECT pRect);

extern float FrameRate;
extern int FrameRateTickBase;

void TFB_FlushGraphics (void); // Only call from main thread!!

void TFB_SetGamma (float gamma);
// Unknown Stuff

extern int ScreenWidth;
extern int ScreenHeight;
extern int ScreenWidthActual;
extern int ScreenHeightActual;
extern int ScreenColorDepth;
extern int GraphicsDriver;

#include "cmap.h"

#endif
