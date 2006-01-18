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
static wchar_t kbdbuf[KBDBUFSIZE];
static wchar_t lastchar;
static int kbdstate[SDLK_LAST];

static BOOLEAN InputInitialized = FALSE;

static BOOLEAN _in_character_mode = FALSE;

#define VCONTROL_VERSION 1

static VControl_NameBinding control_names[] = {
	{ "Menu-Up", (int *)&ImmediateInputState.key[KEY_MENU_UP] },
	{ "Menu-Down", (int *)&ImmediateInputState.key[KEY_MENU_DOWN] },
	{ "Menu-Left", (int *)&ImmediateInputState.key[KEY_MENU_LEFT] },
	{ "Menu-Right", (int *)&ImmediateInputState.key[KEY_MENU_RIGHT] },
	{ "Menu-Select", (int *)&ImmediateInputState.key[KEY_MENU_SELECT] },
	{ "Menu-Cancel", (int *)&ImmediateInputState.key[KEY_MENU_CANCEL] },
	{ "Menu-Special", (int *)&ImmediateInputState.key[KEY_MENU_SPECIAL] },
	{ "Menu-Page-Up", (int *)&ImmediateInputState.key[KEY_MENU_PAGE_UP] },
	{ "Menu-Page-Down", (int *)&ImmediateInputState.key[KEY_MENU_PAGE_DOWN] },
	{ "Menu-Home", (int *)&ImmediateInputState.key[KEY_MENU_HOME] },
	{ "Menu-End", (int *)&ImmediateInputState.key[KEY_MENU_END] },
	{ "Menu-Zoom-In", (int *)&ImmediateInputState.key[KEY_MENU_ZOOM_IN] },
	{ "Menu-Zoom-Out", (int *)&ImmediateInputState.key[KEY_MENU_ZOOM_OUT] },
	{ "Menu-Delete", (int *)&ImmediateInputState.key[KEY_MENU_DELETE] },
	{ "Menu-Backspace", (int *)&ImmediateInputState.key[KEY_MENU_BACKSPACE] },
	{ "Menu-Edit-Cancel", (int *)&ImmediateInputState.key[KEY_MENU_EDIT_CANCEL] },
	{ "Menu-Search", (int *)&ImmediateInputState.key[KEY_MENU_SEARCH] },
	{ "Menu-Next", (int *)&ImmediateInputState.key[KEY_MENU_NEXT] },
	{ "Player-1-Thrust", (int *)&ImmediateInputState.key[KEY_P1_THRUST] },
	{ "Player-1-Left", (int *)&ImmediateInputState.key[KEY_P1_LEFT] },
	{ "Player-1-Right", (int *)&ImmediateInputState.key[KEY_P1_RIGHT] },
	{ "Player-1-Down", (int *)&ImmediateInputState.key[KEY_P1_DOWN] },
	{ "Player-1-Weapon", (int *)&ImmediateInputState.key[KEY_P1_WEAPON] },
	{ "Player-1-Special", (int *)&ImmediateInputState.key[KEY_P1_SPECIAL] },
	{ "Player-1-Escape", (int *)&ImmediateInputState.key[KEY_P1_ESCAPE] },
	{ "Player-2-Thrust", (int *)&ImmediateInputState.key[KEY_P2_THRUST] },
	{ "Player-2-Left", (int *)&ImmediateInputState.key[KEY_P2_LEFT] },
	{ "Player-2-Right", (int *)&ImmediateInputState.key[KEY_P2_RIGHT] },
	{ "Player-2-Down", (int *)&ImmediateInputState.key[KEY_P2_DOWN] },
	{ "Player-2-Weapon", (int *)&ImmediateInputState.key[KEY_P2_WEAPON] },
	{ "Player-2-Special", (int *)&ImmediateInputState.key[KEY_P2_SPECIAL] },
	{ "Lander-Thrust", (int *)&ImmediateInputState.key[KEY_LANDER_THRUST] },
	{ "Lander-Left", (int *)&ImmediateInputState.key[KEY_LANDER_LEFT] },
	{ "Lander-Right", (int *)&ImmediateInputState.key[KEY_LANDER_RIGHT] },
	{ "Lander-Weapon", (int *)&ImmediateInputState.key[KEY_LANDER_WEAPON] },
	{ "Lander-Escape", (int *)&ImmediateInputState.key[KEY_LANDER_ESCAPE] },
	{ "Pause", (int *)&ImmediateInputState.key[KEY_PAUSE] },
	{ "Exit", (int *)&ImmediateInputState.key[KEY_EXIT] },
	{ "Abort", (int *)&ImmediateInputState.key[KEY_ABORT] },
	{ "Debug", (int *)&ImmediateInputState.key[KEY_DEBUG] },
        { "Illegal", NULL}};


static void
initKeyConfig(void) {
	uio_Stream *fp;
	int i, errors;
	
	for (i = 0; i < 2; ++i)
	{
		if ((fp = res_OpenResFile (configDir, "keys.cfg", "rt")) == NULL)
		{
			if (copyFile (contentDir, "starcon.key",
				configDir, "keys.cfg") == -1)
			{
				fprintf (stderr, "Error: Could not copy default key config "
					"to user config dir: %s.\n", strerror (errno));
				exit (EXIT_FAILURE);
			}
			fprintf(stderr, "Copying default key config file to user "
				"config dir.\n");
			
			if ((fp = res_OpenResFile (configDir, "keys.cfg", "rt")) == NULL)
			{
				fprintf (stderr, "Error: Could not open keys.cfg\n");
				exit (EXIT_FAILURE);
			}
		}
		
		errors = VControl_ReadConfiguration (fp);
		res_CloseResFile (fp);
		
		if (errors || (VControl_GetConfigFileVersion () != VCONTROL_VERSION))
		{
			bool do_rename = false;
			if (errors)
				fprintf (stderr, "%d errors encountered in key configuration file.\n", errors);			

			if (VControl_GetValidCount () == 0)
			{
				fprintf (stderr, "\nI didn't understand a single line in your configuration file.\n");
				fprintf (stderr, "This is likely because you're still using a 0.2 era or earlier keys.cfg.\n");
				do_rename = true;
			}
			if (VControl_GetConfigFileVersion () != VCONTROL_VERSION)
			{
				fprintf (stderr, "\nThe control scheme for UQM has changed since you last updated keys.cfg.\n");
				fprintf (stderr, "(I'm using control scheme version %d, while your config file appears to be\nfor version %d.)\n", VCONTROL_VERSION, VControl_GetConfigFileVersion ());
				do_rename = true;
			}

			if (do_rename)
			{
				fprintf (stderr, "\nRenaming keys.cfg to keys.old and retrying.\n");

				if (fileExists2 (configDir, "keys.old"))
					uio_unlink (configDir, "keys.old");
				if ((uio_rename (configDir, "keys.cfg", configDir, "keys.old")) == -1)
				{
					fprintf (stderr, "Error: Renaming failed!\n");
					fprintf (stderr, "You must delete keys.cfg manually before you can run the game.\n");
					exit (EXIT_FAILURE);
				}
				continue;
			}
			
			fprintf (stderr, "\nRepair your keys.cfg file to continue.\n");
			exit (EXIT_FAILURE);
		}
		
		return;
	}

	fprintf (stderr, "Error: Something went wrong and we were looping again and again so aborting.\n");
	fprintf (stderr, "Possible cause is your content dir not being up-to-date.\n");
	exit (EXIT_FAILURE);
}

static void
resetKeyboardState (void)
{
	memset (kbdstate, 0, sizeof (kbdstate));
	ImmediateInputState.key[KEY_CHARACTER] = 0;
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
	resetKeyboardState ();

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

// XXX: not currently used -- character mode is always on
void
EnableCharacterMode (void)
{
	kbdhead = kbdtail = 0;
	lastchar = 0;
	_in_character_mode = TRUE;
	VControl_ResetInput ();
}

// XXX: not currently used -- character mode is always on
void
DisableCharacterMode (void)
{
	VControl_ResetInput ();
	_in_character_mode = FALSE;
	kbdhead = kbdtail = 0;
	lastchar = 0;
}

wchar_t
GetNextCharacter (void)
{
	wchar_t result;
	if (kbdhead == kbdtail)
		return 0;
	result = kbdbuf[kbdhead];
	kbdhead = (kbdhead + 1) & (KBDBUFSIZE - 1);
	return result;
}	

wchar_t
GetLastCharacter (void)
{
	return lastchar;
}	

volatile int MouseButtonDown = 0;

void
ProcessMouseEvent (const SDL_Event *e)
{
	switch (e->type)
	{
	case SDL_MOUSEBUTTONDOWN:
		MouseButtonDown = 1;
		break;
	case SDL_MOUSEBUTTONUP:
		MouseButtonDown = 0;
		break;
	default:
		break;
	}
}


void
ProcessInputEvent (const SDL_Event *Event)
{
	if (!InputInitialized)
		return;
	
	ProcessMouseEvent (Event);
	VControl_HandleEvent (Event);

	{	// process character input event, if any
		SDLKey k = Event->key.keysym.sym;
		wchar_t map_key = Event->key.keysym.unicode;

		if (Event->type == SDL_KEYDOWN)
		{
			// dont care about the non-printable, non-char
			if (!map_key)
				return;

			kbdstate[k]++;
			ImmediateInputState.key[KEY_CHARACTER]++;
			
			lastchar = map_key;
			kbdbuf[kbdtail] = map_key;
			kbdtail = (kbdtail + 1) & (KBDBUFSIZE - 1);
			if (kbdtail == kbdhead)
				kbdhead = (kbdhead + 1) & (KBDBUFSIZE - 1);
		}
		else if (Event->type == SDL_KEYUP)
		{
			if (kbdstate[k] == 0)
			{	// something is fishy -- better to reset the
				// repeatable state to avoid big problems
				ImmediateInputState.key[KEY_CHARACTER] = 0;
			}
			else
			{
				kbdstate[k]--;
				if (ImmediateInputState.key[KEY_CHARACTER] > 0)
					ImmediateInputState.key[KEY_CHARACTER]--;
			}
		}
	}
}

void
TFB_ResetControls (void)
{
	VControl_ResetInput ();
	resetKeyboardState ();
	// flush character buffer
	kbdhead = kbdtail = 0;
	lastchar = 0;
}

void
FlushInput (void)
{
	TFB_ResetControls ();
	FlushInputState ();
}

#endif
