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
#include "uqmversion.h"
#include "controls.h"

//Added by Chris

void Melee (void);

void Victory (void);

void Credits (void);

void FreeHyperData (void);

void FreeIPData (void);

void FreeSC2Data (void);

void FreeLanderData (void);

//End Added by Chris

enum
{
	START_NEW_GAME = 0,
	LOAD_SAVED_GAME,
	PLAY_SUPER_MELEE,
	SETUP_GAME,
	QUIT_GAME
};

static void
DrawRestartMenu (BYTE OldState, BYTE NewState, FRAME f)
{
	RECT r;
	TEXT t;
	UNICODE buf[64];

	SetSemaphore (GraphicsSem);
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
	t.valign = VALIGN_BOTTOM;
	t.CharCount = (COUNT)~0;
	wsprintf (buf, "v%d.%d%s", UQM_MAJOR_VERSION,
			UQM_MINOR_VERSION, UQM_EXTRA_VERSION);
	SetContextForeGroundColor (WHITE_COLOR);
	font_DrawText (&t);

	ClearSemaphore (GraphicsSem);
	(void) OldState;  /* Satisfying compiler (unused parameter) */
}

DWORD InTime;

static BOOLEAN
DoRestart (PMENU_STATE pMS)
{
	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;

	if (!pMS->Initialized)
	{
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
	else if (!(CurrentMenuState.up || CurrentMenuState.down ||
			CurrentMenuState.left || CurrentMenuState.right ||
			CurrentMenuState.select))

	{
		if (GetTimeCounter () - InTime < ONE_SECOND * 15)
			return (TRUE);

		GLOBAL (CurrentActivity) = (ACTIVITY)~0;
		return (FALSE);
	}
	else if (CurrentMenuState.select)
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
				PlaySoundEffect (SetAbsSoundIndex (MenuSounds, 2),
						0, NotPositional (), NULL, GAME_SOUND_PRIORITY);	
				InTime = GetTimeCounter ();
				return TRUE;
				break;
			case QUIT_GAME:
				PlaySoundEffect (SetAbsSoundIndex (MenuSounds, 1),
						0, NotPositional (), NULL, GAME_SOUND_PRIORITY);
				fade_buf[0] = FadeAllToBlack;
				SleepThreadUntil (XFormColorMap ((COLORMAPPTR)fade_buf, ONE_SECOND / 2));

				GLOBAL (CurrentActivity) = CHECK_ABORT;
				break;
		}

		SetSemaphore (GraphicsSem);
		SetFlashRect (NULL_PTR, (FRAME)0);
		ClearSemaphore (GraphicsSem);

		return (FALSE);
	}
	else
	{
		BYTE NewState;

		NewState = pMS->CurState;
		if (CurrentMenuState.up)
		{
			if (NewState-- == START_NEW_GAME)
				NewState = QUIT_GAME;
		}
		else if (CurrentMenuState.down)
		{
			if (NewState++ == QUIT_GAME)
				NewState = START_NEW_GAME;
		}
		if (NewState != pMS->CurState)
		{
			DrawRestartMenu (pMS->CurState, NewState, pMS->CurFrame);
			pMS->CurState = NewState;
		}
	}

	InTime = GetTimeCounter ();
	return (TRUE);
}

BOOLEAN
StartGame (void)
{
	MENU_STATE MenuState;

TimedOut:
	LastActivity = GLOBAL (CurrentActivity);
	GLOBAL (CurrentActivity) = 0;
	if (LastActivity == (ACTIVITY)~0)
	{
		Introduction ();
		if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		{
			TFB_Abort ();
		}
	}

	memset ((PMENU_STATE)&MenuState, 0, sizeof (MenuState));
	MenuState.InputFunc = DoRestart;

	{
		DWORD TimeOut;
		BYTE black_buf[1];
		extern ACTIVITY NextActivity;

TryAgain:
		ReinitQueue (&race_q[0]);
		ReinitQueue (&race_q[1]);
	
		black_buf[0] = FadeAllToBlack;

		SetContext (ScreenContext);

		GLOBAL (CurrentActivity) |= CHECK_ABORT;
		if (GLOBAL_SIS (CrewEnlisted) == (COUNT)~0
				&& GET_GAME_STATE (UTWIG_BOMB_ON_SHIP)
				&& !GET_GAME_STATE (UTWIG_BOMB))
		{
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
#ifdef NOTYET
LastActivity = WON_LAST_BATTLE;
#endif /* NOTYET */
			if (LOBYTE (LastActivity) == WON_LAST_BATTLE)
			{
				Victory ();
				Credits ();
				TimeOut = ONE_SECOND / 2;
			}
		}

		LastActivity = NextActivity = 0;

		{
			RECT r;
			STAMP s;

			s.frame = CaptureDrawable (
					LoadGraphic (RESTART_PMAP_ANIM)
					);
			MenuState.CurFrame = s.frame;
			GetFrameRect (s.frame, &r);
			s.origin.x = (SCREEN_WIDTH - r.extent.width) >> 1;
			s.origin.y = (SCREEN_HEIGHT - r.extent.height) >> 1;
			SleepThreadUntil (XFormColorMap ((COLORMAPPTR)black_buf, TimeOut));
			if (TimeOut == ONE_SECOND / 8)
				SleepThread (ONE_SECOND * 3);

			SetContextBackGroundColor (BLACK_COLOR);
			BatchGraphics ();
			ClearDrawable ();
			FlushColorXForms ();
			SetSemaphore (GraphicsSem);
			DrawStamp (&s);
			ClearSemaphore (GraphicsSem);
			UnbatchGraphics ();

			FlushInput ();
			GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
			DoInput ((PVOID)&MenuState, TRUE);
			
			SetSemaphore (GraphicsSem);
			SetFlashRect ((PRECT)0, (FRAME)0);
			ClearSemaphore (GraphicsSem);
			DestroyDrawable (ReleaseDrawable (s.frame));
			
			if (GLOBAL (CurrentActivity) == (ACTIVITY)~0)
				goto TimedOut;

			if (GLOBAL (CurrentActivity) & CHECK_ABORT)
			{
				TFB_Abort ();
			}

			TimeOut = XFormColorMap ((COLORMAPPTR)black_buf, ONE_SECOND / 2);
		}
		
		SleepThreadUntil (TimeOut);
		FlushColorXForms ();

		SeedRandomNumbers ();

		if (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE)
		{
FreeSC2Data ();
FreeLanderData ();
FreeIPData ();
FreeHyperData ();
			Melee ();
			MenuState.Initialized = FALSE;
			goto TryAgain;
		}

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

	return (FALSE);
}

