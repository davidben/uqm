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

#include "libs/graphics/gfx_common.h"
//#include "starcon.h"
#include "controls.h"

QUEUE disp_q;
SIZE battle_counter;

static void
ProcessInput (void)
{
	BOOLEAN CanRunAway;

	CanRunAway = (BOOLEAN)(
			(LOBYTE (GLOBAL (CurrentActivity)) == IN_ENCOUNTER
			|| LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
			&& GET_GAME_STATE (STARBASE_AVAILABLE)
			&& !GET_GAME_STATE (BOMB_CARRIER)
			);
	for (cur_player = NUM_SIDES - 1; cur_player >= 0; --cur_player)
	{
		HSTARSHIP hBattleShip, hNextShip;

		for (hBattleShip = GetHeadLink (&race_q[cur_player]);
				hBattleShip != 0; hBattleShip = hNextShip)
		{
			BATTLE_INPUT_STATE InputState;
			STARSHIPPTR StarShipPtr;

			StarShipPtr = LockStarShip (&race_q[cur_player], hBattleShip);
			hNextShip = _GetSuccLink (StarShipPtr);

			if (StarShipPtr->hShip)
			{
				CyborgDescPtr = StarShipPtr;

				InputState = (*(PlayerInput[cur_player]))();
#if CREATE_JOURNAL
				JournalInput (InputState);
#endif /* CREATE_JOURNAL */

#ifdef TESTING
if ((InputState & DEVICE_BUTTON3)
		&& LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE)
{
	COUNT i;
	
	for (i = ARILOU_SHIP; i <= BLACK_URQUAN_SHIP; ++i)
		ActivateStarShip (i, SPHERE_TRACKING);
	while (GLOBAL (GameClock.tick_count) > 1)
		ClockTick ();
}
#endif /* TESTING */

				CyborgDescPtr->ship_input_state = 0;
				if (CyborgDescPtr->RaceDescPtr->ship_info.crew_level)
				{
					
					if (InputState & BATTLE_LEFT)
						CyborgDescPtr->ship_input_state |= LEFT;
					else if (InputState & BATTLE_RIGHT)
						CyborgDescPtr->ship_input_state |= RIGHT;
					if (InputState & BATTLE_THRUST)
						CyborgDescPtr->ship_input_state |= THRUST;
					if (InputState & BATTLE_WEAPON)
						CyborgDescPtr->ship_input_state |= WEAPON;
					if (InputState & BATTLE_SPECIAL)
						CyborgDescPtr->ship_input_state |= SPECIAL;

					if (CanRunAway && cur_player == 0 && (InputState & BATTLE_ESCAPE))
					{
						ELEMENTPTR ElementPtr;

						LockElement (StarShipPtr->hShip, &ElementPtr);
						if (GetPrimType (&DisplayArray[ElementPtr->PrimIndex]) == STAMP_PRIM
								&& ElementPtr->life_span == NORMAL_LIFE
								&& !(ElementPtr->state_flags & FINITE_LIFE)
								&& ElementPtr->mass_points != MAX_SHIP_MASS * 10)
						{
							extern void flee_preprocess (PELEMENT);

							battle_counter -= MAKE_WORD (1, 0);

							ElementPtr->turn_wait = 3;
							ElementPtr->thrust_wait = MAKE_BYTE (4, 0);
							ElementPtr->preprocess_func = flee_preprocess;
							ElementPtr->mass_points = MAX_SHIP_MASS * 10;
							ZeroVelocityComponents (&ElementPtr->velocity);
							StarShipPtr->cur_status_flags &=
									~(SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);

							SetPrimColor (&DisplayArray[ElementPtr->PrimIndex],
									BUILD_COLOR (MAKE_RGB15 (0xB, 0x00, 0x00), 0x2E));
							SetPrimType (&DisplayArray[ElementPtr->PrimIndex], STAMPFILL_PRIM);
						
							CyborgDescPtr->ship_input_state = 0;
						}
						UnlockElement (StarShipPtr->hShip);
					}
				}
			}

			UnlockStarShip (&race_q[cur_player], hBattleShip);
		}
	}

	if (GLOBAL (CurrentActivity) & (CHECK_LOAD | CHECK_ABORT))
		GLOBAL (CurrentActivity) &= ~IN_BATTLE;
}

#if DEMO_MODE || CREATE_JOURNAL
DWORD BattleSeed;
#endif /* DEMO_MODE */

static MUSIC_REF BattleRef;

void
BattleSong (BOOLEAN DoPlay)
{
	if (BattleRef == 0)
	{
		if (LOBYTE (GLOBAL (CurrentActivity)) != IN_HYPERSPACE)
			BattleRef = LoadMusicInstance (BATTLE_MUSIC);
		else if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
			BattleRef = LoadMusicInstance (HYPERSPACE_MUSIC);
		else
			BattleRef = LoadMusicInstance (QUASISPACE_MUSIC);
	}

	if (DoPlay)
		PlayMusic (BattleRef, TRUE, 1);
}

void
FreeBattleSong (void)
{
	DestroyMusic (BattleRef);
	BattleRef = 0;
}

typedef struct battlestate_struct {
	BOOLEAN (*InputFunc) (struct battlestate_struct *pInputState);
	COUNT MenuRepeatDelay;
	BOOLEAN first_time;
	DWORD NextTime;
} BATTLE_STATE;

static BOOLEAN
DoBattle (BATTLE_STATE *bs)
{
	extern UWORD nth_frame;
	RECT r;

	bs->MenuRepeatDelay = 0;
	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);

	ProcessInput ();
	LockMutex (GraphicsLock);
	if (bs->first_time)
	{
		r.corner.x = SIS_ORG_X;
		r.corner.y = SIS_ORG_Y;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = SIS_SCREEN_HEIGHT;
		SetTransitionSource (&r);
	}
	BatchGraphics ();
	if ((LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE) &&
			!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
		SeedUniverse ();
	RedrawQueue (TRUE);

	if (bs->first_time)
	{
		bs->first_time = FALSE;
		ScreenTransition (3, &r);
	}
	UnbatchGraphics ();
	UnlockMutex (GraphicsLock);
	if ((!(GLOBAL (CurrentActivity) & IN_BATTLE)) ||
			(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
	{
		return FALSE;
	}
	if (nth_frame)
		TaskSwitch ();
	else
	{
		SleepThreadUntil (bs->NextTime + BATTLE_FRAME_RATE);
		bs->NextTime = GetTimeCounter ();
	}
	return GLOBAL (CurrentActivity) & IN_BATTLE;
}

BOOLEAN
Battle (void)
{
	SIZE num_ships;
	extern SIZE InitShips (void);
	void UninitShips (void);

	LockMutex (GraphicsLock);

	SetResourceIndex (hResIndex);

#if !(DEMO_MODE || CREATE_JOURNAL)
	TFB_SeedRandom (GetTimeCounter ());
#else /* DEMO_MODE */
	if (BattleSeed == 0)
		BattleSeed = TFB_Random ();
	TFB_SeedRandom (BattleSeed);
	BattleSeed = TFB_Random (); /* get next battle seed */
#endif /* DEMO_MODE */

	BattleSong (FALSE);
	
	num_ships = InitShips ();
	if (num_ships)
	{
		BATTLE_STATE bs;

		GLOBAL (CurrentActivity) |= IN_BATTLE;
		battle_counter = MAKE_WORD (
				CountLinks (&race_q[0]),
				CountLinks (&race_q[1])
				);

		while (num_ships--)
		{
			if (!GetNextStarShip (NULL_PTR, num_ships))
				goto AbortBattle;
		}

		BattleSong (TRUE);
		bs.NextTime = 0;
		bs.InputFunc = &DoBattle;
		bs.first_time = (BOOLEAN)(LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE);
		UnlockMutex (GraphicsLock);
		DoInput ((PVOID)&bs, FALSE);
		LockMutex (GraphicsLock);

AbortBattle:
		StopMusic ();
		StopSound ();
	}

	UninitShips ();
	FreeBattleSong ();

	UnlockMutex (GraphicsLock);
	
	return (num_ships < 0);
}

