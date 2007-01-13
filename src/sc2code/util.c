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

#include "commglue.h"
#include "comm.h"
#include "controls.h"
#include "util.h"
#include "setup.h"
#include "libs/inplib.h"
#include "libs/sound/trackplayer.h"
#include "libs/mathlib.h"


void
DrawStarConBox (PRECT pRect, SIZE BorderWidth, COLOR TopLeftColor, COLOR
		BottomRightColor, BOOLEAN FillInterior, COLOR InteriorColor)
{
	RECT locRect;

	if (BorderWidth == 0)
		BorderWidth = 2;
	else
	{
		SetContextForeGroundColor (TopLeftColor);
		locRect.corner = pRect->corner;
		locRect.extent.width = pRect->extent.width;
		locRect.extent.height = 1;
		DrawFilledRectangle (&locRect);
		if (BorderWidth == 2)
		{
			++locRect.corner.x;
			++locRect.corner.y;
			locRect.extent.width -= 2;
			DrawFilledRectangle (&locRect);
		}

		locRect.corner = pRect->corner;
		locRect.extent.width = 1;
		locRect.extent.height = pRect->extent.height;
		DrawFilledRectangle (&locRect);
		if (BorderWidth == 2)
		{
			++locRect.corner.x;
			++locRect.corner.y;
			locRect.extent.height -= 2;
			DrawFilledRectangle (&locRect);
		}

		SetContextForeGroundColor (BottomRightColor);
		locRect.corner.x = pRect->corner.x + pRect->extent.width - 1;
		locRect.corner.y = pRect->corner.y + 1;
		locRect.extent.height = pRect->extent.height - 1;
		DrawFilledRectangle (&locRect);
		if (BorderWidth == 2)
		{
			--locRect.corner.x;
			++locRect.corner.y;
			locRect.extent.height -= 2;
			DrawFilledRectangle (&locRect);
		}

		locRect.corner.x = pRect->corner.x;
		locRect.extent.width = pRect->extent.width;
		locRect.corner.y = pRect->corner.y + pRect->extent.height - 1;
		locRect.extent.height = 1;
		DrawFilledRectangle (&locRect);
		if (BorderWidth == 2)
		{
			++locRect.corner.x;
			--locRect.corner.y;
			locRect.extent.width -= 2;
			DrawFilledRectangle (&locRect);
		}
	}

	if (FillInterior)
	{
		SetContextForeGroundColor (InteriorColor);
		locRect.corner.x = pRect->corner.x + BorderWidth;
		locRect.corner.y = pRect->corner.y + BorderWidth;
		locRect.extent.width = pRect->extent.width - (BorderWidth << 1);
		locRect.extent.height = pRect->extent.height - (BorderWidth << 1);
		DrawFilledRectangle (&locRect);
	}
}

DWORD
SeedRandomNumbers (void)
{
	DWORD cur_time;

	cur_time = GetTimeCounter ();
	TFB_SeedRandom (cur_time);

	return (cur_time);
}

void
WaitForNoInput (SIZE Duration)
{
	BOOLEAN PressState;

	PressState = AnyButtonPress (FALSE);
	if (Duration < 0)
	{
		if (PressState)
			return;
		Duration = -Duration;
	}
	else if (!PressState)
		return;
	
	{
		DWORD TimeOut;
		BOOLEAN ButtonState;

		TimeOut = GetTimeCounter () + Duration;
		do
		{
			ButtonState = AnyButtonPress (FALSE);
			if (PressState)
			{
				PressState = ButtonState;
				ButtonState = 0;
			}
		} while (!ButtonState &&
				(TaskSwitch (), GetTimeCounter ()) <= TimeOut);
	}
}

BOOLEAN
PauseGame (void)
{
	RECT r;
	STAMP s;
	CONTEXT OldContext;
	FRAME F;
	HOT_SPOT OldHot;
	RECT OldRect;

	if (ActivityFrame == 0
			|| (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_PAUSE))
			|| (LastActivity & (CHECK_LOAD | CHECK_RESTART)))
		return (FALSE);
		
	GLOBAL (CurrentActivity) |= CHECK_PAUSE;

	if (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE &&
			LOBYTE (GLOBAL (CurrentActivity)) != WON_LAST_BATTLE)
		SuspendGameClock ();
	if (CommData.ConversationPhrases && PlayingTrack ())
		PauseTrack ();

	LockMutex (GraphicsLock);
	OldContext = SetContext (ScreenContext);
	OldHot = SetFrameHot (Screen, MAKE_HOT_SPOT (0, 0));
	GetContextClipRect (&OldRect);
	SetContextClipRect (NULL_PTR);

	GetFrameRect (ActivityFrame, &r);
	r.corner.x = (SCREEN_WIDTH - r.extent.width) >> 1;
	r.corner.y = (SCREEN_HEIGHT - r.extent.height) >> 1;
	s.origin = r.corner;
	s.frame = ActivityFrame;
	F = CaptureDrawable (LoadDisplayPixmap (&r, (FRAME)0));
	SetSystemRect (&r);
	DrawStamp (&s);

	// Releasing the lock lets the rotate_planet_task
	// draw a frame.  PauseRotate can still allow one more frame
	// to be drawn, so it is safer to just not release the lock
	//UnlockMutex (GraphicsLock);
	FlushGraphics ();
	//LockMutex (GraphicsLock);

	while (ImmediateInputState.menu[KEY_PAUSE])
		TaskSwitch ();

	while (!ImmediateInputState.menu[KEY_PAUSE])
		TaskSwitch ();

	while (ImmediateInputState.menu[KEY_PAUSE])
		TaskSwitch ();

	GamePaused = FALSE;

	s.frame = F;
	DrawStamp (&s);
	DestroyDrawable (ReleaseDrawable (s.frame));
	ClearSystemRect ();

	SetContextClipRect (&OldRect);
	SetFrameHot (Screen, OldHot);
	SetContext (OldContext);

	WaitForNoInput (ONE_SECOND / 4);
	FlushInput ();

	if (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE &&
			LOBYTE (GLOBAL (CurrentActivity)) != WON_LAST_BATTLE)
		ResumeGameClock ();
	if (CommData.ConversationPhrases && PlayingTrack ())
	{
		ResumeTrack ();
		if (CommData.AlienTransitionDesc.AnimFlags & TALK_DONE)
			do_subtitles ((void *)~0);
	}

	UnlockMutex (GraphicsLock);

	TaskSwitch ();
	GLOBAL (CurrentActivity) &= ~CHECK_PAUSE;
	return (TRUE);
}

// Waits for a new button to be pressed
// and returns TRUE if Quit was selected
BOOLEAN
WaitAnyButtonOrQuit (BOOLEAN CheckSpecial)
{
	while (AnyButtonPress (TRUE))
		TaskSwitch ();

	while (!AnyButtonPress (TRUE) &&
			!(GLOBAL (CurrentActivity) & CHECK_ABORT))
		TaskSwitch ();

	/* Satisfy unused parameter */
	(void) CheckSpecial;

	return (GLOBAL (CurrentActivity) & CHECK_ABORT) != 0;
}

