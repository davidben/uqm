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

#include <ctype.h>
#include "starcon.h"
#include "commglue.h"
#include "comm.h"
#include "libs/sound/trackplayer.h"

#define CONFIRM_WIN_WIDTH 80
#define CONFIRM_WIN_HEIGHT 26

static void
DrawConfirmationWindow (BOOLEAN answer)
{
	COLOR oldfg = SetContextForeGroundColor (MENU_BACKGROUND_COLOR);
	FONT  oldfont = SetContextFont (StarConFont);
	RECT r;
	TEXT t;

	r.extent.width = CONFIRM_WIN_WIDTH;
	r.extent.height = CONFIRM_WIN_HEIGHT;
	r.corner.x = (SCREEN_WIDTH - r.extent.width) >> 1;
	r.corner.y = (SCREEN_HEIGHT - r.extent.height) >> 1;

	BatchGraphics ();

	DrawFilledRectangle (&r);
	SetContextForeGroundColor (MENU_TEXT_COLOR);
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + 10;
	t.pStr = "Really quit?";
	t.align = ALIGN_CENTER;
	t.valign = VALIGN_BOTTOM;
	t.CharCount = ~0;
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

	SetSemaphore (GraphicsSem);
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
		BOOLEAN response = TRUE, done;

		in_confirm = TRUE;
		oldContext = SetContext (ScreenContext);

		r.extent.width = CONFIRM_WIN_WIDTH;
		r.extent.height = CONFIRM_WIN_HEIGHT;
		r.corner.x = (SCREEN_WIDTH - r.extent.width) >> 1;
		r.corner.y = (SCREEN_HEIGHT - r.extent.height) >> 1;
		s.origin = r.corner;
		F = CaptureDrawable (LoadDisplayPixmap (&r, (FRAME)0));

		DrawConfirmationWindow (response);

		// Releasing the Semaphore lets the rotate_planet_task
		// draw a frame.  PauseRotate can still allow one more frame
		// to be drawn, so it is safer to just not release the Semaphore
		//ClearSemaphore (GraphicsSem);
		FlushGraphics ();
		//SetSemaphore (GraphicsSem);

		GLOBAL (CurrentActivity) |= CHECK_ABORT;
		FlushInput ();
		done = FALSE;

		do {
			// Forbid recursive calls or pausing here!
			ExitRequested = FALSE;
			GamePaused = FALSE;
			UpdateInputState ();
			if (CurrentMenuState.select)
			{
				done = TRUE;
			}
			else if (CurrentMenuState.cancel)
			{
				done = TRUE;
				response = FALSE;
			}
			else if (CurrentMenuState.left || CurrentMenuState.right)
			{
				response = !response;
				DrawConfirmationWindow (response);
			}
			TaskSwitch ();
		} while (!done);

		s.frame = F;
		DrawStamp (&s);
		DestroyDrawable (ReleaseDrawable (s.frame));
		if (response)
		{
			GameExiting = TRUE;
			result = TRUE;
		}		
		else
		{
			result = FALSE;
			GameExiting = FALSE;
			GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
		}
		ExitRequested = FALSE;
		GamePaused = FALSE;
		FlushInput ();
		SetContext (oldContext);
	}
	ClearSemaphore (GraphicsSem);

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


