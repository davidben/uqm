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

#include "controls.h"
#include "commglue.h"
#include "comm.h"
#include "colors.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "libs/graphics/widgets.h"
#include "libs/inplib.h"
#include "libs/sound/trackplayer.h"

#include <ctype.h>


#define CONFIRM_WIN_WIDTH 80
#define CONFIRM_WIN_HEIGHT 22

static void
DrawConfirmationWindow (BOOLEAN answer)
{
	COLOR oldfg = SetContextForeGroundColor (MENU_TEXT_COLOR);
	COLOR dark, medium;
	FONT  oldfont = SetContextFont (StarConFont);
	FONTEFFECT oldFontEffect = SetContextFontEffect (0, 0, 0);
	RECT r;
	TEXT t;

	dark = BUILD_COLOR (MAKE_RGB15 (0x8, 0x8, 0x8), 0x1F);
	medium = BUILD_COLOR (MAKE_RGB15 (0x10, 0x10, 0x10), 0x19);

	BatchGraphics ();
	r.corner.x = (SCREEN_WIDTH - CONFIRM_WIN_WIDTH) >> 1;
	r.corner.y = (SCREEN_HEIGHT - CONFIRM_WIN_HEIGHT) >> 1;
	r.extent.width = CONFIRM_WIN_WIDTH;
	r.extent.height = CONFIRM_WIN_HEIGHT;
	DrawShadowedBox (&r, MENU_BACKGROUND_COLOR, dark, medium);

	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + 8;
	t.pStr = "Really Quit?";
	t.align = ALIGN_CENTER;
	t.valign = VALIGN_BOTTOM;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += 10;
	t.baseline.x = r.corner.x + (r.extent.width >> 2);
	t.pStr = "Yes";
	SetContextForeGroundColor (answer ? MENU_HIGHLIGHT_COLOR : MENU_TEXT_COLOR);
	font_DrawText (&t);
	t.baseline.x += (r.extent.width >> 1);
	t.pStr = "No";
	SetContextForeGroundColor (answer ? MENU_TEXT_COLOR : MENU_HIGHLIGHT_COLOR);	
	font_DrawText (&t);

	UnbatchGraphics ();

	SetContextFontEffect (oldFontEffect.type, oldFontEffect.from, oldFontEffect.to);
	SetContextFont (oldfont);
	SetContextForeGroundColor (oldfg);
}

/* This code assumes that you aren't in Character Mode.  This is
 * currently safe because VControl doesn't see keystrokes when you
 * are, and thus cannot conclude that an exit is necessary. */
BOOLEAN
DoConfirmExit (void)
{
	BOOLEAN result;
	static BOOLEAN in_confirm = FALSE;
	fprintf (stderr, "Confirming Exit!\n");
	if (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE)
		SuspendGameClock ();
	if (CommData.ConversationPhrases && PlayingTrack ())
		PauseTrack ();

	LockMutex (GraphicsLock);
	if (in_confirm)
	{
		result = FALSE;
		ExitRequested = FALSE;
	}
	else
	{
		RECT r;
		STAMP s;
		FRAME F;
		CONTEXT oldContext;
		BOOLEAN response = FALSE, done;

		in_confirm = TRUE;
		oldContext = SetContext (ScreenContext);

		r.extent.width = CONFIRM_WIN_WIDTH + 4;
		r.extent.height = CONFIRM_WIN_HEIGHT + 4;
		r.corner.x = (SCREEN_WIDTH - r.extent.width) >> 1;
		r.corner.y = (SCREEN_HEIGHT - r.extent.height) >> 1;
		s.origin = r.corner;
		F = CaptureDrawable (LoadDisplayPixmap (&r, (FRAME)0));

		DrawConfirmationWindow (response);

		// Releasing the lock lets the rotate_planet_task
		// draw a frame.  PauseRotate can still allow one more frame
		// to be drawn, so it is safer to just not release the lock
		//UnlockMutex (GraphicsLock);
		FlushGraphics ();
		//LockMutex (GraphicsLock);

		GLOBAL (CurrentActivity) |= CHECK_ABORT;
		FlushInput ();
		done = FALSE;

		do {
			// Forbid recursive calls or pausing here!
			ExitRequested = FALSE;
			GamePaused = FALSE;
			UpdateInputState ();
			if (PulsedInputState.key[KEY_MENU_SELECT])
			{
				done = TRUE;
				PlaySoundEffect (SetAbsSoundIndex (MenuSounds, 1),
						0, NotPositional (), NULL, GAME_SOUND_PRIORITY);
			}
			else if (PulsedInputState.key[KEY_MENU_CANCEL])
			{
				done = TRUE;
				response = FALSE;
			}
			else if (PulsedInputState.key[KEY_MENU_LEFT] || PulsedInputState.key[KEY_MENU_RIGHT])
			{
				response = !response;
				DrawConfirmationWindow (response);
				PlaySoundEffect (SetAbsSoundIndex (MenuSounds, 0),
						0, NotPositional (), NULL, GAME_SOUND_PRIORITY);

			}
			TaskSwitch ();
		} while (!done);

		s.frame = F;
		DrawStamp (&s);
		DestroyDrawable (ReleaseDrawable (s.frame));
		if (response)
		{
			result = TRUE;
		}		
		else
		{
			result = FALSE;
			GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
		}
		ExitRequested = FALSE;
		GamePaused = FALSE;
		FlushInput ();
		SetContext (oldContext);
	}
	UnlockMutex (GraphicsLock);

	if (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE)
		ResumeGameClock ();
	if (CommData.ConversationPhrases && PlayingTrack ())
	{
		ResumeTrack ();
		if (CommData.AlienTransitionDesc.AnimFlags & TALK_DONE)
			do_subtitles ((void *)~0);
	}

	fprintf (stderr, "Exit was %sconfirmed.\n", result ? "" : "NOT ");
	in_confirm = FALSE;
	return (result);
}


