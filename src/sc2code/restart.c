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

#include "restart.h"

#include "colors.h"
#include "controls.h"
#include "credits.h"
#include "encount.h"
#include "fmv.h"
#include "globdata.h"
#include "intel.h"
#include "melee.h"
#include "resinst.h"
#include "nameref.h"
#include "settings.h"
#include "load.h"
#include "setup.h"
#include "sounds.h"
#include "setupmenu.h"
#include "util.h"
#include "starcon.h"
#include "uqmversion.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"


enum
{
	START_NEW_GAME = 0,
	LOAD_SAVED_GAME,
	PLAY_SUPER_MELEE,
	SETUP_GAME,
	QUIT_GAME
};

static void
DrawRestartMenuGraphic (PMENU_STATE pMS)
{
	RECT r;
	STAMP s;

	s.frame = CaptureDrawable (
		LoadGraphic (RESTART_PMAP_ANIM)
		);
	pMS->CurFrame = s.frame;
	GetFrameRect (s.frame, &r);
	s.origin.x = (SCREEN_WIDTH - r.extent.width) >> 1;
	s.origin.y = (SCREEN_HEIGHT - r.extent.height) >> 1;
	
	SetContextBackGroundColor (BLACK_COLOR);
	BatchGraphics ();
	ClearDrawable ();
	FlushColorXForms ();
	LockMutex (GraphicsLock);
	DrawStamp (&s);
	UnlockMutex (GraphicsLock);
	UnbatchGraphics ();
}

static void
DrawRestartMenu (BYTE OldState, BYTE NewState, FRAME f)
{
	RECT r;
	TEXT t;
	UNICODE buf[64];

	LockMutex (GraphicsLock);
	SetContext (ScreenContext);
	r.corner.x = r.corner.y = r.extent.width = r.extent.height = 0;
	SetContextClipRect (&r);
	r.corner.x = 0;
	r.corner.y = 0;
	r.extent.width = SCREEN_WIDTH;
	r.extent.height = SCREEN_HEIGHT;
	SetFlashRect (&r, SetAbsFrameIndex (f, NewState + 1));

	// Put version number in the corner
	SetContextFont (TinyFont);
	t.pStr = buf;
	t.baseline.x = SCREEN_WIDTH - 3;
	t.baseline.y = SCREEN_HEIGHT - 2;
	t.align = ALIGN_RIGHT;
	t.CharCount = (COUNT)~0;
	sprintf (buf, "v%d.%d.%d%s", UQM_MAJOR_VERSION, UQM_MINOR_VERSION,
			UQM_PATCH_VERSION, UQM_EXTRA_VERSION);
	SetContextForeGroundColor (WHITE_COLOR);
	font_DrawText (&t);

	UnlockMutex (GraphicsLock);
	(void) OldState;  /* Satisfying compiler (unused parameter) */
}


static BOOLEAN
DoRestart (PMENU_STATE pMS)
{
	static DWORD InTime;
	static DWORD InactTimeOut;

	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;

	if (!pMS->Initialized)
	{
		if (pMS->hMusic)
		{
			StopMusic ();
			DestroyMusic (pMS->hMusic);
			pMS->hMusic = 0;
		}
		pMS->hMusic = LoadMusic (MAINMENU_MUSIC);
		InactTimeOut = (pMS->hMusic ? 120 : 20) * ONE_SECOND;
		
		PlayMusic (pMS->hMusic, TRUE, 1);
		DrawRestartMenu ((BYTE)~0, pMS->CurState, pMS->CurFrame);
		pMS->Initialized = TRUE;

		{
			BYTE clut_buf[] = {FadeAllToColor};
			DWORD TimeOut = XFormColorMap ((COLORMAPPTR)clut_buf, ONE_SECOND / 2);
			while ((GetTimeCounter () <= TimeOut) &&
			       !(GLOBAL (CurrentActivity) & CHECK_ABORT))
			{
				UpdateInputState ();
				TaskSwitch ();
			}
		}
	}
#ifdef TESTING
else if (InputState & DEVICE_EXIT) return (FALSE);
#endif /* TESTING */
	else if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		return (FALSE);
	}
	else if (!(PulsedInputState.menu[KEY_MENU_UP] || PulsedInputState.menu[KEY_MENU_DOWN] ||
			PulsedInputState.menu[KEY_MENU_LEFT] || PulsedInputState.menu[KEY_MENU_RIGHT] ||
			PulsedInputState.menu[KEY_MENU_SELECT] || MouseButtonDown))

	{
		if (GetTimeCounter () - InTime < InactTimeOut)
			return (TRUE);

		SleepThreadUntil (FadeMusic (0, ONE_SECOND));
		StopMusic ();
		FadeMusic (NORMAL_VOLUME, 0);

		GLOBAL (CurrentActivity) = (ACTIVITY)~0;
		return (FALSE);
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		BYTE fade_buf[1];

		switch (pMS->CurState)
		{
			case LOAD_SAVED_GAME:
				LastActivity = CHECK_LOAD;
				GLOBAL (CurrentActivity) = IN_INTERPLANETARY;
				break;
			case START_NEW_GAME:
				LastActivity = CHECK_LOAD | CHECK_RESTART;
				GLOBAL (CurrentActivity) = IN_INTERPLANETARY;
				break;
			case PLAY_SUPER_MELEE:
				GLOBAL (CurrentActivity) = SUPER_MELEE;
				break;
			case SETUP_GAME:
				LockMutex (GraphicsLock);
				SetFlashRect (NULL_PTR, (FRAME)0);
				UnlockMutex (GraphicsLock);
				SetupMenu ();
				SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);
				InTime = GetTimeCounter ();
				SetTransitionSource (NULL);
				BatchGraphics ();
				DrawRestartMenuGraphic (pMS);
				DrawRestartMenu ((BYTE)~0, pMS->CurState, pMS->CurFrame);
				ScreenTransition (3, NULL);
				UnbatchGraphics ();
				return TRUE;
			case QUIT_GAME:
				fade_buf[0] = FadeAllToBlack;
				SleepThreadUntil (XFormColorMap ((COLORMAPPTR)fade_buf, ONE_SECOND / 2));

				GLOBAL (CurrentActivity) = CHECK_ABORT;
				break;
		}

		LockMutex (GraphicsLock);
		SetFlashRect (NULL_PTR, (FRAME)0);
		UnlockMutex (GraphicsLock);

		return (FALSE);
	}
	else
	{
		BYTE NewState;

		NewState = pMS->CurState;
		if (PulsedInputState.menu[KEY_MENU_UP])
		{
			if (NewState-- == START_NEW_GAME)
				NewState = QUIT_GAME;
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			if (NewState++ == QUIT_GAME)
				NewState = START_NEW_GAME;
		}
		if (NewState != pMS->CurState)
		{
			BatchGraphics ();
			DrawRestartMenu (pMS->CurState, NewState, pMS->CurFrame);
			UnbatchGraphics ();
			pMS->CurState = NewState;
		}
	}

	if (MouseButtonDown)
	{
		LockMutex (GraphicsLock);
		SetFlashRect (NULL_PTR, (FRAME)0);
		UnlockMutex (GraphicsLock);
		MouseError ();
		SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);	
		SetTransitionSource (NULL);
		BatchGraphics ();
		DrawRestartMenuGraphic (pMS);
		DrawRestartMenu ((BYTE)~0, pMS->CurState, pMS->CurFrame);
		ScreenTransition (3, NULL);
		UnbatchGraphics ();
	}

	InTime = GetTimeCounter ();
	return (TRUE);
}

static BOOLEAN
RestartMenu (PMENU_STATE pMS)
{
	DWORD TimeOut;
	BYTE black_buf[1];

	ReinitQueue (&race_q[0]);
	ReinitQueue (&race_q[1]);

	black_buf[0] = FadeAllToBlack;

	SetContext (ScreenContext);

	GLOBAL (CurrentActivity) |= CHECK_ABORT;
	if (GLOBAL_SIS (CrewEnlisted) == (COUNT)~0
			&& GET_GAME_STATE (UTWIG_BOMB_ON_SHIP)
			&& !GET_GAME_STATE (UTWIG_BOMB))
	{	// player blew himself up with Utwig bomb
		BYTE white_buf[] = {FadeAllToWhite};

		SET_GAME_STATE (UTWIG_BOMB_ON_SHIP, 0);

		SleepThreadUntil (XFormColorMap ((COLORMAPPTR)white_buf,
				ONE_SECOND / 8) + ONE_SECOND / 60);
		SetContextBackGroundColor (BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));
		ClearDrawable ();
		FlushColorXForms ();

		TimeOut = ONE_SECOND / 8;
	}
	else
	{
		TimeOut = ONE_SECOND / 2;

		if (LOBYTE (LastActivity) == WON_LAST_BATTLE)
		{
			GLOBAL (CurrentActivity) = WON_LAST_BATTLE;
			Victory ();
			Credits (TRUE);

			FreeGameData ();
			
			TimeOut = ONE_SECOND / 2;
			GLOBAL (CurrentActivity) = CHECK_ABORT;
		}
	}

	LastActivity = NextActivity = 0;

	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)black_buf, TimeOut));
	if (TimeOut == ONE_SECOND / 8)
		SleepThread (ONE_SECOND * 3);
	DrawRestartMenuGraphic (pMS);
	FlushInput ();
	GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
	SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);
	DoInput (pMS, TRUE);
	
	StopMusic ();
	if (pMS->hMusic)
	{
		DestroyMusic (pMS->hMusic);
		pMS->hMusic = 0;
	}

	LockMutex (GraphicsLock);
	SetFlashRect (NULL_PTR, (FRAME)0);
	UnlockMutex (GraphicsLock);
	DestroyDrawable (ReleaseDrawable (pMS->CurFrame));

	if (GLOBAL (CurrentActivity) == (ACTIVITY)~0)
		return (FALSE); // timed out

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE); // quit

	TimeOut = XFormColorMap ((COLORMAPPTR)black_buf, ONE_SECOND / 2);
	
	SleepThreadUntil (TimeOut);
	FlushColorXForms ();

	SeedRandomNumbers ();

	return (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE);
}

static BOOLEAN
TryStartGame (void)
{
	MENU_STATE MenuState;

	LastActivity = GLOBAL (CurrentActivity);
	GLOBAL (CurrentActivity) = 0;

	memset (&MenuState, 0, sizeof (MenuState));
	MenuState.InputFunc = DoRestart;

	while (!RestartMenu (&MenuState))
	{	// spin until a game is started or loaded
		if (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE &&
				!(GLOBAL (CurrentActivity) & CHECK_ABORT))
		{
			FreeGameData ();
			Melee ();
			MenuState.Initialized = FALSE;
		}
		else if (GLOBAL (CurrentActivity) == (ACTIVITY)~0)
		{	// timed out
			BYTE black_buf[] = {FadeAllToBlack};
			SleepThreadUntil (XFormColorMap ((COLORMAPPTR)black_buf,
					ONE_SECOND / 2));
			return (FALSE);
		}
		else if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		{	// quit
			return (FALSE);
		}
	}

	return TRUE;
}

BOOLEAN
StartGame (void)
{
	do
	{
		while (!TryStartGame ())
		{
			if (GLOBAL (CurrentActivity) == (ACTIVITY)~0)
			{	// timed out
				GLOBAL (CurrentActivity) = 0;
				SplashScreen (0);
				Credits (FALSE);
			}

			if (GLOBAL (CurrentActivity) & CHECK_ABORT)
				return (FALSE); // quit
		}

		if (LastActivity & CHECK_RESTART)
		{	// starting a new game
			Introduction ();
		}
	
	} while (GLOBAL (CurrentActivity) & CHECK_ABORT);

	{
		extern STAR_DESC starmap_array[];
		extern const BYTE element_array[];
		extern const PlanetFrame planet_array[];

		star_array = starmap_array;
		Elements = element_array;
		PlanData = planet_array;
	}

	PlayerControl[0] = HUMAN_CONTROL | STANDARD_RATING;
	PlayerControl[1] =  COMPUTER_CONTROL | AWESOME_RATING;
	SetPlayerInput ();

	return (TRUE);
}
