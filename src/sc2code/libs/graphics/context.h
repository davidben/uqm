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

#ifndef _CONTEXT_H
#define _CONTEXT_H

#ifdef MAPPING
typedef struct
{
	MAP_TYPE MapType;
	EXTENT ViewExt; /* Device units per inch */
	EXTENT WinExt; /* Logical units per inch */
	POINT WinOrg;
						/* Translation to device units relative to
						 * (0, 0) is as follows:
						 * x_du = (x_lu - WinOrg.x) * ViewExt.x / WinExt.x;
						 * y_du = (y_lu - WinOrg.y) * ViewExt.y / WinExt.y;
						 */
} MAP_INFO;
typedef MAP_INFO *PMAP_INFO;
#endif /* MAPPING */

typedef struct
{
	CONTEXT_REF ContextRef;
	UWORD Flags;

	DRAW_STATE DrawState;
	COLOR ForeGroundColor, BackGroundColor;
	FRAME ForeGroundFrame, BackGroundFrame;
	void (**func_array) (PRECT pClipRect, PPRIMITIVE PrimPtr);
	FONT Font;
	BG_FUNC BackGroundFunc;

	RECT ClipRect;
#ifdef MAPPING
	MAP_INFO Map;
#endif /* MAPPING */
} CONTEXT_DESC;
typedef CONTEXT_DESC *PCONTEXT_DESC;

#define CONTEXT_PRIORITY DEFAULT_MEM_PRIORITY

#define CONTEXTPTR PCONTEXT_DESC

#define AllocContext() \
		(CONTEXT_REF)mem_allocate ((MEM_SIZE)sizeof (CONTEXT_DESC), \
		MEM_ZEROINIT | MEM_PRIMARY, CONTEXT_PRIORITY, MEM_SIMPLE)
#define LockContext (CONTEXTPTR)mem_lock
#define UnlockContext mem_unlock
#define FreeContext mem_release

extern CONTEXTPTR _pCurContext;
extern PRIMITIVE _locPrim;

#define _get_context_fg_color() (_pCurContext->ForeGroundColor)
#define _get_context_bg_color() (_pCurContext->BackGroundColor)
#define _get_context_draw_state() (_pCurContext->DrawState)
#define _get_context_draw_dest() ((DRAW_DESTINATION)(_pCurContext->DrawState & 0xFF00))
#define _get_context_draw_mode() ((DRAW_MODE)LOBYTE (_pCurContext->DrawState))
#define _get_context_flags() (_pCurContext->Flags)
#define _get_context_fg_frame() (_pCurContext->ForeGroundFrame)
#define _get_context_bg_frame() (_pCurContext->BackGroundFrame)
#define _get_context_font() (_pCurContext->Font)
#define _get_context_bg_func() (_pCurContext->BackGroundFunc)

#ifdef MAPPING
#define _get_context_map_type() (_pCurContext->Map.MapType)
#define _init_context_map(pmt) \
	COORD worg_x, worg_y; \
	SIZE vext_w, vext_h, wext_w, wext_h; \
	\
	if ((*(pmt) = _get_context_map_type ()) != MAP_NOXFORM) \
		worg_x = _pCurContext->Map.WinOrg.x, \
		worg_y = _pCurContext->Map.WinOrg.y, \
		vext_w = _pCurContext->Map.ViewExt.width, \
		vext_h = _pCurContext->Map.ViewExt.height, \
		wext_w = _pCurContext->Map.WinExt.width, \
		wext_h = _pCurContext->Map.WinExt.height
#define _uninit_context_map()
#define LXtoDX(x) ((((x) - worg_x) * vext_w) / wext_w)
#define LYtoDY(y) ((((y) - worg_y) * vext_h) / wext_h)
#define DXtoLX(x) ((((x) * wext_w) / vext_w) + worg_x)
#define DYtoLY(y) ((((y) * wext_h) / vext_h) + worg_y)
#define SwitchContextMapType(t) \
{ \
	_pCurContext->Map.MapType = (t); \
}
#define SwitchContextWinOrigin(org) (_pCurContext->Map.WinOrg = *(org))
#define SwitchContextViewExtents(ext) (_pCurContext->Map.ViewExt = *(ext))
#define SwitchContextWinExtents(ext) (_pCurContext->Map.WinExt = *(ext))
#endif /* MAPPING */

#define SwitchContextDrawState(s) \
{ \
	_pCurContext->DrawState = (s); \
}
#define SwitchContextForeGroundColor(c) \
{ \
	_pCurContext->ForeGroundColor = (c); \
}
#define SwitchContextBackGroundColor(c) \
{ \
	_pCurContext->BackGroundColor = (c); \
}
#define SetContextFlags(f) \
{ \
	_pCurContext->Flags |= (f); \
}
#define UnsetContextFlags(f) \
{ \
	_pCurContext->Flags &= ~(f); \
}
#define SwitchContextFGFrame(f) \
{ \
	_pCurContext->ForeGroundFrame = (f); \
}
#define SwitchContextBGFrame(f) \
{ \
	_pCurContext->BackGroundFrame = (f); \
}
#define SwitchContextFont(f) \
{ \
	_pCurContext->Font = (f); \
}
#define SwitchContextBGFunc(f) \
{ \
	_pCurContext->BackGroundFunc = (f); \
}

/*
#define BATCH_BUILD_PAGE (BATCH_FLAGS)(1 << 0)
#define BATCH_SINGLE (BATCH_FLAGS)(1 << 1)
#define BATCH_UPDATE_DRAWABLE (BATCH_FLAGS)(1 << 2)
*/
#define BATCH_CLIP_GRAPHICS (BATCH_FLAGS)(1 << 3)
#define BATCH_XFORM (BATCH_FLAGS)(1 << 4)
#define BATCH_DIRTY_TILES (BATCH_FLAGS)(1 << 7)

typedef BYTE GRAPHICS_STATUS;

extern GRAPHICS_STATUS _GraphicsStatusFlags;
#define GRAPHICS_ACTIVE (GRAPHICS_STATUS)(1 << 0)
#define GRAPHICS_VISIBLE (GRAPHICS_STATUS)(1 << 1)
#define CONTEXT_ACTIVE (GRAPHICS_STATUS)(1 << 2)
#define DRAWABLE_ACTIVE (GRAPHICS_STATUS)(1 << 3)
#define DISPLAY_ACTIVE (GRAPHICS_STATUS)(1 << 5)
#define DeactivateGraphics() (_GraphicsStatusFlags &= ~GRAPHICS_ACTIVE)
#define ActivateGraphics() (_GraphicsStatusFlags |= GRAPHICS_ACTIVE)
#define GraphicsActive() (_GraphicsStatusFlags & GRAPHICS_ACTIVE)
#define DeactivateVisible() (_GraphicsStatusFlags &= ~GRAPHICS_VISIBLE)
#define ActivateVisible() (_GraphicsStatusFlags |= GRAPHICS_VISIBLE)
#define DeactivateDisplay() (_GraphicsStatusFlags &= ~DISPLAY_ACTIVE)
#define ActivateDisplay() (_GraphicsStatusFlags |= DISPLAY_ACTIVE)
#define DisplayActive() (_GraphicsStatusFlags & DISPLAY_ACTIVE)
#define DeactivateContext() (_GraphicsStatusFlags &= ~CONTEXT_ACTIVE)
#define ActivateContext() (_GraphicsStatusFlags |= CONTEXT_ACTIVE)
#define ContextActive() (_GraphicsStatusFlags & CONTEXT_ACTIVE)
#define DeactivateDrawable() (_GraphicsStatusFlags &= ~DRAWABLE_ACTIVE)
#define ActivateDrawable() (_GraphicsStatusFlags |= DRAWABLE_ACTIVE)
#define DrawableActive() (_GraphicsStatusFlags & DRAWABLE_ACTIVE)

#define SYSTEM_ACTIVE (GRAPHICS_STATUS)( \
									DISPLAY_ACTIVE | CONTEXT_ACTIVE | \
									DRAWABLE_ACTIVE \
								)
#define GraphicsSystemActive() \
		((_GraphicsStatusFlags & SYSTEM_ACTIVE) == SYSTEM_ACTIVE)
#define GraphicsStatus() \
		(_GraphicsStatusFlags & (GRAPHICS_STATUS)(GRAPHICS_ACTIVE \
												| GRAPHICS_VISIBLE))

#endif /* _CONTEXT_H */

