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
#include "libs/graphics/gfx_common.h"

//Added by Chris

void DoShipSpin (COUNT index, MUSIC_REF hMusic);

void GetGaugeRect (PRECT pRect, BOOLEAN IsCrewRect);

//End added by Chris

enum
{
	SHIPYARD_CREW,
	SHIPYARD_SAVELOAD,
	SHIPYARD_EXIT
};

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
		DrawText (&t);
	}
	UnbatchGraphics ();

	SetFlashRect ((PRECT)~0L, (FRAME)0);
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
	DrawText (&t);
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
	COORD x_coords[] =
	{
		19, 60, 116, 157,
	};

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

		pship_win_info->finished_s.x = x_coords[which_window & 3];
		pship_win_info->finished_s.y = 64 + (44 * (which_window >> 2));
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
			pship_win_info->finished_s.x = x_coords[which_window & 3];
			pship_win_info->finished_s.y = 64 + (44 * (which_window >> 2));
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

		SetSemaphore (GraphicsSem);
		OldContext = SetContext (OffScreenContext);
		SetContextFGFrame (Screen);
		SetContextBackGroundColor (BLACK_COLOR);

		AllDoorsFinished = FALSE;
		r.corner.x = r.corner.y = 0;
		r.extent.width = SHIP_WIN_WIDTH;
		r.extent.height = SHIP_WIN_HEIGHT;
		ButtonState = AnyButtonPress (FALSE) ? 1 : 0;
		TimeIn = GetTimeCounter ();

		for (j = 0; (j < SHIP_WIN_FRAMES) && !AllDoorsFinished; j++)
		{
			ClearSemaphore (GraphicsSem);
			SleepThreadUntil (TimeIn + 5);
			TimeIn = GetTimeCounter ();
			SetSemaphore (GraphicsSem);
			BatchGraphics ();
StartFinalPass:
			pship_win_info = &ship_win_info[0];
			for (i = 0; i < num_ships; ++i)
			{
				if (ButtonState)
				{
					if (!AnyButtonPress (FALSE))
						ButtonState = 0;
				}
				else if (AnyButtonPress (FALSE))
				{
					if (YankedStarShipPtr == 0)
						AllDoorsFinished = TRUE;
					else
					{
						pship_win_info->lfdoor_s.origin.x =
								pship_win_info->rtdoor_s.origin.x = 0;
						AllDoorsFinished = TRUE;
					}
					goto StartFinalPass;
				}

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
					if (!AllDoorsFinished)
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
		}

		SetContextClipRect (NULL_PTR);
		SetContext (OldContext);
		ClearSemaphore (GraphicsSem);
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

static BOOLEAN
DoModifyShips (INPUT_STATE InputState, PMENU_STATE pMS)
{
#define MODIFY_CREW_FLAG (1 << 8)
	RECT r;
	HSTARSHIP hStarShip, hNextShip;
	SHIP_FRAGMENTPTR StarShipPtr;

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
		SBYTE dx, dy;
		BYTE NewState;

		dx = GetInputXComponent (InputState);
		dy = GetInputYComponent (InputState);
		NewState = pMS->CurState;
		if (pMS->delta_item & MODIFY_CREW_FLAG)
		{
		}
		else if (dy)
		{
			if (HINIBBLE (NewState))
				NewState = pMS->CurState & 3;
			else
				NewState = (unsigned char)(pMS->CurState + (1 << 2));

			NewState = (NewState + (dy << 2)) & 0xF;
			if (NewState >> 2)
				NewState -= (1 << 2);
			else
				NewState = MAKE_BYTE (pMS->CurState, 0xF);
		}
		else if (dx && !HINIBBLE (NewState))
		{
			NewState = NewState & 3;
			if ((dx += NewState) < 0)
				NewState = (BYTE)(pMS->CurState + 3);
			else if (dx > 3)
				NewState = (BYTE)(pMS->CurState - 3);
			else
				NewState = (BYTE)(pMS->CurState - NewState + dx);
		}

#ifdef WANT_SHIP_SPINS
		if ((InputState & (DEVICE_BUTTON1 | DEVICE_BUTTON2 | DEVICE_BUTTON3))
#else
		if ((InputState & (DEVICE_BUTTON1 | DEVICE_BUTTON2))
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
			if (InputState & DEVICE_BUTTON3)
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
			if ((InputState & DEVICE_BUTTON1)
					|| ((pMS->delta_item & MODIFY_CREW_FLAG)
					&& (dx || dy || (InputState & DEVICE_BUTTON2))))
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
					else if (InputState & DEVICE_BUTTON2)
					{
						pMS->delta_item ^= MODIFY_CREW_FLAG;
						DrawMenuStateStrings (PM_CREW, SHIPYARD_CREW);
					}
					else if (InputState & DEVICE_BUTTON1)
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
				else if (InputState & (DEVICE_BUTTON1 | DEVICE_BUTTON2))
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
						UnlockStarShip (
								&GLOBAL (built_ship_q), hStarShip
								);
						
					CrewTransaction (crew_delta);
				}
			}
			else if (InputState & DEVICE_BUTTON2)
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
					COORD x_coords[] =
					{
						19, 60, 116, 157,
					};

					pMS->flash_rect0.corner.x = x_coords[pMS->CurState & 3];
					pMS->flash_rect0.corner.y = 64 + (44 * (pMS->CurState >> 2));
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

BOOLEAN
DoShipyard (INPUT_STATE InputState, PMENU_STATE pMS)
{
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		goto ExitShipyard;

	if (!pMS->Initialized)
	{
		pMS->InputFunc = DoShipyard;

		{
			STAMP s;

			s.frame = CaptureDrawable (
					LoadGraphic (SHIPYARD_PMAP_ANIM)
					);

			pMS->hMusic = LoadMusicInstance (SHIPYARD_MUSIC);

			BatchGraphics ();

			DrawSISFrame ();
			DrawSISMessage (GAME_STRING (STARBASE_STRING_BASE + 3));
			DrawSISTitle (GAME_STRING (STARBASE_STRING_BASE));

			DrawBluePrint (pMS);
			pMS->ModuleFrame = s.frame;

			DrawMenuStateStrings (PM_CREW, pMS->CurState = SHIPYARD_CREW);

			SetSemaphore (GraphicsSem);
			SetContext (SpaceContext);
			SetContextDrawState (DEST_PIXMAP | DRAW_REPLACE);
			s.origin.x = s.origin.y = 0;
			DrawStamp (&s);

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

			ClearSemaphore (GraphicsSem);

			ShowCombatShip ((COUNT)~0, (SHIP_FRAGMENTPTR)0);
			SetSemaphore (GraphicsSem);
			SetFlashRect ((PRECT)~0L, (FRAME)0);
			ClearSemaphore (GraphicsSem);
		}

		pMS->Initialized = TRUE;
	}
	else if ((InputState & DEVICE_BUTTON2)
			|| ((InputState & DEVICE_BUTTON1)
			&& pMS->CurState == SHIPYARD_EXIT))
	{
ExitShipyard:
		SetSemaphore (GraphicsSem);
		DestroyDrawable (ReleaseDrawable (pMS->ModuleFrame));
		pMS->ModuleFrame = 0;
		ClearSemaphore (GraphicsSem);

		return (FALSE);
	}
	else if (InputState & DEVICE_BUTTON1)
	{
		if (pMS->CurState != SHIPYARD_SAVELOAD)
		{
			pMS->Initialized = FALSE;
			DoModifyShips (InputState, pMS);
		}
		else
		{
			if (GameOptions () == 0)
				goto ExitShipyard;
			DrawMenuStateStrings (PM_CREW, pMS->CurState);
			SetSemaphore (GraphicsSem);
			SetFlashRect ((PRECT)~0L, (FRAME)0);
			ClearSemaphore (GraphicsSem);
		}
	}
	else
	{
		BYTE NewState;

		NewState = pMS->CurState;
		if (GetInputXComponent (InputState) < 0
				|| GetInputYComponent (InputState) < 0)
		{
			if (NewState-- == SHIPYARD_CREW)
				NewState = SHIPYARD_EXIT;
		}
		else if (GetInputXComponent (InputState) > 0
				|| GetInputYComponent (InputState) > 0)
		{
			if (NewState++ == SHIPYARD_EXIT)
				NewState = SHIPYARD_CREW;
		}

		if (NewState != pMS->CurState)
		{
			DrawMenuStateStrings (PM_CREW, NewState);

			pMS->CurState = NewState;
		}
	}

	return (TRUE);
}

