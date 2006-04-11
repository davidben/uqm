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

#include "build.h"
#include "colors.h"
#include "controls.h"
#include "races.h"
#include "shipcont.h"
#include "setup.h"
#include "sounds.h"
#include "libs/gfxlib.h"
#include "libs/tasklib.h"


static int
flash_ship_task (void *data)
{
	DWORD TimeIn;
	COLOR c;
	Task task = (Task) data;

	c = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x00, 0x00), 0x24);
	TimeIn = GetTimeCounter ();
	while (!Task_ReadState (task, TASK_EXIT))
	{
		STAMP s;
		SHIP_FRAGMENTPTR StarShipPtr;
		COLOR OldColor;
		CONTEXT OldContext;

		LockMutex (GraphicsLock);
		s.origin = pMenuState->first_item;
		StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (built_ship_q), (HSTARSHIP)pMenuState->CurFrame);
		s.frame = StarShipPtr->ShipInfo.icons;
		UnlockStarShip (&GLOBAL (built_ship_q),
				(HSTARSHIP)pMenuState->CurFrame);
		OldContext = SetContext (StatusContext);
		if (c >= BUILD_COLOR (MAKE_RGB15 (0x1F, 0x19, 0x19), 0x24))
			c = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x00, 0x00), 0x24);
		else
			c += BUILD_COLOR (MAKE_RGB15 (0x00, 0x02, 0x02), 0x00);
		OldColor = SetContextForeGroundColor (c);
		DrawFilledStamp (&s);
		SetContextForeGroundColor (OldColor);
		SetContext (OldContext);
		UnlockMutex (GraphicsLock);
		SleepThreadUntil (TimeIn + ONE_SECOND / 15);
		TimeIn = GetTimeCounter ();
	}
	FinishTask (task);
	return 0;
}

static HSTARSHIP
MatchSupportShip (PMENU_STATE pMS)
{
	PPOINT pship_pos;
	HSTARSHIP hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q)),
			pship_pos = (PPOINT)pMS->flash_frame0;
			hStarShip; hStarShip = hNextShip, ++pship_pos)
	{
		SHIP_FRAGMENTPTR StarShipPtr;

		StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (built_ship_q), hStarShip);

		if (pship_pos->x == pMS->first_item.x
				&& pship_pos->y == pMS->first_item.y)
		{
			UnlockStarShip (&GLOBAL (built_ship_q), hStarShip);
			return hStarShip;
		}

		hNextShip = _GetSuccLink (StarShipPtr);
		UnlockStarShip (&GLOBAL (built_ship_q), hStarShip);
	}

	return 0;
}

static BOOLEAN
DeltaSupportCrew (SIZE crew_delta)
{
	BOOLEAN ret = FALSE;
	UNICODE buf[40];
	HSTARSHIP hTemplate;
	SHIP_FRAGMENTPTR StarShipPtr, TemplatePtr;

	StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (built_ship_q), (HSTARSHIP)pMenuState->CurFrame);
	hTemplate = GetStarShipFromIndex (&GLOBAL (avail_race_q),
			GET_RACE_ID (StarShipPtr));
	TemplatePtr = (SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (avail_race_q), hTemplate);

	StarShipPtr->ShipInfo.crew_level += crew_delta;

	if (StarShipPtr->ShipInfo.crew_level == 0)
		StarShipPtr->ShipInfo.crew_level = 1;
	else if (StarShipPtr->ShipInfo.crew_level >
			TemplatePtr->RaceDescPtr->ship_info.crew_level &&
			crew_delta > 0)
		StarShipPtr->ShipInfo.crew_level -= crew_delta;
	else
	{
		if (StarShipPtr->ShipInfo.crew_level >=
				TemplatePtr->RaceDescPtr->ship_info.crew_level)
			sprintf (buf, "%u", StarShipPtr->ShipInfo.crew_level);
		else
			sprintf (buf, "%u/%u",
					StarShipPtr->ShipInfo.crew_level,
					TemplatePtr->RaceDescPtr->ship_info.crew_level);

		DrawStatusMessage (buf);
		DeltaSISGauges (-crew_delta, 0, 0);
		if (crew_delta)
		{
			RECT r;

			r.corner.x = 2;
			r.corner.y = 130;
			r.extent.width = STATUS_MESSAGE_WIDTH;
			r.extent.height = STATUS_MESSAGE_HEIGHT;
			SetContext (StatusContext);
			SetFlashRect (&r, (FRAME)0);
		}
		ret = TRUE;
	}

	UnlockStarShip (&GLOBAL (avail_race_q), hTemplate);
	UnlockStarShip (&GLOBAL (built_ship_q), (HSTARSHIP)pMenuState->CurFrame);

	return ret;
}

#define SHIP_TOGGLE ((BYTE)(1 << 7))

static void
RosterCleanup (PMENU_STATE pMS)
{
	if (pMS->flash_task)
	{
		UnlockMutex (GraphicsLock);
		ConcludeTask (pMS->flash_task);
		LockMutex (GraphicsLock);
		pMS->flash_task = 0;
	}

	if (pMS->CurFrame)
	{
		STAMP s;
		SHIP_FRAGMENTPTR StarShipPtr;

		SetContext (StatusContext);
		s.origin = pMS->first_item;
		StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (built_ship_q), (HSTARSHIP)pMS->CurFrame);
		s.frame = StarShipPtr->ShipInfo.icons;
		UnlockStarShip (&GLOBAL (built_ship_q), (HSTARSHIP)pMS->CurFrame);
		if (!(pMS->CurState & SHIP_TOGGLE))
			DrawStamp (&s);
		else
		{
			SetContextForeGroundColor (WHITE_COLOR);
			DrawFilledStamp (&s);
		}
	}
}

static BOOLEAN
DoModifyRoster (PMENU_STATE pMS)
{
	BYTE NewState;
	SBYTE sx, sy;
	RECT r;
	STAMP s;
	SHIP_FRAGMENTPTR StarShipPtr;
	BOOLEAN select, cancel, up, down, horiz;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		LockMutex (GraphicsLock);
		RosterCleanup (pMS);
		UnlockMutex (GraphicsLock);
		pMS->CurFrame = 0;

		return FALSE;
	}

	select = PulsedInputState.menu[KEY_MENU_SELECT];
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL];
	up = PulsedInputState.menu[KEY_MENU_UP];
	down = PulsedInputState.menu[KEY_MENU_DOWN];
	horiz = PulsedInputState.menu[KEY_MENU_LEFT] ||
			PulsedInputState.menu[KEY_MENU_RIGHT];

	if (pMS->Initialized && (pMS->CurState & SHIP_TOGGLE))
	{
		SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN,
				MENU_SOUND_SELECT | MENU_SOUND_CANCEL);
	}
	else
	{
		SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	}

	if (!pMS->Initialized)
	{
		pMS->InputFunc = DoModifyRoster;
		pMS->Initialized = TRUE;

		pMS->CurState = NewState = 0;
		LockMutex (GraphicsLock);
		SetContext (StatusContext);
		goto SelectSupport;
	}
	else if (cancel && !(pMS->CurState & SHIP_TOGGLE))
	{
		LockMutex (GraphicsLock);
		SetFlashRect (NULL_PTR, (FRAME)0);
		RosterCleanup (pMS);
		pMS->CurFrame = 0;
		DrawStatusMessage (NULL_PTR);
		UnlockMutex (GraphicsLock);

		return FALSE;
	}
	else if (select || cancel)
	{
		LockMutex (GraphicsLock);
		pMS->CurState ^= SHIP_TOGGLE;
		if (!(pMS->CurState & SHIP_TOGGLE))
			SetFlashRect (NULL_PTR, (FRAME)0);
		else
		{
			RosterCleanup (pMS);

			r.corner.x = 2;
			r.corner.y = 130;
			r.extent.width = STATUS_MESSAGE_WIDTH;
			r.extent.height = STATUS_MESSAGE_HEIGHT;
			SetContext (StatusContext);
			SetFlashRect (&r, (FRAME)0);
		}
		UnlockMutex (GraphicsLock);
	}
	else if (pMS->CurState & SHIP_TOGGLE)
	{
		SIZE delta = 0;
		BOOLEAN failed = FALSE;

		if (up)
		{
			sy = -1;
			if (GLOBAL_SIS (CrewEnlisted))
				delta = 1;
			else
				failed = TRUE;
		}
		else if (down)
		{
			sy = 1;
			if (GLOBAL_SIS (CrewEnlisted) < GetCPodCapacity (NULL_PTR))
				delta = -1;
			else
				failed = TRUE;
		}
		
		if (delta != 0)
		{
			LockMutex (GraphicsLock);
			failed = !DeltaSupportCrew (delta);
			UnlockMutex (GraphicsLock);
		}
		if (failed)
		{	// not enough room or crew
			PlayMenuSound (MENU_SOUND_FAILURE);
		}
	}
	else
	{
		PPOINT pship_pos;

		NewState = pMS->CurState;
		sx = (SBYTE)((pMS->delta_item + 1) >> 1);
		if (horiz)
		{
			pship_pos = (PPOINT)pMS->flash_frame1;
			if (NewState == (BYTE)(sx - 1))
				NewState = (BYTE)(pMS->delta_item - 1);
			else if (NewState >= (BYTE)sx)
			{
				NewState -= sx;
				if (pship_pos[NewState].y < pship_pos[pMS->CurState].y)
					++NewState;
			}
			else
			{
				NewState += sx;
				if (NewState != (BYTE)sx
						&& pship_pos[NewState].y > pship_pos[pMS->CurState].y)
					--NewState;
			}
		}
		else if (down)
		{
			sy = 1;
			if (++NewState == (BYTE)pMS->delta_item)
				NewState = (BYTE)(sx - 1);
			else if (NewState == (BYTE)sx)
				NewState = 0;
		}
		else if (up)
		{
			sy = -1;
			if (NewState == 0)
				NewState += sx - 1;
			else if (NewState == (BYTE)sx)
				NewState = (BYTE)(pMS->delta_item - 1);
			else
				--NewState;
		}

		if (NewState != pMS->CurState)
		{
			LockMutex (GraphicsLock);
			SetContext (StatusContext);
			s.origin = pMS->first_item;
			StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
					&GLOBAL (built_ship_q),
					(HSTARSHIP)pMS->CurFrame
					);
			s.frame = StarShipPtr->ShipInfo.icons;
			UnlockStarShip (
					&GLOBAL (built_ship_q),
					(HSTARSHIP)pMS->CurFrame
					);
			DrawStamp (&s);
SelectSupport:
			pship_pos = (PPOINT)pMS->flash_frame1;
			pMS->first_item = pship_pos[NewState];
			pMS->CurFrame = (FRAME)MatchSupportShip (pMS);

			DeltaSupportCrew (0);
			UnlockMutex (GraphicsLock);

			pMS->CurState = NewState;
		}

		if (pMS->flash_task == 0)
			pMS->flash_task = AssignTask (flash_ship_task, 2048,
					"flash roster menu");
	}

	return TRUE;
}

BOOLEAN
Roster (void)
{
	COUNT num_support_ships;

	num_support_ships = CountLinks (&GLOBAL (built_ship_q));
	if (num_support_ships)
	{
		SIZE i, j, k, l;
		POINT modified_ship_pos[MAX_COMBAT_SHIPS];
		POINT ship_pos[MAX_COMBAT_SHIPS] =
		{
			SUPPORT_SHIP_PTS
		};
		MENU_STATE MenuState;
		PMENU_STATE pOldMenuState;

		pOldMenuState = pMenuState;
		pMenuState = &MenuState;

		j = 0;
		k = (num_support_ships + 1) >> 1;
		for (i = 0; (int)i < (int)num_support_ships; i += 2)
		{
			modified_ship_pos[j++] = ship_pos[i];
			modified_ship_pos[k++] = ship_pos[i + 1];
		}

		k = (num_support_ships + 1) >> 1;
		for (i = 0; i < k; ++i)
		{
			for (j = k - 1; j > i; --j)
			{
				if (modified_ship_pos[i].y > modified_ship_pos[j].y)
				{
					POINT temp;

					temp = modified_ship_pos[i];
					modified_ship_pos[i] = modified_ship_pos[j];
					modified_ship_pos[j] = temp;
				}
			}
		}

		l = k;
		k = num_support_ships >> 1;
		for (i = 0; i < k; ++i)
		{
			for (j = k - 1; j > i; --j)
			{
				if (modified_ship_pos[i + l].y > modified_ship_pos[j + l].y)
				{
					POINT temp;

					temp = modified_ship_pos[i + l];
					modified_ship_pos[i + l] = modified_ship_pos[j + l];
					modified_ship_pos[j + l] = temp;
				}
			}
		}

		MenuState.InputFunc = DoModifyRoster;
		MenuState.Initialized = FALSE;
		MenuState.CurState = 0;
		MenuState.flash_task = 0;
		MenuState.delta_item = (SIZE)num_support_ships;
		
		MenuState.flash_frame0 = (FRAME)ship_pos;
		MenuState.flash_frame1 = (FRAME)modified_ship_pos;
		SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
		DoInput ((PVOID)&MenuState, TRUE);

		pMenuState = pOldMenuState;
		
		return TRUE;
	}
	
	return FALSE;
}

