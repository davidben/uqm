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

#include <assert.h>
#include <errno.h>
#include <string.h>
#include "starcon.h"
#include "coderes.h"
#include "libs/threadlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/file.h"
#include "options.h"
#include "controls.h"

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
QUEUE race_q[NUM_PLAYERS];
SOUND MenuSounds, GameSounds;
FRAME ActivityFrame, status, flagship_status, misc_data;
CrossThreadMutex GraphicsLock;
CondVar RenderingCond;
STRING GameStrings;

static void
InitPlayerInput (void)
{
	HumanInput[0] = p1_combat_summary;
	HumanInput[1] = p2_combat_summary;
	ComputerInput  = computer_intelligence;
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
				PlayerInput[which_player] = HumanInput[0];
			else
			{
				PlayerInput[which_player] = ComputerInput;
				PlayerControl[which_player] = COMPUTER_CONTROL | AWESOME_RATING;
			}
		}
		else
			PlayerInput[which_player] = HumanInput[which_player];
	}
}

