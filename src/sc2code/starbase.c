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

#include "starcon.h"
#include "libs/graphics/gfx_common.h"

PMENU_STATE pMenuState;

static void
DrawBaseStateStrings (STARBASE_STATE OldState, STARBASE_STATE NewState)
{
	TEXT t;
	//STRING locString;

	SetContext (ScreenContext);
	SetContextFont (StarConFont);
	SetContextForeGroundColor (BLACK_COLOR);

	t.baseline.x = 73 - 4 + SAFE_X;
	t.align = ALIGN_CENTER;

	if (OldState == (STARBASE_STATE)~0)
	{
		t.baseline.y = 106 + 28 + (SAFE_Y + 4);
		for (OldState = TALK_COMMANDER; OldState < DEPART_BASE; ++OldState)
		{
			if (OldState != NewState)
			{
				t.pStr = GAME_STRING (STARBASE_STRING_BASE + 1 + OldState);
				t.CharCount = (COUNT)~0;
				DrawText (&t);
			}
			t.baseline.y += (23 - 4);
		}
	}

	t.baseline.y = 106 + 28 + (SAFE_Y + 4) + ((23 - 4) * OldState);
	t.pStr = GAME_STRING (STARBASE_STRING_BASE + 1 + OldState);
	t.CharCount = (COUNT)~0;
	DrawText (&t);

	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0xA), 0x0E));
	t.baseline.y = 106 + 28 + (SAFE_Y + 4) + ((23 - 4) * NewState);
	t.pStr = GAME_STRING (STARBASE_STRING_BASE + 1 + NewState);
	t.CharCount = (COUNT)~0;
	DrawText (&t);
}

void
DrawShipPiece (PMENU_STATE pMS, COUNT which_piece, COUNT which_slot,
		BOOLEAN DrawBluePrint)
{
	COLOR OldColor;
	RECT r;
	STAMP Side, Top;
	SBYTE RepairSlot;

	RepairSlot = 0;
	switch (which_piece)
	{
		case FUSION_THRUSTER:
		case EMPTY_SLOT + 0:
			Side.origin.x = DRIVE_SIDE_X;
			Side.origin.y = DRIVE_SIDE_Y;
			Top.origin.x = DRIVE_TOP_X;
			Top.origin.y = DRIVE_TOP_Y;
			break;
		case TURNING_JETS:
		case EMPTY_SLOT + 1:
			Side.origin.x = JET_SIDE_X;
			Side.origin.y = JET_SIDE_Y;
			Top.origin.x = JET_TOP_X;
			Top.origin.y = JET_TOP_Y;
			break;
		default:
			if (which_piece < EMPTY_SLOT + 2)
			{
				RepairSlot = 1;
				if (which_piece < EMPTY_SLOT
						&& (which_slot == 0
						|| GLOBAL_SIS (ModuleSlots[
								which_slot - 1
								]) < EMPTY_SLOT))
					++RepairSlot;
			}
			else if (!DrawBluePrint)
			{
				if (which_slot == 0 || which_slot >= NUM_MODULE_SLOTS - 3)
					++which_piece;

				if (which_slot < NUM_MODULE_SLOTS - 1
						&& GLOBAL_SIS (ModuleSlots[
								which_slot + 1
								]) < EMPTY_SLOT)
				{
					RepairSlot = -1;
					if (which_piece == EMPTY_SLOT + 3
							|| which_slot + 1 == NUM_MODULE_SLOTS - 3)
						--RepairSlot;
				}
			}
			Side.origin.x = MODULE_SIDE_X;
			Side.origin.y = MODULE_SIDE_Y;
			Top.origin.x = MODULE_TOP_X;
			Top.origin.y = MODULE_TOP_Y;
			break;
	}

	Side.origin.x += which_slot * SHIP_PIECE_OFFSET;
	if (RepairSlot < 0)
	{
		Side.frame = SetAbsFrameIndex (pMS->ModuleFrame,
				((NUM_MODULES - 1) + (6 - 2)) + (NUM_MODULES + 6)
				- (RepairSlot + 1));
		DrawStamp (&Side);
	}
	else if (RepairSlot)
	{
		r.corner = Side.origin;
		r.extent.width = SHIP_PIECE_OFFSET;
		r.extent.height = 1;
		OldColor = SetContextForeGroundColor (BLACK_COLOR);
		DrawFilledRectangle (&r);
		r.corner.y += 23 - 1;
		DrawFilledRectangle (&r);

		r.extent.width = 1;
		r.extent.height = 8;
		if (RepairSlot == 2)
		{
			r.corner = Side.origin;
			DrawFilledRectangle (&r);
			r.corner.y += 15;
			DrawFilledRectangle (&r);
		}
		if (which_slot < (NUM_MODULE_SLOTS - 1))
		{
			r.corner = Side.origin;
			r.corner.x += SHIP_PIECE_OFFSET;
			DrawFilledRectangle (&r);
			r.corner.y += 15;
			DrawFilledRectangle (&r);
		}
	}

	if (DrawBluePrint)
	{
		if (RepairSlot)
			SetContextForeGroundColor (OldColor);
		Side.frame = SetAbsFrameIndex (pMS->ModuleFrame, which_piece - 1);
		DrawFilledStamp (&Side);
	}
	else
	{
		Top.origin.x += which_slot * SHIP_PIECE_OFFSET;
		if (RepairSlot < 0)
		{
			Top.frame = SetRelFrameIndex (Side.frame, -((NUM_MODULES - 1) + 6));
			DrawStamp (&Top);
		}
		else if (RepairSlot)
		{
			r.corner = Top.origin;
			r.extent.width = SHIP_PIECE_OFFSET;
			r.extent.height = 1;
			DrawFilledRectangle (&r);
			r.corner.y += 32 - 1;
			DrawFilledRectangle (&r);

			r.extent.width = 1;
			r.extent.height = 12;
			if (RepairSlot == 2)
			{
				r.corner = Top.origin;
				DrawFilledRectangle (&r);
				r.corner.y += 20;
				DrawFilledRectangle (&r);
			}
			RepairSlot = (which_slot < NUM_MODULE_SLOTS - 1);
			if (RepairSlot)
			{
				r.corner = Top.origin;
				r.corner.x += SHIP_PIECE_OFFSET;
				DrawFilledRectangle (&r);
				r.corner.y += 20;
				DrawFilledRectangle (&r);
			}
		}

		Top.frame = SetAbsFrameIndex (pMS->ModuleFrame, which_piece);
		DrawStamp (&Top);

		Side.frame = SetRelFrameIndex (Top.frame, (NUM_MODULES - 1) + 6);
		DrawStamp (&Side);

		if (which_slot == 1 && which_piece == EMPTY_SLOT + 2)
		{
			STAMP s;

			s.origin = Top.origin;
			s.origin.x -= SHIP_PIECE_OFFSET;
			s.frame = SetAbsFrameIndex (pMS->ModuleFrame, NUM_MODULES + 5);
			DrawStamp (&s);
			s.origin = Side.origin;
			s.origin.x -= SHIP_PIECE_OFFSET;
			s.frame = SetRelFrameIndex (s.frame, (NUM_MODULES - 1) + 6);
			DrawStamp (&s);
		}

		if (RepairSlot)
		{
			Top.origin.x += SHIP_PIECE_OFFSET;
			Side.origin.x += SHIP_PIECE_OFFSET;
			which_piece = GLOBAL_SIS (ModuleSlots[++which_slot]);
			if (which_piece == EMPTY_SLOT + 2
					&& which_slot >= NUM_MODULE_SLOTS - 3)
				++which_piece;

			Top.frame = SetAbsFrameIndex (pMS->ModuleFrame, which_piece);
			DrawStamp (&Top);

			Side.frame = SetRelFrameIndex (Top.frame, (NUM_MODULES - 1) + 6);
			DrawStamp (&Side);
		}
	}
}

int rotate_starbase(void* blah)
{
	DWORD TimeIn;
	STAMP s;
	
	s.origin.x = s.origin.y = 0;
s.origin.x = SAFE_X, s.origin.y = SAFE_Y + 4;
	s.frame = IncFrameIndex (pMenuState->CurFrame);
	TimeIn = GetTimeCounter ();
	for (;;)
	{
		//CONTEXT OldContext;
		
		SetSemaphore (GraphicsSem);
		DrawStamp (&s);
		s.frame = IncFrameIndex (s.frame);
		if (s.frame == pMenuState->CurFrame)
			s.frame = IncFrameIndex (s.frame);
		ClearSemaphore (GraphicsSem);
		
		SleepThreadUntil (TimeIn + (ONE_SECOND / 20));
		TimeIn = GetTimeCounter ();
	}

	(void) blah;  /* Satisfying compiler (unused parameter) */
		return(0);
}

BOOLEAN
DoStarBase (INPUT_STATE InputState, PMENU_STATE pMS)
{
	if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
	{
		pMS->CurState = DEPART_BASE;
		goto ExitStarBase;
	}

	if (!pMS->Initialized)
	{
		STAMP s;

		LastActivity &= ~CHECK_LOAD;
		pMS->InputFunc = DoStarBase;

		SetSemaphore (GraphicsSem);
		SetFlashRect (NULL_PTR, (FRAME)0);

		if (pMS->hMusic)
		{
			StopMusic ();
			DestroyMusic (pMS->hMusic);
			pMS->hMusic = 0;
		}

		if (pMS->flash_task)
		{
			KillThread (pMS->flash_task);
			pMS->flash_task = 0;
		}

		pMS->Initialized = TRUE;
		SetContext (ScreenContext);
		SetContextDrawState (DEST_PIXMAP | DRAW_REPLACE);

		ClearSemaphore (GraphicsSem);

		s.origin.x = s.origin.y = 0;
s.origin.x = SAFE_X, s.origin.y = SAFE_Y + 4;
		s.frame = CaptureDrawable (LoadGraphic (STARBASE_ANIM));
		pMS->CurFrame = s.frame;
		pMS->hMusic = LoadMusicInstance (STARBASE_MUSIC);

		SetSemaphore (GraphicsSem);
		BatchGraphics ();
		SetContextBackGroundColor (BLACK_COLOR);
		ClearDrawable ();
		DrawStamp (&s);
		DrawBaseStateStrings ((STARBASE_STATE)~0, pMS->CurState);
{
	RECT r;

	r.corner.x = 0;
	r.corner.y = 0;
	r.extent.width = SCREEN_WIDTH;
	r.extent.height = SCREEN_HEIGHT;
	ScreenTransition (3, &r);
}
		PlayMusic (pMS->hMusic, TRUE, 1);
		UnbatchGraphics ();
		pMS->flash_task = CreateThread (rotate_starbase, NULL, 4096,
				"rotate starbase");
		ClearSemaphore (GraphicsSem);
	}
	else if ((InputState & DEVICE_BUTTON1)
			|| GET_GAME_STATE (MOONBASE_ON_SHIP)
			|| GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
	{
ExitStarBase:
		SetSemaphore (GraphicsSem);
		if (pMS->flash_task)
		{
			KillThread (pMS->flash_task);
			pMS->flash_task = 0;
		}
		DestroyDrawable (ReleaseDrawable (pMS->CurFrame));
		pMS->CurFrame = 0;
		ClearSemaphore (GraphicsSem);
		StopMusic ();
		if (pMS->hMusic)
		{
			DestroyMusic (pMS->hMusic);
			pMS->hMusic = 0;
		}

		if (pMS->CurState == DEPART_BASE)
		{
			if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
			{
				SET_GAME_STATE (STARBASE_VISITED, 0);
			}
			return (FALSE);
		}

		pMS->Initialized = FALSE;
		if (pMS->CurState == TALK_COMMANDER)
		{
			InitCommunication (COMMANDER_CONVERSATION);
			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, (BYTE)~0);
		}
		else
		{
			BYTE OldState;

			switch (OldState = pMS->CurState)
			{
				case OUTFIT_STARSHIP:
					pMS->InputFunc = DoOutfit;
					break;
				case SHIPYARD:
					pMS->InputFunc = DoShipyard;
					break;
			}

			DoInput ((PVOID)pMS);

			pMS->Initialized = FALSE;
			pMS->CurState = OldState;
			pMS->InputFunc = DoStarBase;
		}
	}
	else
	{
		STARBASE_STATE NewState;

		NewState = pMS->CurState;
		if (GetInputXComponent (InputState) < 0
				|| GetInputYComponent (InputState) < 0)
		{
			if (NewState-- == TALK_COMMANDER)
				NewState = DEPART_BASE;
		}
		else if (GetInputXComponent (InputState) > 0
				|| GetInputYComponent (InputState) > 0)
		{
			if (NewState++ == DEPART_BASE)
				NewState = TALK_COMMANDER;
		}

		if (NewState != pMS->CurState)
		{
			SetSemaphore (GraphicsSem);
			DrawBaseStateStrings (pMS->CurState, NewState);
			ClearSemaphore (GraphicsSem);
			pMS->CurState = NewState;
		}
	}

	return (TRUE);
}

void
VisitStarBase (void)
{
	MENU_STATE MenuState;
	CONTEXT OldContext;

	if (GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
	{
		CurStarDescPtr = 0;
		goto TimePassage;
	}
	else if (!GET_GAME_STATE (STARBASE_AVAILABLE))
	{
		HSTARSHIP hStarShip;
		SHIP_FRAGMENTPTR FragPtr;

		pMenuState = 0;

		InitCommunication (COMMANDER_CONVERSATION);
		if (!GET_GAME_STATE (PROBE_ILWRATH_ENCOUNTER)
				|| (GLOBAL (CurrentActivity) & CHECK_ABORT))
			goto ExitStarBase;

		hStarShip = CloneShipFragment (
				ILWRATH_SHIP,
				&GLOBAL (npc_built_ship_q), 7
				);
		FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (npc_built_ship_q), hStarShip
				);
		SET_RACE_ID (FragPtr, (BYTE)~0);
		UnlockStarShip (
				&GLOBAL (npc_built_ship_q), hStarShip
				);

		InitCommunication (ILWRATH_CONVERSATION);
		if (GLOBAL_SIS (CrewEnlisted) == (COUNT)~0
				|| (GLOBAL (CurrentActivity) & CHECK_ABORT))
			return;
		SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, (BYTE)~0);

		{
#define LOST_DAYS 14
			COUNT i;
			BYTE clut_buf[1];

			pMenuState = &MenuState;
			InitCommunication (COMMANDER_CONVERSATION);

TimePassage:
			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, (BYTE)~0);
			clut_buf[0] = FadeAllToBlack;
			SleepThreadUntil (XFormColorMap ((COLORMAPPTR)clut_buf, ONE_SECOND * 2));
			for (i = 0; i < LOST_DAYS; ++i)
			{
				while (ClockTick () > 0)
					;

				ResumeGameClock ();
				SleepThread (2);
				SuspendGameClock ();
			}
		}
	}

	pMenuState = &MenuState;

	memset ((PMENU_STATE)&MenuState, 0, sizeof (MenuState));

	MenuState.InputFunc = DoStarBase;
	if (GET_GAME_STATE (MOONBASE_ON_SHIP)
			|| GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
	{
		MenuState.Initialized = TRUE;
		MenuState.CurState = TALK_COMMANDER;
	}

	OldContext = SetContext (ScreenContext);
	DoInput ((PVOID)pMenuState);
	SetContext (OldContext);

	pMenuState = 0;

ExitStarBase:
	if (!(GLOBAL (CurrentActivity) & (CHECK_LOAD | CHECK_ABORT)))
	{
		extern ACTIVITY NextActivity;

		SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 0);
		GLOBAL (CurrentActivity) = CHECK_LOAD;
		NextActivity = MAKE_WORD (IN_INTERPLANETARY, 0)
				| START_INTERPLANETARY;
	}
}

COUNT
WrapText (UNICODE *pStr, COUNT len, TEXT *tarray, SIZE field_width)
{
	COUNT num_lines;

	num_lines = 0;
	do
	{
		RECT r;
		COUNT OldCount;
		
		tarray->align = ALIGN_LEFT; /* set alignment to something */
		tarray->pStr = pStr;
		tarray->CharCount = 1;
		++num_lines;
		
		do
		{
			OldCount = tarray->CharCount;
			while (*++pStr != ' ' && (COUNT)(pStr - tarray->pStr) < len)
				;
			tarray->CharCount = pStr - tarray->pStr;
			TextRect (tarray, &r, NULL_PTR);
		} while (tarray->CharCount < len && r.extent.width < field_width);
	
		if (r.extent.width >= field_width)
		{
			if ((tarray->CharCount = OldCount) == 1)
			{
				do
				{
					++tarray->CharCount;
					TextRect (tarray, &r, NULL_PTR);
				} while (r.extent.width < field_width);
				--tarray->CharCount;
			}
		}
	
		pStr = tarray->pStr + tarray->CharCount;
		len -= tarray->CharCount;
		++tarray;
	
		if (len && (r.extent.width < field_width || OldCount > 1))
		{
			++pStr; /* skip white space */
			--len;
		}

	} while (len);

	return (num_lines);
}

