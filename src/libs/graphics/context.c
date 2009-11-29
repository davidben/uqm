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

GRAPHICS_STATUS _GraphicsStatusFlags;
CONTEXT _pCurContext;

#ifdef DEBUG
// We keep track of all contexts
CONTEXT firstContext;
		// The first one in the list.
CONTEXT *contextEnd = &firstContext;
		// Where to put the next context.
#endif

PRIMITIVE _locPrim;

FONT _CurFontPtr;

CONTEXT
SetContext (CONTEXT Context)
{
	CONTEXT LastContext;

	LastContext = _pCurContext;
	if (Context != LastContext)
	{
		if (LastContext)
		{
			UnsetContextFlags (
					MAKE_WORD (0, GRAPHICS_ACTIVE | DRAWABLE_ACTIVE));
			SetContextFlags (
					MAKE_WORD (0, _GraphicsStatusFlags
							& (GRAPHICS_ACTIVE | DRAWABLE_ACTIVE)));

			DeactivateContext ();
		}

		_pCurContext = Context;
		if (_pCurContext)
		{
			ActivateContext ();

			_GraphicsStatusFlags &= ~(GRAPHICS_ACTIVE | DRAWABLE_ACTIVE);
			_GraphicsStatusFlags |= HIBYTE (_get_context_flags ());

			SetPrimColor (&_locPrim, _get_context_fg_color ());

			_CurFramePtr = _get_context_fg_frame ();
			_CurFontPtr = _get_context_font ();
		}
	}

	return (LastContext);
}

#ifdef DEBUG
CONTEXT
CreateContextAux (const char *name)
#else  /* if !defined(DEBUG) */
CONTEXT
CreateContextAux (void)
#endif  /* !defined(DEBUG) */
{
	CONTEXT NewContext;

	NewContext = AllocContext ();
	if (NewContext)
	{
		/* initialize context */
		CONTEXT OldContext;

#ifdef DEBUG
		NewContext->name = name;
		NewContext->next = NULL;
		*contextEnd = NewContext;
		contextEnd = &NewContext->next;
#endif  /* DEBUG */

		OldContext = SetContext (NewContext);
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));
		SetContextBackGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00));
		SetContextClipping (TRUE);
		SetContext (OldContext);
	}

	return NewContext;
}

#ifdef DEBUG
// Loop through the list of context to the pointer which points to the
// specified context. This is either 'firstContext' or the address of
// the 'next' field of some other context.
static CONTEXT *
FindContextPtr (CONTEXT context) {
	CONTEXT *ptr;
	
	for (ptr = &firstContext; *ptr != NULL; ptr = &(*ptr)->next) {
		if (*ptr == context)
			break;
	}
	return ptr;
}
#endif  /* DEBUG */

BOOLEAN
DestroyContext (CONTEXT ContextRef)
{
	if (ContextRef == 0)
		return (FALSE);

	if (_pCurContext && _pCurContext == ContextRef)
		SetContext ((CONTEXT)0);

#ifdef DEBUG
	// Unlink the context.
	{
		CONTEXT *contextPtr = FindContextPtr (ContextRef);
		if (contextEnd == &ContextRef->next)
			contextEnd = contextPtr;
		*contextPtr = ContextRef->next;
	}
#endif  /* DEBUG */

	FreeContext (ContextRef);
	return TRUE;
}

Color
SetContextForeGroundColor (Color color)
{
	Color oldColor;

	if (!ContextActive ())
		return (BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));

	oldColor = _get_context_fg_color ();
	if (!sameColor(oldColor, color))
	{
		SwitchContextForeGroundColor (color);

		if (!(_get_context_fbk_flags () & FBK_IMAGE))
		{
			SetContextFBkFlags (FBK_DIRTY);
		}
	}
	SetPrimColor (&_locPrim, color);

	return (oldColor);
}

Color
GetContextForeGroundColor (void)
{
	if (!ContextActive ())
		return (BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));

	return _get_context_fg_color ();
}

Color
SetContextBackGroundColor (Color color)
{
	Color oldColor;

	if (!ContextActive ())
		return (BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00));

	oldColor = _get_context_bg_color ();
	if (!sameColor(oldColor, color))
		SwitchContextBackGroundColor (color);

	return oldColor;
}

Color
GetContextBackGroundColor (void)
{
	if (!ContextActive ())
		return (BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00));

	return _get_context_bg_color ();
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
SetContextClipRect (RECT *lpRect)
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
GetContextClipRect (RECT *lpRect)
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
		FRAME EffectFrame = _get_context_fonteff ();
		
		TFB_DrawImage_Image (EffectFrame->image,
				-EffectFrame->HotSpot.x, -EffectFrame->HotSpot.y,
				0, NULL, img);
	}
	else
	{	// solid color backing
		RECT r = { {0, 0}, {w, h} };
		Color color = _get_context_fg_color ();

		TFB_DrawImage_Rect (&r, color, img);
	}
	
	_pCurContext->FontBacking = img;
	UnsetContextFBkFlags (FBK_DIRTY);
}

#ifdef DEBUG
const char *
GetContextName (CONTEXT context)
{
	return context->name;
}

CONTEXT
GetFirstContext (void)
{
	return firstContext;
}

CONTEXT
GetNextContext (CONTEXT context)
{
	return context->next;
}
#endif  /* DEBUG */

