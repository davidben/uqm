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


Uint8 KeyboardDown[512];
#define KBDBUFSIZE (1 << 8)
int kbdhead, kbdtail;
Uint16 kbdbuf[KBDBUFSIZE];

SDLKey ControlA;
SDLKey ControlB;
SDLKey ControlC;
SDLKey ControlX;
SDLKey ControlStart;
SDLKey ControlLeftShift;
SDLKey ControlRightShift;


static UNICODE PauseKey;
static UNICODE ExitKey;
static UNICODE OtherKey;

static BOOLEAN (* PauseFunc) (void);


int 
TFB_InitInput (int driver, int flags)
{
	int i;
	SDL_Joystick *Joystick1;

	atexit (TFB_UninitInput);

	if ((SDL_InitSubSystem(SDL_INIT_JOYSTICK)) == -1)
	{
		fprintf (stderr, "Couldn't initialize joystick subsystem: %s\n", SDL_GetError());
		exit(-1);
	}

	fprintf (stderr, "%i joysticks were found.\n", SDL_NumJoysticks ());
	
	if (SDL_NumJoysticks () > 0)
	{
		fprintf (stderr, "The names of the joysticks are:\n");
		for(i = 0; i < SDL_NumJoysticks (); i++)
		{
			fprintf (stderr, "    %s\n", SDL_JoystickName (i));
		}
		SDL_JoystickEventState (SDL_ENABLE);
		Joystick1 = SDL_JoystickOpen (0);
		if (SDL_NumJoysticks () > 1)
			SDL_JoystickOpen(1);
	}

	for(i = 0; i < 512; i++)
	{
		KeyboardDown[i] = FALSE;
		//KeyboardStroke[i] = FALSE;
	}

	ControlA =          SDLK_f;
	ControlB =          SDLK_d;
	ControlC =          SDLK_s;
	ControlX =          SDLK_a;
	ControlStart =      SDLK_RETURN;
	ControlLeftShift =  SDLK_LSHIFT;
	ControlRightShift = SDLK_RSHIFT;

	return 0;
}

void
TFB_UninitInput (void)
{
}

void
ProcessKeyboardEvent(const SDL_Event *Event)
{
    if(Event->key.keysym.sym == SDLK_BACKQUOTE ||
	   Event->key.keysym.sym == SDLK_WORLD_7)
	{
		exit(0);
	}
	else
	{
		if (Event->type == SDL_KEYDOWN)
		{
			if ((kbdbuf[kbdtail] = Event->key.keysym.unicode)
					|| (kbdbuf[kbdtail] = Event->key.keysym.sym) <= 0x7F)
			{
				kbdtail = (kbdtail + 1) & (KBDBUFSIZE - 1);
				if (kbdtail == kbdhead)
					kbdhead = (kbdhead + 1) & (KBDBUFSIZE - 1);
			}
			KeyboardDown[Event->key.keysym.sym]=TRUE;
		}
		else
		{
			KeyboardDown[Event->key.keysym.sym]=FALSE;
		}
	}
}

void
ProcessJoystickEvent (const SDL_Event* Event)
{
	SDL_Event PseudoEvent;

	return;

	if (Event->type == SDL_JOYAXISMOTION)
	{
		if (Event->jaxis.axis == 0)
		{
			//x-axis

			if (Event->jaxis.value <= -5) //Left
			{
				PseudoEvent.type = SDL_KEYDOWN;
				PseudoEvent.key.keysym.sym = SDLK_LEFT;
				ProcessKeyboardEvent (&PseudoEvent);

				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_RIGHT;
				ProcessKeyboardEvent (&PseudoEvent);
			}
			if (Event->jaxis.value >= 5) //Right
			{
				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_LEFT;
				ProcessKeyboardEvent (&PseudoEvent);

				PseudoEvent.type = SDL_KEYDOWN;
				PseudoEvent.key.keysym.sym = SDLK_RIGHT;
				ProcessKeyboardEvent (&PseudoEvent);
			}
			if (Event->jaxis.value > -5 && Event->jaxis.value < 5)
					//Neither Left nor Right
			{
				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_LEFT;
				ProcessKeyboardEvent (&PseudoEvent);

				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_RIGHT;
				ProcessKeyboardEvent (&PseudoEvent);
			}
		}
		if (Event->jaxis.axis == 1)
		{
			//y-axis

			if (Event->jaxis.value <= -5) //Down
			{
				PseudoEvent.type = SDL_KEYDOWN;
				PseudoEvent.key.keysym.sym = SDLK_DOWN;
				ProcessKeyboardEvent (&PseudoEvent);

				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_UP;
				ProcessKeyboardEvent (&PseudoEvent);
			}
			if (Event->jaxis.value >= 5) //Up
			{
				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_DOWN;
				ProcessKeyboardEvent (&PseudoEvent);

				PseudoEvent.type = SDL_KEYDOWN;
				PseudoEvent.key.keysym.sym = SDLK_UP;
				ProcessKeyboardEvent (&PseudoEvent);
			}
			if (Event->jaxis.value >-5 && Event->jaxis.value < 5)
					//Neither Down nor Up
			{
				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_DOWN;
				ProcessKeyboardEvent (&PseudoEvent);

				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_UP;
				ProcessKeyboardEvent (&PseudoEvent);
			}
		}
	}
	if (Event->type == SDL_JOYBUTTONDOWN)
	{
		fprintf (stderr, "Joystick %i down\n", Event->jbutton.button);

		if (Event->jbutton.button % 3 == 0)
		{
			//Mimic the Primary key

			PseudoEvent.type=SDL_KEYDOWN;
			PseudoEvent.key.keysym.sym = ControlA;
			ProcessKeyboardEvent (&PseudoEvent);
		}
		if (Event->jbutton.button % 3 == 1)
		{
			//Mimic the Secondary key

			PseudoEvent.type = SDL_KEYDOWN;
			PseudoEvent.key.keysym.sym = ControlB;
			ProcessKeyboardEvent (&PseudoEvent);
		}
		if (Event->jbutton.button % 3 == 2)
		{
			//Mimic the Tertiary key

			PseudoEvent.type = SDL_KEYDOWN;
			PseudoEvent.key.keysym.sym = ControlC;
			ProcessKeyboardEvent (&PseudoEvent);
		}
	}
	if (Event->type == SDL_JOYBUTTONUP)
	{
		if (Event->jbutton.button % 3 == 0)
		{
			//Mimic the Primary key

			PseudoEvent.type = SDL_KEYUP;
			PseudoEvent.key.keysym.sym = ControlA;
			ProcessKeyboardEvent (&PseudoEvent);
		}
		if (Event->jbutton.button % 3 == 1)
		{
			//Mimic the Secondary key

			PseudoEvent.type = SDL_KEYUP;
			PseudoEvent.key.keysym.sym = ControlB;
			ProcessKeyboardEvent (&PseudoEvent);
		}
		if (Event->jbutton.button % 3 == 2)
		{
			//Mimic the Tertiary key

			PseudoEvent.type = SDL_KEYUP;
			PseudoEvent.key.keysym.sym = ControlC;
			ProcessKeyboardEvent (&PseudoEvent);
		}
	}
}

// Status: Unimplemented
extern BOOLEAN
InitInput (UNICODE Pause, UNICODE Exit, BOOLEAN (*PFunc) (void))
//Be careful with this one.
{
	PauseKey = Pause;
	ExitKey = Exit;
		
	// Find a unique byte ID not equal to PauseKey, ExitKey, or 0.
	OtherKey = 1;
	while (OtherKey == PauseKey || OtherKey == ExitKey)
		OtherKey++;
				
	PauseFunc = PFunc;

	return (TRUE);
}

//Status: Ignored
BOOLEAN
UninitInput () // Looks like it'll be empty
{
		BOOLEAN ret;

		// fprintf (stderr, "Unimplemented function activated: UninitInput()\n");
		ret = TRUE;
		return (ret);
}

void
FlushInput ()
{
	kbdtail = kbdhead = 0;
}

//Status: Unimplemented
UNICODE
KBDToUNICODE (UNICODE SK_in)
		//This one'll probably require a big table. Arg.
{
	return (SK_in);
}

Uint16
GetUNICODEKey ()
{
	if (kbdtail != kbdhead)
	{
		Uint16 ch;

		ch = kbdbuf[kbdhead];
		kbdhead = (kbdhead + 1) & (KBDBUFSIZE - 1);
		return (ch);
	}

	return (0);
}

// Status: Unimplemented
UNICODE
KeyHit () // Does this clear the top of a queue, or just read it?
{
	int i;

	for (i = 0; i < sizeof (KeyboardDown) / sizeof (KeyboardDown[0]); ++i)
	{
		if (KeyboardDown[i])
		{
			if (i == SDLK_RETURN)
				return ('\n');
			else if (i >= 256)
			{
				switch (i)
				{
					case SDLK_KP_ENTER:
						return ('\n');
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
					case SDLK_RCTRL:
						return (SK_CTL);
					case SDLK_LALT:
					case SDLK_RALT:
						return (SK_ALT);
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
						return ((UNICODE) ((i - SDLK_F1) + SK_F1));
				}
				continue;
			}

			return ((UNICODE) i);
		}
	}

	return (0);
}

//Status: Unimplemented
int
KeyDown (UNICODE which_scan)
		// This might use SK_* stuff, of just plain old chars
{
	int i;

	i = which_scan;
	if (i == '\n')
	{
		if (KeyboardDown[SDLK_KP_ENTER])
			return (1);
		i = SDLK_RETURN;
	}
	else if (i >= 128)
	{
		switch (i)
		{
			case SK_LF_ARROW:
				if (KeyboardDown[SDLK_KP4])
					return (1);
				i = SDLK_LEFT;
				break;
			case SK_RT_ARROW:
				if (KeyboardDown[SDLK_KP6])
					return (1);
				i = SDLK_RIGHT;
				break;
			case SK_UP_ARROW:
				if (KeyboardDown[SDLK_KP8])
					return (1);
				i = SDLK_UP;
				break;
			case SK_DN_ARROW:
				if (KeyboardDown[SDLK_KP2])
					return (1);
				i = SDLK_DOWN;
				break;
			case SK_HOME:
				if (KeyboardDown[SDLK_KP7])
					return (1);
				i = SDLK_HOME;
				break;
			case SK_PAGE_UP:
				if (KeyboardDown[SDLK_KP9])
					return (1);
				i = SDLK_PAGEUP;
				break;
			case SK_END:
				if (KeyboardDown[SDLK_KP1])
					return (1);
				i = SDLK_END;
				break;
			case SK_PAGE_DOWN:
				if (KeyboardDown[SDLK_KP3])
					return (1);
				i = SDLK_PAGEDOWN;
				break;
			case SK_INSERT:
				if (KeyboardDown[SDLK_KP0])
					return (1);
				i = SDLK_INSERT;
				break;
			case SK_DELETE:
				if (KeyboardDown[SDLK_KP_PERIOD])
					return (1);
				i = SDLK_DELETE;
				break;
			case SK_KEYPAD_PLUS:
				i = SDLK_KP_PLUS;
				break;
			case SK_KEYPAD_MINUS:
				i = SDLK_KP_MINUS;
				break;
			case SK_LF_SHIFT:
				i = SDLK_LSHIFT;
				break;
			case SK_RT_SHIFT:
				i = SDLK_RSHIFT;
				break;
			case SK_CTL:
				if (KeyboardDown[SDLK_LCTRL])
					return (1);
				i = SDLK_RCTRL;
				break;
			case SK_ALT:
				if (KeyboardDown[SDLK_LALT])
					return (1);
				i = SDLK_RALT;
				break;
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
				i = (i - SK_F1) + SDLK_F1;
		}
	}

	return (KeyboardDown[i]);
}

INPUT_STATE
AnyButtonPress (BOOLEAN DetectSpecial)
{
	int i;

	if (DetectSpecial)
	{
		if (KeyDown (PauseKey) && PauseFunc && (*PauseFunc) ())
			;
	}

	for (i = 0; i < sizeof (KeyboardDown) / sizeof (KeyboardDown[0]); ++i)
	{
		if (KeyboardDown[i])
			return (DEVICE_BUTTON1);
	}
#if 0
	for (i = 0; i < JOYSTICKS_AVAIL; i++)
		if (read_joystick (i))
			return (DEVICE_BUTTON1);
#endif
		
	return (0);
}

// Status: Ignored - The keyboard will serve as the joystick, thus a port is always open.
BOOLEAN
_joystick_port_active(COUNT port) //SDL handles this nicely.
{
	fprintf (stderr, "Unimplemented function activated: _joystick_port_active()\n");
	return (0);
}

INPUT_STATE
_get_pause_exit_state (void)
{
	INPUT_STATE InputState;

	InputState = 0;
	if (KeyDown (PauseKey))
	{
		if (PauseFunc && (*PauseFunc) ())
			;
	}
	else if (KeyDown (ExitKey))
	{
		InputState = DEVICE_EXIT;
	}
#if 0
	for (i = 0; i < JOYSTICKS_AVAIL; i++)
	{
		DWORD joy;
		
		joy = read_joystick (i);
		if (joy & ControlStart) // pause
		{
			if (PauseFunc && (*PauseFunc) ())
// while (KeyDown (PauseKey))
				;
		}
		else if (joy & ControlX) // exit
		{
// while (KeyDown (ExitKey))
// TaskSwitch ();

			InputState = DEVICE_EXIT;
		}
	}
#endif

	return (InputState);
}

// Status: Unimplemented
INPUT_STATE
_get_serial_keyboard_state (INPUT_REF ref, INPUT_STATE InputState)
		// I hope these are defined somewhere...
{
	Uint16 key;

	if ((key = GetUNICODEKey ()) == '\r')
		key = '\n';
	SetInputUNICODE (&InputState, key);
	SetInputDevType (&InputState, KEYBOARD_DEVICE);

	return (InputState);
}

// Status: Unimplemented
INPUT_STATE
_get_joystick_keyboard_state (INPUT_REF InputRef, INPUT_STATE InputState)
		// see line above.
{
	SBYTE dx, dy;
	BYTEPTR KeyEquivalentPtr;

	KeyEquivalentPtr = GetInputDeviceKeyEquivalentPtr (InputRef);
	dx = (SBYTE)(KeyDown (KeyEquivalentPtr[1]) -
			KeyDown (KeyEquivalentPtr[0]));
	SetInputXComponent (&InputState, dx);

	KeyEquivalentPtr += 2;
	dy = (SBYTE)(KeyDown (KeyEquivalentPtr[1]) -
			KeyDown (KeyEquivalentPtr[0]));
	SetInputYComponent (&InputState, dy);

	KeyEquivalentPtr += 2;
	if (KeyDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_BUTTON1;
	if (KeyDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_BUTTON2;
	if (KeyDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_BUTTON3;

	if (KeyDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_LEFTSHIFT;
	if (KeyDown (*KeyEquivalentPtr++))
		InputState |= DEVICE_RIGHTSHIFT;

	if (InputState)
		SetInputDevType (&InputState, JOYSTICK_DEVICE);

	return (InputState);
}

/*
// Status: Implemented
INPUT_STATE
_get_joystick_state (INPUT_REF ref, INPUT_STATE InputState)
		// consistant, at least.
{
// fprintf (stderr, "Half-implemented function activated: _get_joystick_state()\n");

	if (KeyboardStroke[SDLK_LEFT])
	{
		SetInputXComponent (&InputState, -1);
// fprintf (stderr, "LEFT!\n");
		KeyboardStroke[SDLK_LEFT] = FALSE;
	}
	if (KeyboardStroke[SDLK_RIGHT])
	{
		SetInputXComponent (&InputState, 1);
// fprintf (stderr, "RIGHT!\n");
		KeyboardStroke[SDLK_RIGHT] = FALSE;
	}

	if (KeyboardStroke[SDLK_UP])
	{
		SetInputYComponent (&InputState, -1);
// fprintf (stderr, "UP!\n");
		KeyboardStroke[SDLK_UP] = FALSE;
	}
	if (KeyboardStroke[SDLK_DOWN])
	{
		SetInputYComponent (&InputState, 1);
// fprintf (stderr, "DOWN!\n");
		KeyboardStroke[SDLK_DOWN] = FALSE;
	}

	if (KeyboardStroke[ControlA])
	{
		InputState |= DEVICE_BUTTON1;
// fprintf (stderr, "BUTTON1!\n");
		KeyboardStroke[ControlA]=FALSE;
	}
	if (KeyboardStroke[ControlB])
	{
		InputState |= DEVICE_BUTTON2;
// fprintf (stderr, "BUTTON2!\n");
		KeyboardStroke[ControlB] = FALSE;
	}
	if (KeyboardStroke[ControlC])
	{
		InputState |= DEVICE_BUTTON3;
// fprintf (stderr, "BUTTON3!\n");
		KeyboardStroke[ControlC] = FALSE;
	}
	if (KeyboardStroke[ControlX])
	{
		InputState |= DEVICE_EXIT;
// fprintf (stderr, "BUTTONX!\n");
		KeyboardStroke[ControlX] = FALSE;
	}
	if (KeyboardStroke[ControlStart])
	{
		InputState |= DEVICE_PAUSE;
// fprintf (stderr, "CONTROLSTART!\n");
		KeyboardStroke[ControlStart] = FALSE;
	}
	if (KeyboardStroke[ControlLeftShift])
	{
		InputState |= DEVICE_LEFTSHIFT;
// fprintf (stderr, "LEFTSHIFT!\n");
		KeyboardStroke[ControlLeftShift] = FALSE;
	}
	if (KeyboardStroke[ControlRightShift])
	{
		InputState |= DEVICE_RIGHTSHIFT;
// fprintf (stderr, "RIGHTSHIFT!\n");
		KeyboardStroke[ControlRightShift] = FALSE;
	}
	
	return (InputState);
}
*/

INPUT_STATE
_get_joystick_state (INPUT_REF ref, INPUT_STATE InputState)
		// consistant, at least.
{
#if 0
	if (ref == JoystickInput[1] && 0)
	{
		//fprintf (stderr, "ref\n");
		
		return(0);
	}
	
// fprintf (stderr, "Half-implemented function activated: _get_joystick_state()\n");

	if (KeyboardDown[SDLK_LEFT])
	{
		SetInputXComponent (&InputState, -1);
		//fprintf (stderr, "LEFT!\n");
	}
	if (KeyboardDown[SDLK_RIGHT])
	{
		SetInputXComponent (&InputState, 1);
		//fprintf (stderr, "RIGHT!\n");
	}

	if (KeyboardDown[SDLK_UP])
	{
		SetInputYComponent (&InputState, -1);
		//fprintf (stderr, "UP!\n");
	}
	if (KeyboardDown[SDLK_DOWN])
	{
		SetInputYComponent (&InputState, 1);
		//fprintf (stderr, "DOWN!\n");
	}

	if (KeyboardDown[ControlA])
	{
		InputState |= DEVICE_BUTTON1;
		//fprintf (stderr, "BUTTON1!\n");
	}
	if (KeyboardDown[ControlB])
	{
		InputState |= DEVICE_BUTTON2;
		//fprintf (stderr, "BUTTON2!\n");
	}
	if (KeyboardDown[ControlC])
	{
		InputState |= DEVICE_BUTTON3;
		//fprintf (stderr, "BUTTON3!\n");
	}
	if (KeyboardDown[ControlX])
	{
		InputState |= DEVICE_EXIT;
		//fprintf (stderr, "BUTTONX!\n");
	}
	if (KeyboardDown[ControlStart])
	{
		InputState |= DEVICE_PAUSE;
		//fprintf (stderr, "CONTROLSTART!\n");
	}
	if (KeyboardDown[ControlLeftShift])
	{
		InputState |= DEVICE_LEFTSHIFT;
		//fprintf (stderr, "LEFTSHIFT!\n");
	}
	if (KeyboardDown[ControlRightShift])
	{
		InputState |= DEVICE_RIGHTSHIFT;
		//fprintf (stderr, "RIGHTSHIFT!\n");
	}
#endif
	
	return (InputState);
}

#endif
