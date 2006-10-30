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

#include "setup.h"

#include "coderes.h"
#include "controls.h"
#include "options.h"
#include "nameref.h"
#ifdef NETPLAY
#	include "netplay/netmelee.h"
#endif
#include "init.h"
#include "intel.h"
#include "resinst.h"
#include "sounds.h"
#include "libs/compiler.h"
#include "libs/uio.h"
#include "libs/file.h"
#include "libs/graphics/gfx_common.h"
#include "libs/threadlib.h"
#include "libs/vidlib.h"
#include "libs/log.h"

#include <assert.h>
#include <errno.h>
#include <string.h>


ACTIVITY LastActivity;
BYTE PlayerControl[NUM_PLAYERS];

// XXX: These declarations should really go to the file they belong to.
MEM_HANDLE hResIndex;
CONTEXT ScreenContext;
CONTEXT SpaceContext;
CONTEXT StatusContext;
CONTEXT OffScreenContext;
CONTEXT TaskContext;
SIZE screen_width, screen_height;
FRAME Screen;
FONT StarConFont;
FONT MicroFont;
FONT TinyFont;
QUEUE race_q[NUM_PLAYERS];
FRAME ActivityFrame;
FRAME StatusFrame;
FRAME FlagStatFrame;
FRAME MiscDataFrame;
FRAME FontGradFrame;
Mutex GraphicsLock;
STRING GameStrings;

uio_Repository *repository;
uio_DirHandle *rootDir;


static void
InitPlayerInput (void)
{
	HumanInput[0] = p1_combat_summary;
	HumanInput[1] = p2_combat_summary;
	ComputerInput = computer_intelligence;
#ifdef NETPLAY
	NetworkInput = networkBattleInput;
#endif
}

void
UninitPlayerInput (void)
{
#if DEMO_MODE
	DestroyInputDevice (ReleaseInputDevice (DemoInput));
#endif /* DEMO_MODE */
}

BOOLEAN
LoadKernel (int argc, char *argv[])
{
#define MIN_K_REQUIRED (580000L / 1024)
	if (!InitGraphics (argc, argv, MIN_K_REQUIRED))
		return FALSE;
	InitSound (argc, argv);
	InitVideo (TRUE);

	ScreenContext = CaptureContext (CreateContext ());
	if (ScreenContext == NULL)
		return FALSE;

	Screen = CaptureDrawable (CreateDisplay (WANT_MASK | WANT_PIXMAP,
				&screen_width, &screen_height));
	if (Screen == NULL)
		return FALSE;

	SetContext (ScreenContext);
	SetContextFGFrame (Screen);
	SetFrameHot (Screen, MAKE_HOT_SPOT (0, 0));

	hResIndex = InitResourceSystem ("starcon.lst", RES_INDEX, NULL);
	if (hResIndex == 0)
		return FALSE;
	INIT_INSTANCES ();

	{
		COLORMAP ColorMapTab;

		ColorMapTab = CaptureColorMap (LoadColorMap (STARCON_COLOR_MAP));
		SetColorMap (GetColorMapAddress (ColorMapTab));
		DestroyColorMap (ReleaseColorMap (ColorMapTab));
	}

	InitPlayerInput ();

	GLOBAL (CurrentActivity) = (ACTIVITY)~0;
	return TRUE;
}

BOOLEAN
InitContexts (void)
{
	RECT r;
	
	StatusContext = CaptureContext (CreateContext ());
	if (StatusContext == NULL)
		return FALSE;

	SetContext (StatusContext);
	SetContextFGFrame (Screen);
	r.corner.x = SPACE_WIDTH + SAFE_X;
	r.corner.y = SAFE_Y;
	r.extent.width = STATUS_WIDTH;
	r.extent.height = STATUS_HEIGHT;
	SetContextClipRect (&r);
	
	SpaceContext = CaptureContext (CreateContext ());
	if (SpaceContext == NULL)
		return FALSE;
		
	OffScreenContext = CaptureContext (CreateContext ());
	if (OffScreenContext == NULL)
		return FALSE;

	if (!InitQueue (&disp_q, 100, sizeof (ELEMENT)))
		return FALSE;

	return TRUE;
}

BOOLEAN
InitKernel (void)
{
	COUNT counter;

	for (counter = 0; counter < NUM_PLAYERS; ++counter)
		InitQueue (&race_q[counter], MAX_SHIPS_PER_SIDE, sizeof (STARSHIP));

	StarConFont = CaptureFont (LoadFont (STARCON_FONT));
	if (StarConFont == NULL)
		return FALSE;

	TinyFont = CaptureFont (LoadFont (TINY_FONT));
	if (TinyFont == NULL)
		return FALSE;

	ActivityFrame = CaptureDrawable (LoadGraphic (ACTIVITY_ANIM));
	if (ActivityFrame == NULL)
		return FALSE;

	StatusFrame = CaptureDrawable (LoadGraphic (STATUS_MASK_PMAP_ANIM));
	if (StatusFrame == NULL)
		return FALSE;

	GameStrings = CaptureStringTable (LoadStringTable (STARCON_GAME_STRINGS));
	if (GameStrings == 0)
		return FALSE;

	MicroFont = CaptureFont (LoadFont (MICRO_FONT));
	if (MicroFont == NULL)
		return FALSE;

	MenuSounds = CaptureSound (LoadSound (MENU_SOUNDS));
	if (MenuSounds == 0)
		return FALSE;

	InitSpace ();

	return TRUE;
}

BOOLEAN
InitGameKernel (void)
{
	if (ActivityFrame == 0)
	{
		InitKernel ();
		InitContexts ();
	}
	return TRUE;
}

void
SetPlayerInput (void)
{
	COUNT which_player;

	for (which_player = 0; which_player < NUM_PLAYERS; ++which_player)
	{
		if (PlayerControl[which_player] & COMPUTER_CONTROL)
			PlayerInput[which_player] = ComputerInput;
		else if (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE)
		{
			// Full game.
			if (which_player == 0)
				PlayerInput[which_player] = HumanInput[0];
			else
			{
				PlayerInput[which_player] = ComputerInput;
				PlayerControl[which_player] = COMPUTER_CONTROL | AWESOME_RATING;
			}
		}
#ifdef NETPLAY
		else if (PlayerControl[which_player] & NETWORK_CONTROL)
			PlayerInput[which_player] = NetworkInput;
#endif
		else
			PlayerInput[which_player] = HumanInput[which_player];
	}
}

int
initIO (void)
{
	uio_init ();
	repository = uio_openRepository (0);

	rootDir = uio_openDir (repository, "/", 0);
	if (rootDir == NULL)
	{
		log_add (log_Fatal, "Could not open '/' dir.");
		return -1;
	}
	return 0;
}

void
uninitIO (void)
{
	uio_closeDir (rootDir);
	uio_closeRepository (repository);
	uio_unInit ();
}

