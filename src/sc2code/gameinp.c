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
	DWORD key [NUM_KEYS];
} MENU_ANNOTATIONS;


CONTROLLER_INPUT_STATE CurrentInputState, PulsedInputState;
static CONTROLLER_INPUT_STATE CachedInputState, OldInputState;
static MENU_ANNOTATIONS RepeatDelays, Times;
static DWORD GestaltRepeatDelay, GestaltTime;
static BOOLEAN OldGestalt, CachedGestalt;
static DWORD _max_accel, _min_accel, _step_accel;
static BOOLEAN _gestalt_keys;
int ExitState;

static MENU_SOUND_FLAGS sound_0, sound_1;

volatile CONTROLLER_INPUT_STATE ImmediateInputState;

static void
_clear_menu_state (void)
{
	PulsedInputState.key[KEY_MENU_UP]        = 0;
	PulsedInputState.key[KEY_MENU_DOWN]      = 0;
	PulsedInputState.key[KEY_MENU_LEFT]      = 0;
	PulsedInputState.key[KEY_MENU_RIGHT]     = 0;
	PulsedInputState.key[KEY_MENU_SELECT]    = 0;
	PulsedInputState.key[KEY_MENU_CANCEL]    = 0;
	PulsedInputState.key[KEY_MENU_SPECIAL]   = 0;
	PulsedInputState.key[KEY_MENU_PAGE_UP]   = 0;
	PulsedInputState.key[KEY_MENU_PAGE_DOWN] = 0;
	PulsedInputState.key[KEY_MENU_ZOOM_IN]   = 0;
	PulsedInputState.key[KEY_MENU_ZOOM_OUT]  = 0;
	PulsedInputState.key[KEY_MENU_DELETE]       = 0;
	CachedInputState.key[KEY_MENU_UP]        = 0;
	CachedInputState.key[KEY_MENU_DOWN]      = 0;
	CachedInputState.key[KEY_MENU_LEFT]      = 0;
	CachedInputState.key[KEY_MENU_RIGHT]     = 0;
	CachedInputState.key[KEY_MENU_SELECT]    = 0;
	CachedInputState.key[KEY_MENU_CANCEL]    = 0;
	CachedInputState.key[KEY_MENU_SPECIAL]   = 0;
	CachedInputState.key[KEY_MENU_PAGE_UP]   = 0;
	CachedInputState.key[KEY_MENU_PAGE_DOWN] = 0;
	CachedInputState.key[KEY_MENU_ZOOM_IN]   = 0;
	CachedInputState.key[KEY_MENU_ZOOM_OUT]  = 0;
	CachedInputState.key[KEY_MENU_DELETE]       = 0;
	CachedGestalt = FALSE;
}

void
ResetKeyRepeat (void)
{
	DWORD initTime = GetTimeCounter ();
	RepeatDelays.key[KEY_MENU_UP]        = _max_accel;
	RepeatDelays.key[KEY_MENU_DOWN]      = _max_accel;
	RepeatDelays.key[KEY_MENU_LEFT]      = _max_accel;
	RepeatDelays.key[KEY_MENU_RIGHT]     = _max_accel;
	RepeatDelays.key[KEY_MENU_SELECT]    = _max_accel;
	RepeatDelays.key[KEY_MENU_CANCEL]    = _max_accel;
	RepeatDelays.key[KEY_MENU_SPECIAL]   = _max_accel;
	RepeatDelays.key[KEY_MENU_PAGE_UP]   = _max_accel;
	RepeatDelays.key[KEY_MENU_PAGE_DOWN] = _max_accel;
	RepeatDelays.key[KEY_MENU_ZOOM_IN]   = _max_accel;
	RepeatDelays.key[KEY_MENU_ZOOM_OUT]  = _max_accel;
	RepeatDelays.key[KEY_MENU_DELETE]       = _max_accel;
	GestaltRepeatDelay     = _max_accel;
	Times.key[KEY_MENU_UP]        = initTime;
	Times.key[KEY_MENU_DOWN]      = initTime;
	Times.key[KEY_MENU_LEFT]      = initTime;
	Times.key[KEY_MENU_RIGHT]     = initTime;
	Times.key[KEY_MENU_SELECT]    = initTime;
	Times.key[KEY_MENU_CANCEL]    = initTime;
	Times.key[KEY_MENU_SPECIAL]   = initTime;
	Times.key[KEY_MENU_PAGE_UP]   = initTime;
	Times.key[KEY_MENU_PAGE_DOWN] = initTime;
	Times.key[KEY_MENU_ZOOM_IN]   = initTime;
	Times.key[KEY_MENU_ZOOM_OUT]  = initTime;
	Times.key[KEY_MENU_DELETE]       = initTime;
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
	CachedGestalt = ImmediateInputState.key[KEY_MENU_UP] || ImmediateInputState.key[KEY_MENU_DOWN] || ImmediateInputState.key[KEY_MENU_LEFT] || ImmediateInputState.key[KEY_MENU_RIGHT];
	CurrentGestalt = PulsedInputState.key[KEY_MENU_UP] || PulsedInputState.key[KEY_MENU_DOWN] || PulsedInputState.key[KEY_MENU_LEFT] || PulsedInputState.key[KEY_MENU_RIGHT];
	if (OldGestalt && CachedGestalt)
	{
		if (NewTime - GestaltTime < GestaltRepeatDelay)
		{
			PulsedInputState.key[KEY_MENU_UP] = 0;
			PulsedInputState.key[KEY_MENU_DOWN] = 0;
			PulsedInputState.key[KEY_MENU_LEFT] = 0;
			PulsedInputState.key[KEY_MENU_RIGHT] = 0;
		}
		else
		{
			PulsedInputState.key[KEY_MENU_UP] = CachedInputState.key[KEY_MENU_UP];
			PulsedInputState.key[KEY_MENU_DOWN] = CachedInputState.key[KEY_MENU_DOWN];
			PulsedInputState.key[KEY_MENU_LEFT] = CachedInputState.key[KEY_MENU_LEFT];
			PulsedInputState.key[KEY_MENU_RIGHT] = CachedInputState.key[KEY_MENU_RIGHT];
			if (GestaltRepeatDelay > _min_accel)
				GestaltRepeatDelay -= _step_accel;
			if (GestaltRepeatDelay < _min_accel)
				GestaltRepeatDelay = _min_accel;
			GestaltTime = NewTime;
		}
	}
	else
	{
		PulsedInputState.key[KEY_MENU_UP] = CachedInputState.key[KEY_MENU_UP];
		PulsedInputState.key[KEY_MENU_DOWN] = CachedInputState.key[KEY_MENU_DOWN];
		PulsedInputState.key[KEY_MENU_LEFT] = CachedInputState.key[KEY_MENU_LEFT];
		PulsedInputState.key[KEY_MENU_RIGHT] = CachedInputState.key[KEY_MENU_RIGHT];
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
	OldInputState = CachedInputState;
	CachedInputState = ImmediateInputState;
	NewTime = GetTimeCounter ();
	if (_gestalt_keys)
	{
		_check_gestalt (NewTime);
	}
	else
	{
		_check_for_pulse(&PulsedInputState.key[KEY_MENU_UP], &CachedInputState.key[KEY_MENU_UP], &OldInputState.key[KEY_MENU_UP], &RepeatDelays.key[KEY_MENU_UP], &NewTime, &Times.key[KEY_MENU_UP]);
		_check_for_pulse(&PulsedInputState.key[KEY_MENU_DOWN], &CachedInputState.key[KEY_MENU_DOWN], &OldInputState.key[KEY_MENU_DOWN], &RepeatDelays.key[KEY_MENU_DOWN], &NewTime, &Times.key[KEY_MENU_DOWN]);
		_check_for_pulse(&PulsedInputState.key[KEY_MENU_LEFT], &CachedInputState.key[KEY_MENU_LEFT], &OldInputState.key[KEY_MENU_LEFT], &RepeatDelays.key[KEY_MENU_LEFT], &NewTime, &Times.key[KEY_MENU_LEFT]);
		_check_for_pulse(&PulsedInputState.key[KEY_MENU_RIGHT], &CachedInputState.key[KEY_MENU_RIGHT], &OldInputState.key[KEY_MENU_RIGHT], &RepeatDelays.key[KEY_MENU_RIGHT], &NewTime, &Times.key[KEY_MENU_RIGHT]);
	}
	_check_for_pulse(&PulsedInputState.key[KEY_MENU_SELECT], &CachedInputState.key[KEY_MENU_SELECT], &OldInputState.key[KEY_MENU_SELECT], &RepeatDelays.key[KEY_MENU_SELECT], &NewTime, &Times.key[KEY_MENU_SELECT]);
	_check_for_pulse(&PulsedInputState.key[KEY_MENU_CANCEL], &CachedInputState.key[KEY_MENU_CANCEL], &OldInputState.key[KEY_MENU_CANCEL], &RepeatDelays.key[KEY_MENU_CANCEL], &NewTime, &Times.key[KEY_MENU_CANCEL]);
	_check_for_pulse(&PulsedInputState.key[KEY_MENU_SPECIAL], &CachedInputState.key[KEY_MENU_SPECIAL], &OldInputState.key[KEY_MENU_SPECIAL], &RepeatDelays.key[KEY_MENU_SPECIAL], &NewTime, &Times.key[KEY_MENU_SPECIAL]);
	_check_for_pulse(&PulsedInputState.key[KEY_MENU_PAGE_UP], &CachedInputState.key[KEY_MENU_PAGE_UP], &OldInputState.key[KEY_MENU_PAGE_UP], &RepeatDelays.key[KEY_MENU_PAGE_UP], &NewTime, &Times.key[KEY_MENU_PAGE_UP]);
	_check_for_pulse(&PulsedInputState.key[KEY_MENU_PAGE_DOWN], &CachedInputState.key[KEY_MENU_PAGE_DOWN], &OldInputState.key[KEY_MENU_PAGE_DOWN], &RepeatDelays.key[KEY_MENU_PAGE_DOWN], &NewTime, &Times.key[KEY_MENU_PAGE_DOWN]);
	_check_for_pulse(&PulsedInputState.key[KEY_MENU_ZOOM_IN], &CachedInputState.key[KEY_MENU_ZOOM_IN], &OldInputState.key[KEY_MENU_ZOOM_IN], &RepeatDelays.key[KEY_MENU_ZOOM_IN], &NewTime, &Times.key[KEY_MENU_ZOOM_IN]);
	_check_for_pulse(&PulsedInputState.key[KEY_MENU_ZOOM_OUT], &CachedInputState.key[KEY_MENU_ZOOM_OUT], &OldInputState.key[KEY_MENU_ZOOM_OUT], &RepeatDelays.key[KEY_MENU_ZOOM_OUT], &NewTime, &Times.key[KEY_MENU_ZOOM_OUT]);
	_check_for_pulse(&PulsedInputState.key[KEY_MENU_DELETE], &CachedInputState.key[KEY_MENU_DELETE], &OldInputState.key[KEY_MENU_DELETE], &RepeatDelays.key[KEY_MENU_DELETE], &NewTime, &Times.key[KEY_MENU_DELETE]);

	if (CurrentInputState.key[KEY_PAUSE])
		GamePaused = TRUE;

	if (CurrentInputState.key[KEY_EXIT])
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
FlushInputState (void)
{
	_clear_menu_state ();
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
		MENU_SOUND_FLAGS input;
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

		if (CurrentInputState.key[KEY_EXIT])
			ExitState = ConfirmExit ();

		input = MENU_SOUND_NONE;
		if (PulsedInputState.key[KEY_MENU_UP]) input |= MENU_SOUND_UP;
		if (PulsedInputState.key[KEY_MENU_DOWN]) input |= MENU_SOUND_DOWN;
		if (PulsedInputState.key[KEY_MENU_LEFT]) input |= MENU_SOUND_LEFT;
		if (PulsedInputState.key[KEY_MENU_RIGHT]) input |= MENU_SOUND_RIGHT;
		if (PulsedInputState.key[KEY_MENU_SELECT]) input |= MENU_SOUND_SELECT;
		if (PulsedInputState.key[KEY_MENU_CANCEL]) input |= MENU_SOUND_CANCEL;
		if (PulsedInputState.key[KEY_MENU_SPECIAL]) input |= MENU_SOUND_SPECIAL;
		if (PulsedInputState.key[KEY_MENU_PAGE_UP]) input |= MENU_SOUND_PAGEUP;
		if (PulsedInputState.key[KEY_MENU_PAGE_DOWN]) input |= MENU_SOUND_PAGEDOWN;
		if (PulsedInputState.key[KEY_MENU_DELETE]) input |= MENU_SOUND_DELETE;
			
		if (MenuSounds
				&& (pSolarSysState == 0
						/* see if in menu */
				|| pSolarSysState->MenuState.CurState
				|| pSolarSysState->MenuState.Initialized > 2)
		                && (input & (sound_0 | sound_1))
#ifdef NEVER
				&& !PLRPlaying ((MUSIC_REF)~0)
#endif /* NEVER */
				)
		{
			SOUND S;

			S = MenuSounds;
			if (input & sound_1)
				S = SetAbsSoundIndex (S, 1);

			PlaySoundEffect (S, 0, NotPositional (), NULL, 0);
		}
	} while ((*((PINPUT_STATE_DESC)pInputState)->InputFunc)
			(pInputState));
	if (resetInput)
	{
		TFB_ResetControls ();
	}

}

void
SetMenuSounds (MENU_SOUND_FLAGS s0, MENU_SOUND_FLAGS s1)
{
	sound_0 = s0;
	sound_1 = s1;
}

void
GetMenuSounds (MENU_SOUND_FLAGS *s0, MENU_SOUND_FLAGS *s1)
{
	*s0 = sound_0;
	*s1 = sound_1;
}


BATTLE_INPUT_STATE
p1_combat_summary (void)
{
	BATTLE_INPUT_STATE InputState = 0;
	if (CurrentInputState.key[KEY_P1_THRUST])
		InputState |= BATTLE_THRUST;
	if (CurrentInputState.key[KEY_P1_LEFT])
		InputState |= BATTLE_LEFT;
	if (CurrentInputState.key[KEY_P1_RIGHT])
		InputState |= BATTLE_RIGHT;
	if (CurrentInputState.key[KEY_P1_WEAPON])
		InputState |= BATTLE_WEAPON;
	if (CurrentInputState.key[KEY_P1_SPECIAL])
		InputState |= BATTLE_SPECIAL;
	if (CurrentInputState.key[KEY_P1_ESCAPE])
		InputState |= BATTLE_ESCAPE;
	if (CurrentInputState.key[KEY_P1_DOWN])
		InputState |= BATTLE_DOWN;
	return InputState;
}

BATTLE_INPUT_STATE
p2_combat_summary (void)
{
	BATTLE_INPUT_STATE InputState = 0;
	if (CurrentInputState.key[KEY_P2_THRUST])
		InputState |= BATTLE_THRUST;
	if (CurrentInputState.key[KEY_P2_LEFT])
		InputState |= BATTLE_LEFT;
	if (CurrentInputState.key[KEY_P2_RIGHT])
		InputState |= BATTLE_RIGHT;
	if (CurrentInputState.key[KEY_P2_WEAPON])
		InputState |= BATTLE_WEAPON;
	if (CurrentInputState.key[KEY_P2_SPECIAL])
		InputState |= BATTLE_SPECIAL;
	if (CurrentInputState.key[KEY_P2_DOWN])
		InputState |= BATTLE_DOWN;
	return InputState;
}

BOOLEAN
AnyButtonPress (BOOLEAN CheckSpecial)
{
	(void) CheckSpecial;   // Ignored
	UpdateInputState ();
	return CurrentInputState.key[KEY_P1_THRUST]
	     ||CurrentInputState.key[KEY_P1_LEFT]  
	     ||CurrentInputState.key[KEY_P1_RIGHT] 
	     ||CurrentInputState.key[KEY_P1_DOWN]
	     ||CurrentInputState.key[KEY_P1_WEAPON]
	     ||CurrentInputState.key[KEY_P1_SPECIAL]
	     ||CurrentInputState.key[KEY_P1_ESCAPE]
	     ||CurrentInputState.key[KEY_P2_THRUST]
	     ||CurrentInputState.key[KEY_P2_LEFT]  
	     ||CurrentInputState.key[KEY_P2_RIGHT] 
	     ||CurrentInputState.key[KEY_P2_DOWN]
	     ||CurrentInputState.key[KEY_P2_WEAPON]
	     ||CurrentInputState.key[KEY_P2_SPECIAL]
	     ||CurrentInputState.key[KEY_LANDER_THRUST]
	     ||CurrentInputState.key[KEY_LANDER_LEFT]  
	     ||CurrentInputState.key[KEY_LANDER_RIGHT] 
	     ||CurrentInputState.key[KEY_LANDER_WEAPON]
	     ||CurrentInputState.key[KEY_LANDER_ESCAPE]
	     ||PulsedInputState.key[KEY_MENU_UP]
	     ||PulsedInputState.key[KEY_MENU_DOWN]
	     ||PulsedInputState.key[KEY_MENU_LEFT]
	     ||PulsedInputState.key[KEY_MENU_RIGHT]
	     ||PulsedInputState.key[KEY_MENU_PAGE_UP]
	     ||PulsedInputState.key[KEY_MENU_PAGE_DOWN]
	     ||PulsedInputState.key[KEY_MENU_ZOOM_IN]
	     ||PulsedInputState.key[KEY_MENU_ZOOM_OUT]
	     ||PulsedInputState.key[KEY_MENU_SELECT]
	     ||PulsedInputState.key[KEY_MENU_CANCEL]
	     ||PulsedInputState.key[KEY_MENU_SPECIAL];
}

BOOLEAN
ConfirmExit (void)
{
	DWORD old_max_accel, old_min_accel, old_step_accel;
	BOOLEAN old_gestalt_keys, result;

	old_max_accel = _max_accel;
	old_min_accel = _min_accel;
	old_step_accel = _step_accel;
	old_gestalt_keys = _gestalt_keys;
		
	SetDefaultMenuRepeatDelay ();
		
	result = DoConfirmExit ();
	
	SetMenuRepeatDelay (old_min_accel, old_max_accel, old_step_accel, old_gestalt_keys);
	return result;
}
