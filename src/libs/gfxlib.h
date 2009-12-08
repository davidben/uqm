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

typedef struct Color Color;
struct Color {
	BYTE r;
	BYTE g;
	BYTE b;
	BYTE a;  // Currently unused
};

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

static inline bool
sameColor(Color c1, Color c2)
{
	return c1.r == c2.r &&
			c1.g == c2.g &&
			c1.b == c2.b &&
			c1.a == c2.a;
}

// Transform a 5-bits color component to an 8-bits color component.
// Form 1, calculates '(r5 / 31.0) * 255.0, highest value is 0xff:
#define CC5TO8(c) (((c) << 3) | ((c) >> 2))
// Form 2, calculates '(r5 / 32.0) * 256.0, highest value is 0xf8:
//#define CC5TO8(c) ((c) << 3)

#define BUILD_COLOR(col, c256) col
		// BUILD_COLOR used to combine a 15-bit RGB color tripple with a
		// destination VGA palette index into a 32-bit value.
		// Now, it is an empty wrapper which returns the first argument,
		// which is of type Color, and ignores the second argument,
		// the palette index.
		//
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

// Turn a 15 bits color into a 24-bits color.
// r, g, and b are each 5-bits color components.
static inline Color
colorFromRgb15 (BYTE r, BYTE g, BYTE b)
{
	Color c;
	c.r = CC5TO8 (r);
	c.g = CC5TO8 (g);
	c.b = CC5TO8 (b);
	c.a = 0xff;

	return c;
}
#define MAKE_RGB15(r, g, b) colorFromRgb15 ((r), (g), (b))

#ifdef NOTYET  /* Need C'99 support */
#define MAKE_RGB15(r, g, b) (Color) { \
		.r = CC5TO8 (r), \
		.g = CC5TO8 (g), \
		.b = CC5TO8 (b), \
		.a = 0xff \
	}
#endif

// Temporary, until we can use C'99 features. Then MAKE_RGB15 will be usable
// anywhere.
// This define is intended for global initialisations, where the
// expression must be constant.
#define MAKE_RGB15_INIT(r, g, b) { \
		CC5TO8 (r), \
		CC5TO8 (g), \
		CC5TO8 (b), \
		0xff \
	}

static inline Color
buildColorRgba (BYTE r, BYTE g, BYTE b, BYTE a)
{
	Color c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = a;

	return c;
}
#define BUILD_COLOR_RGBA(r, g, b, a) \
		buildColorRgba ((r), (g), (b), (a))


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

static inline POINT
MAKE_POINT (COORD x, COORD y)
{
	POINT pt = {x, y};
	return pt;
}

static inline bool
pointsEqual (POINT p1, POINT p2)
{
	return p1.x == p2.x && p1.y == p2.y;
}

static inline bool
extentsEqual (EXTENT e1, EXTENT e2)
{
	return e1.width == e2.width && e1.height == e2.height;
}

static inline bool
rectsEqual (RECT r1, RECT r2)
{
	return pointsEqual (r1.corner, r2.corner)
			&& extentsEqual (r1.extent, r2.extent);
}

static inline bool
pointWithinRect (RECT r, POINT p)
{
	return p.x >= r.corner.x && p.y >= r.corner.y
			&& p.x < r.corner.x + r.extent.width
			&& p.y < r.corner.y + r.extent.height;
}

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
// This flag is currently unused but it might make sense to restore it
#define BATCH_BUILD_PAGE (BATCH_FLAGS)(1 << 0)

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
extern Color SetContextForeGroundColor (Color Color);
extern Color GetContextForeGroundColor (void);
extern Color SetContextBackGroundColor (Color Color);
extern Color GetContextBackGroundColor (void);
extern FRAME SetContextFGFrame (FRAME Frame);
extern FRAME GetContextFGFrame (void);
// Context cliprect defines the drawing bounds. Additionally, all
// drawing positions (x,y) are relative to the cliprect corner.
extern BOOLEAN SetContextClipRect (RECT *pRect);
// The returned rect is always filled in. If the context cliprect
// is undefined, the returned rect has foreground frame dimensions.
extern BOOLEAN GetContextClipRect (RECT *pRect);
// The actual origin will be orgOffset + context ClipRect.corner
extern POINT SetContextOrigin (POINT orgOffset);

extern TIME_VALUE DrawablesIntersect (INTERSECT_CONTROL *pControl0,
		INTERSECT_CONTROL *pControl1, TIME_VALUE max_time_val);
extern void DrawStamp (STAMP *pStamp);
extern void DrawFilledStamp (STAMP *pStamp);
extern void DrawPoint (POINT *pPoint);
extern void DrawRectangle (RECT *pRect);
extern void DrawFilledRectangle (RECT *pRect);
extern void DrawLine (LINE *pLine);
extern void font_DrawText (TEXT *pText);
extern void font_DrawTracedText (TEXT *pText, Color text, Color trace);
extern void DrawBatch (PRIMITIVE *pBasePrim, PRIM_LINKS PrimLinks,
		BATCH_FLAGS BatchFlags);
extern void BatchGraphics (void);
extern void UnbatchGraphics (void);
extern void FlushGraphics (void);
extern void ClearDrawable (void);
#ifdef DEBUG
extern CONTEXT CreateContextAux (const char *name);
#define CreateContext(name) CreateContextAux((name))
#else  /* if !defined(DEBUG) */
extern CONTEXT CreateContextAux (void);
#define CreateContext(name) CreateContextAux()
#endif  /* !defined(DEBUG) */
extern BOOLEAN DestroyContext (CONTEXT ContextRef);
extern DRAWABLE CreateDisplay (CREATE_FLAGS CreateFlags, SIZE *pwidth,
		SIZE *pheight);
extern DRAWABLE CreateDrawable (CREATE_FLAGS CreateFlags, SIZE width,
		SIZE height, COUNT num_frames);
extern BOOLEAN DestroyDrawable (DRAWABLE Drawable);
extern BOOLEAN GetFrameRect (FRAME Frame, RECT *pRect);
#ifdef DEBUG
extern const char *GetContextName (CONTEXT context);
extern CONTEXT GetFirstContext (void);
extern CONTEXT GetNextContext (CONTEXT context);
extern size_t GetContextCount (void);
#endif  /* DEBUG */

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
// The returned pRect is relative to the context drawing origin
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
extern DRAWABLE RotateFrame (FRAME Frame, int angle_deg);
extern void SetFrameTransparentColor (FRAME Frame, Color c32k);

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

void SetSystemRect (const RECT *pRect);
void ClearSystemRect (void);

#endif /* _GFXLIB_H */
