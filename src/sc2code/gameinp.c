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

#include "controls.h"
#include "init.h"
#include "planets/planets.h"
#include "settings.h"
#include "sounds.h"
#include "libs/inplib.h"
#include "libs/timelib.h"
#include "libs/threadlib.h"


battle_summary_func ComputerInput, HumanInput[NUM_PLAYERS];
battle_summary_func PlayerInput[NUM_PLAYERS];

#define ACCELERATION_INCREMENT (ONE_SECOND / 12)
#define MENU_REPEAT_DELAY (ONE_SECOND >> 1)


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
	int i;
	for (i = 0; i < NUM_KEYS; i++)
	{
		PulsedInputState.key[i] = 0;
		CachedInputState.key[i] = 0;
	}
	CachedGestalt = FALSE;
}

void
ResetKeyRepeat (void)
{
	DWORD initTime = GetTimeCounter ();
	int i;
	for (i = 0; i < NUM_KEYS; i++)
	{
		RepeatDelays.key[i] = _max_accel;
		Times.key[i] = initTime;
	}
	GestaltRepeatDelay = _max_accel;
	GestaltTime = initTime;
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
	int i;
	OldGestalt = CachedGestalt;

	CachedGestalt = 0;
	CurrentGestalt = 0;
	for (i = 0; i < NUM_KEYS; i++)
	{
		CachedGestalt |= ImmediateInputState.key[i];
		CurrentGestalt |= PulsedInputState.key[i];
	}

	if (OldGestalt && CachedGestalt)
	{
		if (NewTime - GestaltTime < GestaltRepeatDelay)
		{
			for (i = 0; i < NUM_KEYS; i++)
			{
				PulsedInputState.key[i] = 0;
			}
		}
		else
		{
			for (i = 0; i < NUM_KEYS; i++)
			{
				PulsedInputState.key[i] = CachedInputState.key[i];
			}
			if (GestaltRepeatDelay > _min_accel)
				GestaltRepeatDelay -= _step_accel;
			if (GestaltRepeatDelay < _min_accel)
				GestaltRepeatDelay = _min_accel;
			GestaltTime = NewTime;
		}
	}
	else
	{
		for (i = 0; i < NUM_KEYS; i++)
		{
			PulsedInputState.key[i] = CachedInputState.key[i];
		}
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
		int i;
		for (i = 0; i < NUM_KEYS; i++)
		{
			_check_for_pulse(&PulsedInputState.key[i], &CachedInputState.key[i], &OldInputState.key[i], &RepeatDelays.key[i], &NewTime, &Times.key[i]);
		}
	}

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
	int i;
	(void) CheckSpecial;   // Ignored
	UpdateInputState ();
	for (i = 0; i < NUM_KEYS; i++)
	{
		if (CurrentInputState.key[i])
			return TRUE;
	}
	return FALSE;
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
