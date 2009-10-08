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

#ifndef _GFXLIB_H
#define _GFXLIB_H

#include "libs/compiler.h"
#include "libs/reslib.h"

typedef struct context_desc CONTEXT_DESC;
typedef struct frame_desc FRAME_DESC;
typedef struct font_desc FONT_DESC;
typedef struct drawable_desc DRAWABLE_DESC;

typedef CONTEXT_DESC *CONTEXT;
typedef FRAME_DESC *FRAME;
typedef FONT_DESC *FONT;
typedef DRAWABLE_DESC *DRAWABLE;

typedef UWORD TIME_VALUE;

#define TIME_SHIFT 8
#define MAX_TIME_VALUE ((1 << TIME_SHIFT) + 1)

typedef SWORD COORD;
typedef DWORD COLOR;

#define BUILD_COLOR(c32k,c256) \
	(COLOR)(((DWORD)(c32k)<<8)|(BYTE)(c256))
		// BUILD_COLOR combines a 15-bit RGB color tripple with a
		// destination VGA palette index into a 32-bit value.
		// It is a remnant of 8bpp hardware paletted display (VGA).
		// The palette index would be overwritten with the RGB value
		// and the drawing op would use this index on screen.
		// The palette indices 0-15, as used in DOS SC2, are unchanged
		// from the standard VGA palette and are identical to 16-color EGA.
		// Various frames, borders, menus, etc. frequently refer to these
		// first 16 colors and normally do not change the RGB values from
		// the standard ones (see colors.h; most likely unchanged from SC1)
		// The palette index is meaningless in UQM for the most part.
		// New code should just use index 0.
#define COLOR_32k(c) (UWORD)((COLOR)(c)>>8)
#define COLOR_256(c) LOBYTE((COLOR)c)
#define MAKE_RGB15(r,g,b) (UWORD)(((r)<<10)|((g)<<5)|(b))
#define BUILD_COLOR_RGBA(r,g,b,a) (DWORD)(((r)<<24)|((g)<<16)|((b)<<8)|(a))

typedef BYTE CREATE_FLAGS;
#define WANT_MASK (CREATE_FLAGS)(1 << 0)
#define WANT_PIXMAP (CREATE_FLAGS)(1 << 1)
#define MAPPED_TO_DISPLAY (CREATE_FLAGS)(1 << 2)
#define WANT_ALPHA (CREATE_FLAGS)(1 << 3)

typedef struct extent
{
	COORD width, height;
} EXTENT;

typedef struct point
{
	COORD x, y;
} POINT;

typedef struct stamp
{
	POINT origin;
	FRAME frame;
} STAMP;

typedef struct rect
{
	POINT corner;
	EXTENT extent;
} RECT;

typedef struct line
{
	POINT first, second;
} LINE;

typedef enum
{
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT
} TEXT_ALIGN;

typedef enum
{
	VALIGN_TOP,
	VALIGN_MIDDLE,
	VALIGN_BOTTOM
} TEXT_VALIGN;

typedef struct text
{
	POINT baseline;
	const UNICODE *pStr;
	TEXT_ALIGN align;
	COUNT CharCount;
} TEXT;

#include "strlib.h"

typedef STRING_TABLE COLORMAP_REF;
typedef STRING COLORMAP;
typedef STRINGPTR COLORMAPPTR;

#include "graphics/prim.h"

typedef BYTE BATCH_FLAGS;

#define BATCH_BUILD_PAGE (BATCH_FLAGS)(1 << 0)
#define BATCH_SINGLE (BATCH_FLAGS)(1 << 1)

typedef struct
{
	TIME_VALUE last_time_val;
	POINT EndPoint;
	STAMP IntersectStamp;
} INTERSECT_CONTROL;

typedef BYTE INTERSECT_CODE;

#define INTERSECT_LEFT (INTERSECT_CODE)(1 << 0)
#define INTERSECT_TOP (INTERSECT_CODE)(1 << 1)
#define INTERSECT_RIGHT (INTERSECT_CODE)(1 << 2)
#define INTERSECT_BOTTOM (INTERSECT_CODE)(1 << 3)
#define INTERSECT_NOCLIP (INTERSECT_CODE)(1 << 7)
#define INTERSECT_ALL_SIDES (INTERSECT_CODE)(INTERSECT_LEFT | \
								 INTERSECT_TOP | \
								 INTERSECT_RIGHT | \
								 INTERSECT_BOTTOM)

typedef POINT HOT_SPOT;

extern HOT_SPOT MAKE_HOT_SPOT (COORD, COORD);

extern INTERSECT_CODE BoxIntersect (RECT *pr1, RECT *pr2, RECT *printer);
extern void BoxUnion (RECT *pr1, RECT *pr2, RECT *punion);

enum
{
	FadeAllToWhite = 250,
	FadeSomeToWhite,
	FadeAllToBlack,
	FadeAllToColor,
	FadeSomeToBlack,
	FadeSomeToColor
};

extern BOOLEAN InitGraphics (int argc, char *argv[], COUNT
		KbytesRequired);
extern void UninitGraphics (void);

extern CONTEXT SetContext (CONTEXT Context);
extern COLOR SetContextForeGroundColor (COLOR Color);
extern COLOR SetContextBackGroundColor (COLOR Color);
extern FRAME SetContextFGFrame (FRAME Frame);
extern BOOLEAN SetContextClipping (BOOLEAN ClipStatus);
extern BOOLEAN SetContextClipRect (RECT *pRect);
extern BOOLEAN GetContextClipRect (RECT *pRect);
extern TIME_VALUE DrawablesIntersect (INTERSECT_CONTROL *pControl0,
		INTERSECT_CONTROL *pControl1, TIME_VALUE max_time_val);
extern void DrawStamp (STAMP *pStamp);
extern void DrawFilledStamp (STAMP *pStamp);
extern void DrawPoint (POINT *pPoint);
extern void DrawRectangle (RECT *pRect);
extern void DrawFilledRectangle (RECT *pRect);
extern void DrawLine (LINE *pLine);
extern void font_DrawText (TEXT *pText);
extern void font_DrawTracedText (TEXT *pText, COLOR text, COLOR trace);
extern void DrawBatch (PRIMITIVE *pBasePrim, PRIM_LINKS PrimLinks,
		BATCH_FLAGS BatchFlags);
extern void BatchGraphics (void);
extern void UnbatchGraphics (void);
extern void FlushGraphics (void);
extern void ClearBackGround (RECT *pClipRect);
extern void ClearDrawable (void);
extern CONTEXT CreateContext (void);
extern BOOLEAN DestroyContext (CONTEXT ContextRef);
extern DRAWABLE CreateDisplay (CREATE_FLAGS CreateFlags, SIZE *pwidth,
		SIZE *pheight);
extern DRAWABLE CreateDrawable (CREATE_FLAGS CreateFlags, SIZE width,
		SIZE height, COUNT num_frames);
extern BOOLEAN DestroyDrawable (DRAWABLE Drawable);
extern BOOLEAN GetFrameRect (FRAME Frame, RECT *pRect);

extern HOT_SPOT SetFrameHot (FRAME Frame, HOT_SPOT HotSpot);
extern HOT_SPOT GetFrameHot (FRAME Frame);
extern BOOLEAN InstallGraphicResTypes (void);
extern DRAWABLE LoadGraphicFile (const char *pStr);
extern FONT LoadFontFile (const char *pStr);
extern void *LoadGraphicInstance (RESOURCE res);
extern DRAWABLE LoadDisplayPixmap (RECT *area, FRAME frame);
extern FRAME SetContextFontEffect (FRAME EffectFrame);
extern FONT SetContextFont (FONT Font);
extern BOOLEAN DestroyFont (FONT FontRef);
extern BOOLEAN TextRect (TEXT *pText, RECT *pRect, BYTE *pdelta);
extern BOOLEAN GetContextFontLeading (SIZE *pheight);
extern BOOLEAN GetContextFontLeadingWidth (SIZE *pwidth);
extern COUNT GetFrameCount (FRAME Frame);
extern COUNT GetFrameIndex (FRAME Frame);
extern FRAME SetAbsFrameIndex (FRAME Frame, COUNT FrameIndex);
extern FRAME SetRelFrameIndex (FRAME Frame, SIZE FrameOffs);
extern FRAME SetEquFrameIndex (FRAME DstFrame, FRAME SrcFrame);
extern FRAME IncFrameIndex (FRAME Frame);
extern FRAME DecFrameIndex (FRAME Frame);
extern DRAWABLE RotateFrame (FRAME Frame, COUNT angle);
extern void SetFrameTransparentColor (FRAME Frame, COLOR c32k);

extern FRAME CaptureDrawable (DRAWABLE Drawable);
extern DRAWABLE ReleaseDrawable (FRAME Frame);

extern DRAWABLE GetFrameParentDrawable (FRAME Frame);

extern BOOLEAN SetColorMap (COLORMAPPTR ColorMapPtr);
extern DWORD XFormColorMap (COLORMAPPTR ColorMapPtr, SIZE TimeInterval);
extern void FlushColorXForms (void);
#define InitColorMapResources InitStringTableResources
#define LoadColorMapFile LoadStringTableFile
#define LoadColorMapInstance LoadStringTableInstance
#define CaptureColorMap CaptureStringTable
#define ReleaseColorMap ReleaseStringTable
#define DestroyColorMap DestroyStringTable
#define GetColorMapRef GetStringTable
#define GetColorMapCount GetStringTableCount
#define GetColorMapIndex GetStringTableIndex
#define SetAbsColorMapIndex SetAbsStringTableIndex
#define SetRelColorMapIndex SetRelStringTableIndex
#define GetColorMapLength GetStringLengthBin
#define GetColorMapAddress GetStringAddress
#define GetColorMapContents GetStringContents

void SetSystemRect (RECT *pRect);
void ClearSystemRect (void);

#endif /* _GFXLIB_H */
