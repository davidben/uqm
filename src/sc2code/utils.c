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

	SeedRandom (cur_time = GetTimeCounter ());

	return (cur_time);
}

void
WaitForNoInput (SIZE Duration)
{
	INPUT_STATE PressState;

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
		INPUT_STATE ButtonState;

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
	BOOLEAN ClockActive;
	CONTEXT OldContext;
	FRAME F;
	HOT_SPOT OldHot;

	if (ActivityFrame == 0
			|| (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_PAUSE))
			|| (LastActivity & (CHECK_LOAD | CHECK_RESTART)))
		return (FALSE);
		
	GLOBAL (CurrentActivity) |= CHECK_PAUSE;

	ClockActive = (BOOLEAN)(
			LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE
			&& GameClockRunning ()
			);
	if (ClockActive)
		SuspendGameClock ();
	else if (CommData.ConversationPhrases && PlayingTrack ())
		PauseTrack ();

	SetSemaphore (GraphicsSem);
	OldContext = SetContext (ScreenContext);
	OldHot = SetFrameHot (Screen, MAKE_HOT_SPOT (0, 0));

	GetFrameRect (ActivityFrame, &r);
	r.corner.x = (SCREEN_WIDTH - r.extent.width) >> 1;
	r.corner.y = (SCREEN_HEIGHT - r.extent.height) >> 1;
	s.origin = r.corner;
	s.frame = ActivityFrame;
	F = CaptureDrawable (LoadDisplayPixmap (&r, (FRAME)0));
	DrawStamp (&s);

	FlushGraphics ();

	{
		BYTE scan;

		scan = KBDToUNICODE (SK_F1);
		while (KeyDown (scan))
			TaskSwitch ();
	}

	FlushInput ();
	while (KeyHit () != SK_F1)
		TaskSwitch ();

	s.frame = F;
	DrawStamp (&s);
	DestroyDrawable (ReleaseDrawable (s.frame));

	SetFrameHot (Screen, OldHot);
	SetContext (OldContext);

	WaitForNoInput (ONE_SECOND / 4);
	FlushInput ();
	ClearSemaphore (GraphicsSem);

	if (ClockActive)
		ResumeGameClock ();
	else if (CommData.ConversationPhrases && PlayingTrack ())
		ResumeTrack ();

	TaskSwitch ();
	GLOBAL (CurrentActivity) &= ~CHECK_PAUSE;
	return (TRUE);
}
