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

#include "colors.h"
#include "controls.h"
#include "dummy.h"
#include "gamestr.h"
#include "options.h"
#include "setup.h"
#include "sounds.h"
#include "planets/lander.h"
#include "planets/planets.h"
#include "libs/inplib.h"

#include <ctype.h>
#include <string.h>


#define NUM_CELL_COLS MAP_WIDTH / 6
#define NUM_CELL_ROWS MAP_HEIGHT / 6
#define MAX_CELL_COLS 40

extern FRAME SpaceJunkFrame;

static void
ClearReportArea (void)
{
	COUNT i;
	RECT r;
	STAMP s;
	PRIMITIVE prim_row[MAX_CELL_COLS];

	if (optWhichFonts == OPT_PC)
		s.frame = SetAbsFrameIndex (SpaceJunkFrame, 21);
	else
		s.frame = SetAbsFrameIndex (SpaceJunkFrame, 18);
	GetFrameRect (s.frame, &r);
	s.origin.x = 1 + (r.extent.width >> 1) - 1;
	s.origin.y = 1;
	for (i = 0; i < NUM_CELL_COLS; ++i)
	{
		prim_row[i].Object.Stamp = s;
		s.origin.x += r.extent.width + 1;

		SetPrimNextLink (&prim_row[i], i + 1);
		if (optWhichFonts == OPT_PC)
			SetPrimType (&prim_row[i], STAMP_PRIM);
		else
		{
			SetPrimType (&prim_row[i], STAMPFILL_PRIM);
			SetPrimColor (&prim_row[i], BUILD_COLOR (MAKE_RGB15 (0x00, 0x07, 0x00), 0x57));
		}
	}
	SetPrimNextLink (&prim_row[i - 1], END_OF_LIST);

	SetContextBackGroundColor (BLACK_COLOR);
	ClearDrawable ();
	SetContextClipping (FALSE);
	for (i = 0; i < NUM_CELL_ROWS; ++i)
	{
		DrawBatch (prim_row, 0, 0);
		r.corner.y -= r.extent.height + 1;
		SetFrameHot (s.frame, MAKE_HOT_SPOT (r.corner.x, r.corner.y));
	}
	SetContextClipping (TRUE);
	r.corner.y = 0;
	SetFrameHot (s.frame, MAKE_HOT_SPOT (r.corner.x, r.corner.y));
}

static void
MakeReport (SOUND ReadOutSounds, UNICODE *pStr, COUNT StrLen)
{
	BYTE ButtonState;
	int end_page_len;
	UNICODE last_c, end_page_buf[80];
	COUNT row_cells;
	BOOLEAN Sleepy;
	RECT r;
	TEXT t;

	wsprintf (end_page_buf, "%s\n", GAME_STRING (SCAN_STRING_BASE + NUM_SCAN_TYPES));
	end_page_len = wstrlen (end_page_buf);

	GetFrameRect (SetAbsFrameIndex (SpaceJunkFrame, 18), &r);

	t.align = ALIGN_LEFT;
	t.CharCount = 1;
	t.pStr = pStr;
	last_c = *pStr;

	Sleepy = TRUE;
	UnlockMutex (GraphicsLock);

	FlushInput ();
	goto InitPageCell;
	while (StrLen)
	{
		COUNT col_cells;
		UNICODE *pLastStr;
		UNICODE *pTempStr;
		COUNT lf_pos;

		pLastStr = t.pStr;

		// scan for LFs in the remaining string
		// trailing LF will be ignored
		for (lf_pos = StrLen, pTempStr = t.pStr;
				lf_pos && *pTempStr != '\n';
				--lf_pos, ++pTempStr)
			;

		col_cells = 0;
		// check if the remaining text fits on current screen
		if (row_cells == NUM_CELL_ROWS - 1
				&& (StrLen > NUM_CELL_COLS || lf_pos > 1))
		{
			col_cells = (NUM_CELL_COLS >> 1) - (end_page_len >> 1);
			t.pStr = end_page_buf;
			StrLen += end_page_len;
		}
		t.baseline.x = 1 + (r.extent.width >> 1)
				+ (col_cells * (r.extent.width + 1)) - 1;
		do
		{
			COUNT word_chars;
			UNICODE *pStr;

			pStr = t.pStr;
			while (isgraph (*pStr))
				++pStr;

			word_chars = (COUNT)(pStr - t.pStr);
			if ((col_cells += word_chars) <= NUM_CELL_COLS)
			{
				DWORD TimeOut;

				if (StrLen -= word_chars)
					--StrLen;
				TimeOut = GetTimeCounter ();
				while (word_chars--)
				{
					if (!Sleepy || (GLOBAL (CurrentActivity) & CHECK_ABORT))
						font_DrawText (&t);
					else
					{
						LockMutex (GraphicsLock);
						font_DrawText (&t);
						UnlockMutex (GraphicsLock);

						PlaySound (ReadOutSounds, NotPositional (), NULL,
								GAME_SOUND_PRIORITY);

						if (t.pStr[0] == ',')
							TimeOut += ONE_SECOND / 4;
						if (t.pStr[0] == '.' || t.pStr[0] == '!' || t.pStr[0] == '?')
							TimeOut += ONE_SECOND / 2;
						else
							TimeOut += ONE_SECOND / 20;
						if (word_chars == 0)
							TimeOut += ONE_SECOND / 20;

						TaskSwitch();
						while (GetTimeCounter () < TimeOut)
						{
							if (ButtonState)
							{
								if (!AnyButtonPress (TRUE))
									ButtonState = 0;
							}
							else if (AnyButtonPress (TRUE))
							{
								Sleepy = FALSE;
								LockMutex (GraphicsLock);
								BatchGraphics ();
								break;
							}
							TaskSwitch();
						}
					}
					++t.pStr;
					t.baseline.x += r.extent.width + 1;
				}

				++col_cells;
				last_c = *t.pStr++;
				t.baseline.x += r.extent.width + 1;
			}
		} while (col_cells <= NUM_CELL_COLS && last_c != '\n' && StrLen);

		t.baseline.y += r.extent.height + 1;
		if (++row_cells == NUM_CELL_ROWS || StrLen == 0)
		{
			t.pStr = pLastStr;
			if (!Sleepy)
			{
				UnbatchGraphics ();
				UnlockMutex (GraphicsLock);
			}

			if (WaitAnyButtonOrQuit (TRUE))
				break;

InitPageCell:
			ButtonState = 1;
			t.baseline.y = r.extent.height + 1;
			row_cells = 0;
			if (StrLen)
			{
				LockMutex (GraphicsLock);
				if (!Sleepy)
					BatchGraphics ();
				ClearReportArea();
				SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0, 0x1F, 0), 0xFF));
				if (Sleepy)
					UnlockMutex (GraphicsLock);
			}
		}
	}
	LockMutex (GraphicsLock);
}

void
DoDiscoveryReport (SOUND ReadOutSounds)
{
	POINT old_curs;
	CONTEXT OldContext;
	extern void DrawScannedObjects (BOOLEAN Reversed);

	if (pMenuState)
	{
		old_curs = pMenuState->flash_rect0.corner;
		pMenuState->flash_rect0.corner.x = /* disable cursor */
				pMenuState->flash_rect0.corner.x = -1000;
	}

	OldContext = SetContext (ScanContext);
	{
		FONT OldFont;

		OldFont = SetContextFont (pSolarSysState->SysInfo.PlanetInfo.LanderFont);
		if (optWhichFonts == OPT_PC)
		{
			SetContextFontEffect (ALTERNATE_EFFECT,
				BUILD_COLOR_RGBA (0x00, 0x00, 0xD6, 0xFF),
				BUILD_COLOR_RGBA (0x00, 0x04, 0xFF, 0xFF));
			MakeReport (ReadOutSounds,
				(UNICODE *)GetStringAddress (pSolarSysState->SysInfo.PlanetInfo.DiscoveryString),
				GetStringLength (pSolarSysState->SysInfo.PlanetInfo.DiscoveryString));
			SetContextFontEffect (0, 0, 0);
		}
		else
			MakeReport (ReadOutSounds,
				(UNICODE *)GetStringAddress (pSolarSysState->SysInfo.PlanetInfo.DiscoveryString),
				GetStringLength (pSolarSysState->SysInfo.PlanetInfo.DiscoveryString));
		SetContextFont (OldFont);
	}
#ifdef OLD
	ClearDrawable ();
	if (pSolarSysState->MenuState.Initialized >= 3)
		DrawScannedObjects (FALSE);
#else /* !OLD */
	if (pSolarSysState->MenuState.Initialized < 3)
		ClearDrawable ();
#endif /* OLD */
	SetContext (OldContext);

	UnlockMutex (GraphicsLock);
	FlushInput ();
	while (AnyButtonPress (TRUE))
		TaskSwitch ();
	LockMutex (GraphicsLock);

	if (pMenuState)
		pMenuState->flash_rect0.corner = old_curs;
}


