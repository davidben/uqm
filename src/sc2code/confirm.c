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
#include "commglue.h"
#include "libs/sound/trackplayer.h"

//Added by Chris

void WaitForNoInput (SIZE Duration);

//End Added by Chris

INPUT_STATE
ConfirmExit (void)
{
	INPUT_STATE InputState;

	if (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE)
		SuspendGameClock ();
	else if (CommData.ConversationPhrases && PlayingTrack ())
		PauseTrack ();

	SetSemaphore (GraphicsSem);
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		InputState = DEVICE_EXIT;
	else
	{
		RECT r;
		STAMP s;
		FRAME F;
		CONTEXT oldContext;

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

		{
			INPUT_STATE PressState;

			GLOBAL (CurrentActivity) |= CHECK_ABORT;

			PressState = GetInputState (NormalInput);
			do
			{
				TaskSwitch ();
				InputState = GetInputState (NormalInput);
				if (PressState)
				{
					PressState = InputState;
					InputState = 0;
				}
			} while (!InputState);
		}

		s.frame = F;
		DrawStamp (&s);
		DestroyDrawable (ReleaseDrawable (s.frame));

		if (InputState & DEVICE_BUTTON2)
			InputState = DEVICE_EXIT;
		else
		{
			InputState = 0;
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

	return (InputState);
}


