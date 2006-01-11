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

#ifndef TFB_DRAW_H
#define TFB_DRAW_H

#include "libs/threadlib.h"


typedef void *TFB_Canvas;

typedef enum {
	TFB_SCREEN_MAIN,
	TFB_SCREEN_EXTRA,
	TFB_SCREEN_TRANSITION,

	TFB_GFX_NUMSCREENS
} SCREEN;

typedef enum {
	TFB_SCALE_STEP, /* not really a scaler */
	TFB_SCALE_NEAREST,
	TFB_SCALE_TRILINEAR
} SCALE;

typedef struct tfb_palette
{
	UBYTE r;
	UBYTE g;
	UBYTE b;
	UBYTE unused;
} TFB_Palette;

#include "graphics/gfx_common.h"
#include "cmap.h"

typedef struct tfb_image
{
	TFB_Canvas NormalImg;
	TFB_Canvas ScaledImg;
	TFB_Canvas MipmapImg;
	TFB_Canvas FilledImg;
	TFB_Palette *Palette;
	int colormap_index;
	int colormap_version;
	HOT_SPOT NormalHs;
	HOT_SPOT MipmapHs;
	HOT_SPOT last_scale_hs;
	int last_scale_type;
	TFB_Palette last_fill;
	EXTENT extent;
	Mutex mutex;
	BOOLEAN dirty;
} TFB_Image;

typedef struct tfb_char
{
	EXTENT extent;
	EXTENT disp;
		// Display extent
	HOT_SPOT HotSpot;
	BYTE* data;
	DWORD pitch;
		// Pitch is for storing all chars of a page
		// in one rectangular pixel matrix
} TFB_Char;

// we do not support paletted format for now
typedef struct tfb_pixelformat
{
	int BitsPerPixel;
	int BytesPerPixel;
	DWORD Rmask, Gmask, Bmask, Amask;
	DWORD Rshift, Gshift, Bshift, Ashift;
	DWORD Rloss, Gloss, Bloss, Aloss;
} TFB_PixelFormat;

// Drawing commands

void TFB_DrawScreen_Line (int x1, int y1, int x2, int y2, int r, int g, int b, SCREEN dest);
void TFB_DrawScreen_Rect (PRECT rect, int r, int g, int b, SCREEN dest);
void TFB_DrawScreen_Image (TFB_Image *img, int x, int y, int scale, TFB_ColorMap *cmap, SCREEN dest);
void TFB_DrawScreen_Copy (PRECT r, SCREEN src, SCREEN dest);
void TFB_DrawScreen_FilledImage (TFB_Image *img, int x, int y, int scale, int r, int g, int b, SCREEN dest);
void TFB_DrawScreen_FontChar (TFB_Char *, TFB_Image *backing, int x, int y, SCREEN dest);

void TFB_DrawScreen_CopyToImage (TFB_Image *img, PRECT lpRect, SCREEN src);
void TFB_DrawScreen_DeleteImage (TFB_Image *img);
void TFB_DrawScreen_DeleteData (void *);
void TFB_DrawScreen_WaitForSignal (void);
void TFB_DrawScreen_ReinitVideo (int driver, int flags, int width, int height);
void TFB_DrawScreen_SetPalette (int paletteIndex, int r, int g, int b);

TFB_Image *TFB_DrawImage_New (TFB_Canvas canvas);
TFB_Image *TFB_DrawImage_CreateForScreen (int w, int h, BOOLEAN withalpha);
TFB_Image *TFB_DrawImage_New_Rotated (TFB_Image *img, int angle);
void TFB_DrawImage_Delete (TFB_Image *image);
void TFB_DrawImage_FixScaling (TFB_Image *image, int target, int type);

void TFB_DrawImage_Line (int x1, int y1, int x2, int y2, int r, int g, int b, TFB_Image *dest);
void TFB_DrawImage_Rect (PRECT rect, int r, int g, int b, TFB_Image *image);
void TFB_DrawImage_Image (TFB_Image *img, int x, int y, int scale, TFB_ColorMap *cmap, TFB_Image *target);
void TFB_DrawImage_FilledImage (TFB_Image *img, int x, int y, int scale, int r, int g, int b, TFB_Image *target);
void TFB_DrawImage_FontChar (TFB_Char *, TFB_Image *backing, int x, int y, TFB_Image *target);

TFB_Canvas TFB_DrawCanvas_New_TrueColor (int w, int h, BOOLEAN hasalpha);
TFB_Canvas TFB_DrawCanvas_New_ForScreen (int w, int h, BOOLEAN withalpha);
TFB_Canvas TFB_DrawCanvas_New_Paletted (int w, int h, TFB_Palette *palette, int transparent_index);
TFB_Canvas TFB_DrawCanvas_New_ScaleTarget (TFB_Canvas canvas, TFB_Canvas oldcanvas, int type, int last_type);
TFB_Canvas TFB_DrawCanvas_New_RotationTarget (TFB_Canvas src, int angle);
TFB_Canvas TFB_DrawCanvas_ToScreenFormat (TFB_Canvas canvas);
BOOLEAN TFB_DrawCanvas_IsPaletted (TFB_Canvas canvas);
void TFB_DrawCanvas_Rescale_Nearest (TFB_Canvas src, TFB_Canvas dst, EXTENT size);
void TFB_DrawCanvas_Rescale_Trilinear (TFB_Canvas src, TFB_Canvas dst, TFB_Canvas mipmap, EXTENT size);
void TFB_DrawCanvas_GetScaledExtent (TFB_Canvas src_canvas, HOT_SPOT src_hs,
		TFB_Canvas src_mipmap, HOT_SPOT mm_hs,
		int scale, PEXTENT size, HOT_SPOT *hs);
void TFB_DrawCanvas_Rotate (TFB_Canvas src, TFB_Canvas dst, int angle, EXTENT size);
void TFB_DrawCanvas_GetRotatedExtent (TFB_Canvas src, int angle, PEXTENT size);
void TFB_DrawCanvas_GetExtent (TFB_Canvas canvas, PEXTENT size);

void TFB_DrawCanvas_Delete (TFB_Canvas canvas);

void TFB_DrawCanvas_Line (int x1, int y1, int x2, int y2, int r, int g, int b, TFB_Canvas dest);
void TFB_DrawCanvas_Rect (PRECT rect, int r, int g, int b, TFB_Canvas image);
void TFB_DrawCanvas_Image (TFB_Image *img, int x, int y, int scale, TFB_ColorMap *cmap, TFB_Canvas target);
void TFB_DrawCanvas_FilledImage (TFB_Image *img, int x, int y, int scale, int r, int g, int b, TFB_Canvas target);
void TFB_DrawCanvas_FontChar (TFB_Char *, TFB_Image *backing, int x, int y, TFB_Canvas target);

TFB_Palette *TFB_DrawCanvas_ExtractPalette (TFB_Canvas canvas);
void TFB_DrawCanvas_SetPalette (TFB_Canvas target, TFB_Palette *palette);
int TFB_DrawCanvas_GetTransparentIndex (TFB_Canvas canvas);
void TFB_DrawCanvas_SetTransparentIndex (TFB_Canvas canvas, int i, BOOLEAN rleaccel);
BOOLEAN TFB_DrawCanvas_GetTransparentColor (TFB_Canvas canvas, int *r, int *g, int *b);
void TFB_DrawCanvas_SetTransparentColor (TFB_Canvas canvas, int r, int g, int b, BOOLEAN rleaccel);
void TFB_DrawCanvas_Initialize (void);
void TFB_DrawCanvas_GetScreenFormat (TFB_PixelFormat *fmt);
void* TFB_DrawCanvas_GetLine (TFB_Canvas canvas, int line);
void TFB_DrawCanvas_GetPixel (TFB_Canvas canvas, int x, int y, int *r, int *g, int *b);

#endif
