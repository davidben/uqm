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

#include <assert.h>
#include <errno.h>
#include "libs/graphics/sdl/sdl_common.h"
#include "libs/input/input_common.h"
#include "libs/input/sdl/vcontrol.h"
#include "controls.h"
#include "libs/file.h"
#include "options.h"


#define KBDBUFSIZE (1 << 8)
static int kbdhead=0, kbdtail=0;
static UNICODE kbdbuf[KBDBUFSIZE];

static BOOLEAN InputInitialized = FALSE;
UWORD UNICODEToKBD (UNICODE which_key);

static BOOLEAN _in_character_mode = FALSE;

static VControl_NameBinding control_names[] = {
	{ "Menu-Up", (int *)&ImmediateMenuState.up },
	{ "Menu-Down", (int *)&ImmediateMenuState.down },
	{ "Menu-Left", (int *)&ImmediateMenuState.left },
	{ "Menu-Right", (int *)&ImmediateMenuState.right },
	{ "Menu-Select", (int *)&ImmediateMenuState.select },
	{ "Menu-Cancel", (int *)&ImmediateMenuState.cancel },
	{ "Menu-Special", (int *)&ImmediateMenuState.special },
	{ "Menu-Page-Up", (int *)&ImmediateMenuState.page_up },
	{ "Menu-Page-Down", (int *)&ImmediateMenuState.page_down },
	{ "Menu-Zoom-In", (int *)&ImmediateMenuState.zoom_in },
	{ "Menu-Zoom-Out", (int *)&ImmediateMenuState.zoom_out },
	{ "Menu-Delete", (int *)&ImmediateMenuState.del },
	{ "Player-1-Thrust", (int *)&ImmediateInputState.p1_thrust },
	{ "Player-1-Left", (int *)&ImmediateInputState.p1_left },
	{ "Player-1-Right", (int *)&ImmediateInputState.p1_right },
	{ "Player-1-Down", (int *)&ImmediateInputState.p1_down },
	{ "Player-1-Weapon", (int *)&ImmediateInputState.p1_weapon },
	{ "Player-1-Special", (int *)&ImmediateInputState.p1_special },
	{ "Player-1-Escape", (int *)&ImmediateInputState.p1_escape },
	{ "Player-2-Thrust", (int *)&ImmediateInputState.p2_thrust },
	{ "Player-2-Left", (int *)&ImmediateInputState.p2_left },
	{ "Player-2-Right", (int *)&ImmediateInputState.p2_right },
	{ "Player-2-Down", (int *)&ImmediateInputState.p2_down },
	{ "Player-2-Weapon", (int *)&ImmediateInputState.p2_weapon },
	{ "Player-2-Special", (int *)&ImmediateInputState.p2_special },
	{ "Lander-Thrust", (int *)&ImmediateInputState.lander_thrust },
	{ "Lander-Left", (int *)&ImmediateInputState.lander_left },
	{ "Lander-Right", (int *)&ImmediateInputState.lander_right },
	{ "Lander-Weapon", (int *)&ImmediateInputState.lander_weapon },
	{ "Lander-Escape", (int *)&ImmediateInputState.lander_escape },
	{ "Pause", (int *)&ImmediateInputState.pause },
	{ "Exit", (int *)&ImmediateInputState.exit },
	{ "Abort", (int *)&ImmediateInputState.abort },
	{ "Debug", (int *)&ImmediateInputState.debug }};


static void
initKeyConfig(void) {
	uio_Stream *fp;
	int errors;
	
	fp = res_OpenResFile (configDir, "keys.cfg", "rt");
	if (fp == NULL)
	{
		if (copyFile (contentDir, "starcon.key",
				configDir, "keys.cfg") == -1)
		{
			fprintf(stderr, "Warning: Could not copy default key config "
					"to user config dir: %s.\n", strerror (errno));
		}
		else
		{
			fprintf(stderr, "Copying default key config file to user "
					"config dir.\n");
		}
		fp = res_OpenResFile (contentDir, "starcon.key", "rt");
	}
	errors = VControl_ReadConfiguration (fp);
	res_CloseResFile (fp);
	if (errors)
	{
		fprintf (stderr, "%d errors encountered in key configuration "
				"file.\n", errors);
		/* This code should go away in 0.3; it's just
		 * to force people to upgrade and forestall a 
		 * bunch of "none of my keys work anymore" bugs.
		 */
		if (errors > 5)
		{
			fprintf (stderr, "Hey!  you haven't updated your keys.cfg to "
					"use the new system, have you?\nDelete keys.cfg and "
					"try again.\n");
			fprintf (stderr, "If you've done that and it STILL doesn't "
					"work, make sure your content/starcon.key is up to"
					"date.\n");
		}
		else
		{
			fprintf (stderr, "Repair your keys.cfg file to continue.\n");
		}
		
		exit (1);
	}		

}

int 
TFB_InitInput (int driver, int flags)
{
	int i;
	int nJoysticks;
	(void)driver;
	(void)flags;

	GamePaused = ExitRequested = FALSE;

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
		fprintf (stderr, "The names of the joysticks are:\n");
		for(i = 0; i < nJoysticks; i++)
		{
			fprintf (stderr, "    %s\n", SDL_JoystickName (i));
		}
		SDL_JoystickEventState (SDL_ENABLE);
	}

	_in_character_mode = FALSE;

	/* Prepare the Virtual Controller system. */
	VControl_Init ();
	VControl_RegisterNameTable (control_names);

	initKeyConfig ();
	
	VControl_ResetInput ();
	InputInitialized = TRUE;

	atexit (TFB_UninitInput);
	return 0;
}

void
TFB_UninitInput (void)
{
	VControl_Uninit ();
}

void
EnableCharacterMode (void)
{
	kbdhead = kbdtail = 0;
	_in_character_mode = TRUE;
	VControl_ResetInput ();
	
}

void
DisableCharacterMode (void)
{
	VControl_ResetInput ();
	_in_character_mode = FALSE;
	kbdhead = kbdtail = 0;
}

UNICODE
GetCharacter (void)
{
	UNICODE result;
	while (kbdhead == kbdtail)
		TaskSwitch ();
	result = kbdbuf[kbdhead];
	kbdhead = (kbdhead + 1) & (KBDBUFSIZE - 1);
	return result;
}	

void
ProcessInputEvent(const SDL_Event *Event)
{
	if (!InputInitialized)
		return;
	if (!_in_character_mode)
	{
		VControl_HandleEvent (Event);
	}
	else
	{
		SDLKey k = Event->key.keysym.sym;
		UNICODE map_key;
		if (Event->type != SDL_KEYDOWN)
			return;
		if (!Event->key.keysym.unicode &&
				(k == SDLK_CAPSLOCK ||
				k == SDLK_NUMLOCK))
			return;
		if (Event->key.keysym.unicode != 0)
			map_key = (UNICODE) Event->key.keysym.unicode;
		else
			map_key = KBDToUNICODE(k);
	
		kbdbuf[kbdtail] = map_key;
		kbdtail = (kbdtail + 1) & (KBDBUFSIZE - 1);
		if (kbdtail == kbdhead)
			kbdhead = (kbdhead + 1) & (KBDBUFSIZE - 1);
	}
}

void
TFB_ResetControls (void)
{
	VControl_ResetInput ();
}

void
FlushInput (void)
{
	TFB_ResetControls ();
	FlushInputState ();
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

#endif
