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

#include "../build.h"
#include "../colors.h"
#include "../controls.h"
#include "../races.h"
#include "../shipcont.h"
#include "../setup.h"
#include "../sounds.h"
#include "port.h"
#include "libs/gfxlib.h"
#include "libs/tasklib.h"


// Ship icon positions in status display around the flagship
static const POINT ship_pos[MAX_COMBAT_SHIPS] =
{
	SUPPORT_SHIP_PTS
};

// Ship icon positions split into (lower half) left and right (upper)
// and sorted in the Y coord. These are used for navigation around the
// escort positions.
static POINT sorted_ship_pos[MAX_COMBAT_SHIPS];

static SHIP_FRAGMENT* LockSupportShip (MENU_STATE *pMS, HSHIPFRAG *phFrag);

static void
drawSupportShip (MENU_STATE *pMS, BOOLEAN filled)
{
	STAMP s;

	if (!pMS->flash_frame0)
		return;

	s.origin = pMS->first_item;
	s.frame = pMS->flash_frame0;
	
	if (filled)
		DrawFilledStamp (&s);
	else
		DrawStamp (&s);
}

static void
getSupportShipIcon (MENU_STATE *pMS)
{
	HSHIPFRAG hShipFrag;
	SHIP_FRAGMENT *ShipFragPtr;

	pMS->flash_frame0 = NULL;
	ShipFragPtr = LockSupportShip (pMS, &hShipFrag);
	if (!ShipFragPtr)
		return;

	pMS->flash_frame0 = ShipFragPtr->icons;
	UnlockShipFrag (&GLOBAL (built_ship_q), hShipFrag);
}

static void
flashSupportShip (MENU_STATE *pMS)
{
	static COLOR c = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x00, 0x00), 0x24);
	static TimeCount NextTime = 0;

	if (GetTimeCounter () >= NextTime)
	{
		NextTime = GetTimeCounter () + (ONE_SECOND / 15);
		
		if (c >= BUILD_COLOR (MAKE_RGB15 (0x1F, 0x19, 0x19), 0x24))
			c = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x00, 0x00), 0x24);
		else
			c += BUILD_COLOR (MAKE_RGB15 (0x00, 0x02, 0x02), 0x00);
		SetContextForeGroundColor (c);

		drawSupportShip (pMS, TRUE);
	}
}

static SHIP_FRAGMENT *
LockSupportShip (MENU_STATE *pMS, HSHIPFRAG *phFrag)
{
	const POINT *pship_pos;
	HSHIPFRAG hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q)),
			pship_pos = ship_pos;
			hStarShip; hStarShip = hNextShip, ++pship_pos)
	{
		SHIP_FRAGMENT *StarShipPtr;

		StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);

		if (pship_pos->x == pMS->first_item.x
				&& pship_pos->y == pMS->first_item.y)
		{
			*phFrag = hStarShip;
			return StarShipPtr;
		}

		hNextShip = _GetSuccLink (StarShipPtr);
		UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
	}

	return NULL;
}

static BOOLEAN
DeltaSupportCrew (MENU_STATE *pMS, SIZE crew_delta)
{
	BOOLEAN ret = FALSE;
	UNICODE buf[40];
	HFLEETINFO hTemplate;
	HSHIPFRAG hShipFrag;
	SHIP_FRAGMENT *StarShipPtr;
	FLEET_INFO *TemplatePtr;

	StarShipPtr = LockSupportShip (pMS, &hShipFrag);
	if (!StarShipPtr)
		return FALSE;

	hTemplate = GetStarShipFromIndex (&GLOBAL (avail_race_q),
			StarShipPtr->race_id);
	TemplatePtr = LockFleetInfo (&GLOBAL (avail_race_q), hTemplate);

	StarShipPtr->crew_level += crew_delta;

	if (StarShipPtr->crew_level == 0)
		StarShipPtr->crew_level = 1;
	else if (StarShipPtr->crew_level > TemplatePtr->crew_level &&
			crew_delta > 0)
		StarShipPtr->crew_level -= crew_delta;
	else
	{
		if (StarShipPtr->crew_level >= TemplatePtr->crew_level)
			sprintf (buf, "%u", StarShipPtr->crew_level);
		else
			sprintf (buf, "%u/%u",
					StarShipPtr->crew_level,
					TemplatePtr->crew_level);

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
			SetFlashRect (&r);
		}
		ret = TRUE;
	}

	UnlockFleetInfo (&GLOBAL (avail_race_q), hTemplate);
	UnlockShipFrag (&GLOBAL (built_ship_q), hShipFrag);

	return ret;
}

#define SHIP_TOGGLE ((BYTE)(1 << 7))

static void
RosterCleanup (MENU_STATE *pMS)
{
	SetContext (StatusContext);
	SetContextForeGroundColor (WHITE_COLOR);
	drawSupportShip (pMS, (pMS->CurState & SHIP_TOGGLE));
}

static BOOLEAN
DoModifyRoster (MENU_STATE *pMS)
{
	BYTE NewState;
	RECT r;
	BOOLEAN select, cancel, up, down, horiz;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return FALSE;

	select = PulsedInputState.menu[KEY_MENU_SELECT];
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL];
	up = PulsedInputState.menu[KEY_MENU_UP];
	down = PulsedInputState.menu[KEY_MENU_DOWN];
	// Left or right produces the same effect because there are 2 columns
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
		pMS->CurState = 0;
		pMS->first_item = sorted_ship_pos[pMS->CurState];

		LockMutex (GraphicsLock);
		SetContext (StatusContext);
		getSupportShipIcon (pMS);
		DeltaSupportCrew (pMS, 0);
		UnlockMutex (GraphicsLock);

		return TRUE;
	}
	else if (cancel && !(pMS->CurState & SHIP_TOGGLE))
	{
		LockMutex (GraphicsLock);
		SetFlashRect (NULL);
		RosterCleanup (pMS);
		DrawStatusMessage (NULL);
		UnlockMutex (GraphicsLock);

		return FALSE;
	}
	else if (select || cancel)
	{
		LockMutex (GraphicsLock);
		pMS->CurState ^= SHIP_TOGGLE;
		if (!(pMS->CurState & SHIP_TOGGLE))
			SetFlashRect (NULL);
		else
		{
			RosterCleanup (pMS);

			r.corner.x = 2;
			r.corner.y = 130;
			r.extent.width = STATUS_MESSAGE_WIDTH;
			r.extent.height = STATUS_MESSAGE_HEIGHT;
			SetContext (StatusContext);
			SetFlashRect (&r);
		}
		UnlockMutex (GraphicsLock);
	}
	else if (pMS->CurState & SHIP_TOGGLE)
	{
		SIZE delta = 0;
		BOOLEAN failed = FALSE;

		if (up)
		{
			if (GLOBAL_SIS (CrewEnlisted))
				delta = 1;
			else
				failed = TRUE;
		}
		else if (down)
		{
			if (GLOBAL_SIS (CrewEnlisted) < GetCPodCapacity (NULL))
				delta = -1;
			else
				failed = TRUE;
		}
		
		if (delta != 0)
		{
			LockMutex (GraphicsLock);
			failed = !DeltaSupportCrew (pMS, delta);
			UnlockMutex (GraphicsLock);
		}
		if (failed)
		{	// not enough room or crew
			PlayMenuSound (MENU_SOUND_FAILURE);
		}
	}
	else
	{
		POINT *pship_pos = sorted_ship_pos;
		BYTE num_escorts = (BYTE) pMS->delta_item;
		BYTE top_right = (num_escorts + 1) >> 1;

		NewState = pMS->CurState;
		
		if (horiz)
		{
			if (NewState == top_right - 1)
				NewState = num_escorts - 1;
			else if (NewState >= top_right)
			{
				NewState -= top_right;
				if (pship_pos[NewState].y < pship_pos[pMS->CurState].y)
					++NewState;
			}
			else
			{
				NewState += top_right;
				if (NewState != top_right
						&& pship_pos[NewState].y > pship_pos[pMS->CurState].y)
					--NewState;
			}
		}
		else if (down)
		{
			++NewState;
			if (NewState == num_escorts)
				NewState = top_right;
			else if (NewState == top_right)
				NewState = 0;
		}
		else if (up)
		{
			if (NewState == 0)
				NewState = top_right - 1;
			else if (NewState == top_right)
				NewState = num_escorts - 1;
			else
				--NewState;
		}

		LockMutex (GraphicsLock);
		BatchGraphics ();
		SetContext (StatusContext);

		if (NewState != pMS->CurState)
		{
			// Draw the previous escort in unselected state
			drawSupportShip (pMS, FALSE);
			// Select the new one
			pMS->first_item = pship_pos[NewState];
			getSupportShipIcon (pMS);
			DeltaSupportCrew (pMS, 0);
			pMS->CurState = NewState;
		}

		flashSupportShip (pMS);

		UnbatchGraphics ();
		UnlockMutex (GraphicsLock);
	}

	SleepThread (ONE_SECOND / 30);

	return TRUE;
}

static int
compShipPos (const void *ptr1, const void *ptr2)
{
	POINT *pt1 = (POINT *) ptr1;
	POINT *pt2 = (POINT *) ptr2;

	// Ships on the left in the lower half
	if (pt1->x < pt2->x)
		return -1;
	else if (pt1->x > pt2->x)
		return 1;

	// and ordered on Y
	if (pt1->y < pt2->y)
		return -1;
	else if (pt1->y > pt2->y)
		return 1;
	else
		return 0;
}

BOOLEAN
Roster (void)
{
	SIZE num_support_ships;

	num_support_ships = CountLinks (&GLOBAL (built_ship_q));
	if (num_support_ships)
	{
		MENU_STATE MenuState;
		MENU_STATE *pOldMenuState;

		pOldMenuState = pMenuState;
		pMenuState = &MenuState;

		// Get the ship positions we will use and sort on X then Y
		assert (sizeof (sorted_ship_pos) == sizeof (ship_pos));
		memcpy (sorted_ship_pos, ship_pos, sizeof (ship_pos));
		qsort (sorted_ship_pos, num_support_ships,
				sizeof (sorted_ship_pos[0]), compShipPos);

		memset (&MenuState, 0, sizeof (MenuState));
		MenuState.InputFunc = DoModifyRoster;
		MenuState.Initialized = FALSE;
		MenuState.delta_item = num_support_ships;
		
		SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
		DoInput (&MenuState, TRUE);

		pMenuState = pOldMenuState;
		
		return TRUE;
	}
	
	return FALSE;
}

