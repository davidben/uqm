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

#include "gfxintrn.h"
#include "gfxother.h"

GRAPHICS_STATUS _GraphicsStatusFlags;
CONTEXTPTR _pCurContext;

PRIMITIVE _locPrim;

FONTPTR _CurFontPtr;

CONTEXT
SetContext (CONTEXT Context)
{
	CONTEXT LastContext;

	LastContext = (CONTEXT)_pCurContext;
	if (Context != LastContext)
	{
		if (LastContext)
		{
			UnsetContextFlags (
					MAKE_WORD (0, GRAPHICS_ACTIVE | DRAWABLE_ACTIVE)
					);
			SetContextFlags (
					MAKE_WORD (0, _GraphicsStatusFlags
							& (GRAPHICS_ACTIVE | DRAWABLE_ACTIVE))
					);

			DeactivateContext ();
		}

		_pCurContext = (CONTEXTPTR)Context;
		if (_pCurContext)
		{
			ActivateContext ();

			_GraphicsStatusFlags &= ~(GRAPHICS_ACTIVE | DRAWABLE_ACTIVE);
			_GraphicsStatusFlags |= HIBYTE (_get_context_flags ());

			SetPrimColor (&_locPrim, _get_context_fg_color ());

			_CurFramePtr = (FRAMEPTR)_get_context_fg_frame ();
			_CurFontPtr = (FONTPTR)_get_context_font ();
		}
	}

	return (LastContext);
}

CONTEXT_REF
CreateContext (void)
{
	CONTEXT_REF ContextRef;

	ContextRef = AllocContext ();
	if (ContextRef)
	{
		CONTEXT OldContext;

				/* initialize context */
		OldContext = SetContext (CaptureContext (ContextRef));
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));
		SetContextBackGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00));
		SetContextClipping (TRUE);
		ReleaseContext (SetContext (OldContext));
	}

	return (ContextRef);
}

BOOLEAN
DestroyContext (CONTEXT_REF ContextRef)
{
	if (ContextRef == 0)
		return (FALSE);

	if (_pCurContext && _pCurContext->ContextRef == ContextRef)
		SetContext ((CONTEXT)0);

	return (FreeContext (ContextRef));
}

CONTEXT
CaptureContext (CONTEXT_REF ContextRef)
{
	CONTEXTPTR ContextPtr;

	ContextPtr = LockContext (ContextRef);
	if (ContextPtr)
		ContextPtr->ContextRef = ContextRef;

	return ((CONTEXT)ContextPtr);
}

CONTEXT_REF
ReleaseContext (CONTEXT Context)
{
	CONTEXTPTR ContextPtr;

	ContextPtr = (CONTEXTPTR)Context;
	if (ContextPtr)
	{
		CONTEXT_REF ContextRef;

		ContextRef = ContextPtr->ContextRef;
		UnlockContext (ContextRef);

		return (ContextRef);
	}

	return (0);
}

COLOR
SetContextForeGroundColor (COLOR Color)
{
	COLOR oldColor;

	if (!ContextActive ())
		return (BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));

	if ((oldColor = _get_context_fg_color ()) != Color)
	{
		SwitchContextForeGroundColor (Color);

		if (!(_get_context_fbk_flags () & FBK_IMAGE))
		{
			SetContextFBkFlags (FBK_DIRTY);
		}
	}
	SetPrimColor (&_locPrim, Color);

	return (oldColor);
}

COLOR
SetContextBackGroundColor (COLOR Color)
{
	COLOR oldColor;

	if (!ContextActive ())
		return (BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00));

	if ((oldColor = _get_context_bg_color ()) != Color)
	{
		SwitchContextBackGroundColor (Color);
	}

	return (oldColor);
}

BOOLEAN
SetContextClipping (BOOLEAN ClipStatus)
{
	BOOLEAN oldClipStatus;

	if (!ContextActive ())
		return (TRUE);

	oldClipStatus = (_get_context_flags () & BATCH_CLIP_GRAPHICS) != 0;
	if (ClipStatus)
	{
		SetContextFlags (BATCH_CLIP_GRAPHICS);
	}
	else
	{
		UnsetContextFlags (BATCH_CLIP_GRAPHICS);
	}

	return (oldClipStatus);
}

BOOLEAN
SetContextClipRect (PRECT lpRect)
{
	if (!ContextActive ())
		return (FALSE);

	if (lpRect)
		_pCurContext->ClipRect = *lpRect;
	else
		_pCurContext->ClipRect.extent.width = 0;

	return (TRUE);
}

BOOLEAN
GetContextClipRect (PRECT lpRect)
{
	if (!ContextActive ())
		return (FALSE);

	*lpRect = _pCurContext->ClipRect;
	return (lpRect->extent.width != 0);
}


FRAME
SetContextFontEffect (FRAME EffectFrame)
{
	FRAME LastEffect;

	if (!ContextActive ())
		return (NULL);

	LastEffect = _get_context_fonteff ();
	if (EffectFrame != LastEffect)
	{
		SwitchContextFontEffect (EffectFrame);

		if (EffectFrame != 0)
		{
			SetContextFBkFlags (FBK_IMAGE);
		}
		else
		{
			UnsetContextFBkFlags (FBK_IMAGE);
		}
	}

	return LastEffect;
}

void
FixContextFontEffect (void)
{
	SIZE w, h;
	TFB_Image* img;

	if (!ContextActive () || (_get_context_font_backing () != 0
			&& !(_get_context_fbk_flags () & FBK_DIRTY)))
		return;

	if (!GetContextFontLeading (&h) || !GetContextFontLeadingWidth (&w))
		return;

	img = _pCurContext->FontBacking;
	if (img)
		TFB_DrawScreen_DeleteImage (img);

	img = TFB_DrawImage_CreateForScreen (w, h, TRUE);
	if (_get_context_fbk_flags () & FBK_IMAGE)
	{	// image pattern backing
		FRAMEPTR EffectFrame = (FRAMEPTR)_get_context_fonteff ();
		
		TFB_DrawImage_Image (EffectFrame->image,
				-EffectFrame->HotSpot.x, -EffectFrame->HotSpot.y,
				0, NULL, img);
	}
	else
	{	// solid color backing
		TFB_Palette color;
		RECT r = { {0, 0}, {w, h} };

		COLORtoPalette (_get_context_fg_color (), &color);
		TFB_DrawImage_Rect (&r, color.r, color.g, color.b, img);
	}
	
	_pCurContext->FontBacking = img;
	UnsetContextFBkFlags (FBK_DIRTY);
}
