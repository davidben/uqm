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
#include "libs/sound/trackplayer.h"

//Added by Chris

void WaitForNoInput (SIZE Duration);

//End Added by Chris

/* TODO: Replace the TO EXIT PRESS B code with a 'real' confirmation
   screen.  To avoid confusion (especially now that there is no
   in-game model of the 3DO controller exits are currently always
   confirmed. */

BOOLEAN
ConfirmExit (void)
{
#if 0
	BOOLEAN result;
	fprintf (stderr, "Confirming Exit!\n");
	if (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE)
		SuspendGameClock ();
	else if (CommData.ConversationPhrases && PlayingTrack ())
		PauseTrack ();

	SetSemaphore (GraphicsSem);
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
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
		UNICODE response;

		oldContext = SetContext (ScreenContext);

		s.frame = SetAbsFrameIndex (ActivityFrame, 1);
		GetFrameRect (s.frame, &r);
		r.corner.x = (SCREEN_WIDTH - r.extent.width) >> 1;
		r.corner.y = (SCREEN_HEIGHT - r.extent.height) >> 1;
		s.origin = r.corner;
		F = CaptureDrawable (LoadDisplayPixmap (&r, (FRAME)0));
		DrawStamp (&s);

		// Releasing the Semaphore lets the rotate_planet_task
		// draw a frame.  PauseRotate can still allow one more frame
		// to be drawn, so it is safer to just not release the Semaphore
		//ClearSemaphore (GraphicsSem);
		FlushGraphics ();
		//SetSemaphore (GraphicsSem);

		/* Possible bug... ConfirmExit is assuming
		 * CharacterMode is off. */
		EnableCharacterMode ();
		GLOBAL (CurrentActivity) |= CHECK_ABORT;
		do {
			response = toupper (GetCharacter ());
		} while (response != 'Y' && response != 'N');
		DisableCharacterMode ();
		ExitRequested = FALSE;

		s.frame = F;
		DrawStamp (&s);
		DestroyDrawable (ReleaseDrawable (s.frame));
		if (response == 'Y')
		{
			GameExiting = TRUE;
			result = TRUE;
		}		
		else
		{
			result = FALSE;
			GameExiting = FALSE;
			GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
			WaitForNoInput (ONE_SECOND / 4);
			FlushInput ();
		}
		SetContext (oldContext);
	}
	ClearSemaphore (GraphicsSem);

	if (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE)
		ResumeGameClock ();
	else if (CommData.ConversationPhrases && PlayingTrack ())
		ResumeTrack ();

	fprintf (stderr, "Exit was %sconfirmed.\n", result ? "" : "NOT ");
	return (result);
#endif
	GLOBAL (CurrentActivity) |= CHECK_ABORT;
	ExitRequested = FALSE;
	GameExiting = TRUE;
	return TRUE;
}


