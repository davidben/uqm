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
#include "controls.h"

battle_summary_func ComputerInput, HumanInput[NUM_PLAYERS];
battle_summary_func PlayerInput[NUM_PLAYERS];

#define ACCELERATION_INCREMENT (ONE_SECOND / 12)

typedef struct
{
	BOOLEAN (*InputFunc) (PVOID pInputState);
	COUNT MenuRepeatDelay;
} INPUT_STATE_DESC;
typedef INPUT_STATE_DESC *PINPUT_STATE_DESC;

/* These static variables are the values that are set by the controllers. */

typedef struct 
{
	DWORD up, down, left, right, select, cancel, special;
	DWORD page_up, page_down, zoom_in, zoom_out;
} MENU_ANNOTATIONS;


CONTROLLER_INPUT_STATE CurrentInputState;
MENU_INPUT_STATE CurrentMenuState;
static MENU_INPUT_STATE CachedMenuState, OldMenuState;
static MENU_ANNOTATIONS RepeatDelays, Times;
static DWORD GestaltRepeatDelay, GestaltTime;
static BOOLEAN OldGestalt, CachedGestalt;
static DWORD _max_accel, _min_accel, _step_accel;
static BOOLEAN _gestalt_keys;
int ExitState;

volatile CONTROLLER_INPUT_STATE ImmediateInputState;
volatile MENU_INPUT_STATE       ImmediateMenuState;

static void
_clear_menu_state (void)
{
	CurrentMenuState.up        = 0;
	CurrentMenuState.down      = 0;
	CurrentMenuState.left      = 0;
	CurrentMenuState.right     = 0;
	CurrentMenuState.select    = 0;
	CurrentMenuState.cancel    = 0;
	CurrentMenuState.special   = 0;
	CurrentMenuState.page_up   = 0;
	CurrentMenuState.page_down = 0;
	CurrentMenuState.zoom_in   = 0;
	CurrentMenuState.zoom_out  = 0;
	CachedMenuState.up        = 0;
	CachedMenuState.down      = 0;
	CachedMenuState.left      = 0;
	CachedMenuState.right     = 0;
	CachedMenuState.select    = 0;
	CachedMenuState.cancel    = 0;
	CachedMenuState.special   = 0;
	CachedMenuState.page_up   = 0;
	CachedMenuState.page_down = 0;
	CachedMenuState.zoom_in   = 0;
	CachedMenuState.zoom_out  = 0;
}

void
ResetKeyRepeat (void)
{
	DWORD initTime = GetTimeCounter ();
	RepeatDelays.up        = _max_accel;
	RepeatDelays.down      = _max_accel;
	RepeatDelays.left      = _max_accel;
	RepeatDelays.right     = _max_accel;
	RepeatDelays.select    = _max_accel;
	RepeatDelays.cancel    = _max_accel;
	RepeatDelays.special   = _max_accel;
	RepeatDelays.page_up   = _max_accel;
	RepeatDelays.page_down = _max_accel;
	RepeatDelays.zoom_in   = _max_accel;
	RepeatDelays.zoom_out  = _max_accel;
	GestaltRepeatDelay     = _max_accel;
	Times.up        = initTime;
	Times.down      = initTime;
	Times.left      = initTime;
	Times.right     = initTime;
	Times.select    = initTime;
	Times.cancel    = initTime;
	Times.special   = initTime;
	Times.page_up   = initTime;
	Times.page_down = initTime;
	Times.zoom_in   = initTime;
	Times.zoom_out  = initTime;
	GestaltTime     = initTime;
}

static void
_check_for_pulse (int *current, int *cached, int *old, DWORD *accel, DWORD *newtime, DWORD *oldtime)
{
	if (*cached && *old)
	{
		if (*newtime - *oldtime < *accel)
		{
			*current = 0;
		} 
		else
		{
			*current = *cached;
			if (*accel > _min_accel)
				*accel -= _step_accel;
			if (*accel < _min_accel)
				*accel = _min_accel;
			*oldtime = *newtime;
		}
	}
	else
	{
		*current = *cached;
		*oldtime = *newtime;
		*accel = _max_accel;
	}
}

static void
_check_gestalt (DWORD NewTime)
{
	BOOLEAN CurrentGestalt;
	OldGestalt = CachedGestalt;
	CachedGestalt = ImmediateMenuState.up || ImmediateMenuState.down || ImmediateMenuState.left || ImmediateMenuState.right;
	CurrentGestalt = CurrentMenuState.up || CurrentMenuState.down || CurrentMenuState.left || CurrentMenuState.right;
	if (OldGestalt && CachedGestalt)
	{
		if (NewTime - GestaltTime < GestaltRepeatDelay)
		{
			CurrentMenuState.up = 0;
			CurrentMenuState.down = 0;
			CurrentMenuState.left = 0;
			CurrentMenuState.right = 0;
		}
		else
		{
			CurrentMenuState.up = CachedMenuState.up;
			CurrentMenuState.down = CachedMenuState.down;
			CurrentMenuState.left = CachedMenuState.left;
			CurrentMenuState.right = CachedMenuState.right;
			if (GestaltRepeatDelay > _min_accel)
				GestaltRepeatDelay -= _step_accel;
			if (GestaltRepeatDelay < _min_accel)
				GestaltRepeatDelay = _min_accel;
			GestaltTime = NewTime;
		}
	}
	else
	{
		CurrentMenuState.up = CachedMenuState.up;
		CurrentMenuState.down = CachedMenuState.down;
		CurrentMenuState.left = CachedMenuState.left;
		CurrentMenuState.right = CachedMenuState.right;
		GestaltTime = NewTime;
		GestaltRepeatDelay = _max_accel;
	}
}

void
UpdateInputState (void)
{
	DWORD NewTime;
	/* First, if the game is, in fact, paused, we stall until
	 * unpaused.  Every thread with control over game logic calls
	 * UpdateInputState routinely, so we handle pause and exit
	 * state updates here. */

	if (GamePaused)
		PauseGame ();

	if (ExitRequested)
	{
		ConfirmExit ();
	}

	CurrentInputState = ImmediateInputState;
	OldMenuState = CachedMenuState;
	CachedMenuState = ImmediateMenuState;
	NewTime = GetTimeCounter ();
	if (_gestalt_keys)
	{
		_check_gestalt (NewTime);
	}
	else
	{
		_check_for_pulse(&CurrentMenuState.up, &CachedMenuState.up, &OldMenuState.up, &RepeatDelays.up, &NewTime, &Times.up);
		_check_for_pulse(&CurrentMenuState.down, &CachedMenuState.down, &OldMenuState.down, &RepeatDelays.down, &NewTime, &Times.down);
		_check_for_pulse(&CurrentMenuState.left, &CachedMenuState.left, &OldMenuState.left, &RepeatDelays.left, &NewTime, &Times.left);
		_check_for_pulse(&CurrentMenuState.right, &CachedMenuState.right, &OldMenuState.right, &RepeatDelays.right, &NewTime, &Times.right);
	}
	_check_for_pulse(&CurrentMenuState.select, &CachedMenuState.select, &OldMenuState.select, &RepeatDelays.select, &NewTime, &Times.select);
	_check_for_pulse(&CurrentMenuState.cancel, &CachedMenuState.cancel, &OldMenuState.cancel, &RepeatDelays.cancel, &NewTime, &Times.cancel);
	_check_for_pulse(&CurrentMenuState.special, &CachedMenuState.special, &OldMenuState.special, &RepeatDelays.special, &NewTime, &Times.special);
	_check_for_pulse(&CurrentMenuState.page_up, &CachedMenuState.page_up, &OldMenuState.page_up, &RepeatDelays.page_up, &NewTime, &Times.page_up);
	_check_for_pulse(&CurrentMenuState.page_down, &CachedMenuState.page_down, &OldMenuState.page_down, &RepeatDelays.page_down, &NewTime, &Times.page_down);
	_check_for_pulse(&CurrentMenuState.zoom_in, &CachedMenuState.zoom_in, &OldMenuState.zoom_in, &RepeatDelays.zoom_in, &NewTime, &Times.zoom_in);
	_check_for_pulse(&CurrentMenuState.zoom_out, &CachedMenuState.zoom_out, &OldMenuState.zoom_out, &RepeatDelays.zoom_out, &NewTime, &Times.zoom_out);

	if (CurrentInputState.pause)
		GamePaused = TRUE;

	if (CurrentInputState.exit)
		ExitRequested = TRUE;
}


void
SetMenuRepeatDelay (DWORD min, DWORD max, DWORD step, BOOLEAN gestalt)
{
	_min_accel = min;
	_max_accel = max;
	_step_accel = step;
	_gestalt_keys = gestalt;
	_clear_menu_state ();
	ResetKeyRepeat ();
}

void
SetDefaultMenuRepeatDelay ()
{
	_min_accel = ACCELERATION_INCREMENT;
	_max_accel = MENU_REPEAT_DELAY;
	_step_accel = ACCELERATION_INCREMENT;
	_gestalt_keys = FALSE;
	_clear_menu_state ();
	ResetKeyRepeat ();
}

void
DoInput (PVOID pInputState, BOOLEAN resetInput)
{
	SetMenuRepeatDelay (ACCELERATION_INCREMENT, MENU_REPEAT_DELAY, ACCELERATION_INCREMENT, FALSE);
	if (resetInput)
	{
		TFB_ResetControls ();
	}
	do
	{
		TaskSwitch ();

		UpdateInputState ();

#if DEMO_MODE || CREATE_JOURNAL
		if (ArrowInput != DemoInput)
#endif
		{
#if CREATE_JOURNAL
			JournalInput (InputState);
#endif /* CREATE_JOURNAL */
		}

		if (CurrentInputState.exit)
			ExitState = ConfirmExit ();
			
		if (MenuSounds
				&& (pSolarSysState == 0
						/* see if in menu */
				|| pSolarSysState->MenuState.CurState
				|| pSolarSysState->MenuState.Initialized > 2)
				&& (CurrentMenuState.select
				||  CurrentMenuState.up
				||  CurrentMenuState.down
				||  CurrentMenuState.left
				||  CurrentMenuState.right)
#ifdef NEVER
				&& !PLRPlaying ((MUSIC_REF)~0)
#endif /* NEVER */
				)
		{
			SOUND S;

			S = MenuSounds;
			if (CurrentMenuState.select)
				S = SetAbsSoundIndex (S, 1);

			PlaySoundEffect (S, 0, 0);
		}
	} while ((*((PINPUT_STATE_DESC)pInputState)->InputFunc)
			(pInputState));
	if (resetInput)
	{
		TFB_ResetControls ();
	}

}

BATTLE_INPUT_STATE
p1_combat_summary (void)
{
	BATTLE_INPUT_STATE InputState = 0;
	if (CurrentInputState.p1_thrust)
		InputState |= BATTLE_THRUST;
	if (CurrentInputState.p1_left)
		InputState |= BATTLE_LEFT;
	if (CurrentInputState.p1_right)
		InputState |= BATTLE_RIGHT;
	if (CurrentInputState.p1_weapon)
		InputState |= BATTLE_WEAPON;
	if (CurrentInputState.p1_special)
		InputState |= BATTLE_SPECIAL;
	if (CurrentInputState.p1_escape)
		InputState |= BATTLE_ESCAPE;
	return InputState;
}

BATTLE_INPUT_STATE
p2_combat_summary (void)
{
	BATTLE_INPUT_STATE InputState = 0;
	if (CurrentInputState.p2_thrust)
		InputState |= BATTLE_THRUST;
	if (CurrentInputState.p2_left)
		InputState |= BATTLE_LEFT;
	if (CurrentInputState.p2_right)
		InputState |= BATTLE_RIGHT;
	if (CurrentInputState.p2_weapon)
		InputState |= BATTLE_WEAPON;
	if (CurrentInputState.p2_special)
		InputState |= BATTLE_SPECIAL;
	return InputState;
}

BOOLEAN
AnyButtonPress (BOOLEAN CheckSpecial)
{
	(void) CheckSpecial;   // Ignored
	UpdateInputState ();
	return CurrentInputState.p1_thrust
	     ||CurrentInputState.p1_left  
	     ||CurrentInputState.p1_right 
	     ||CurrentInputState.p1_weapon
	     ||CurrentInputState.p1_special
	     ||CurrentInputState.p1_escape
	     ||CurrentInputState.p2_thrust
	     ||CurrentInputState.p2_left  
	     ||CurrentInputState.p2_right 
	     ||CurrentInputState.p2_weapon
	     ||CurrentInputState.p2_special
	     ||CurrentInputState.lander_thrust
	     ||CurrentInputState.lander_left  
	     ||CurrentInputState.lander_right 
	     ||CurrentInputState.lander_weapon
	     ||CurrentInputState.lander_escape
	     ||CurrentMenuState.up
	     ||CurrentMenuState.down
	     ||CurrentMenuState.left
	     ||CurrentMenuState.right
	     ||CurrentMenuState.page_up
	     ||CurrentMenuState.page_down
	     ||CurrentMenuState.zoom_in
	     ||CurrentMenuState.zoom_out
	     ||CurrentMenuState.select
	     ||CurrentMenuState.cancel
	     ||CurrentMenuState.special;
}
