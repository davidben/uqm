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
#include "coderes.h"
#include "libs/threadlib.h"
#include "libs/graphics/gfx_common.h"

//Added by Chris

BOOLEAN InitSpace (void);

extern BOOLEAN InitVideo (BOOLEAN UseCDROM);

//End Added by Chris

ACTIVITY LastActivity;
BYTE PlayerControl[NUM_PLAYERS];

MEM_HANDLE hResIndex;
CONTEXT ScreenContext, SpaceContext, StatusContext, OffScreenContext,
						TaskContext;
SIZE screen_width, screen_height;
FRAME Screen;
FONT StarConFont, MicroFont, TinyFont;
INPUT_REF ArrowInput, ComputerInput, NormalInput, SerialInput,
						JoystickInput[NUM_PLAYERS], KeyboardInput[NUM_PLAYERS],
						CombinedInput[NUM_PLAYERS], PlayerInput[NUM_PLAYERS];
QUEUE race_q[NUM_PLAYERS];
SOUND MenuSounds, GameSounds;
FRAME ActivityFrame, status, flagship_status, misc_data;
Semaphore GraphicsSem;
CondVar RenderingCond;
STRING GameStrings;

static UWORD ParseKeyString(char* line)
{
	char key[16];
	char* index = strchr(line+1, '"');
	if (index == NULL)
		return 0;
	memset(key, 0, sizeof(key));
	strncpy(key, line+1, index - (line + 1));
	if (strchr(key, ',') != NULL && strlen(key) != 1)
	{
		// We have a chorded key!
		UWORD k1, k2;
		char l[16];
		char* comma = line;
		memset(l, 0, sizeof(l));
		while (strchr(comma + 1, ',') != NULL)
			comma = strchr(comma + 1, ',');
		strncpy(l, line, comma - line);
		strcat(l, "\"");
		k1 = ParseKeyString(l);
		memset(l, 0, sizeof(l));
		l[0] = '"';
		strncpy(l + 1, comma + 1, index - (comma + 1));
		strcat(l, "\"");
		k2 = ParseKeyString(l);
		return KEYCHORD(k1, k2);
	}
		
	if (!strcmp(key, "Left"))
		return SK_LF_ARROW;
	else if (!strcmp(key, "Right"))
		return SK_RT_ARROW;
	else if (!strcmp(key, "Up"))
		return SK_UP_ARROW;
	else if (!strcmp(key, "Down"))
		return SK_DN_ARROW;
	else if (!strcmp(key, "RCtrl"))
		return SK_RT_CTL;
	else if (!strcmp(key, "LCtrl"))
		return SK_LF_CTL;
	else if (!strcmp(key, "RAlt"))
		return SK_RT_ALT;
	else if (!strcmp(key, "LAlt"))
		return SK_LF_ALT;
	else if (!strcmp(key, "LShift"))
		return SK_LF_SHIFT;
	else if (!strcmp(key, "RShift"))
		return SK_RT_SHIFT;
	else if (!strcmp(key, "Enter"))
		return '\n';
	else if (!strcmp(key, "F1"))
		return SK_F1;
	else if (!strcmp(key, "F2"))
		return SK_F2;
	else if (!strcmp(key, "F3"))
		return SK_F3;
	else if (!strcmp(key, "F4"))
		return SK_F4;
	else if (!strcmp(key, "F5"))
		return SK_F5;
	else if (!strcmp(key, "F6"))
		return SK_F6;
	else if (!strcmp(key, "F7"))
		return SK_F7;
	else if (!strcmp(key, "F8"))
		return SK_F8;
	else if (!strcmp(key, "F9"))
		return SK_F9;
	else if (!strcmp(key, "F10"))
		return SK_F10;
	else if (!strcmp(key, "F11"))
		return SK_F11;
	else if (!strcmp(key, "F12"))
		return SK_F12;
	else if (!strcmp(key, "KP+"))
		return SK_KEYPAD_PLUS;
	else if (!strcmp(key, "KP-"))
		return SK_KEYPAD_MINUS;
	else
		return key[0];
}

static MEM_HANDLE
LoadKeyConfig (FILE *fp, DWORD length)
{
	UWORD buf[18];
	int p;
	int i;
	char line[256];
	extern BOOLEAN PauseGame (void);

	UNICODE specialKeys[3];

	// Look for pause, exit, abort keys
	for (i = 0; i < 3; ++i)
	{
		for (;;)
		{
			if (feof(fp) || ferror(fp))
				break;
			fgets(line, 256, fp);
			if (line[0] != '#')
				break;
		}
		if (line[0] == '"')
		{
			specialKeys[i] = (UNICODE)(ParseKeyString(line) & 0xFF);
			fprintf(stderr, "Special %i = %i\n", i, specialKeys[i]);
		}
	}

	// For each of two players
	for (p = 0; p < 2; ++p)
	{
		int joyN = -1;
		int joyThresh = 0;
		memset(buf, 0xFF, sizeof(buf));
		// Read in 9 inputs
		for (i = 0; i < 9; ++i)
		{
			// Look for valid lines
			for (;;)
			{
				if (feof(fp) || ferror(fp))
					break;
				fgets(line, 256, fp);

				if (line[0] != '#')
					break;
			}
			// line now contains what should be a valid line
			if (strstr(line, "<Joyport") != NULL)
			{
				joyN = atoi(line + 8);
				joyThresh = atoi(strchr(line, '-') + 10);
				--i;
				continue;
			}
			else if (line[0] == '"')
			{
				char joy[16];
				char* index;
				memset(joy, 0, sizeof(joy));
				index = strchr(line+1, '"');
				if (index != NULL)
					index = strchr(index+1, '<');
				if (index != NULL)
				{
					char* index2 = strchr(index+1, '>');
					if (index2 != NULL)
						strncpy(joy, index+1, index2 - (index + 1));
				}
				else
				{
					joy[0] = 0;
				}
				buf[i] = ParseKeyString (line);
				fprintf(stderr, "Player %i, key %i = %i\n", p, i, buf[i]);
				if (joy[0] != 0)
				{
					char type;
					if (strchr(joy, '-') != NULL)
						type = *(strchr(joy, '-') + 1);
					else
						type = 0;

					if (type == 'A')
					{
						int axis = atoi(strchr(joy, '-') + 2);
						char sign = joy[strlen(joy)-1];
						int s;
						if (sign == '+')
							s = 1;
						else if (sign == '-')
							s = -1;
						else
							s = 1;
						buf[9 + i] = JOYAXIS(axis, s);
					}
					else if (type == 'B')
					{
						int button = atoi(strchr(joy, '-') + 2);
						buf[9 + i] = JOYBUTTON(button);
					}
					else if (type == 'C')
					{
						int b1 = atoi(strchr(joy, '-') + 2);
						int b2 = atoi(strchr(joy, ',') + 1);
						buf[9 + i] = JOYCHORD(b1, b2);
					}
				}
			}
		}
		KeyboardInput[p] = CaptureInputDevice (
			CreateJoystickKeyboardDevice(buf[0], buf[1], buf[2], buf[3], buf[4],
				buf[5], buf[6], buf[7], buf[8])
			);
		if (joyN != -1)
		{
			JoystickInput[p] = CaptureInputDevice (
				CreateJoystickDevice(joyN, joyThresh, buf[9], buf[10], buf[11], 
					buf[12], buf[13], buf[14], buf[15], buf[16], buf[17])
				);
		}
		else
		{
			JoystickInput[p] = 0;
		}
	}
	ArrowInput = KeyboardInput[0];
	(void) length;  /* Satisfying compiler (unused parameter) */
	InitInput(PauseGame, specialKeys[0], specialKeys[1], specialKeys[2]);
	return (NULL_HANDLE);
}

static void
InitPlayerInput (void)
{
	INPUT_DEVICE InputDevice;

	SerialInput = CaptureInputDevice (CreateSerialKeyboardDevice ());

	InputDevice = CreateInternalDevice (game_input);
	NormalInput = CaptureInputDevice (InputDevice);
	InputDevice = CreateInternalDevice (computer_intelligence);
	ComputerInput = CaptureInputDevice (InputDevice);
	InputDevice = CreateInternalDevice (combined_input0);
	CombinedInput[0] = CaptureInputDevice (InputDevice);
	InputDevice = CreateInternalDevice (combined_input1);
	CombinedInput[1] = CaptureInputDevice (InputDevice);

#if DEMO_MODE
	InputDevice = CreateInternalDevice (demo_input);
	DemoInput = CaptureInputDevice (InputDevice);
#endif /* DEMO_MODE */
}

void
UninitPlayerInput (void)
{
	COUNT which_player;

#if DEMO_MODE
	DestroyInputDevice (ReleaseInputDevice (DemoInput));
#endif /* DEMO_MODE */
	DestroyInputDevice (ReleaseInputDevice (ComputerInput));
	DestroyInputDevice (ReleaseInputDevice (NormalInput));

	DestroyInputDevice (ReleaseInputDevice (ArrowInput));
	DestroyInputDevice (ReleaseInputDevice (SerialInput));
	for (which_player = 0; which_player < NUM_PLAYERS; ++which_player)
	{
		DestroyInputDevice (ReleaseInputDevice (JoystickInput[which_player]));
		DestroyInputDevice (ReleaseInputDevice (KeyboardInput[which_player]));
		DestroyInputDevice (ReleaseInputDevice (CombinedInput[which_player]));
	}

	UninitInput ();
}

BOOLEAN
LoadKernel (int argc, char *argv[])
{
#define MIN_K_REQUIRED (580000L / 1024)
	if (!InitGraphics (argc, argv, MIN_K_REQUIRED))
		return (FALSE);
	InitSound (argc, argv);
	InitVideo (TRUE);
	if ((ScreenContext = CaptureContext (CreateContext ())) == 0)
		return(FALSE);

	if ((Screen = CaptureDrawable (
			CreateDisplay (WANT_MASK | WANT_PIXMAP, &screen_width, &screen_height)
			)) == 0)
		return(FALSE);

	SetContext (ScreenContext);
	SetContextFGFrame (Screen);
	SetFrameHot (Screen, MAKE_HOT_SPOT (0, 0));

	if ((hResIndex = InitResourceSystem ("starcon", RES_INDEX, NULL)) == 0)
		return (FALSE);
	INIT_INSTANCES ();

	InstallResTypeVectors (KEY_CONFIG, LoadKeyConfig, NULL_PTR);
	GetResource (JOYSTICK_KEYS);

	{
		COLORMAP ColorMapTab;

		ColorMapTab = CaptureColorMap (LoadColorMap (STARCON_COLOR_MAP));
		SetColorMap (GetColorMapAddress (ColorMapTab));
		DestroyColorMap (ReleaseColorMap (ColorMapTab));
	}

	InitPlayerInput ();

	GLOBAL (CurrentActivity) = (ACTIVITY)~0;
	return (TRUE);
}

BOOLEAN
InitContexts (void)
{
	RECT r;
	if ((StatusContext = CaptureContext (
			CreateContext ()
			)) == 0)
		return (FALSE);

	SetContext (StatusContext);
	SetContextFGFrame (Screen);
	r.corner.x = SPACE_WIDTH + SAFE_X;
	r.corner.y = SAFE_Y;
	r.extent.width = STATUS_WIDTH;
	r.extent.height = STATUS_HEIGHT;
	SetContextClipRect (&r);
	
	if ((SpaceContext = CaptureContext (
			CreateContext ()
			)) == 0)
		return (FALSE);
		
	if ((OffScreenContext = CaptureContext (
			CreateContext ()
			)) == 0)
		return (FALSE);

	if (!InitQueue (&disp_q, 100, sizeof (ELEMENT)))
		return (FALSE);

	return (TRUE);
}

BOOLEAN
InitKernel (void)
{
	COUNT counter;

	for (counter = 0; counter < NUM_PLAYERS; ++counter)
	{
		InitQueue (&race_q[counter], MAX_SHIPS_PER_SIDE, sizeof (STARSHIP));
	}

	if ((StarConFont = CaptureFont (
			LoadGraphic (STARCON_FONT)
			)) == 0)
		return (FALSE);

	if ((TinyFont = CaptureFont (
			LoadGraphic (TINY_FONT)
			)) == 0)
		return (FALSE);
	if ((ActivityFrame = CaptureDrawable (
			LoadGraphic (ACTIVITY_ANIM)
			)) == 0)
		return (FALSE);

	if ((status = CaptureDrawable (
			LoadGraphic (STATUS_MASK_PMAP_ANIM)
			)) == 0)
		return (FALSE);

	if ((GameStrings = CaptureStringTable (
			LoadStringTableInstance (STARCON_GAME_STRINGS)
			)) == 0)
		return (FALSE);

	if ((MicroFont = CaptureFont (
			LoadGraphic (MICRO_FONT)
			)) == 0)
		return (FALSE);

	if ((MenuSounds = CaptureSound (
			LoadSound (MENU_SOUNDS)
			)) == 0)
		return (FALSE);

	InitSpace ();

	return (TRUE);
}

BOOLEAN
InitGameKernel (void)
{
	if (ActivityFrame == 0)
	{
		InitKernel ();
		InitContexts ();
	}
	return(TRUE);
}

void
SetPlayerInput (void)
{
	COUNT which_player;

	for (which_player = 0; which_player < NUM_PLAYERS; ++which_player)
	{
		if (!(PlayerControl[which_player] & HUMAN_CONTROL))
			PlayerInput[which_player] = ComputerInput;
		else if (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE)
		{
			if (which_player == 0)
				PlayerInput[which_player] = NormalInput;
			else
			{
				PlayerInput[which_player] = ComputerInput;
				PlayerControl[which_player] = COMPUTER_CONTROL | AWESOME_RATING;
			}
		}
		else
			PlayerInput[which_player] = CombinedInput[which_player];
	}
}

