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

#define NUM_MELEE_COLS_ORIG 6
#define PICK_X_OFFS 57
#define PICK_Y_OFFS 24
#define PICK_SIDE_OFFS 100

#define NAME_AREA_HEIGHT 7
#define MELEE_WIDTH 133
#define MELEE_HEIGHT (48 + NAME_AREA_HEIGHT)

static void
DrawMeleeFrame (STARSHIPPTR LastStarShipPtr, COUNT which_player)
{
	TEXT t;
	UNICODE buf[10];
	STAMP s;
	RECT r;

	GetFrameRect (PickMeleeFrame, &r);
	r.corner.x = PICK_X_OFFS - 3;
	r.corner.y = PICK_Y_OFFS - 9 + ((1 - which_player) * PICK_SIDE_OFFS);

	s.origin = r.corner;
	s.frame = PickMeleeFrame;
	DrawStamp (&s);

	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + (r.extent.height - NAME_AREA_HEIGHT + 4);
	r.corner.x += 2;
	r.corner.y += 2;
	r.extent.width -= (2 * 2) + (ICON_WIDTH + 2) + 1;
	r.extent.height -= (2 * 2) + NAME_AREA_HEIGHT;
	SetContextForeGroundColor (PICK_BG_COLOR);
	DrawFilledRectangle (&r);
	r.corner.x += 2;
	r.extent.width += (ICON_WIDTH + 2) - (2 * 2);
	r.corner.y += r.extent.height;
	r.extent.height = NAME_AREA_HEIGHT;
	DrawFilledRectangle (&r);
	t.align = ALIGN_CENTER;
	t.pStr = pMeleeState->TeamImage[1 - which_player].TeamName;
	t.CharCount = (COUNT)~0;
	SetContextFont (TinyFont);
	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0xA, 0xA, 0x1F), 0x9));
	DrawText (&t);

	r.corner.x = PICK_X_OFFS - 3;
	r.corner.y = PICK_Y_OFFS - 9 + ((1 - which_player) * PICK_SIDE_OFFS);

	wsprintf (buf, "%d", pMeleeState->star_bucks[1 - which_player]);
	t.baseline.x = r.corner.x + 4;
	t.baseline.y = r.corner.y + 7;
	t.align = ALIGN_LEFT;
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x4, 0x5, 0x1F), 0x4B));
	DrawText (&t);

	if (LastStarShipPtr == 0 || LastStarShipPtr->special_counter == 0)
	{
		COUNT cur_bucks;
		HSTARSHIP hBattleShip, hNextShip;

		cur_bucks = 0;
		for (hBattleShip = GetHeadLink (&race_q[which_player]);
				hBattleShip != 0; hBattleShip = hNextShip)
		{
			int col;
			STARSHIPPTR StarShipPtr;

			StarShipPtr = LockStarShip (&race_q[which_player], hBattleShip);
			if (StarShipPtr == LastStarShipPtr)
				LastStarShipPtr->RaceResIndex = 0;

			col = StarShipPtr->ShipFacing;
			s.origin.x = r.corner.x + 4
					+ ((ICON_WIDTH + 2) * (col % NUM_MELEE_COLS_ORIG));
			s.origin.y = r.corner.y + 10
					+ ((ICON_HEIGHT + 2) * (col / NUM_MELEE_COLS_ORIG));
			s.frame = StarShipPtr->RaceDescPtr->ship_info.icons;
			DrawStamp (&s);

			if (StarShipPtr->RaceResIndex)
				cur_bucks += StarShipPtr->special_counter;
			else
			{
				extern FRAME status;

				// Draw the X through destroyed ships
				--s.origin.x;
				--s.origin.y;
				s.frame = SetAbsFrameIndex (status, 3);
				DrawStamp (&s);
			}
			hNextShip = _GetSuccLink (StarShipPtr);
			UnlockStarShip (&race_q[which_player], hBattleShip);
		}

		r.extent.width -= 4;
		t.baseline.x = r.corner.x + r.extent.width;
		t.baseline.y = r.corner.y + 7;
		r.corner.x += r.extent.width - (6 * 3);
		r.corner.y += 2;
		r.extent.width = (6 * 3);
		r.extent.height = 7 - 2;
		SetContextForeGroundColor (PICK_BG_COLOR);
		DrawFilledRectangle (&r);

		wsprintf (buf, "%d", cur_bucks);
		t.align = ALIGN_RIGHT;
		t.pStr = buf;
		t.CharCount = (COUNT)~0;
		SetContextFont (TinyFont);
		SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x13, 0x00, 0x00), 0x2C));
		DrawText (&t);
	}
}

HSTARSHIP
GetMeleeStarShip (STARSHIPPTR LastStarShipPtr, COUNT which_player)
{
	COUNT ships_left, row, col;
	DWORD NewTime, OldTime, LastTime;
	INPUT_STATE OldInputState;
	HSTARSHIP hBattleShip, hNextShip;
	STARSHIPPTR StarShipPtr;
	RECT flash_rect;

	if (!(GLOBAL (CurrentActivity) & IN_BATTLE))
		return (0);

	SetContext (SpaceContext);

	DrawMeleeFrame (LastStarShipPtr, which_player);

	if (LOBYTE (battle_counter) == 0 || HIBYTE (battle_counter) == 0)
	{
		DWORD TimeOut;
		INPUT_STATE PressState, ButtonState;

		DrawMeleeFrame (LastStarShipPtr, 1 - which_player);

		TimeOut = GetTimeCounter () + (ONE_SECOND * 4);
		ClearSemaphore (GraphicsSem);

		PressState = AnyButtonPress (TRUE);
		do
		{
			ButtonState = AnyButtonPress (TRUE);
			if (PressState)
			{
				PressState = ButtonState;
				ButtonState = 0;
			}
		} while (!ButtonState
				&& (!(PlayerControl[0] & PlayerControl[1] & PSYTRON_CONTROL)
				|| (TaskSwitch (), GetTimeCounter ()) < TimeOut));

		if (ButtonState)
			ButtonState = GetInputState (NormalInput);
		if (ButtonState & DEVICE_EXIT)
			ConfirmExit ();

		SetSemaphore (GraphicsSem);

		return (0);
	}

	if (LastStarShipPtr == 0 && which_player)
	{
		BYTE fade_buf[] = {FadeAllToColor};
						
		SleepThreadUntil (XFormColorMap
				((COLORMAPPTR) fade_buf, ONE_SECOND / 2) + 2);
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
		INPUT_STATE InputState;

		SleepThread (1);
		NewTime = GetTimeCounter ();
		
		if ((InputState = GetInputState (PlayerInput[which_player])) == 0)
			InputState = GetInputState (ArrowInput);
		if (InputState)
			LastTime = NewTime;
		else if (!(PlayerControl[1 - which_player] & PSYTRON_CONTROL)
				&& NewTime - LastTime >= ONE_SECOND * 3)
			InputState = GetInputState (PlayerInput[1 - which_player]);
		if (InputState & DEVICE_EXIT)
		{
			if (ConfirmExit ())
			{
				hBattleShip = 0;
				break;
			}
			continue;
		}

		if (InputState == OldInputState
				&& NewTime - OldTime < (DWORD)MENU_REPEAT_DELAY)
			InputState = 0;
		else
		{
			OldInputState = InputState;
			OldTime = NewTime;
		}

		if (InputState & DEVICE_BUTTON1)
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
			if (GetInputXComponent (InputState) < 0)
			{
				if (new_col-- == 0)
					new_col = NUM_MELEE_COLS_ORIG;
			}
			else if (GetInputXComponent (InputState) > 0)
			{
				if (new_col++ == NUM_MELEE_COLS_ORIG)
					new_col = 0;
			}
			if (GetInputYComponent (InputState) < 0)
			{
				if (new_row-- == 0)
					new_row = NUM_MELEE_ROWS - 1;
			}
			else if (GetInputYComponent (InputState) > 0)
			{
				if (++new_row == NUM_MELEE_ROWS)
					new_row = 0;
			}

			if (new_row != row || new_col != col)
			{
				COUNT ship_index;

				row = new_row;
				col = new_col;

				PlaySoundEffect (MenuSounds, 0, 0);
				SetSemaphore (GraphicsSem);
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
				ClearSemaphore (GraphicsSem);
			}
		}
	}

	SetSemaphore (GraphicsSem);
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

		PlaySoundEffect (SetAbsSoundIndex (MenuSounds, 1), 0, 0);

		while (ChannelPlaying (0))
			;
	}

	return (hBattleShip);
}


