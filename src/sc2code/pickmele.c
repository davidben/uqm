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
#include "melee.h"

#define NUM_MELEE_COLS_ORIG NUM_MELEE_COLUMNS
#define PICK_X_OFFS 57
#define PICK_Y_OFFS 24
#define PICK_SIDE_OFFS 100


HSTARSHIP
GetMeleeStarShip (STARSHIPPTR LastStarShipPtr, COUNT which_player)
{
	COUNT ships_left, row, col;
	DWORD NewTime, OldTime, LastTime;
	BATTLE_INPUT_STATE OldInputState;
	HSTARSHIP hBattleShip, hNextShip;
	STARSHIPPTR StarShipPtr;
	RECT flash_rect;
	TEXT t;
	UNICODE buf[10];
	STAMP s;
	CONTEXT OldContext;

	if (!(GLOBAL (CurrentActivity) & IN_BATTLE))
		return (0);

	s.frame = SetAbsFrameIndex (PickMeleeFrame, which_player);

	OldContext = SetContext (OffScreenContext);
	SetContextFGFrame (s.frame);
	if (LastStarShipPtr == 0 || LastStarShipPtr->special_counter == 0)
	{
		COUNT	cur_bucks;

		cur_bucks = 0;
		for (hBattleShip = GetHeadLink (&race_q[which_player]);
				hBattleShip != 0; hBattleShip = hNextShip)
		{
			StarShipPtr = LockStarShip (&race_q[which_player], hBattleShip);
			if (StarShipPtr == LastStarShipPtr)
			{
				extern FRAME status;

				LastStarShipPtr->RaceResIndex = 0;

				col = LastStarShipPtr->ShipFacing;
				s.origin.x = 3
					+ ((ICON_WIDTH + 2) * (col % NUM_MELEE_COLS_ORIG));
				s.origin.y = 9
					+ ((ICON_HEIGHT + 2) * (col / NUM_MELEE_COLS_ORIG));
				s.frame = SetAbsFrameIndex (status, 3);
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

		GetFrameRect (s.frame, &flash_rect);
		flash_rect.extent.width -= 4;
		t.baseline.x = flash_rect.extent.width;
		flash_rect.corner.x = flash_rect.extent.width - (6 * 3);
		flash_rect.corner.y = 2;
		flash_rect.extent.width = (6 * 3);
		flash_rect.extent.height = 7 - 2;
		SetContextForeGroundColor (PICK_BG_COLOR);
		DrawFilledRectangle (&flash_rect);

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

		UpdateInputState ();
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

		/*
		if (ButtonState)
			ButtonState = GetInputState (NormalInput);
		if (ButtonState & DEVICE_EXIT)
			ConfirmExit ();
		*/

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

	row = 0;
	col = NUM_MELEE_COLS_ORIG;
	if (which_player == 0)
		ships_left = LOBYTE (battle_counter);
	else
		ships_left = HIBYTE (battle_counter);

	flash_rect.extent.width = (ICON_WIDTH + 2);
	flash_rect.extent.height = (ICON_HEIGHT + 2);

	NewTime = OldTime = LastTime = GetTimeCounter ();
	OldInputState = 0;
	goto ChangeSelection;
	for (;;)
	{
		BATTLE_INPUT_STATE InputState;

		SleepThread (ONE_SECOND / 120);
		NewTime = GetTimeCounter ();
		
		UpdateInputState ();
		InputState = (*(PlayerInput[which_player])) ();
		if (InputState)
			LastTime = NewTime;
		else if (!(PlayerControl[1 - which_player] & PSYTRON_CONTROL)
				&& NewTime - LastTime >= ONE_SECOND * 3)
		  InputState = (*(PlayerInput[1 - which_player])) ();
		if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		{
			hBattleShip = 0;
			GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
			break;
		}

		if (InputState == OldInputState
				&& NewTime - OldTime < (DWORD)MENU_REPEAT_DELAY)
			InputState = 0;
		else
		{
			OldInputState = InputState;
			OldTime = NewTime;
		}

		if (InputState & BATTLE_WEAPON || CurrentMenuState.select)
		{
			if (hBattleShip || (col == NUM_MELEE_COLS_ORIG && ConfirmExit ()))
			{
				GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
				break;
			}
		}
		else
		{
			COUNT new_row, new_col;

			new_row = row;
			new_col = col;
			if ((InputState & BATTLE_LEFT) || CurrentMenuState.left)
			{
				if (new_col-- == 0)
					new_col = NUM_MELEE_COLS_ORIG;
			}
			else if ((InputState & BATTLE_RIGHT) || CurrentMenuState.right)
			{
				if (new_col++ == NUM_MELEE_COLS_ORIG)
					new_col = 0;
			}
			if ((InputState & BATTLE_THRUST) || CurrentMenuState.up)
			{
				if (new_row-- == 0)
					new_row = NUM_MELEE_ROWS - 1;
			}
			else if ((InputState & BATTLE_DOWN) || CurrentMenuState.down)
			{
				if (++new_row == NUM_MELEE_ROWS)
					new_row = 0;
			}

			if (new_row != row || new_col != col)
			{
				COUNT ship_index;

				row = new_row;
				col = new_col;

				PlaySoundEffect (MenuSounds, 0, NotPositional (), NULL, 0);
				LockMutex (GraphicsLock);
ChangeSelection:
				flash_rect.corner.x = PICK_X_OFFS
						+ ((ICON_WIDTH + 2) * col);
				flash_rect.corner.y = PICK_Y_OFFS
						+ ((ICON_HEIGHT + 2) * row)
						+ ((1 - which_player) * PICK_SIDE_OFFS);
				SetFlashRect (&flash_rect, (FRAME)0);

				hBattleShip = GetHeadLink (&race_q[which_player]);
				if (col == NUM_MELEE_COLS_ORIG)
				{
					if (row)
						hBattleShip = 0;
					else
					{
						ship_index = (COUNT)TFB_Random () % ships_left;
						for (hBattleShip = GetHeadLink (&race_q[which_player]);
								hBattleShip != 0; hBattleShip = hNextShip)
						{
							StarShipPtr = LockStarShip (&race_q[which_player], hBattleShip);
							if (StarShipPtr->RaceResIndex && ship_index-- == 0)
							{
								UnlockStarShip (&race_q[which_player], hBattleShip);
								break;
							}
							hNextShip = _GetSuccLink (StarShipPtr);
							UnlockStarShip (&race_q[which_player], hBattleShip);
						}
					}
				}
				else
				{
					ship_index = (row * NUM_MELEE_COLS_ORIG) + col;
					for (hBattleShip = GetHeadLink (&race_q[which_player]);
							hBattleShip != 0; hBattleShip = hNextShip)
					{
						StarShipPtr = LockStarShip (&race_q[which_player], hBattleShip);
						if (StarShipPtr->ShipFacing == ship_index)
						{
							hNextShip = hBattleShip;
							if (StarShipPtr->RaceResIndex == 0)
								hBattleShip = 0;
							UnlockStarShip (&race_q[which_player], hNextShip);
							break;
						}
						hNextShip = _GetSuccLink (StarShipPtr);
						UnlockStarShip (&race_q[which_player], hBattleShip);
					}
				}
				UnlockMutex (GraphicsLock);
			}
		}
	}

	LockMutex (GraphicsLock);
	SetFlashRect (NULL_PTR, (FRAME)0);
	
	if (hBattleShip == 0)
		GLOBAL (CurrentActivity) &= ~IN_BATTLE;
	else
	{
		StarShipPtr = LockStarShip (&race_q[which_player], hBattleShip);
		OwnStarShip (StarShipPtr,
				1 << which_player, StarShipPtr->captains_name_index);
		StarShipPtr->captains_name_index = 0;
		UnlockStarShip (&race_q[which_player], hBattleShip);

		PlaySoundEffect (SetAbsSoundIndex (MenuSounds, 1), 0, NotPositional (), NULL, 0);

		WaitForSoundEnd (0);
	}

	return (hBattleShip);
}


