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
#include "tfb_prim.h"
#include "gfxother.h"
#include "libs/log.h"

extern void FixContextFontEffect (void);
static inline TFB_Char *getCharFrame (FONT_DESC *fontPtr, wchar_t ch);

static BYTE char_delta_array[MAX_DELTAS];
		// XXX: This does not seem to be used.


FONT
SetContextFont (FONT Font)
{
	FONT LastFont;

	LastFont = (FONT)_CurFontPtr;
	_CurFontPtr = (FONTPTR)Font;
	if (ContextActive ())
		SwitchContextFont (Font);

	return (LastFont);
}

BOOLEAN
DestroyFont (FONT_REF FontRef)
{
	if (FontRef == 0)
		return (FALSE);

	if (_CurFontPtr && _CurFontPtr->FontRef == FontRef)
		SetContextFont ((FONT)0);

	return (FreeFont (FontRef));
}

FONT
CaptureFont (FONT_REF FontRef)
{
	FONTPTR FontPtr;

	FontPtr = LockFont (FontRef);
	if (FontPtr)
		FontPtr->FontRef = FontRef;

	return ((FONT)FontPtr);
}

FONT_REF
ReleaseFont (FONT Font)
{
	FONTPTR FontPtr;

	FontPtr = (FONTPTR)Font;
	if (FontPtr)
	{
		FONT_REF FontRef;

		FontRef = FontPtr->FontRef;
		UnlockFont (FontRef);

		return (FontRef);
	}

	return (0);
}

void
font_DrawText (PTEXT lpText)
{
	FixContextFontEffect ();
	SetPrimType (&_locPrim, TEXT_PRIM);
	_locPrim.Object.Text = *lpText;
	DrawBatch (&_locPrim, 0, BATCH_SINGLE);
}

BOOLEAN
GetContextFontLeading (PSIZE pheight)
{
	if (_CurFontPtr != 0)
	{
		*pheight = (SIZE)_CurFontPtr->Leading;
		return (TRUE);
	}

	*pheight = 0;
	return (FALSE);
}

BOOLEAN
GetContextFontLeadingWidth (PSIZE pwidth)
{
	if (_CurFontPtr != 0)
	{
		*pwidth = (SIZE)_CurFontPtr->LeadingWidth;
		return (TRUE);
	}

	*pwidth = 0;
	return (FALSE);
}

BOOLEAN
TextRect (PTEXT lpText, PRECT pRect, PBYTE pdelta)
{
	FONTPTR FontPtr;

	FontPtr = _CurFontPtr;
	if (FontPtr != 0 && lpText->CharCount != 0)
	{
		COORD top_y, bot_y;
		SIZE width;
		wchar_t next_ch;
		const unsigned char *pStr;
		COUNT num_chars;
	
		num_chars = lpText->CharCount;
		/* At this point lpText->CharCount contains the *maximum* number of
		 * characters that lpText->pStr may contain.
		 * After the while loop below, it will contain the actual number.
		 */
		if (pdelta == 0)
		{
			pdelta = char_delta_array;
			if (num_chars > MAX_DELTAS)
			{
				num_chars = MAX_DELTAS;
				lpText->CharCount = MAX_DELTAS;
			}
		}

		top_y = 0;
		bot_y = 0;
		width = 0;
		pStr = lpText->pStr;
		if (num_chars > 0)
		{
			next_ch = getCharFromString (&pStr);
			if (next_ch == '\0')
				num_chars = 0;
		}
		while (num_chars--)
		{
			wchar_t ch;
			SIZE last_width;
			TFB_Char *charFrame;

			last_width = width;

			ch = next_ch;
			if (num_chars > 0)
			{
				next_ch = getCharFromString (&pStr);
				if (next_ch == '\0')
				{
					lpText->CharCount -= num_chars;
					num_chars = 0;
				}
			}

			charFrame = getCharFrame (FontPtr, ch);
			if (charFrame != NULL && charFrame->disp.width)
			{
				COORD y;

				y = -charFrame->HotSpot.y;
				if (y < top_y)
					top_y = y;
				y += charFrame->disp.height;
				if (y > bot_y)
					bot_y = y;

				width += charFrame->disp.width;
#if 0
				if (num_chars && next_ch < (UNICODE) MAX_CHARS
						&& !(FontPtr->KernTab[ch]
						& (FontPtr->KernTab[next_ch] >> 2)))
					width -= FontPtr->KernAmount;
#endif
			}

			*pdelta++ = (BYTE)(width - last_width);
		}

		if (width > 0 && (bot_y -= top_y) > 0)
		{
			/* subtract off default character spacing */
			if (pdelta[-1] > 0)
			{
				--pdelta[-1];
				--width;
			}

			if (lpText->align == ALIGN_LEFT)
				pRect->corner.x = 0;
			else if (lpText->align == ALIGN_CENTER)
				pRect->corner.x = -(width >> 1);
			else
				pRect->corner.x = -width;
			pRect->corner.y = top_y;
			pRect->extent.width = width;
			pRect->extent.height = bot_y;

			pRect->corner.x += lpText->baseline.x;
			pRect->corner.y += lpText->baseline.y;

			return (TRUE);
		}
	}

	pRect->corner = lpText->baseline;
	pRect->extent.width = 0;
	pRect->extent.height = 0;

	return (FALSE);
}

void
_text_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
	FONTPTR FontPtr;

	COUNT num_chars;
	wchar_t next_ch;
	const unsigned char *pStr;
	TEXTPTR TextPtr;
	POINT origin;
	TFB_Palette color;
	TFB_Image *backing;

	FontPtr = _CurFontPtr;
	if (FontPtr == NULL)
		return;
	backing = _get_context_font_backing ();
	if (!backing)
		return;
	
	COLORtoPalette (_get_context_fg_color (), &color);

	TextPtr = &PrimPtr->Object.Text;
	origin.x = _save_stamp.origin.x;
	origin.y = TextPtr->baseline.y;
	num_chars = TextPtr->CharCount;
	if (num_chars == 0)
		return;

	pStr = TextPtr->pStr;

	next_ch = getCharFromString (&pStr);
	if (next_ch == '\0')
		num_chars = 0;
	while (num_chars--)
	{
		wchar_t ch;
		TFB_Char* fontChar;

		ch = next_ch;
		if (num_chars > 0)
		{
			next_ch = getCharFromString (&pStr);
			if (next_ch == '\0')
				num_chars = 0;
		}

		fontChar = getCharFrame (FontPtr, ch);
		if (fontChar != NULL && fontChar->disp.width)
		{
			RECT r;

			r.corner.x = origin.x - fontChar->HotSpot.x;
			r.corner.y = origin.y - fontChar->HotSpot.y;
			r.extent.width = fontChar->disp.width;
			r.extent.height = fontChar->disp.height;
			_save_stamp.origin = r.corner;
			if (BoxIntersect (&r, pClipRect, &r))
			{
				TFB_Prim_FontChar (&origin, fontChar, backing);
			}

			origin.x += fontChar->disp.width;
#if 0
			if (num_chars && next_ch < (UNICODE) MAX_CHARS
					&& !(FontPtr->KernTab[ch]
					& (FontPtr->KernTab[next_ch] >> 2)))
				origin.x -= FontPtr->KernAmount;
#endif
		}
	}
}

static inline TFB_Char *
getCharFrame (FONT_DESC *fontPtr, wchar_t ch)
{
	wchar_t pageStart = ch & CHARACTER_PAGE_MASK;
	size_t charIndex;

	FONT_PAGE *page = fontPtr->fontPages;
	for (;;)
	{
		if (page == NULL)
			return NULL;

		if (page->pageStart == pageStart)
			break;

		page = page->next;
	}

	charIndex = ch - page->firstChar;
	if (ch >= page->firstChar && charIndex < page->numChars
			&& page->charDesc[charIndex].data)
	{
		return &page->charDesc[charIndex];
	}
	else
	{
		log_add (log_Debug, "Character %u not present", (unsigned int) ch);
		return NULL;
	}
}

