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
#include "controls.h"
#include "libs/graphics/gfx_common.h"

//Added by Chris

#ifdef WANT_SHIP_SPINS
void DoShipSpin (COUNT index, MUSIC_REF hMusic); // Defined in fmv.c
#endif

void GetGaugeRect (PRECT pRect, BOOLEAN IsCrewRect); // Defined in sis.c

//End added by Chris

#ifdef USE_3DO_HANGAR
// 3DO 4x3 hangar layout
#	define HANGAR_SHIPS_ROW  4
#	define HANGAR_Y          64
#	define HANGAR_DY         44

static const COORD hangar_x_coords[HANGAR_SHIPS_ROW] =
{
	19, 60, 116, 157
};

#else // use PC hangar
// modified PC 6x2 hangar layout
#	define HANGAR_SHIPS_ROW  6
#	define HANGAR_Y          88
#	define HANGAR_DY         84

static const COORD hangar_x_coords[HANGAR_SHIPS_ROW] =
{
	0, 38, 76,  131, 169, 207
};

#	define WANT_HANGAR_ANIMATION
#endif // USE_3DO_HANGAR

#define HANGAR_SHIPS      12
#define HANGAR_ROWS       (HANGAR_SHIPS / HANGAR_SHIPS_ROW)
#define HANGAR_ANIM_RATE  15 // fps

enum
{
	SHIPYARD_CREW,
	SHIPYARD_SAVELOAD,
	SHIPYARD_EXIT
};

int hangar_anim_func (void* data)
{
	DWORD TimeIn;
	STAMP s;
	Task task = (Task) data;
	COLORMAP ColorMap;
	RECT ClipRect;
	
	if (!pMenuState->CurString)
	{
		FinishTask (task);
		return -1;
	}
	s.origin.x = s.origin.y = 0;
	s.frame = SetAbsFrameIndex (pMenuState->CurFrame, 24);
	ClipRect = pMenuState->flash_rect1;
	ColorMap = SetAbsColorMapIndex (pMenuState->CurString, 0);
	
	TimeIn = GetTimeCounter ();
	while (!Task_ReadState (task, TASK_EXIT))
	{
		CONTEXT OldContext;
		RECT OldClipRect;

		SetSemaphore (GraphicsSem);
		OldContext = SetContext (ScreenContext);
		GetContextClipRect (&OldClipRect);
		SetContextClipRect (&ClipRect);

		ColorMap = SetRelColorMapIndex (ColorMap, 1);
		SetColorMap (GetColorMapAddress (ColorMap));
		DrawStamp (&s);
		
		SetContextClipRect (&OldClipRect);
		SetContext (OldContext);
		ClearSemaphore (GraphicsSem);
		
		SleepThreadUntil (TimeIn + ONE_SECOND / HANGAR_ANIM_RATE);
		TimeIn = GetTimeCounter ();
	}

	FinishTask (task);
	return(0);
}

#ifdef WANT_SHIP_SPINS
static void
SpinStarShip (HSTARSHIP hStarShip)
{
	COUNT Index;
	HSTARSHIP hNextShip, hShip;
	STARSHIPPTR StarShipPtr;
	extern QUEUE master_q;

	StarShipPtr = LockStarShip (&GLOBAL (built_ship_q), hStarShip);

	for (Index = 0, hShip = GetHeadLink (&master_q);
			hShip; hShip = hNextShip, ++Index)
	{
		STARSHIPPTR SSPtr;

		SSPtr = LockStarShip (&master_q, hShip);
		if (StarShipPtr->RaceResIndex == SSPtr->RaceResIndex)
			break;
		hNextShip = _GetSuccLink (SSPtr);
		UnlockStarShip (&master_q, hShip);
	}

	UnlockStarShip (&master_q, hStarShip);
				
	if (Index < NUM_MELEE_SHIPS)
		DoShipSpin (Index, pMenuState->hMusic);
}
#endif

static COUNT
GetAvailableRaceCount (void)
{
	COUNT Index;
	HSTARSHIP hStarShip, hNextShip;

	Index = 0;
	for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip; hStarShip = hNextShip)
	{
		SHIP_FRAGMENTPTR StarShipPtr;

		StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (avail_race_q), hStarShip
				);
		if (StarShipPtr->ShipInfo.ship_flags & GOOD_GUY)
			++Index;

		hNextShip = _GetSuccLink (StarShipPtr);
		UnlockStarShip (
				&GLOBAL (avail_race_q), hStarShip
				);
	}

	return (Index);
}

static HSTARSHIP
GetAvailableRaceFromIndex (BYTE Index)
{
	HSTARSHIP hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip; hStarShip = hNextShip)
	{
		SHIP_FRAGMENTPTR StarShipPtr;

		StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (avail_race_q), hStarShip
				);
		if ((StarShipPtr->ShipInfo.ship_flags & GOOD_GUY) && Index-- == 0)
		{
			UnlockStarShip (
					&GLOBAL (avail_race_q), hStarShip
					);
			return (hStarShip);
		}

		hNextShip = _GetSuccLink (StarShipPtr);
		UnlockStarShip (
				&GLOBAL (avail_race_q), hStarShip
				);
	}

	return (0);
}

static void
DrawRaceStrings (BYTE NewRaceItem)
{
	RECT r;
	STAMP s;
	CONTEXT OldContext;
	
	SetSemaphore (GraphicsSem);

	OldContext = SetContext (StatusContext);
	GetContextClipRect (&r);
	s.origin.x = RADAR_X - r.corner.x;
	s.origin.y = RADAR_Y - r.corner.y;
	r.corner.x = s.origin.x - 1;
	r.corner.y = s.origin.y - 11;
	r.extent.width = RADAR_WIDTH + 2;
	r.extent.height = 11;
	BatchGraphics ();
	ClearSISRect (CLEAR_SIS_RADAR);
	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0xA, 0xA, 0xA), 0x08));
	DrawFilledRectangle (&r);
	r.corner = s.origin;
	r.extent.width = RADAR_WIDTH;
	r.extent.height = RADAR_HEIGHT;
	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00));
	DrawFilledRectangle (&r);
	if (NewRaceItem != (BYTE)~0)
	{
		TEXT t;
		HSTARSHIP hStarShip;
		STARSHIPPTR StarShipPtr;
		UNICODE buf[10];
		COUNT ShipCost[] =
		{
			RACE_SHIP_COST
		};

		hStarShip = GetAvailableRaceFromIndex (NewRaceItem);
		NewRaceItem = GetIndexFromStarShip (&GLOBAL (avail_race_q), hStarShip);
		s.frame = SetAbsFrameIndex (pMenuState->ModuleFrame, 3 + NewRaceItem);
		DrawStamp (&s);
		StarShipPtr = LockStarShip (&GLOBAL (avail_race_q), hStarShip);
		s.frame = StarShipPtr->RaceDescPtr->ship_info.melee_icon;
		UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);

		t.baseline.x = s.origin.x + RADAR_WIDTH - 2;
		t.baseline.y = s.origin.y + RADAR_HEIGHT - 2;
		s.origin.x += (RADAR_WIDTH >> 1);
		s.origin.y += (RADAR_HEIGHT >> 1);
		DrawStamp (&s);
		t.align = ALIGN_RIGHT;
		t.CharCount = (COUNT)~0;
		t.pStr = buf;
		wsprintf (buf, "%u", ShipCost[NewRaceItem]);
		SetContextFont (TinyFont);
		SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x1F, 0x00), 0x02));
		font_DrawText (&t);
	}
	UnbatchGraphics ();

	// Flash the ship purchase menu even when optMenu == OPT_PC
	{
		RECT flash_r;
		GetContextClipRect (&flash_r);
		flash_r.corner.x = RADAR_X - flash_r.corner.x;
		flash_r.corner.y = RADAR_Y - flash_r.corner.y;
		flash_r.extent.width = RADAR_WIDTH;
		flash_r.extent.height = RADAR_HEIGHT;
		SetFlashRect (&flash_r, (FRAME)0);
	}
	//SetFlashRect ((PRECT)~0L, (FRAME)0);
	SetContext (OldContext);
	ClearSemaphore (GraphicsSem);
}

#define SHIP_WIN_WIDTH 34
#define SHIP_WIN_HEIGHT (SHIP_WIN_WIDTH + 6)
#define SHIP_WIN_FRAMES ((SHIP_WIN_WIDTH >> 1) + 1)

static void
ShowShipCrew (SHIP_FRAGMENTPTR StarShipPtr, PRECT pRect)
{
	RECT r;
	TEXT t;
	UNICODE buf[20];
	HSTARSHIP hTemplate;
	SHIP_FRAGMENTPTR TemplatePtr;

	hTemplate = GetStarShipFromIndex (
			&GLOBAL (avail_race_q),
			GET_RACE_ID (StarShipPtr)
			);
	TemplatePtr = (SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (avail_race_q), hTemplate
			);
	if (StarShipPtr->ShipInfo.crew_level >=
			TemplatePtr->RaceDescPtr->ship_info.crew_level)
		wsprintf (buf, "%u", StarShipPtr->ShipInfo.crew_level);
	else
		wsprintf (buf, "%u/%u",
				StarShipPtr->ShipInfo.crew_level,
				TemplatePtr->RaceDescPtr->ship_info.crew_level);
	UnlockStarShip (
			&GLOBAL (avail_race_q), hTemplate
			);

	r = *pRect;
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + r.extent.height - 1;
	t.align = ALIGN_CENTER;
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	if (r.corner.y)
	{
		r.corner.y = t.baseline.y - 6;
		r.extent.width = SHIP_WIN_WIDTH;
		r.extent.height = 6;
		SetContextForeGroundColor (BLACK_COLOR);
		DrawFilledRectangle (&r);
	}
	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x00), 0x02));
	font_DrawText (&t);
}

static void
ShowCombatShip (COUNT which_window, SHIP_FRAGMENTPTR YankedStarShipPtr)
{
	COUNT i, num_ships;
	HSTARSHIP hStarShip, hNextShip;
	SHIP_FRAGMENTPTR StarShipPtr;
	struct
	{
		SHIP_FRAGMENTPTR StarShipPtr;
		POINT finished_s;
		STAMP ship_s, lfdoor_s, rtdoor_s;
	} ship_win_info[MAX_COMBAT_SHIPS], *pship_win_info;

	num_ships = 1;
	pship_win_info = &ship_win_info[0];
	if (YankedStarShipPtr)
	{
		pship_win_info->StarShipPtr = YankedStarShipPtr;

		pship_win_info->lfdoor_s.origin.x = -(SHIP_WIN_WIDTH >> 1);
		pship_win_info->rtdoor_s.origin.x = (SHIP_WIN_WIDTH >> 1);
		pship_win_info->lfdoor_s.origin.y =
				pship_win_info->rtdoor_s.origin.y = 0;
		pship_win_info->lfdoor_s.frame =
				IncFrameIndex (pMenuState->ModuleFrame);
		pship_win_info->rtdoor_s.frame =
				IncFrameIndex (pship_win_info->lfdoor_s.frame);

		pship_win_info->ship_s.origin.x = (SHIP_WIN_WIDTH >> 1) + 1;
		pship_win_info->ship_s.origin.y = (SHIP_WIN_WIDTH >> 1);
		pship_win_info->ship_s.frame =
				YankedStarShipPtr->ShipInfo.melee_icon;

		pship_win_info->finished_s.x = hangar_x_coords[
				which_window % HANGAR_SHIPS_ROW];
		pship_win_info->finished_s.y = HANGAR_Y + (HANGAR_DY *
				(which_window / HANGAR_SHIPS_ROW));
	}
	else
	{
		if (which_window == (COUNT)~0)
		{
			hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
			num_ships = CountLinks (&GLOBAL (built_ship_q));
		}
		else
		{
			HSTARSHIP hTailShip;

			hTailShip = GetTailLink (&GLOBAL (built_ship_q));
			RemoveQueue (&GLOBAL (built_ship_q), hTailShip);

			hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
			while (hStarShip)
			{
				StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
						&GLOBAL (built_ship_q),
						hStarShip
						);
				if (GET_GROUP_LOC (StarShipPtr) > which_window)
				{
					UnlockStarShip (
							&GLOBAL (built_ship_q),
							hStarShip
							);
					break;
				}
				hNextShip = _GetSuccLink (StarShipPtr);
				UnlockStarShip (
						&GLOBAL (built_ship_q),
						hStarShip
						);

				hStarShip = hNextShip;
			}
			InsertQueue (&GLOBAL (built_ship_q),
					hTailShip, hStarShip);

			hStarShip = hTailShip;
			StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
					&GLOBAL (built_ship_q),
					hStarShip
					);
			SET_GROUP_LOC (StarShipPtr, which_window);
			UnlockStarShip (
					&GLOBAL (built_ship_q),
					hStarShip
					);
		}

		for (i = 0; i < num_ships; ++i)
		{
			StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
					&GLOBAL (built_ship_q),
					hStarShip
					);
			hNextShip = _GetSuccLink (StarShipPtr);

			pship_win_info->StarShipPtr = StarShipPtr;

			pship_win_info->lfdoor_s.origin.x = -1;
			pship_win_info->rtdoor_s.origin.x = 1;
			pship_win_info->lfdoor_s.origin.y =
					pship_win_info->rtdoor_s.origin.y = 0;
			pship_win_info->lfdoor_s.frame =
					IncFrameIndex (pMenuState->ModuleFrame);
			pship_win_info->rtdoor_s.frame =
					IncFrameIndex (pship_win_info->lfdoor_s.frame);

			pship_win_info->ship_s.origin.x = (SHIP_WIN_WIDTH >> 1) + 1;
			pship_win_info->ship_s.origin.y = (SHIP_WIN_WIDTH >> 1);
			pship_win_info->ship_s.frame =
					StarShipPtr->ShipInfo.melee_icon;

			which_window = GET_GROUP_LOC (StarShipPtr);
			pship_win_info->finished_s.x = hangar_x_coords[
					which_window % HANGAR_SHIPS_ROW];
			pship_win_info->finished_s.y = HANGAR_Y + (HANGAR_DY *
					(which_window / HANGAR_SHIPS_ROW));
			++pship_win_info;

			UnlockStarShip (
					&GLOBAL (built_ship_q),
					hStarShip
					);
			hStarShip = hNextShip;
		}
	}

	if (num_ships)
	{
		BYTE ButtonState;
		BOOLEAN AllDoorsFinished;
		DWORD TimeIn;
		RECT r;
		CONTEXT OldContext;
		int j;


		AllDoorsFinished = FALSE;
		r.corner.x = r.corner.y = 0;
		r.extent.width = SHIP_WIN_WIDTH;
		r.extent.height = SHIP_WIN_HEIGHT;
		ButtonState = AnyButtonPress (FALSE) ? 1 : 0;
		if (ButtonState)
		{
			while (ButtonState)
				if (!AnyButtonPress (FALSE))
					ButtonState = 0;
				else
					TaskSwitch();
		}
		TimeIn = GetTimeCounter ();

		for (j = 0; (j < SHIP_WIN_FRAMES) && !AllDoorsFinished; j++)
		{
			SleepThreadUntil (TimeIn + ONE_SECOND / 24);
			TimeIn = GetTimeCounter ();
			if (AnyButtonPress (FALSE))
			{
				if (YankedStarShipPtr != 0)
				{
					ship_win_info[0].lfdoor_s.origin.x = 0;
					ship_win_info[0].rtdoor_s.origin.x = 0;
				}
				AllDoorsFinished = TRUE;
				while (AnyButtonPress (FALSE))
					TaskSwitch();
			}
			SetSemaphore (GraphicsSem);
			OldContext = SetContext (OffScreenContext);
			SetContextFGFrame (Screen);
			SetContextBackGroundColor (BLACK_COLOR);

			BatchGraphics ();
			pship_win_info = &ship_win_info[0];
			for (i = 0; i < num_ships; ++i)
			{

				{
					RECT ClipRect;

					ClipRect.corner.x = SIS_ORG_X + pship_win_info->finished_s.x;
					ClipRect.corner.y = SIS_ORG_Y + pship_win_info->finished_s.y;
					ClipRect.extent.width = SHIP_WIN_WIDTH;
					ClipRect.extent.height = SHIP_WIN_HEIGHT;
					SetContextClipRect (&ClipRect);
					
					ClearDrawable ();
					DrawStamp (&pship_win_info->ship_s);
					ShowShipCrew (pship_win_info->StarShipPtr, &r);
					if (!AllDoorsFinished || YankedStarShipPtr)
					{
						DrawStamp (&pship_win_info->lfdoor_s);
						DrawStamp (&pship_win_info->rtdoor_s);
						if (YankedStarShipPtr)
						{
							++pship_win_info->lfdoor_s.origin.x;
							--pship_win_info->rtdoor_s.origin.x;
						}
						else
						{
							--pship_win_info->lfdoor_s.origin.x;
							++pship_win_info->rtdoor_s.origin.x;
						}
					}
				}
				++pship_win_info;
			}

			UnbatchGraphics ();
			SetContextClipRect (NULL_PTR);
			SetContext (OldContext);
			ClearSemaphore (GraphicsSem);
		}
	}
}

static void
CrewTransaction (SIZE crew_delta)
{
	if (crew_delta)
	{
		SIZE crew_bought;

		crew_bought = (SIZE)MAKE_WORD (
				GET_GAME_STATE (CREW_PURCHASED0),
				GET_GAME_STATE (CREW_PURCHASED1)
				) + crew_delta;
		if (crew_bought < 0)
		{
			if (crew_delta < 0)
				crew_bought = 0;
			else
				crew_bought = 0x7FFF;
		}
		else if (crew_delta > 0)
		{
			if (crew_bought >= CREW_EXPENSE_THRESHOLD
					&& crew_bought - crew_delta < CREW_EXPENSE_THRESHOLD)
			{
				GLOBAL (CrewCost) += 2;

				ClearSemaphore (GraphicsSem);
				DrawMenuStateStrings (PM_CREW, SHIPYARD_CREW);
				SetSemaphore (GraphicsSem);
			}
		}
		else
		{
			if (crew_bought < CREW_EXPENSE_THRESHOLD
					&& crew_bought - crew_delta >= CREW_EXPENSE_THRESHOLD)
			{
				GLOBAL (CrewCost) -= 2;

				ClearSemaphore (GraphicsSem);
				DrawMenuStateStrings (PM_CREW, SHIPYARD_CREW);
				SetSemaphore (GraphicsSem);
			}
		}
		if (!(ActivateStarShip (
				SHOFIXTI_SHIP, CHECK_ALLIANCE
				) & GOOD_GUY))
		{
			SET_GAME_STATE (CREW_PURCHASED0, LOBYTE (crew_bought));
			SET_GAME_STATE (CREW_PURCHASED1, HIBYTE (crew_bought));
		}
	}
}

/* in this routine, the least significant byte of pMS->CurState is used
 * to store the current selected ship index
 * a special case for the row is hi-nibble == -1 (0xf), which specifies
 * SIS as the selected ship
 * some bitwise math is still done to scroll through ships, for it to work
 * ships per row number must divide 0xf0 without remainder
 */
static BOOLEAN
DoModifyShips (PMENU_STATE pMS)
{
#define MODIFY_CREW_FLAG (1 << 8)
	RECT r;
	HSTARSHIP hStarShip, hNextShip;
	SHIP_FRAGMENTPTR StarShipPtr;
	BOOLEAN select, cancel;
#ifdef WANT_SHIP_SPINS
	BOOLEAN special;

	special = CurrentMenuState.special;
#endif /* WANT_SHIP_SPINS */
	select = CurrentMenuState.select;
	cancel = CurrentMenuState.cancel;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		pMS->InputFunc = DoShipyard;
		return (TRUE);
	}

	if (!pMS->Initialized)
	{
		pMS->InputFunc = DoModifyShips;
		pMS->Initialized = TRUE;
		pMS->CurState = MAKE_BYTE (0, 0xF);
		pMS->delta_item = 0;

		SetSemaphore (GraphicsSem);
		SetContext (SpaceContext);
		goto ChangeFlashRect;
	}
	else
	{
		SBYTE dx = 0, dy = 0;
		BYTE NewState;

		if (CurrentMenuState.right) dx = 1;
		if (CurrentMenuState.left) dx = -1;
		if (CurrentMenuState.up) dy = -1;
		if (CurrentMenuState.down) dy = 1;
		NewState = pMS->CurState;
		if (pMS->delta_item & MODIFY_CREW_FLAG)
		{
		}
		else if (dy)
		{
			if (HINIBBLE (NewState))
				NewState = pMS->CurState % HANGAR_SHIPS_ROW;
			else
				NewState = (unsigned char)(pMS->CurState + HANGAR_SHIPS_ROW);

			NewState += dy * HANGAR_SHIPS_ROW;
			if (NewState / HANGAR_SHIPS_ROW > 0
					&& NewState / HANGAR_SHIPS_ROW <= HANGAR_ROWS)
				NewState -= HANGAR_SHIPS_ROW;
			else if (NewState / HANGAR_SHIPS_ROW > HANGAR_ROWS + 1)
				/* negative number - select last row */
				NewState = pMS->CurState % HANGAR_SHIPS_ROW
						+ HANGAR_SHIPS_ROW * (HANGAR_ROWS - 1);
			else
				// select SIS
				NewState = MAKE_BYTE (pMS->CurState, 0xF);
		}
		else if (dx && !HINIBBLE (NewState))
		{
			NewState = NewState % HANGAR_SHIPS_ROW;
			if ((dx += NewState) < 0)
				NewState = (BYTE)(pMS->CurState + (HANGAR_SHIPS_ROW - 1));
			else if (dx > HANGAR_SHIPS_ROW - 1)
				NewState = (BYTE)(pMS->CurState - (HANGAR_SHIPS_ROW - 1));
			else
				NewState = (BYTE)(pMS->CurState - NewState + dx);
		}

#ifdef WANT_SHIP_SPINS
		if (select || cancel || special
#else
		if (select || cancel
#endif
				|| NewState != pMS->CurState
				|| ((pMS->delta_item & MODIFY_CREW_FLAG) && (dx || dy)))
		{
			for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
					hStarShip; hStarShip = hNextShip)
			{
				StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
						&GLOBAL (built_ship_q), hStarShip
						);

				if (GET_GROUP_LOC (StarShipPtr) == pMS->CurState)
				{
					UnlockStarShip (
							&GLOBAL (built_ship_q), hStarShip
							);
					break;
				}

				hNextShip = _GetSuccLink (StarShipPtr);
				UnlockStarShip (
						&GLOBAL (built_ship_q), hStarShip
						);
			}

			SetSemaphore (GraphicsSem);

#ifdef WANT_SHIP_SPINS
			if (special)
			{
				HSTARSHIP hSpinShip;
				
				if ((hSpinShip = hStarShip)
						|| (HINIBBLE (pMS->CurState) == 0
						&& (hSpinShip = GetAvailableRaceFromIndex (
								LOBYTE (pMS->delta_item)
								))))
				{
					SetFlashRect (NULL_PTR, (FRAME)0);
					SpinStarShip (hSpinShip);
					if (hStarShip)
						goto ChangeFlashRect;
					SetFlashRect ((PRECT)~0L, (FRAME)0);
				}
			}
			else
#endif
			if (select || ((pMS->delta_item & MODIFY_CREW_FLAG)
					&& (dx || dy || cancel)))
			{
				COUNT ShipCost[] =
				{
					RACE_SHIP_COST
				};

				if (hStarShip == 0 && HINIBBLE (pMS->CurState) == 0)
				{
					COUNT Index;

// SetFlashRect (NULL_PTR, (FRAME)0);
					ClearSemaphore (GraphicsSem);
					if (!(pMS->delta_item & MODIFY_CREW_FLAG))
					{
						pMS->delta_item = MODIFY_CREW_FLAG;
						DrawRaceStrings (0);
						return (TRUE);
					}
					else if (cancel)
					{
						pMS->delta_item ^= MODIFY_CREW_FLAG;
						SetFlashRect ((PRECT)~0L, (FRAME)0);
						DrawMenuStateStrings (PM_CREW, SHIPYARD_CREW);
					}
					else if (select)
					{
						Index = GetIndexFromStarShip (
								&GLOBAL (avail_race_q),
								GetAvailableRaceFromIndex (LOBYTE (pMS->delta_item))
								);

						if (GLOBAL_SIS (ResUnits) >= (DWORD)ShipCost[Index]
								&& CloneShipFragment (Index,
								&GLOBAL (built_ship_q), 1))
						{
							ShowCombatShip ((COUNT)pMS->CurState, (SHIP_FRAGMENTPTR)0);
							//Reset flash rectangle
							SetSemaphore (GraphicsSem);
							SetFlashRect ((PRECT)~0L, (FRAME)0);
							ClearSemaphore (GraphicsSem);
							DrawMenuStateStrings (PM_CREW, SHIPYARD_CREW);

							SetSemaphore (GraphicsSem);
							DeltaSISGauges (
									UNDEFINED_DELTA, UNDEFINED_DELTA, -((int)ShipCost[Index])
									);
							r.corner.x = pMS->flash_rect0.corner.x;
							r.corner.y = pMS->flash_rect0.corner.y
									+ pMS->flash_rect0.extent.height - 6;
							r.extent.width = SHIP_WIN_WIDTH;
							r.extent.height = 5;
							SetContext (SpaceContext);
							SetFlashRect (&r, (FRAME)0);
							ClearSemaphore (GraphicsSem);
						}
							
						return (TRUE);
					}
					else
					{
						Index = GetAvailableRaceCount ();
						NewState = LOBYTE (pMS->delta_item);
						if (dx < 0 || dy < 0)
						{
							if (NewState-- == 0)
								NewState = Index - 1;
						}
						else if (dx > 0 || dy > 0)
						{
							if (++NewState == Index)
								NewState = 0;
						}
						
						if (NewState != LOBYTE (pMS->delta_item))
						{
							DrawRaceStrings (NewState);
							pMS->delta_item = NewState | MODIFY_CREW_FLAG;
						}
						
						return (TRUE);
					}
					SetSemaphore (GraphicsSem);
					goto ChangeFlashRect;
				}
				else if (select || cancel)
				{					
					if (!(pMS->delta_item ^= MODIFY_CREW_FLAG))
						goto ChangeFlashRect;
					else if (hStarShip == 0)
					{
						SetContext (StatusContext);
						GetGaugeRect (&r, TRUE);
						SetFlashRect (&r, (FRAME)0);
						SetContext (SpaceContext);
					}
					else
					{
						r.corner.x = pMS->flash_rect0.corner.x;
						r.corner.y = pMS->flash_rect0.corner.y
								+ pMS->flash_rect0.extent.height - 6;
						r.extent.width = SHIP_WIN_WIDTH;
						r.extent.height = 5;
						SetContext (SpaceContext);
						SetFlashRect (&r, (FRAME)0);
					}
				}
				else if (pMS->delta_item & MODIFY_CREW_FLAG)
				{
					SIZE crew_delta, crew_bought;

					if (hStarShip)
						StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
								&GLOBAL (built_ship_q), hStarShip
								);

					crew_delta = 0;
					if (dx < 0 || dy < 0)
					{
						if (hStarShip == 0)
						{
							if (GetCPodCapacity (&r.corner) > GetCrewCount ()
									&& GLOBAL_SIS (ResUnits) >=
									(DWORD)GLOBAL (CrewCost))
							{
								DrawPoint (&r.corner);
								DeltaSISGauges (1, 0, -GLOBAL (CrewCost));
								crew_delta = 1;

								SetContext (StatusContext);
								GetGaugeRect (&r, TRUE);
								SetFlashRect (&r, (FRAME)0);
								SetContext (SpaceContext);
							}
						}
						else
						{
							HSTARSHIP hTemplate;
							SHIP_FRAGMENTPTR TemplatePtr;

							hTemplate = GetStarShipFromIndex (
									&GLOBAL (avail_race_q),
									GET_RACE_ID (StarShipPtr)
									);
							TemplatePtr = (SHIP_FRAGMENTPTR)LockStarShip (
									&GLOBAL (avail_race_q), hTemplate
									);
							if (GLOBAL_SIS (ResUnits) >=
									(DWORD)GLOBAL (CrewCost)
									&& StarShipPtr->ShipInfo.crew_level <
									StarShipPtr->ShipInfo.max_crew
									&& StarShipPtr->ShipInfo.crew_level <
									TemplatePtr->RaceDescPtr->ship_info.crew_level)
							{
								DeltaSISGauges (0, 0, -GLOBAL (CrewCost));
								++StarShipPtr->ShipInfo.crew_level;
								crew_delta = 1;

								ShowShipCrew (StarShipPtr, &pMS->flash_rect0);
								r.corner.x = pMS->flash_rect0.corner.x;
								r.corner.y = pMS->flash_rect0.corner.y
										+ pMS->flash_rect0.extent.height - 6;
								r.extent.width = SHIP_WIN_WIDTH;
								r.extent.height = 5;
								SetContext (SpaceContext);
								SetFlashRect (&r, (FRAME)0);
							}
							UnlockStarShip (
									&GLOBAL (avail_race_q), hTemplate
									);
						}
					}
					else if (dx > 0 || dy > 0)
					{
						crew_bought = (SIZE)MAKE_WORD (
								GET_GAME_STATE (CREW_PURCHASED0),
								GET_GAME_STATE (CREW_PURCHASED1)
								);
						if (hStarShip == 0)
						{
							if (GetCrewCount ())
							{
								DeltaSISGauges (-1, 0, GLOBAL (CrewCost)
										- (crew_bought == CREW_EXPENSE_THRESHOLD ? 2 : 0));
								crew_delta = -1;

								GetCPodCapacity (&r.corner);
								SetContextForeGroundColor (BLACK_COLOR);
								DrawPoint (&r.corner);

								SetContext (StatusContext);
								GetGaugeRect (&r, TRUE);
								SetFlashRect (&r, (FRAME)0);
								SetContext (SpaceContext);
							}
						}
						else if (StarShipPtr->ShipInfo.crew_level > 1)
						{
							DeltaSISGauges (0, 0, GLOBAL (CrewCost)
									- (crew_bought == CREW_EXPENSE_THRESHOLD ? 2 : 0));
							crew_delta = -1;
							--StarShipPtr->ShipInfo.crew_level;

							ShowShipCrew (StarShipPtr, &pMS->flash_rect0);
							r.corner.x = pMS->flash_rect0.corner.x;
							r.corner.y = pMS->flash_rect0.corner.y
									+ pMS->flash_rect0.extent.height - 6;
							r.extent.width = SHIP_WIN_WIDTH;
							r.extent.height = 5;
							SetContext (SpaceContext);
							SetFlashRect (&r, (FRAME)0);
						}
						else
						{
							COUNT ResUnits;

							ResUnits = (COUNT)ShipCost[
									GET_RACE_ID (StarShipPtr)
									];
							crew_delta = -(StarShipPtr->ShipInfo.crew_level - 1);
							crew_bought = (SIZE)MAKE_WORD (
									GET_GAME_STATE (CREW_PURCHASED0),
									GET_GAME_STATE (CREW_PURCHASED1)
									) - CREW_EXPENSE_THRESHOLD;
							if (crew_bought <= 0 || (crew_bought += crew_delta) >= 0)
								crew_bought = -crew_delta;
							else
							{
								ResUnits += -crew_bought
										* (GLOBAL (CrewCost) - 2);
								crew_bought -= crew_delta;
							}
							ResUnits += crew_bought * GLOBAL (CrewCost);
							
							SetFlashRect (NULL_PTR, (FRAME)0);
							ClearSemaphore (GraphicsSem);							
							ShowCombatShip ((COUNT)pMS->CurState, StarShipPtr);							
							SetSemaphore (GraphicsSem);
							pMS->flash_rect0.extent.width = SHIP_WIN_WIDTH;
							SetFlashRect (&pMS->flash_rect0, (FRAME)0);

							UnlockStarShip (
									&GLOBAL (built_ship_q), hStarShip
									);
		
							RemoveQueue (
									&GLOBAL (built_ship_q), hStarShip
									);
							FreeStarShip (
									&GLOBAL (built_ship_q), hStarShip
									);

							DeltaSISGauges (
									UNDEFINED_DELTA, UNDEFINED_DELTA, ResUnits
									);
							hStarShip = 0;

							pMS->delta_item ^= MODIFY_CREW_FLAG;
						}
					}

					if (hStarShip)
					{
						UnlockStarShip (
								&GLOBAL (built_ship_q), hStarShip
								);
					}
						
					CrewTransaction (crew_delta);
				}
			}
			else if (cancel)
			{
				ClearSemaphore (GraphicsSem);

				pMS->InputFunc = DoShipyard;
				DrawMenuStateStrings (PM_CREW, pMS->CurState = SHIPYARD_CREW);
				SetSemaphore (GraphicsSem);
				SetFlashRect ((PRECT)~0L, (FRAME)0);
				ClearSemaphore (GraphicsSem);

				return (TRUE);
			}
			else
			{
				pMS->CurState = NewState;

ChangeFlashRect:
				if (HINIBBLE (pMS->CurState))
				{
					pMS->flash_rect0.corner.x =
							pMS->flash_rect0.corner.y = 0;
					pMS->flash_rect0.extent.width = SIS_SCREEN_WIDTH;
					pMS->flash_rect0.extent.height = 61;
				}
				else
				{
					pMS->flash_rect0.corner.x = hangar_x_coords[
							pMS->CurState % HANGAR_SHIPS_ROW];
					pMS->flash_rect0.corner.y = HANGAR_Y + (HANGAR_DY *
							(pMS->CurState / HANGAR_SHIPS_ROW));
					pMS->flash_rect0.extent.width = SHIP_WIN_WIDTH;
					pMS->flash_rect0.extent.height = SHIP_WIN_HEIGHT;
				}
				SetFlashRect (&pMS->flash_rect0, (FRAME)0);
			}
			ClearSemaphore (GraphicsSem);
		}
	}

	return (TRUE);
}

static void
DrawBluePrint (PMENU_STATE pMS)
{
	COUNT num_frames;
	STAMP s;

	SetSemaphore (GraphicsSem);
	SetContext (SpaceContext);

	pMS->ModuleFrame = CaptureDrawable (
			LoadGraphic (SISBLU_MASK_ANIM)
			);

	BatchGraphics ();

	s.origin.x = s.origin.y = 0;
	s.frame = DecFrameIndex (pMS->ModuleFrame);
	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x16), 0x01));
	DrawFilledStamp (&s);

	for (num_frames = 0; num_frames < NUM_DRIVE_SLOTS; ++num_frames)
	{
		DrawShipPiece (pMS, GLOBAL_SIS (DriveSlots[num_frames]),
				num_frames, TRUE);
	}
	for (num_frames = 0; num_frames < NUM_JET_SLOTS; ++num_frames)
	{
		DrawShipPiece (pMS, GLOBAL_SIS (JetSlots[num_frames]),
				num_frames, TRUE);
	}
	for (num_frames = 0; num_frames < NUM_MODULE_SLOTS; ++num_frames)
	{
		BYTE which_piece;

		which_piece = GLOBAL_SIS (ModuleSlots[num_frames]);

		if (!(pMS->CurState == SHIPYARD && which_piece == CREW_POD))
			DrawShipPiece (pMS, which_piece, num_frames, TRUE);
	}

	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0xA, 0xA, 0x1F), 0x09));
	for (num_frames = 0; num_frames < NUM_MODULE_SLOTS; ++num_frames)
	{
		BYTE which_piece;

		which_piece = GLOBAL_SIS (ModuleSlots[num_frames]);
		if (pMS->CurState == SHIPYARD && which_piece == CREW_POD)
			DrawShipPiece (pMS, which_piece, num_frames, TRUE);
	}

	{
		num_frames = GLOBAL_SIS (CrewEnlisted);
		GLOBAL_SIS (CrewEnlisted) = 0;

		while (num_frames--)
		{
			POINT pt;

			GetCPodCapacity (&pt);
			DrawPoint (&pt);

			++GLOBAL_SIS (CrewEnlisted);
		}
	}
	{
		RECT r;

		num_frames = GLOBAL_SIS (TotalElementMass);
		GLOBAL_SIS (TotalElementMass) = 0;

		r.extent.width = 9;
		r.extent.height = 1;
		while (num_frames)
		{
			COUNT m;

			m = num_frames < SBAY_MASS_PER_ROW ?
					num_frames : SBAY_MASS_PER_ROW;
			GLOBAL_SIS (TotalElementMass) += m;
			GetSBayCapacity (&r.corner);
			DrawFilledRectangle (&r);
			num_frames -= m;
		}
	}
	if (GLOBAL_SIS (FuelOnBoard) > FUEL_RESERVE)
	{
		DWORD FuelVolume;
		RECT r;

		FuelVolume = GLOBAL_SIS (FuelOnBoard) - FUEL_RESERVE;
		GLOBAL_SIS (FuelOnBoard) = FUEL_RESERVE;

		r.extent.width = 3;
		r.extent.height = 1;
		while (FuelVolume)
		{
			COUNT m;

			GetFTankCapacity (&r.corner);
			DrawPoint (&r.corner);
			r.corner.x += r.extent.width + 1;
			DrawPoint (&r.corner);
			r.corner.x -= r.extent.width;
			SetContextForeGroundColor (SetContextBackGroundColor (BLACK_COLOR));
			DrawFilledRectangle (&r);
			m = FuelVolume < FUEL_VOLUME_PER_ROW ?
					(COUNT)FuelVolume : FUEL_VOLUME_PER_ROW;
			GLOBAL_SIS (FuelOnBoard) += m;
			FuelVolume -= m;
		}
	}

	UnbatchGraphics ();

	DestroyDrawable (ReleaseDrawable (pMS->ModuleFrame));
	pMS->ModuleFrame = 0;

	ClearSemaphore (GraphicsSem);
}

void
BeginHangarAnim (PMENU_STATE pMS)
{
#ifdef WANT_HANGAR_ANIMATION
	CONTEXT OldContext;
	RECT ClipRect;

	OldContext = SetContext (SpaceContext);
	GetContextClipRect (&ClipRect);

	// start hangar power-lines animation
	GetContextClipRect (&ClipRect);
	pMS->flash_rect1 = ClipRect;
	pMS->CurFrame = pMS->ModuleFrame;
	pMS->flash_task = AssignTask (hangar_anim_func, 4096,
			"hangar pal-rot animation");

	SetContext (OldContext);
#endif
}

void
EndHangarAnim (PMENU_STATE pMS)
{
	if (pMS->flash_task)
	{
		ConcludeTask (pMS->flash_task);
		pMS->flash_task = 0;
	}
}

BOOLEAN
DoShipyard (PMENU_STATE pMS)
{
	BOOLEAN select, cancel;
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		goto ExitShipyard;

	select = CurrentMenuState.select;
	cancel = CurrentMenuState.cancel;

	if (!pMS->Initialized)
	{
		pMS->InputFunc = DoShipyard;

		{
			STAMP s;
			RECT r, old_r;

			s.frame = CaptureDrawable (
					LoadGraphic (SHIPYARD_PMAP_ANIM)
					);

			pMS->CurString = CaptureColorMap (LoadColorMapFile (
					"lbm/dockpani.ct"));

			pMS->hMusic = LoadMusicInstance (SHIPYARD_MUSIC);

			SetTransitionSource (NULL);
			BatchGraphics ();

			DrawSISFrame ();
			SetSemaphore(GraphicsSem);
			DrawSISMessage (GAME_STRING (STARBASE_STRING_BASE + 3));
			DrawSISTitle (GAME_STRING (STARBASE_STRING_BASE));
			ClearSemaphore(GraphicsSem);
			DrawBluePrint (pMS);
			pMS->ModuleFrame = s.frame;

			DrawMenuStateStrings (PM_CREW, pMS->CurState = SHIPYARD_CREW);

			SetSemaphore (GraphicsSem);
			SetContext (SpaceContext);
			s.origin.x = s.origin.y = 0;
#ifdef USE_3DO_HANGAR
			DrawStamp (&s);
#else // PC hangar
			// the PC ship dock needs to overwrite the border
			// expand the clipping rect by 1 pixel
			GetContextClipRect (&old_r);
			r = old_r;
			r.corner.x--;
			r.extent.width += 2;
			r.extent.height += 1;
			SetContextClipRect (&r);
			DrawStamp (&s);
			SetContextClipRect (&old_r);
#endif // USE_3DO_HANGAR
			
			SetContextFont (TinyFont);

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
			BeginHangarAnim (pMS);
			ClearSemaphore (GraphicsSem);

			ShowCombatShip ((COUNT)~0, (SHIP_FRAGMENTPTR)0);
			SetSemaphore (GraphicsSem);
			SetFlashRect ((PRECT)~0L, (FRAME)0);
			ClearSemaphore (GraphicsSem);
		}

		pMS->Initialized = TRUE;
	}
	else if (cancel || (select && pMS->CurState == SHIPYARD_EXIT))
	{
ExitShipyard:
		SetSemaphore (GraphicsSem);
		EndHangarAnim (pMS);
		DestroyDrawable (ReleaseDrawable (pMS->ModuleFrame));
		pMS->ModuleFrame = 0;
		pMS->CurFrame = 0;
		DestroyColorMap (ReleaseColorMap (pMS->CurString));
		pMS->CurString = 0;
		ClearSemaphore (GraphicsSem);

		return (FALSE);
	}
	else if (select)
	{
		if (pMS->CurState != SHIPYARD_SAVELOAD)
		{
			pMS->Initialized = FALSE;
			DoModifyShips (pMS);
		}
		else
		{
			EndHangarAnim (pMS);
			if (GameOptions () == 0)
				goto ExitShipyard;
			DrawMenuStateStrings (PM_CREW, pMS->CurState);
			SetSemaphore (GraphicsSem);
			SetFlashRect ((PRECT)~0L, (FRAME)0);
			BeginHangarAnim (pMS);
			ClearSemaphore (GraphicsSem);
		}
	}
	else
		DoMenuChooser (pMS, PM_CREW);

	return (TRUE);
}
