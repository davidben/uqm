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


extern FRAME Build_Font_Effect (FRAME FramePtr, DWORD from, DWORD to,
		BYTE type);
static inline FRAME_DESC *getCharFrame (FONT_DESC *fontPtr, wchar_t ch);

static BYTE char_delta_array[MAX_DELTAS];
		// XXX: This does not seem to be used.

static FONTEFFECT FontEffect = {0,0,0,0};

FONTEFFECT
SetContextFontEffect (BYTE type, DWORD from, DWORD to)
{
	FONTEFFECT oldFontEffect = FontEffect;

	if (from == to)
		FontEffect.Use = 0;
	else
	{
		FontEffect.Use = 1;
		FontEffect.from = from;
		FontEffect.to = to;
		FontEffect.type = type;
	}

	if (!oldFontEffect.Use)
		// reset fields so that when struct passed back
		// the logic works correctly
		oldFontEffect.to = oldFontEffect.from = 0;

	return oldFontEffect;
}


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
		next_ch = getCharFromString (&pStr);
		if (next_ch == '\0')
			num_chars = 0;
		while (num_chars--)
		{
			wchar_t ch;
			SIZE last_width;
			FRAME_DESC *charFrame;

			last_width = width;

			ch = next_ch;
			next_ch = getCharFromString (&pStr);
			if (next_ch == '\0')
			{
				lpText->CharCount -= num_chars;
				num_chars = 0;
			}

			charFrame = getCharFrame (FontPtr, ch);
			if (charFrame != NULL && GetFrameWidth (charFrame))
			{
				COORD y;

				y = -charFrame->HotSpot.y;
				if (y < top_y)
					top_y = y;
				y += GetFrameHeight (charFrame);
				if (y > bot_y)
					bot_y = y;

				width += GetFrameWidth (charFrame);
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
			--pdelta[-1];
			--width;

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

	FontPtr = _CurFontPtr;
	if (FontPtr != 0)
	{
		COUNT num_chars;
		wchar_t next_ch;
		const unsigned char *pStr;
		TEXTPTR TextPtr;
		PRIMITIVE locPrim;
		TFB_Palette color;
		DWORD c32k;

		if (FontEffect.Use)
			SetPrimType (&locPrim, STAMP_PRIM);
		else
			SetPrimType (&locPrim, STAMPFILL_PRIM);
		c32k = _get_context_fg_color () >> 8;
		color.r = (UBYTE)((c32k >> (10 - (8 - 5))) & 0xF8);
		color.g = (UBYTE)((c32k >> (5 - (8 - 5))) & 0xF8);
		color.b = (UBYTE)((c32k << (8 - 5)) & 0xF8);

		TextPtr = &PrimPtr->Object.Text;
		locPrim.Object.Stamp.origin.x = _save_stamp.origin.x;
		locPrim.Object.Stamp.origin.y = TextPtr->baseline.y;
		num_chars = TextPtr->CharCount;

		pStr = TextPtr->pStr;
		next_ch = getCharFromString (&pStr);
		if (next_ch == '\0')
			num_chars = 0;
		while (num_chars--)
		{
			wchar_t ch;

			ch = next_ch;
			next_ch = getCharFromString (&pStr);
			if (next_ch == '\0')
				num_chars = 0;

			locPrim.Object.Stamp.frame = getCharFrame (FontPtr, ch);
			if (locPrim.Object.Stamp.frame != NULL &&
					GetFrameWidth (locPrim.Object.Stamp.frame))
			{
				RECT r;

				r.corner.x = locPrim.Object.Stamp.origin.x
						- ((FRAMEPTR)locPrim.Object.Stamp.frame)->HotSpot.x;
				r.corner.y = locPrim.Object.Stamp.origin.y
						- ((FRAMEPTR)locPrim.Object.Stamp.frame)->HotSpot.y;
				r.extent.width = GetFrameWidth (locPrim.Object.Stamp.frame);
				r.extent.height = GetFrameHeight (locPrim.Object.Stamp.frame);
				_save_stamp.origin = r.corner;
				if (BoxIntersect (&r, pClipRect, &r))
				{
					if (FontEffect.Use)
					{
						FRAME origFrame = locPrim.Object.Stamp.frame;
						locPrim.Object.Stamp.frame = Build_Font_Effect(
								locPrim.Object.Stamp.frame, FontEffect.from,
								FontEffect.to, FontEffect.type);
						TFB_Prim_Stamp (&locPrim.Object.Stamp);
						DestroyDrawable (ReleaseDrawable (
								locPrim.Object.Stamp.frame));
						locPrim.Object.Stamp.frame = origFrame;
					}
					else
						TFB_Prim_StampFill (&locPrim.Object.Stamp, &color);
				}

				locPrim.Object.Stamp.origin.x += GetFrameWidth (
						locPrim.Object.Stamp.frame);
#if 0
				if (num_chars && next_ch < (UNICODE) MAX_CHARS
						&& !(FontPtr->KernTab[ch]
						& (FontPtr->KernTab[next_ch] >> 2)))
					locPrim.Object.Stamp.origin.x -= FontPtr->KernAmount;
#endif
			}
		}
	}
}

static inline FRAME_DESC *
getCharFrame (FONT_DESC *fontPtr, wchar_t ch) {
	if (ch > MAX_CHARS)
		return NULL;
	return &fontPtr->CharDesc[ch - FIRST_CHAR];
}

