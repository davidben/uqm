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
#include "gamestr.h"
#include "shipcont.h"
#include "setup.h"
#include "sounds.h"
#include "util.h"


void
ShowRemainingCapacity (void)
{
	RECT r;
	TEXT rt;
	CONTEXT OldContext;
	UNICODE rt_amount_buf[40];

	OldContext = SetContext (StatusContext);
	SetContextFont (TinyFont);

	sprintf (rt_amount_buf, "%u",
			GetSBayCapacity (NULL_PTR)
			- GLOBAL_SIS (TotalElementMass));
	rt.baseline.x = 59;
	rt.baseline.y = 113;
	rt.align = ALIGN_RIGHT;
	rt.pStr = rt_amount_buf;
	rt.CharCount = (COUNT)~0;

	r.corner.x = 40;
	r.corner.y = rt.baseline.y - 6;
	r.extent.width = rt.baseline.x - r.corner.x + 1;
	r.extent.height = 7;

	BatchGraphics ();
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01));
	DrawFilledRectangle (&r);
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09));
	font_DrawText (&rt);
	UnbatchGraphics ();
	
	SetContext (OldContext);
}

void
DrawCargoStrings (BYTE OldElement, BYTE NewElement)
{
	COORD y, cy;
	TEXT rt;
	RECT r;
	CONTEXT OldContext;
	UNICODE rt_amount_buf[40];

	LockMutex (GraphicsLock);

	OldContext = SetContext (StatusContext);
	SetContextFont (TinyFont);

	BatchGraphics ();

	y = 41;
	rt.align = ALIGN_RIGHT;
	rt.pStr = rt_amount_buf;

	if (OldElement > NUM_ELEMENT_CATEGORIES)
	{
		STAMP s;

		r.corner.x = 2;
		r.extent.width = FIELD_WIDTH + 1;

		{
			TEXT ct;

			r.corner.y = 20;
			r.extent.height = 109;
			DrawStarConBox (&r, 1,
					BUILD_COLOR (MAKE_RGB15 (0x10, 0x10, 0x10), 0x19),
					BUILD_COLOR (MAKE_RGB15 (0x08, 0x08, 0x08), 0x1F),
					TRUE,
					BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01));

			SetContextFont (StarConFont);
			ct.baseline.x = (STATUS_WIDTH >> 1) - 1;
			ct.baseline.y = 27;
			ct.align = ALIGN_CENTER;
			ct.pStr = GAME_STRING (CARGO_STRING_BASE);
			ct.CharCount = (COUNT)~0;
			SetContextForeGroundColor (
					BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x1F), 0x0B));
			font_DrawText (&ct);

			SetContextFont (TinyFont);
		}

		r.corner.x = 7;
		r.extent.width = 7;
		r.extent.height = 7;

		s.origin.x = r.corner.x + (r.extent.width >> 1);
		s.frame = SetAbsFrameIndex (
				MiscDataFrame,
				(NUM_SCANDOT_TRANSITIONS << 1) + 3
				);
		cy = y;

		rt.baseline.y = cy - 7;
		rt.CharCount = 1;

		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09));
		rt.baseline.x = 32;
		rt_amount_buf[0] = '$';
		font_DrawText (&rt);

		rt.baseline.x = 58;
		rt_amount_buf[0] = '#';
		font_DrawText (&rt);

		for (OldElement = 0;
				OldElement < NUM_ELEMENT_CATEGORIES; ++OldElement)
		{
			SetContextForeGroundColor (BLACK_COLOR);
			r.corner.y = cy - 6;
			DrawFilledRectangle (&r);

			s.origin.y = r.corner.y + (r.extent.height >> 1);
			DrawStamp (&s);
			s.frame = SetRelFrameIndex (s.frame, 5);

			if (OldElement != NewElement)
			{
				rt.baseline.y = cy;

				SetContextForeGroundColor (
						BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09));
				rt.baseline.x = 32;
				sprintf (rt_amount_buf, "%u",
						GLOBAL (ElementWorth[OldElement]));
				rt.CharCount = (COUNT)~0;
				font_DrawText (&rt);

				SetContextForeGroundColor (
						BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x14), 0x03));
				rt.baseline.x = 58;
				sprintf (rt_amount_buf, "%u",
						GLOBAL_SIS (ElementAmounts[OldElement]));
				rt.CharCount = (COUNT)~0;
				font_DrawText (&rt);
			}

			cy += 9;
		}

		OldElement = NewElement;

		rt.baseline.y = 125;

		SetContextForeGroundColor (BLACK_COLOR);
		r.corner.y = rt.baseline.y - 6;
		DrawFilledRectangle (&r);

		s.origin.y = r.corner.y + (r.extent.height >> 1);
		s.frame = SetAbsFrameIndex (s.frame, 68);
		DrawStamp (&s);

		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x14), 0x03));
		rt.baseline.x = 58;
		sprintf (rt_amount_buf, "%u", GLOBAL_SIS (TotalBioMass));
		rt.CharCount = (COUNT)~0;
		font_DrawText (&rt);

		r.corner.x = 4;
		r.corner.y = 117;
		r.extent.width = 56;
		r.extent.height = 1;
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09));
		DrawFilledRectangle (&r);

		{
			TEXT lt;
			
			lt.baseline.x = 5;
			lt.baseline.y = 113;
			lt.align = ALIGN_LEFT;
			lt.pStr = GAME_STRING (CARGO_STRING_BASE + 1);
			lt.CharCount = (COUNT)~0;
			font_DrawText (&lt);
		}

		ShowRemainingCapacity ();
	}

	r.corner.x = 19;
	r.extent.width = 40;
	r.extent.height = 7;

	if (OldElement != NewElement)
	{
		if (OldElement == NUM_ELEMENT_CATEGORIES)
			cy = 125;
		else
			cy = y + (OldElement * 9);
		r.corner.y = cy - 6;
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01));
		DrawFilledRectangle (&r);

		rt.baseline.y = cy;

		if (OldElement == NUM_ELEMENT_CATEGORIES)
			sprintf (rt_amount_buf, "%u", GLOBAL_SIS (TotalBioMass));
		else
		{
			SetContextForeGroundColor (
					BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09));
			rt.baseline.x = 32;
			sprintf (rt_amount_buf, "%u", GLOBAL (ElementWorth[OldElement]));
			rt.CharCount = (COUNT)~0;
			font_DrawText (&rt);
			sprintf (rt_amount_buf, "%u", GLOBAL_SIS (ElementAmounts[OldElement]));
		}

		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x14), 0x03));
		rt.baseline.x = 58;
		rt.CharCount = (COUNT)~0;
		font_DrawText (&rt);
	}

	if (NewElement != (BYTE)~0)
	{
		if (NewElement == NUM_ELEMENT_CATEGORIES)
			cy = 125;
		else
			cy = y + (NewElement * 9);
		r.corner.y = cy - 6;
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09));
		DrawFilledRectangle (&r);

		rt.baseline.y = cy;

		if (NewElement == NUM_ELEMENT_CATEGORIES)
			sprintf (rt_amount_buf, "%u", GLOBAL_SIS (TotalBioMass));
		else
		{
			SetContextForeGroundColor (
					BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x14), 0x03));
			rt.baseline.x = 32;
			sprintf (rt_amount_buf, "%u", GLOBAL (ElementWorth[NewElement]));
			rt.CharCount = (COUNT)~0;
			font_DrawText (&rt);
			sprintf (rt_amount_buf, "%u", GLOBAL_SIS (ElementAmounts[NewElement]));
		}

		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x1F), 0x0B));
		rt.baseline.x = 58;
		rt.CharCount = (COUNT)~0;
		font_DrawText (&rt);
	}

	UnbatchGraphics ();
	SetContext (OldContext);
	UnlockMutex (GraphicsLock);
}

static BOOLEAN
DoDiscardCargo (PMENU_STATE pMS)
{
	BYTE NewState;
	BOOLEAN select, cancel, back, forward;
	select = PulsedInputState.menu[KEY_MENU_SELECT];
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL];
	back = PulsedInputState.menu[KEY_MENU_UP] || PulsedInputState.menu[KEY_MENU_LEFT];
	forward = PulsedInputState.menu[KEY_MENU_DOWN] || PulsedInputState.menu[KEY_MENU_RIGHT];

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	if (!(pMS->Initialized & 1))
	{
		pMS->InputFunc = DoDiscardCargo;
		++pMS->Initialized;

		NewState = pMS->CurState;
		pMS->CurState = (BYTE)~0;
		goto SelectCargo;
	}
	else if (cancel)
	{
		LockMutex (GraphicsLock);
		ClearSISRect (DRAW_SIS_DISPLAY);
		UnlockMutex (GraphicsLock);

		return (FALSE);
	}
	else if (select)
	{
		if (GLOBAL_SIS (ElementAmounts[pMS->CurState - 1]))
		{
			--GLOBAL_SIS (ElementAmounts[pMS->CurState - 1]);
			DrawCargoStrings ((BYTE)(pMS->CurState - 1), (BYTE)(pMS->CurState - 1));

			LockMutex (GraphicsLock);
			--GLOBAL_SIS (TotalElementMass);
			ShowRemainingCapacity ();
			UnlockMutex (GraphicsLock);
		}
		else
		{	// no element left in cargo hold
			PlayMenuSound (MENU_SOUND_FAILURE);
		}
	}
	else
	{
		NewState = pMS->CurState - 1;
		if (back)
		{
			if (NewState-- == 0)
				NewState = NUM_ELEMENT_CATEGORIES - 1;
		}
		else if (forward)
		{
			if (++NewState == NUM_ELEMENT_CATEGORIES)
				NewState = 0;
		}

		if (++NewState != pMS->CurState)
		{
SelectCargo:
			DrawCargoStrings ((BYTE)(pMS->CurState - 1), (BYTE)(NewState - 1));
			LockMutex (GraphicsLock);
			DrawStatusMessage (GAME_STRING (NewState - 1 + (CARGO_STRING_BASE + 2)));
			UnlockMutex (GraphicsLock);

			pMS->CurState = NewState;
		}
	}

	return (TRUE);
}

void
Cargo (PMENU_STATE pMS)
{
	pMS->InputFunc = DoDiscardCargo;
	--pMS->Initialized;
	pMS->CurState = 1;

	LockMutex (GraphicsLock);
	DrawStatusMessage ((UNICODE *)~0);
	UnlockMutex (GraphicsLock);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	DoInput ((PVOID)pMS, TRUE);

	pMS->InputFunc = DoFlagshipCommands;
	pMS->CurState = CARGO + 1;
}

