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

typedef struct
{
	BOOLEAN (*InputFunc) (INPUT_STATE InputState, PVOID pInputState);
	COUNT MenuRepeatDelay;
} INPUT_STATE_DESC;
typedef INPUT_STATE_DESC *PINPUT_STATE_DESC;

INPUT_STATE OldInputState;

void
DoInput (PVOID pInputState)
{
	DWORD NewTime;
	INPUT_STATE InputState;

	NewTime = GetTimeCounter ();
	((PINPUT_STATE_DESC)pInputState)->MenuRepeatDelay = MENU_REPEAT_DELAY;
	do
	{
		NewTime = TaskSwitch ();

		InputState = GetInputState (NormalInput);

#if DEMO_MODE || CREATE_JOURNAL
		if (ArrowInput != DemoInput)
#endif
		{
			static DWORD OldTime;
				int test1,test2;

				test1=NewTime - OldTime;
				test2=(DWORD)((PINPUT_STATE_DESC)pInputState)->MenuRepeatDelay;

			if (InputState == OldInputState
					&& NewTime - OldTime <
					(DWORD)((PINPUT_STATE_DESC)pInputState)->MenuRepeatDelay)
				InputState = 0;
			else
			{
#define ACCELERATION_INCREMENT 10
				if (InputState != OldInputState)
					((PINPUT_STATE_DESC)pInputState)->MenuRepeatDelay =
							MENU_REPEAT_DELAY;
				else if (((PINPUT_STATE_DESC)pInputState)->MenuRepeatDelay
						<= ACCELERATION_INCREMENT)
					((PINPUT_STATE_DESC)pInputState)->MenuRepeatDelay =
							ACCELERATION_INCREMENT;
				else
					((PINPUT_STATE_DESC)pInputState)->MenuRepeatDelay -=
							ACCELERATION_INCREMENT;

				if (OldInputState != (INPUT_STATE)~0L)
					OldInputState = InputState;
				else
				{
					OldInputState = InputState;
					InputState = 0;
				}

				OldTime = NewTime;
			}

#if CREATE_JOURNAL
			JournalInput (InputState);
#endif /* CREATE_JOURNAL */
		}

		if (InputState & DEVICE_EXIT)
			InputState = ConfirmExit ();
			
		if (MenuSounds
				&& (pSolarSysState == 0
						/* see if in menu */
				|| pSolarSysState->MenuState.CurState
				|| pSolarSysState->MenuState.Initialized > 2)
				&& ((InputState & DEVICE_BUTTON1)
				|| GetInputXComponent (InputState)
				|| GetInputYComponent (InputState))
#ifdef NEVER
				&& !PLRPlaying ((MUSIC_REF)~0)
#endif /* NEVER */
				)
		{
			SOUND S;

			S = MenuSounds;
			if (InputState & DEVICE_BUTTON1)
				S = SetAbsSoundIndex (S, 1);

			PlaySoundEffect (S, 0, 0);
		}

	} while ((*((PINPUT_STATE_DESC)pInputState)->InputFunc)
			(InputState, pInputState));

	OldInputState = (INPUT_STATE)~0L;
}

INPUT_STATE
game_input (INPUT_REF InputRef, INPUT_STATE InputState)
{
	if (LastActivity & (CHECK_LOAD | CHECK_RESTART))
	{
		InputState = 0;
	}
	else if (InputState == 0
			&& (InputState = GetInputState (ArrowInput)) == 0
			&& (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE
					|| (InputState = GetInputState (CombinedInput[0])) == 0))
	{
		if ((InputState = GetInputState (JoystickInput[0])) == 0)
			InputState = GetInputState (JoystickInput[1]);
	}
	(void) InputRef;  /* Satisfying compiler (unused parameter) */
	return (InputState);
}

INPUT_STATE
combined_input0 (INPUT_REF InputRef, INPUT_STATE InputState)
{
	if (LastActivity & (CHECK_LOAD | CHECK_RESTART))
	{
		InputState = 0;
	}
	else if (InputState == 0
			&& (InputState = GetInputState (JoystickInput[0])) == 0)
	{
		InputState = GetInputState (KeyboardInput[0]);
	}

	(void) InputRef;  /* Satisfying compiler (unused parameter) */
	return (InputState);
}


INPUT_STATE
combined_input1 (INPUT_REF InputRef, INPUT_STATE InputState)
{
	if (LastActivity & (CHECK_LOAD | CHECK_RESTART))
	{
		InputState = 0;
	}
	else if (InputState == 0
			&& (InputState = GetInputState (JoystickInput[1])) == 0)
	{
		InputState = GetInputState (KeyboardInput[1]);
	}

	(void) InputRef;  /* Satisfying compiler (unused parameter) */
	return (InputState);
}
