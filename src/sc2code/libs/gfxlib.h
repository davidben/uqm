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

#include "memlib.h"

#define CONTEXT PVOID
#define FRAME PVOID
#define FONT PVOID
#define CYCLE_REF DWORD

typedef CONTEXT *PCONTEXT;
typedef FRAME *PFRAME;
typedef FONT *PFONT;

typedef UWORD TIME_VALUE;
typedef TIME_VALUE *PTIME_VALUE;

#define TIME_SHIFT 8
#define MAX_TIME_VALUE ((1 << TIME_SHIFT) + 1)

typedef SWORD COORD;
typedef COORD *PCOORD;

typedef DWORD COLOR;
typedef COLOR *PCOLOR;

#define BUILD_COLOR(c32k,c256) \
	(COLOR)(((DWORD)(c32k)<<8)|(BYTE)(c256))
#define COLOR_32k(c) (UWORD)((COLOR)(c)>>8)
#define COLOR_256(c) LOBYTE((COLOR)c)
#define MAKE_RGB15(r,g,b) (UWORD)(((r)<<10)|((g)<<5)|(b))

typedef BYTE CREATE_FLAGS;
#define WANT_MASK (CREATE_FLAGS)(1 << 0)
#define WANT_PIXMAP (CREATE_FLAGS)(1 << 1)
#define MAPPED_TO_DISPLAY (CREATE_FLAGS)(1 << 2)

typedef enum
{
	MAP_ISOTROPIC, /* == X/Y scale is the same */
	MAP_ANISOTROPIC, /* == X/Y scale different (e.g. aspect ratio) */
	MAP_NOXFORM /* == X/Y scale is ignored */
} MAP_TYPE;

typedef enum
{
	DEST_MASK = MAKE_WORD (0, 0),
	DEST_PIXMAP = MAKE_WORD (0, 1)
} DRAW_DESTINATION;

typedef enum
{
	DRAW_SUBTRACTIVE = 0, /* implied only to active page */
	DRAW_ADDITIVE, /* implied only to active page */
	DRAW_REPLACE
} DRAW_MODE;

typedef UWORD DRAW_STATE;

typedef struct extent
{
	COORD width, height;
} EXTENT;
typedef EXTENT *PEXTENT;

typedef struct point
{
	COORD x, y;
} POINT;
typedef POINT *PPOINT;

typedef struct stamp
{
	POINT origin;
	FRAME frame;
} STAMP;
typedef STAMP *PSTAMP;

typedef struct rect
{
	POINT corner;
	EXTENT extent;
} RECT;
typedef RECT *PRECT;

typedef struct line
{
	POINT first, second;
} LINE;
typedef LINE *PLINE;

typedef enum
{
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT
} TEXT_ALIGN;

typedef struct text
{
	POINT baseline;
	UNICODE *pStr;
	TEXT_ALIGN align;
	COUNT CharCount;
} TEXT;
typedef TEXT *PTEXT;

typedef MEM_HANDLE POLYREF;

typedef PVOID POLYGON;
typedef POLYGON *PPOLYGON;

#include "strlib.h"

typedef STRING_TABLE COLORMAP_REF;
typedef STRING COLORMAP;
typedef STRINGPTR COLORMAPPTR;

typedef struct stamp_cmap
{
	POINT origin;
	FRAME frame;
	COLORMAPPTR CMap;
} STAMP_CMAP;
typedef STAMP_CMAP *PSTAMP_CMAP;

typedef struct composite
{
	POINT origin;
	PFRAME FrameList;
	COUNT NumFrames;
} COMPOSITE;
typedef COMPOSITE *PCOMPOSITE;

enum gfx_object
{
	POINT_PRIM = 0,
	STAMP_PRIM,
	STAMPFILL_PRIM,
	LINE_PRIM,
	TEXT_PRIM,
	STAMPCMAP_PRIM,
	RECT_PRIM,
	RECTFILL_PRIM,
	POLY_PRIM,
	POLYFILL_PRIM,
	COMPOSITE_PRIM,
	COMPOSITEFILL_PRIM,

	NUM_PRIMS
};
typedef BYTE GRAPHICS_PRIM;

typedef union
{
   POINT Point;
   STAMP Stamp;
   LINE Line;
   TEXT Text;
   STAMP_CMAP StampCMap;
   RECT Rect;
   POLYGON Polygon;
   COMPOSITE Composite;
} PRIM_DESC;
typedef PRIM_DESC *PPRIM_DESC;

typedef DWORD PRIM_LINKS;

typedef struct
{
	PRIM_LINKS Links;
	DWORD TypeAndColor;
	PRIM_DESC Object;
} PRIMITIVE;
typedef PRIMITIVE *PPRIMITIVE;

#define END_OF_LIST ((COUNT)0xFFFF)

#define PRIM_COLOR_SHIFT 8
#define PRIM_COLOR_MASK ((DWORD)~0L << PRIM_COLOR_SHIFT)
#define PRIM_TYPE_MASK (~PRIM_COLOR_MASK)

#define GetPredLink(l) LOWORD(l)
#define GetSuccLink(l) HIWORD(l)
#define MakeLinks MAKE_DWORD
#define SetPrimLinks(pPrim,p,s) ((pPrim)->Links = MakeLinks (p, s))
#define GetPrimLinks(pPrim) ((pPrim)->Links)
#define SetPrimType(pPrim,t) \
		((pPrim)->TypeAndColor = ((pPrim)->TypeAndColor & PRIM_COLOR_MASK) \
				| ((t) & PRIM_TYPE_MASK))
#define GetPrimType(pPrim) \
		((GRAPHICS_PRIM)LOBYTE ((pPrim)->TypeAndColor))
#define SetPrimColor(pPrim,c) \
		((pPrim)->TypeAndColor = ((pPrim)->TypeAndColor & PRIM_TYPE_MASK) \
				| ((c) << PRIM_COLOR_SHIFT))
#define GetPrimColor(pPrim) \
		((COLOR)((pPrim)->TypeAndColor >> PRIM_COLOR_SHIFT))

typedef BYTE BATCH_FLAGS;

#define BATCH_BUILD_PAGE (BATCH_FLAGS)(1 << 0)
#define BATCH_SINGLE (BATCH_FLAGS)(1 << 1)

typedef struct
{
	TIME_VALUE last_time_val;
	POINT EndPoint;
	STAMP IntersectStamp;
} INTERSECT_CONTROL;
typedef INTERSECT_CONTROL *PINTERSECT_CONTROL;

typedef MEM_HANDLE CONTEXT_REF;
typedef CONTEXT_REF *PCONTEXT_REF;

typedef DWORD DRAWABLE;
typedef DRAWABLE *PDRAWABLE;

#define BUILD_DRAWABLE(h,i) ((DRAWABLE)MAKE_DWORD(h,i))

typedef MEM_HANDLE FONT_REF;
typedef FONT_REF *PFONT_REF;

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

typedef DWORD HOT_SPOT;
#define MAKE_HOT_SPOT MAKE_DWORD
#define GET_HOT_X(h) ((short)LOWORD(h))
#define GET_HOT_Y(h) ((short)HIWORD(h))

extern INTERSECT_CODE BoxIntersect (PRECT pr1, PRECT pr2, PRECT
		printer);
extern void BoxUnion (PRECT pr1, PRECT pr2, PRECT punion);

typedef void (*BG_FUNC) (PRECT pClipRect);

#endif /* _GFXLIB_H */


#ifndef _GFX_PROTOS
#define _GFX_PROTOS

extern BOOLEAN InitGraphics (int argc, char *argv[], COUNT
		KbytesRequired);
extern void UninitGraphics (void);

extern CONTEXT SetContext (CONTEXT Context);
extern CONTEXT CaptureContext (CONTEXT_REF ContextRef);
extern CONTEXT_REF ReleaseContext (CONTEXT Context);
extern MAP_TYPE SetContextMapType (MAP_TYPE MapType);
extern BOOLEAN SetContextWinOrigin (PPOINT pOrg);
extern BOOLEAN SetContextViewExtents (PEXTENT pExtent);
extern BOOLEAN SetContextWinExtents (PEXTENT pExtent);
extern BOOLEAN GetContextWinOrigin (PPOINT pOrg);
extern BOOLEAN GetContextViewExtents (PEXTENT pExtent);
extern BOOLEAN GetContextWinExtents (PEXTENT pExtent);
extern BOOLEAN LOGtoDEV (PPOINT pSrcPt, PPOINT pDstPt, COUNT NumPoints);
extern BOOLEAN DEVtoLOG (PPOINT pSrcPt, PPOINT pDstPt, COUNT NumPoints);
extern DRAW_STATE GetContextDrawState (void);
extern DRAW_STATE SetContextDrawState (DRAW_STATE DrawState);
extern COLOR SetContextForeGroundColor (COLOR Color);
extern COLOR SetContextBackGroundColor (COLOR Color);
extern FRAME SetContextFGFrame (FRAME Frame);
extern FRAME SetContextBGFrame (FRAME Frame);
extern BG_FUNC SetContextBGFunc (BG_FUNC BGFunc);
extern BOOLEAN SetContextClipping (BOOLEAN ClipStatus);
extern BOOLEAN SetContextClipRect (PRECT pRect);
extern BOOLEAN GetContextClipRect (PRECT pRect);
extern TIME_VALUE DrawablesIntersect (PINTERSECT_CONTROL pControl0,
		PINTERSECT_CONTROL pControl1, TIME_VALUE max_time_val);
extern void DrawStamp (PSTAMP pStamp);
extern void DrawFilledStamp (PSTAMP pStamp);
extern void DrawStampCMap (PSTAMP_CMAP pStampCmap);
extern void DrawPoint (PPOINT pPoint);
extern void DrawRectangle (PRECT pRect);
extern void DrawFilledRectangle (PRECT pRect);
extern void DrawLine (PLINE pLine);
extern void DrawText (PTEXT pText);
extern void DrawPolygon (POLYGON Polygon);
extern void DrawFilledPolygon (POLYGON Polygon);
extern void DrawBatch (PPRIMITIVE pBasePrim, PRIM_LINKS PrimLinks,
		BATCH_FLAGS BatchFlags);
extern void BatchGraphics (void);
extern void UnbatchGraphics (void);
extern void FlushGraphics (void);
extern void ClearDrawable (void);
extern CONTEXT_REF CreateContext (void);
extern BOOLEAN DestroyContext (CONTEXT_REF ContextRef);
extern DRAWABLE CreateDisplay (CREATE_FLAGS CreateFlags, PSIZE pwidth,
		PSIZE pheight);
extern DRAWABLE CreateDrawable (CREATE_FLAGS CreateFlags, SIZE width,
		SIZE height, COUNT num_frames);
extern DRAWABLE CopyDrawable (DRAWABLE Drawable);
extern BOOLEAN DestroyDrawable (DRAWABLE Drawable);
extern BOOLEAN GetFrameRect (FRAME Frame, PRECT pRect);

extern HOT_SPOT SetFrameHot (FRAME Frame, HOT_SPOT HotSpot);
extern HOT_SPOT GetFrameHot (FRAME Frame);
extern BOOLEAN InstallGraphicResTypes (COUNT cel_type, COUNT font_type);
extern DWORD LoadCelFile (PVOID pStr);
extern DWORD LoadFontFile (PVOID pStr);
extern DWORD LoadGraphicInstance (DWORD res);
extern DWORD LoadGraphic (DWORD res);
extern DRAWABLE LoadDisplayPixmap (PRECT area, FRAME frame);
extern FONT SetContextFont (FONT Font);
extern BOOLEAN DestroyFont (FONT_REF FontRef);
extern FONT CaptureFont (FONT_REF FontRef);
extern FONT_REF ReleaseFont (FONT Font);
extern BOOLEAN TextRect (PTEXT pText, PRECT pRect, PBYTE pdelta);
extern BOOLEAN GetContextFontLeading (PSIZE pheight);
extern COUNT GetFrameCount (FRAME Frame);
extern COUNT GetFrameIndex (FRAME Frame);
extern FRAME SetAbsFrameIndex (FRAME Frame, COUNT FrameIndex);
extern FRAME SetRelFrameIndex (FRAME Frame, SIZE FrameOffs);
extern FRAME SetEquFrameIndex (FRAME DstFrame, FRAME SrcFrame);
extern FRAME IncFrameIndex (FRAME Frame);
extern FRAME DecFrameIndex (FRAME Frame);

extern void XFlipFrame (FRAME Frame);
extern FRAME CaptureDrawable (DRAWABLE Drawable);

extern DRAWABLE ReleaseDrawable (FRAME Frame);
extern MEM_HANDLE GetFrameHandle (FRAME Frame);

extern POLYREF CreatePolygon (COUNT PtCount);
extern POLYGON CapturePolygon (POLYREF PolyRef);
extern BOOLEAN SetPolygonPoint (POLYGON Polygon, COUNT PtIndex, COORD x,
		COORD y);
extern BOOLEAN GetPolygonPoint (POLYGON Polygon, COUNT PtIndex, PCOORD
		px, PCOORD py);
extern POLYREF ReleasePolygon (POLYGON Polygon);
extern BOOLEAN DestroyPolygon (POLYREF PolyRef);

extern DRAWABLE CreateOverlayWindow (CREATE_FLAGS CreateFlags, FRAME
		Display, SIZE height);
extern void ShowOverlayWin (FRAME WinFrame, COUNT TimeInterval);
extern void HideOverlayWin (FRAME WinFrame, COUNT TimeInterval);
extern BOOLEAN DestroyOverlayWindow (FRAME Display, DRAWABLE Drawable);
extern void ScrollDisplay (FRAME Display, COORD x, COORD y);

extern BOOLEAN ReadColorMap (COLORMAPPTR ColorMapPtr);
extern BOOLEAN SetColorMap (COLORMAPPTR ColorMapPtr, int type);
extern BOOLEAN BatchColorMap (COLORMAPPTR ColorMapPtr);
extern CYCLE_REF CycleColorMap (COLORMAPPTR ColorMapPtr, COUNT Cycles,
		SIZE TimeInterval);
extern void StopCycleColorMap (CYCLE_REF CycleRef);
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
#define GetColorMapLength GetStringLength
#define GetColorMapAddress GetStringAddress
#define GetColorMapContents GetStringContents

#endif /* _GFX_PROTOS */

