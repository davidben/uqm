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

#ifdef GFXMODULE_SDL

#include "libs/graphics/sdl/sdl_common.h"
#include "libs/input/input_common.h"

static Uint8 KeyboardDown[SDLK_LAST];
#define KBDBUFSIZE (1 << 8)
static int kbdhead=0, kbdtail=0;
static UNICODE kbdbuf[KBDBUFSIZE];

static int nJoysticks = 0;
static SDL_Joystick **Joysticks = 0;

static UBYTE **JoybuttonDown = 0;
static SWORD **JoyaxisValues = 0;

static BOOLEAN (* PauseFunc) (void) = 0;
static BOOLEAN InputInitialized = FALSE;
UWORD UNICODEToKBD (UNICODE which_key);

UNICODE PauseKey = 0;
UNICODE ExitKey = 0;
UNICODE AbortKey = 0;

int 
TFB_InitInput (int driver, int flags)
{
	int i;
	(void)driver;
	(void)flags;

	atexit (TFB_UninitInput);

	SDL_EnableUNICODE(1);
	
	if ((SDL_InitSubSystem(SDL_INIT_JOYSTICK)) == -1)
	{
		fprintf (stderr, "Couldn't initialize joystick subsystem: %s\n", SDL_GetError());
		exit(-1);
	}

	fprintf (stderr, "%i joysticks were found.\n", SDL_NumJoysticks ());
	
	nJoysticks = SDL_NumJoysticks ();
	if (nJoysticks > 0)
	{
		Joysticks = HMalloc(sizeof(SDL_Joystick*) * nJoysticks);
		JoybuttonDown = HMalloc(sizeof(UBYTE*) * nJoysticks);
		JoyaxisValues = HMalloc(sizeof(SWORD*) * nJoysticks);
		fprintf (stderr, "The names of the joysticks are:\n");
		for(i = 0; i < nJoysticks; i++)
		{
			fprintf (stderr, "    %s\n", SDL_JoystickName (i));
			Joysticks[i] = SDL_JoystickOpen (i);
			JoybuttonDown[i] = HMalloc(sizeof(UBYTE) * 
					SDL_JoystickNumButtons(Joysticks[i]));
			memset(JoybuttonDown[i], 0, sizeof(UBYTE) *
					SDL_JoystickNumButtons(Joysticks[i]));
			JoyaxisValues[i] = HMalloc(sizeof(SWORD) *
					SDL_JoystickNumAxes(Joysticks[i]));
			memset(JoyaxisValues[i], 0, sizeof(SWORD) *
					SDL_JoystickNumAxes(Joysticks[i]));
			fprintf (stderr, "    %i axes, %i buttons\n", 
					SDL_JoystickNumAxes(Joysticks[i]),
					SDL_JoystickNumButtons(Joysticks[i]));
		}
		SDL_JoystickEventState (SDL_ENABLE);
	}

	for (i = 0; i < SDLK_LAST; i++)
	{
		KeyboardDown[i] = FALSE;
	}

	return 0;
}

void
TFB_UninitInput (void)
{
	int j;
	for (j = 0; j < nJoysticks; ++j)
	{
		HFree(JoyaxisValues[j]);
		HFree(JoybuttonDown[j]);
		SDL_JoystickClose(Joysticks[j]);
		Joysticks[j] = 0;
	}
	HFree(Joysticks);
}

void
ProcessKeyboardEvent(const SDL_Event *Event)
{
	SDLKey k = Event->key.keysym.sym;
        UNICODE map_key;
	if (!InputInitialized)
		return;

	if (Event->type == SDL_KEYDOWN)
	{
		if (Event->key.keysym.unicode != 0)
			map_key = (UNICODE) Event->key.keysym.unicode;
		else
			map_key = KBDToUNICODE(k);
		kbdbuf[kbdtail] = map_key;
		kbdtail = (kbdtail + 1) & (KBDBUFSIZE - 1);
		if (kbdtail == kbdhead)
			kbdhead = (kbdhead + 1) & (KBDBUFSIZE - 1);
		if (map_key == AbortKey)
			exit(0);
		//Store the actual key (without nay shift/ctrl modifiers)
		//since the modifiers can't be retreived on a key-up
		KeyboardDown[k] |= 0x3;
	}
	else
	{
		KeyboardDown[k] &= (~0x1);
	}
}

void
ProcessJoystickEvent (const SDL_Event* Event)
{
	if (!InputInitialized)
		return;

	switch (Event->type)
	{
		case SDL_JOYAXISMOTION:
			JoyaxisValues[Event->jaxis.which][Event->jaxis.axis] = Event->jaxis.value;
			break;
		case SDL_JOYBUTTONDOWN:
			JoybuttonDown[Event->jbutton.which][Event->jbutton.button] |= 0x3;
			break;
		case SDL_JOYBUTTONUP:
			JoybuttonDown[Event->jbutton.which][Event->jbutton.button] &= (~0x1);
			break;
	}
}

extern BOOLEAN
InitInput (BOOLEAN (*PFunc) (void), UNICODE Pause, UNICODE Exit, UNICODE Abort)
{
	PauseFunc = PFunc;
	PauseKey = Pause;
	ExitKey = Exit;
	AbortKey = Abort;
	InputInitialized = TRUE;
	return (TRUE);
}

//Status: Ignored
BOOLEAN
UninitInput (void) // Looks like it'll be empty
{
	BOOLEAN ret;

	// fprintf (stderr, "Unimplemented function activated: UninitInput()\n");
	ret = TRUE;
	return (ret);
}

void
FlushInput (void)
{
	int i;
	kbdtail = kbdhead = 0;
	for (i = 0; i < SDLK_LAST; ++i)
	{
		if (KeyboardDown[i] == 0x2)
		    KeyboardDown[i] &= ~0x2;
	}
}

// Translates from SDLKeys to values defined in inplib.h
UNICODE
KBDToUNICODE (UWORD SK_in)
{
	switch (SK_in)
	{
		case SDLK_KP_ENTER:
		case SDLK_RETURN:
			return '\n';
		case SDLK_KP4:
		case SDLK_LEFT:
			return (SK_LF_ARROW);
		case SDLK_KP6:
		case SDLK_RIGHT:
			return (SK_RT_ARROW);
		case SDLK_KP8:
		case SDLK_UP:
			return (SK_UP_ARROW);
		case SDLK_KP2:
		case SDLK_DOWN:
			return (SK_DN_ARROW);
		case SDLK_KP7:
		case SDLK_HOME:
			return (SK_HOME);
		case SDLK_KP9:
		case SDLK_PAGEUP:
			return (SK_PAGE_UP);
		case SDLK_KP1:
		case SDLK_END:
			return (SK_END);
		case SDLK_KP3:
		case SDLK_PAGEDOWN:
			return (SK_PAGE_DOWN);
		case SDLK_KP0:
		case SDLK_INSERT:
			return (SK_INSERT);
		case SDLK_KP_PERIOD:
		case SDLK_DELETE:
			return (SK_DELETE);
		case SDLK_KP_PLUS:
			return (SK_KEYPAD_PLUS);
		case SDLK_KP_MINUS:
			return (SK_KEYPAD_MINUS);
		case SDLK_LSHIFT:
			return (SK_LF_SHIFT);
		case SDLK_RSHIFT:
			return (SK_RT_SHIFT);
		case SDLK_LCTRL:
			return (SK_LF_CTL);
		case SDLK_RCTRL:
			return (SK_RT_CTL);
		case SDLK_LALT:
			return (SK_LF_ALT);
		case SDLK_RALT:
			return (SK_RT_ALT);
		case SDLK_F1:
		case SDLK_F2:
		case SDLK_F3:
		case SDLK_F4:
		case SDLK_F5:
		case SDLK_F6:
		case SDLK_F7:
		case SDLK_F8:
		case SDLK_F9:
		case SDLK_F10:
		case SDLK_F11:
		case SDLK_F12:
			return ((UNICODE) ((SK_in - SDLK_F1) + SK_F1));
		default:
			return (UNICODE)SK_in;
	}
}

// Translates from values defined in inplib.h to SDLKey values
UWORD
UNICODEToKBD (UNICODE which_key)
{
	switch (which_key)
	{
		case '\r':
		case '\n':
			return SDLK_RETURN;
		case SK_LF_ARROW:
			return SDLK_LEFT;
		case SK_RT_ARROW:
			return SDLK_RIGHT;
		case SK_UP_ARROW:
			return SDLK_UP;
		case SK_DN_ARROW:
			return SDLK_DOWN;
		case SK_HOME:
			return SDLK_HOME;
		case SK_PAGE_UP:
			return SDLK_PAGEUP;
		case SK_END:
			return SDLK_END;
		case SK_PAGE_DOWN:
			return SDLK_PAGEDOWN;
		case SK_INSERT:
			return SDLK_INSERT;
		case SK_DELETE:
			return SDLK_DELETE;
		case SK_KEYPAD_PLUS:
			return SDLK_KP_PLUS;
		case SK_KEYPAD_MINUS:
			return SDLK_KP_MINUS;
		case SK_LF_SHIFT:
			return SDLK_LSHIFT;
		case SK_RT_SHIFT:
			return SDLK_RSHIFT;
		case SK_LF_CTL:
			return SDLK_LCTRL;
		case SK_RT_CTL:
			return SDLK_RCTRL;
		case SK_LF_ALT:
			return SDLK_LALT;
		case SK_RT_ALT:
			return SDLK_RALT;
		case SK_F1:
		case SK_F2:
		case SK_F3:
		case SK_F4:
		case SK_F5:
		case SK_F6:
		case SK_F7:
		case SK_F8:
		case SK_F9:
		case SK_F10:
		case SK_F11:
		case SK_F12:
			return (which_key - SK_F1) + SDLK_F1;
		default:
			return which_key;
	}
}

UNICODE
GetUNICODEKey (void)
{
	if (kbdtail != kbdhead)
	{
		UNICODE ch;

		ch = kbdbuf[kbdhead];
		kbdhead = (kbdhead + 1) & (KBDBUFSIZE - 1);
		return (ch);
	}
	return (0);
}

BOOLEAN
KeyDown (UNICODE which_scan)
{
	UWORD i;

	i = UNICODEToKBD(which_scan);
	if (KeyboardDown[i] != 0)
	{
		KeyboardDown[i] &= (~0x2);
		return TRUE;
	}
	else
		return FALSE;
}

INPUT_STATE
AnyButtonPress (BOOLEAN DetectSpecial)
{
	int i,j;

	if (DetectSpecial)
	{
		if (KeyDown (PauseKey) && PauseFunc)
			PauseFunc();
	}

	for (i = 0; i < SDLK_LAST; ++i)
	{
		if (KeyboardDown[i])
		{
			KeyboardDown[i] &= ~0x2;
			return (DEVICE_BUTTON1);
		}
	}
	for (j = 0; j < nJoysticks; ++j)
	{
		for (i = 0; i < SDL_JoystickNumButtons(Joysticks[j]); ++i)
		{
			if (JoybuttonDown[j][i])
			{
				JoybuttonDown[j][i] &= ~0x2;
				return (DEVICE_BUTTON1);
			}
		}
	}
	return (0);
}

BOOLEAN
_joystick_port_active(COUNT port) //SDL handles this nicely.
{
	return SDL_JoystickOpened(port);
}

INPUT_STATE
_get_pause_exit_state (void)
{
	INPUT_STATE InputState;

	InputState = 0;
	if (KeyDown (PauseKey) && PauseFunc)
	{
		PauseFunc();
	}
	else if (KeyDown (ExitKey))
	{
		InputState = DEVICE_EXIT;
	}
	return (InputState);
}

INPUT_STATE
_get_serial_keyboard_state (INPUT_REF ref, INPUT_STATE InputState)
{
	Uint16 key;
	(void) ref;

	if ((key = GetUNICODEKey ()) == '\r')
		key = '\n';
	if (key != 0)
	{
		SetInputUNICODE (&InputState, key);
		SetInputDevType (&InputState, KEYBOARD_DEVICE);
	}
	else
		InputState = 0;

	return (InputState);
}

static BOOLEAN
GetKeyboardDown (UWORD value)
{
	if (KEYISCHORD(value))
	{
		int k1, k2;
		k1 = KeyDown(KEYGETKEY(value));
		k2 = KeyDown(KEYGETCHORD(value));
		if (k1 && k2)
			return TRUE;
	}
	else
	{
		if (KeyDown(KEYGETKEY(value)))
			return TRUE;
	}
	return FALSE;
}

INPUT_STATE
_get_joystick_keyboard_state (INPUT_REF InputRef, INPUT_STATE InputState)
{
	SBYTE dx, dy;
	UWORD* KeyEquivalentPtr;

	KeyEquivalentPtr = GetInputDeviceKeyEquivalentPtr (InputRef);
	dx = (SBYTE)(GetKeyboardDown (KeyEquivalentPtr[1]) -
			GetKeyboardDown (KeyEquivalentPtr[0]));
	SetInputXComponent (&InputState, dx);

	KeyEquivalentPtr += 2;
	dy = (SBYTE)(GetKeyboardDown (KeyEquivalentPtr[1]) -
			GetKeyboardDown (KeyEquivalentPtr[0]));
	SetInputYComponent (&InputState, dy);

	KeyEquivalentPtr += 2;
	if (GetKeyboardDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_BUTTON1;
	if (GetKeyboardDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_BUTTON2;
	if (GetKeyboardDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_BUTTON3;
	if (GetKeyboardDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_BUTTON4;

	if (GetKeyboardDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_LEFTSHIFT;
	if (GetKeyboardDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_RIGHTSHIFT;

	if (InputState)
		SetInputDevType (&InputState, JOYSTICK_DEVICE);

	return (InputState);
}

static BOOLEAN
GetJoystickDown (int port, UWORD map, UWORD threshold)
{
	int axis, sign;
	if (JOYISAXIS(map))
	{
		axis = JOYGETVALUE(map);
		sign = JOYGETSIGN(map);
		if ((sign * JoyaxisValues[port][axis]) > threshold)
			return TRUE;
	}
	else
	{
		if (JOYISCHORD(map))
		{
			UBYTE b1 = JoybuttonDown[port][JOYGETVALUE(map)];
			UBYTE b2 = JoybuttonDown[port][JOYGETCHORD(map)];
			JoybuttonDown[port][JOYGETVALUE(map)] &= (~0x2);
			JoybuttonDown[port][JOYGETCHORD(map)] &= (~0x2);
			if (b1 && b2)
				return TRUE;
		}
		else if (JoybuttonDown[port][JOYGETVALUE(map)])
		{
			JoybuttonDown[port][JOYGETVALUE(map)] &= (~0x2);
			return TRUE;
		}
	}
	return FALSE;
}

INPUT_STATE
_get_joystick_state (INPUT_REF ref, INPUT_STATE InputState)
{
	SBYTE dx, dy;
	UWORD *JoystickButtons = GetInputDeviceJoystickButtonPtr(ref);
	UWORD threshold = GetInputDeviceJoystickThreshold(ref);
	int port = GetInputDeviceJoystickPort(ref);

	if (GetJoystickDown(port, JoystickButtons[0], threshold))
		dx = -1;
	else if (GetJoystickDown(port, JoystickButtons[1], threshold))
		dx = 1;
	else
		dx = 0;
	
	if (GetJoystickDown(port, JoystickButtons[2], threshold))
		dy = -1;
	else if (GetJoystickDown(port, JoystickButtons[3], threshold))
		dy = 1;
	else
		dy = 0;

	SetInputXComponent(&InputState, dx);
	SetInputYComponent(&InputState, dy);

	if (GetJoystickDown(port, JoystickButtons[4], threshold))
		InputState |= DEVICE_BUTTON1;
	if (GetJoystickDown(port, JoystickButtons[5], threshold))
		InputState |= DEVICE_BUTTON2;
	if (GetJoystickDown(port, JoystickButtons[6], threshold))
		InputState |= DEVICE_BUTTON3;
	if (GetJoystickDown(port, JoystickButtons[7], threshold))
		InputState |= DEVICE_LEFTSHIFT;
	if (GetJoystickDown(port, JoystickButtons[8], threshold))
		InputState |= DEVICE_RIGHTSHIFT;

	if (InputState)
		SetInputDevType(&InputState, JOYSTICK_DEVICE);
	return InputState;
}

#endif
