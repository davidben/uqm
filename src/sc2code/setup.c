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

static MEM_HANDLE
LoadKeyConfig (FILE *fp, DWORD length)
{
	COUNT i;
	BYTE buf[38], *pbuf;

	ReadResFile (buf, 1, sizeof (buf), fp);

	// new temporary melee keys 2002/11/22
	// mapped as left, right, thrust, ?, ?, fire, special, ?, ?
	KeyboardInput[0] = CaptureInputDevice 
	(
		CreateJoystickKeyboardDevice (SK_LF_ARROW, SK_RT_ARROW, SK_UP_ARROW, 0, 0, SK_RT_SHIFT, SK_CTL, 0, 0x1b)
	);
	KeyboardInput[1] = CaptureInputDevice 
	(
		CreateJoystickKeyboardDevice ('s', 'f', 'e', 0, 0, 'q', 'a', 0, 0x1b)
	);

	// old melee key code
	/*for (i = 0, pbuf = buf; i < NUM_PLAYERS; ++i, pbuf += 10)
	{
		KeyboardInput[i] = CaptureInputDevice (
				CreateJoystickKeyboardDevice (
						pbuf[2], pbuf[4], pbuf[6], 0,
						0, pbuf[8], pbuf[0],
						0, 0x1b
						)
				);

	}*/

	(void) length;  /* Satisfying compiler (unused parameter) */
	return (NULL_HANDLE);
}

static void
InitPlayerInput (void)
{
	INPUT_DEVICE InputDevice;
	extern BOOLEAN PauseGame (void);

	InitInput (SK_F1, SK_F10, PauseGame);

	JoystickInput[0] = CaptureInputDevice (CreateJoystickDevice (0));
	JoystickInput[1] = CaptureInputDevice (CreateJoystickDevice (1));

	SerialInput = CaptureInputDevice (CreateSerialKeyboardDevice ());
	ArrowInput = CaptureInputDevice (CreateJoystickKeyboardDevice (
			SK_LF_ARROW, SK_RT_ARROW, SK_UP_ARROW, SK_DN_ARROW,
			'\n', ' ', 0x1b, SK_KEYPAD_MINUS, SK_KEYPAD_PLUS
			));

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

