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

#include "pickmele.h"

#include "build.h"
#include "controls.h"
#include "intel.h"
#include "melee.h"
#include "races.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "libs/inplib.h"
#include "libs/mathlib.h"


#define NUM_MELEE_COLS_ORIG NUM_MELEE_COLUMNS
#define PICK_X_OFFS 57
#define PICK_Y_OFFS 24
#define PICK_SIDE_OFFS 100

typedef struct getmelee_struct {
	BOOLEAN (*InputFunc) (struct getmelee_struct *pInputState);
	COUNT MenuRepeatDelay;
	BOOLEAN Initialized;
	HSTARSHIP hBattleShip;
	COUNT row, col, ships_left, which_player;
	RECT flash_rect;
} GETMELEE_STATE;


/* TODO: Include player timeouts */
static BOOLEAN
DoGetMelee (GETMELEE_STATE *gms)
{
	BOOLEAN left, right, up, down, select;
	HSTARSHIP hNextShip;
	COUNT which_player = gms->which_player;

	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);

	if (!gms->Initialized)
	{
		gms->Initialized = TRUE;
		gms->row = 0;
		gms->col = NUM_MELEE_COLS_ORIG;

		goto ChangeSelection;
	}

	SleepThread (ONE_SECOND / 120);
	
	if (PlayerInput[which_player] == ComputerInput)
	{
		/* TODO: Make this a frame-by-frame thing.  This code is currently
		 *       copied from intel.c's computer_intelligence */
		SleepThread (ONE_SECOND >> 1);
		left = right = up = down = FALSE;
		select = TRUE;		
	}
	else if (which_player == 0)
	{
		left = PulsedInputState.key[KEY_P1_LEFT] ||
				PulsedInputState.key[KEY_MENU_LEFT];
		right = PulsedInputState.key[KEY_P1_RIGHT] ||
				PulsedInputState.key[KEY_MENU_RIGHT];
		up = PulsedInputState.key[KEY_P1_THRUST] ||
				PulsedInputState.key[KEY_MENU_UP];
		down = PulsedInputState.key[KEY_P1_DOWN] ||
				PulsedInputState.key[KEY_MENU_DOWN];
		select = PulsedInputState.key[KEY_P1_WEAPON] ||
				PulsedInputState.key[KEY_MENU_SELECT];
	}
	else
	{
		left = PulsedInputState.key[KEY_P2_LEFT] ||
				PulsedInputState.key[KEY_MENU_LEFT];
		right = PulsedInputState.key[KEY_P2_RIGHT] ||
				PulsedInputState.key[KEY_MENU_RIGHT];
		up = PulsedInputState.key[KEY_P2_THRUST] ||
				PulsedInputState.key[KEY_MENU_UP];
		down = PulsedInputState.key[KEY_P2_DOWN] ||
				PulsedInputState.key[KEY_MENU_DOWN];
		select = PulsedInputState.key[KEY_P2_WEAPON] ||
				PulsedInputState.key[KEY_MENU_SELECT];
	}

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		gms->hBattleShip = 0;
		GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
		return FALSE;
	}

	if (select)
	{
		if (gms->hBattleShip || (gms->col == NUM_MELEE_COLS_ORIG && ConfirmExit ()))
		{
			GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
			return FALSE;
		}
	}
	else
	{
		COUNT new_row, new_col;
		
		new_row = gms->row;
		new_col = gms->col;
		if (left)
		{
			if (new_col-- == 0)
				new_col = NUM_MELEE_COLS_ORIG;
		}
		else if (right)
		{
			if (new_col++ == NUM_MELEE_COLS_ORIG)
				new_col = 0;
		}
		if (up)
		{
			if (new_row-- == 0)
				new_row = NUM_MELEE_ROWS - 1;
		}
		else if (down)
		{
			if (++new_row == NUM_MELEE_ROWS)
				new_row = 0;
		}
		
		if (new_row != gms->row || new_col != gms->col)
		{
			COUNT ship_index;
			
			gms->row = new_row;
			gms->col = new_col;
			
			PlaySoundEffect (MenuSounds, 0, NotPositional (), NULL, 0);
ChangeSelection:
			LockMutex (GraphicsLock);
			gms->flash_rect.corner.x = PICK_X_OFFS
				+ ((ICON_WIDTH + 2) * gms->col);
			gms->flash_rect.corner.y = PICK_Y_OFFS
				+ ((ICON_HEIGHT + 2) * gms->row)
				+ ((1 - which_player) * PICK_SIDE_OFFS);
			SetFlashRect (&gms->flash_rect, (FRAME)0);
			
			gms->hBattleShip = GetHeadLink (&race_q[which_player]);
			if (gms->col == NUM_MELEE_COLS_ORIG)
			{
				if (gms->row)
					gms->hBattleShip = 0;
				else
				{
					ship_index = (COUNT)TFB_Random () % gms->ships_left;
					for (gms->hBattleShip = GetHeadLink (&race_q[which_player]);
					     gms->hBattleShip != 0; gms->hBattleShip = hNextShip)
					{
						STARSHIPPTR StarShipPtr = LockStarShip (&race_q[which_player], gms->hBattleShip);
						if (StarShipPtr->RaceResIndex && ship_index-- == 0)
						{
							UnlockStarShip (&race_q[which_player], gms->hBattleShip);
							break;
						}
						hNextShip = _GetSuccLink (StarShipPtr);
						UnlockStarShip (&race_q[which_player], gms->hBattleShip);
					}
				}
			}
			else
			{
				ship_index = (gms->row * NUM_MELEE_COLS_ORIG) + gms->col;
				for (gms->hBattleShip = GetHeadLink (&race_q[which_player]);
				     gms->hBattleShip != 0; gms->hBattleShip = hNextShip)
				{
					STARSHIPPTR StarShipPtr = LockStarShip (&race_q[which_player], gms->hBattleShip);
					if (StarShipPtr->ShipFacing == ship_index)
					{
						hNextShip = gms->hBattleShip;
						if (StarShipPtr->RaceResIndex == 0)
							gms->hBattleShip = 0;
						UnlockStarShip (&race_q[which_player], hNextShip);
						break;
					}
					hNextShip = _GetSuccLink (StarShipPtr);
					UnlockStarShip (&race_q[which_player], gms->hBattleShip);
				}
			}
			UnlockMutex (GraphicsLock);
		}
	}
	return TRUE;
}

HSTARSHIP
GetMeleeStarShip (STARSHIPPTR LastStarShipPtr, COUNT which_player)
{
	COUNT ships_left;
	TEXT t;
	UNICODE buf[10];
	STAMP s;
	CONTEXT OldContext;
	GETMELEE_STATE gmstate;
	STARSHIPPTR StarShipPtr;

	if (!(GLOBAL (CurrentActivity) & IN_BATTLE))
		return (0);

	s.frame = SetAbsFrameIndex (PickMeleeFrame, which_player);

	OldContext = SetContext (OffScreenContext);
	SetContextFGFrame (s.frame);
	if (LastStarShipPtr == 0 || LastStarShipPtr->special_counter == 0)
	{
		COUNT	cur_bucks;
		HSTARSHIP hBattleShip, hNextShip;

		cur_bucks = 0;
		for (hBattleShip = GetHeadLink (&race_q[which_player]);
				hBattleShip != 0; hBattleShip = hNextShip)
		{
			StarShipPtr = LockStarShip (&race_q[which_player], hBattleShip);
			if (StarShipPtr == LastStarShipPtr)
			{
				LastStarShipPtr->RaceResIndex = 0;

				gmstate.col = LastStarShipPtr->ShipFacing;
				s.origin.x = 3
					+ ((ICON_WIDTH + 2) * (gmstate.col % NUM_MELEE_COLS_ORIG));
				s.origin.y = 9
					+ ((ICON_HEIGHT + 2) * (gmstate.col / NUM_MELEE_COLS_ORIG));
				s.frame = SetAbsFrameIndex (StatusFrame, 3);
				DrawStamp (&s);
				s.frame = SetAbsFrameIndex (PickMeleeFrame, which_player);
			}
			else if (StarShipPtr->RaceResIndex)
			{
				cur_bucks += StarShipPtr->special_counter;
			}
			hNextShip = _GetSuccLink (StarShipPtr);
			UnlockStarShip (&race_q[which_player], hBattleShip);
		}

		GetFrameRect (s.frame, &gmstate.flash_rect);
		gmstate.flash_rect.extent.width -= 4;
		t.baseline.x = gmstate.flash_rect.extent.width;
		gmstate.flash_rect.corner.x = gmstate.flash_rect.extent.width - (6 * 3);
		gmstate.flash_rect.corner.y = 2;
		gmstate.flash_rect.extent.width = (6 * 3);
		gmstate.flash_rect.extent.height = 7 - 2;
		SetContextForeGroundColor (PICK_BG_COLOR);
		DrawFilledRectangle (&gmstate.flash_rect);

		wsprintf (buf, "%d", cur_bucks);
		t.baseline.y = 7;
		t.align = ALIGN_RIGHT;
		t.pStr = buf;
		t.CharCount = (COUNT)~0;
		SetContextFont (TinyFont);
		SetContextForeGroundColor (BUILD_COLOR (
				MAKE_RGB15 (0x13, 0x00, 0x00), 0x2C));
		font_DrawText (&t);
	}

	SetContext (SpaceContext);
	
	s.origin.x = PICK_X_OFFS - 3;
	s.origin.y = PICK_Y_OFFS - 9
		+ ((1 - which_player) * PICK_SIDE_OFFS);

	DrawStamp (&s);

	if (LOBYTE (battle_counter) == 0 || HIBYTE (battle_counter) == 0)
	{
		DWORD TimeOut;
		BOOLEAN PressState, ButtonState;

		s.origin.y = PICK_Y_OFFS - 9 + (which_player * PICK_SIDE_OFFS);
		s.frame = SetAbsFrameIndex (PickMeleeFrame,
				(COUNT) (1 - which_player));
		DrawStamp (&s);

		TimeOut = GetTimeCounter () + (ONE_SECOND * 4);
		SetContext (OldContext);
		UnlockMutex (GraphicsLock);

		PressState = AnyButtonPress (TRUE);
		do
		{
			ButtonState = AnyButtonPress (TRUE);
			if (PressState)
			{
				PressState = ButtonState;
				ButtonState = FALSE;
			}
		} while (!ButtonState
				&& (!(PlayerControl[0] & PlayerControl[1] & PSYTRON_CONTROL)
				|| (TaskSwitch (), GetTimeCounter ()) < TimeOut));

		LockMutex (GraphicsLock);

		return (0);
	}

	if (LastStarShipPtr == 0 && which_player)
	{
		BYTE fade_buf[] = {FadeAllToColor};
						
		SleepThreadUntil (XFormColorMap
				((COLORMAPPTR) fade_buf, ONE_SECOND / 2)
				+ ONE_SECOND / 60);
		FlushColorXForms ();
	}

	if (which_player == 0)
		ships_left = LOBYTE (battle_counter);
	else
		ships_left = HIBYTE (battle_counter);

	gmstate.flash_rect.extent.width = (ICON_WIDTH + 2);
	gmstate.flash_rect.extent.height = (ICON_HEIGHT + 2);
	gmstate.InputFunc = DoGetMelee;
	SetDefaultMenuRepeatDelay ();
	gmstate.Initialized = FALSE;
	gmstate.ships_left = ships_left;
	gmstate.which_player = which_player;

	UnlockMutex (GraphicsLock);
	ResetKeyRepeat ();
	DoInput ((PVOID)&gmstate, FALSE);

	LockMutex (GraphicsLock);
	SetFlashRect (NULL_PTR, (FRAME)0);
	
	if (gmstate.hBattleShip == 0)
		GLOBAL (CurrentActivity) &= ~IN_BATTLE;
	else
	{
		StarShipPtr = LockStarShip (&race_q[which_player], gmstate.hBattleShip);
		OwnStarShip (StarShipPtr,
				1 << which_player, StarShipPtr->captains_name_index);
		StarShipPtr->captains_name_index = 0;
		UnlockStarShip (&race_q[which_player], gmstate.hBattleShip);

		PlaySoundEffect (SetAbsSoundIndex (MenuSounds, 1), 0,
				NotPositional (), NULL, 0);

		WaitForSoundEnd (0);
	}

	return (gmstate.hBattleShip);
}


