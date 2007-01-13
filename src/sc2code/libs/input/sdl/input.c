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
#include "libs/input/sdl/keynames.h"
#include "misc.h"
#include "controls.h"
#include "libs/file.h"
#include "libs/log.h"
#include "libs/reslib.h"
#include "options.h"


#define KBDBUFSIZE (1 << 8)
static int kbdhead=0, kbdtail=0;
static wchar_t kbdbuf[KBDBUFSIZE];
static wchar_t lastchar;
static int num_keys = 0;
static int *kbdstate = NULL;
		// Holds all SDL keys +1 for holding invalid values

static BOOLEAN InputInitialized = FALSE;

static BOOLEAN _in_character_mode = FALSE;

#define VCONTROL_VERSION 4

static VControl_NameBinding control_names[] = {
	{ "Menu-Up", (int *)&ImmediateInputState.menu[KEY_MENU_UP] },
	{ "Menu-Down", (int *)&ImmediateInputState.menu[KEY_MENU_DOWN] },
	{ "Menu-Left", (int *)&ImmediateInputState.menu[KEY_MENU_LEFT] },
	{ "Menu-Right", (int *)&ImmediateInputState.menu[KEY_MENU_RIGHT] },
	{ "Menu-Select", (int *)&ImmediateInputState.menu[KEY_MENU_SELECT] },
	{ "Menu-Cancel", (int *)&ImmediateInputState.menu[KEY_MENU_CANCEL] },
	{ "Menu-Special", (int *)&ImmediateInputState.menu[KEY_MENU_SPECIAL] },
	{ "Menu-Page-Up", (int *)&ImmediateInputState.menu[KEY_MENU_PAGE_UP] },
	{ "Menu-Page-Down", (int *)&ImmediateInputState.menu[KEY_MENU_PAGE_DOWN] },
	{ "Menu-Home", (int *)&ImmediateInputState.menu[KEY_MENU_HOME] },
	{ "Menu-End", (int *)&ImmediateInputState.menu[KEY_MENU_END] },
	{ "Menu-Zoom-In", (int *)&ImmediateInputState.menu[KEY_MENU_ZOOM_IN] },
	{ "Menu-Zoom-Out", (int *)&ImmediateInputState.menu[KEY_MENU_ZOOM_OUT] },
	{ "Menu-Delete", (int *)&ImmediateInputState.menu[KEY_MENU_DELETE] },
	{ "Menu-Backspace", (int *)&ImmediateInputState.menu[KEY_MENU_BACKSPACE] },
	{ "Menu-Edit-Cancel", (int *)&ImmediateInputState.menu[KEY_MENU_EDIT_CANCEL] },
	{ "Menu-Search", (int *)&ImmediateInputState.menu[KEY_MENU_SEARCH] },
	{ "Menu-Next", (int *)&ImmediateInputState.menu[KEY_MENU_NEXT] },
	{ "Template-1-Up", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_1][KEY_UP] },
	{ "Template-1-Down", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_1][KEY_DOWN] },
	{ "Template-1-Left", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_1][KEY_LEFT] },
	{ "Template-1-Right", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_1][KEY_RIGHT] },
	{ "Template-1-Weapon", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_1][KEY_WEAPON] },
	{ "Template-1-Special", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_1][KEY_SPECIAL] },
	{ "Template-1-Escape", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_1][KEY_ESCAPE] },
	{ "Template-2-Up", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_2][KEY_UP] },
	{ "Template-2-Down", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_2][KEY_DOWN] },
	{ "Template-2-Left", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_2][KEY_LEFT] },
	{ "Template-2-Right", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_2][KEY_RIGHT] },
	{ "Template-2-Weapon", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_2][KEY_WEAPON] },
	{ "Template-2-Special", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_2][KEY_SPECIAL] },
	{ "Template-2-Escape", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_2][KEY_ESCAPE] },
	{ "Template-3-Up", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_3][KEY_UP] },
	{ "Template-3-Down", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_3][KEY_DOWN] },
	{ "Template-3-Left", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_3][KEY_LEFT] },
	{ "Template-3-Right", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_3][KEY_RIGHT] },
	{ "Template-3-Weapon", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_3][KEY_WEAPON] },
	{ "Template-3-Special", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_3][KEY_SPECIAL] },
	{ "Template-3-Escape", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_KB_3][KEY_ESCAPE] },
	{ "Template-4-Up", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_1][KEY_UP] },
	{ "Template-4-Down", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_1][KEY_DOWN] },
	{ "Template-4-Left", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_1][KEY_LEFT] },
	{ "Template-4-Right", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_1][KEY_RIGHT] },
	{ "Template-4-Weapon", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_1][KEY_WEAPON] },
	{ "Template-4-Special", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_1][KEY_SPECIAL] },
	{ "Template-4-Escape", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_1][KEY_ESCAPE] },
	{ "Template-5-Up", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_2][KEY_UP] },
	{ "Template-5-Down", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_2][KEY_DOWN] },
	{ "Template-5-Left", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_2][KEY_LEFT] },
	{ "Template-5-Right", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_2][KEY_RIGHT] },
	{ "Template-5-Weapon", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_2][KEY_WEAPON] },
	{ "Template-5-Special", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_2][KEY_SPECIAL] },
	{ "Template-5-Escape", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_2][KEY_ESCAPE] },
	{ "Template-6-Up", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_3][KEY_UP] },
	{ "Template-6-Down", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_3][KEY_DOWN] },
	{ "Template-6-Left", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_3][KEY_LEFT] },
	{ "Template-6-Right", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_3][KEY_RIGHT] },
	{ "Template-6-Weapon", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_3][KEY_WEAPON] },
	{ "Template-6-Special", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_3][KEY_SPECIAL] },
	{ "Template-6-Escape", (int *)&ImmediateInputState.key[CONTROL_TEMPLATE_JOY_3][KEY_ESCAPE] },
	{ "Pause", (int *)&ImmediateInputState.menu[KEY_PAUSE] },
	{ "Exit", (int *)&ImmediateInputState.menu[KEY_EXIT] },
	{ "Abort", (int *)&ImmediateInputState.menu[KEY_ABORT] },
	{ "Debug", (int *)&ImmediateInputState.menu[KEY_DEBUG] },
	{ "Illegal", NULL}};


static void
initKeyConfig (void)
{
	uio_Stream *fp;
	int i, errors;
	
	for (i = 0; i < 2; ++i)
	{
		if ((fp = res_OpenResFile (configDir, "keys.cfg", "rt")) == NULL)
		{
			if (copyFile (contentDir, "starcon.key",
				configDir, "keys.cfg") == -1)
			{
				log_add (log_Fatal, "Error: Could not copy default key config "
					"to user config dir: %s.", strerror (errno));
				exit (EXIT_FAILURE);
			}
			log_add (log_Info, "Copying default key config file to user "
				"config dir.");
			
			if ((fp = res_OpenResFile (configDir, "keys.cfg", "rt")) == NULL)
			{
				log_add (log_Fatal, "Error: Could not open keys.cfg");
				exit (EXIT_FAILURE);
			}
		}
		
		errors = VControl_ReadConfiguration (fp);
		res_CloseResFile (fp);
		
		if (errors || (VControl_GetConfigFileVersion () != VCONTROL_VERSION))
		{
			bool do_rename = false;
			if (errors)
				log_add (log_Warning, "%d errors encountered in key configuration file.", errors);

			if (VControl_GetValidCount () == 0)
			{
				log_add (log_Error, "\nI didn't understand a single line in your configuration file.");
				log_add (log_Error, "This is likely because you're still using a 0.2 era or earlier keys.cfg.");
				do_rename = true;
			}
			if (VControl_GetConfigFileVersion () != VCONTROL_VERSION)
			{
				log_add (log_Error, "\nThe control scheme for UQM has changed since you last updated keys.cfg.");
				log_add (log_Error, "(I'm using control scheme version %d, while your config file appears to be\nfor version %d.)", VCONTROL_VERSION, VControl_GetConfigFileVersion ());
				do_rename = true;
			}

			if (do_rename)
			{
				log_add (log_Error, "\nRenaming keys.cfg to keys.old and retrying.");

				if (fileExists2 (configDir, "keys.old"))
					uio_unlink (configDir, "keys.old");
				if ((uio_rename (configDir, "keys.cfg", configDir, "keys.old")) == -1)
				{
					log_add (log_Fatal, "Error: Renaming failed!");
					log_add (log_Fatal, "You must delete keys.cfg manually before you can run the game.");
					exit (EXIT_FAILURE);
				}
				continue;
			}
			
			log_add (log_Fatal, "\nRepair your keys.cfg file to continue.");
			exit (EXIT_FAILURE);
		}
		
		return;
	}

	log_add (log_Fatal, "Error: Something went wrong and we were looping again and again so aborting.");
	log_add (log_Fatal, "Possible cause is your content dir not being up-to-date.");
	exit (EXIT_FAILURE);
}

static void
resetKeyboardState (void)
{
	memset (kbdstate, 0, sizeof (int) * num_keys);
	ImmediateInputState.menu[KEY_MENU_ANY] = 0;
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
	(void)SDL_GetKeyState (&num_keys);
	kbdstate = (int *)HMalloc (sizeof (int) * (num_keys + 1));
	

#ifdef HAVE_JOYSTICK
	if ((SDL_InitSubSystem(SDL_INIT_JOYSTICK)) == -1)
	{
		log_add (log_Fatal, "Couldn't initialize joystick subsystem: %s",
				SDL_GetError());
		exit (EXIT_FAILURE);
	}

	log_add (log_Info, "%i joysticks were found.", SDL_NumJoysticks ());
	
	nJoysticks = SDL_NumJoysticks ();
	if (nJoysticks > 0)
	{
		log_add (log_Info, "The names of the joysticks are:");
		for (i = 0; i < nJoysticks; i++)
		{
			log_add (log_Info, "    %s", SDL_JoystickName (i));
		}
		SDL_JoystickEventState (SDL_ENABLE);
	}
#endif /* HAVE_JOYSTICK */

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
	HFree (kbdstate);
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

	if (Event->type == SDL_KEYDOWN || Event->type == SDL_KEYUP)
	{	// process character input event, if any
		SDLKey k = Event->key.keysym.sym;
		wchar_t map_key = Event->key.keysym.unicode;

		if (k < 0 || k > num_keys)
			k = num_keys; // for unknown keys

		if (Event->type == SDL_KEYDOWN)
		{
			int newtail;

			// dont care about the non-printable, non-char
			if (!map_key)
				return;

			kbdstate[k]++;
			
			newtail = (kbdtail + 1) & (KBDBUFSIZE - 1);
			// ignore the char if the buffer is full
			if (newtail != kbdhead)
			{
				kbdbuf[kbdtail] = map_key;
				kbdtail = newtail;
				lastchar = map_key;
				ImmediateInputState.menu[KEY_MENU_ANY]++;
			}
		}
		else if (Event->type == SDL_KEYUP)
		{
			if (kbdstate[k] == 0)
			{	// something is fishy -- better to reset the
				// repeatable state to avoid big problems
				ImmediateInputState.menu[KEY_MENU_ANY] = 0;
			}
			else
			{
				kbdstate[k]--;
				if (ImmediateInputState.menu[KEY_MENU_ANY] > 0)
					ImmediateInputState.menu[KEY_MENU_ANY]--;
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

void
InterrogateInputState (int template, int control, int index, char *buffer, int maxlen)
{
	int i;
	VCONTROL_GESTURE g;

	VControl_StartIter (&ImmediateInputState.key[template][control]);
	i = 0;
	while (VControl_NextBinding (&g))
	{
		if (i++ < index)
		{
			continue;
		}
		switch (g.type)
		{
		case VCONTROL_KEY:
			snprintf (buffer, maxlen, "%s", VControl_code2name (g.gesture.key));
			buffer[maxlen-1] = 0;
			break;
		case VCONTROL_JOYBUTTON:
			snprintf (buffer, maxlen, "[J%d B%d]", g.gesture.button.port, g.gesture.button.index + 1);
			buffer[maxlen-1] = 0;
			break;
		case VCONTROL_JOYAXIS:
			snprintf (buffer, maxlen, "[J%d A%d %c]", g.gesture.axis.port, g.gesture.axis.index, g.gesture.axis.polarity > 0 ? '+' : '-');
			break;
		case VCONTROL_JOYHAT:
			snprintf (buffer, maxlen, "[J%d H%d %d]", g.gesture.hat.port, g.gesture.hat.index, g.gesture.hat.dir);
			break;
		default:
			/* Something we don't handle yet */
			buffer[0] = 0;
			break;
		}
		return;
	}
	/* There aren't that many bindings, so return blank string */
	buffer[0] = 0;
}

void
RemoveInputState (int template, int control, int index)
{
	int i;
	VCONTROL_GESTURE g;

	VControl_StartIter (&ImmediateInputState.key[template][control]);
	i = 0;
	while (VControl_NextBinding (&g))
	{
		if (i++ < index)
		{
			continue;
		}
		VControl_RemoveGestureBinding (&g, &ImmediateInputState.key[template][control]);
		return;
	}
	/* No such gesture! */
	return;
}

void
RebindInputState (int template, int control, int index)
{
	SDL_Event e;

	/* Remove the old binding on this spot */
	RemoveInputState (template, control, index);

	/* Wait for the next interesting bit of user input */
	VControl_ClearEvent ();
	while (!VControl_GetLastEvent (&e))
	{
		TaskSwitch ();
	}

	/* And now, add the new binding. */
	VControl_AddBinding (&e, &ImmediateInputState.key[template][control]);
}

void
SaveKeyConfiguration (uio_DirHandle *path, const char *fname)
{
	uio_Stream *f;
	
	f = res_OpenResFile (path, fname, "wb");
	if (f) 
	{
		VControl_Dump (f);
		res_CloseResFile (f);
	}
}

#endif
