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
#include "libs/graphics/gfx_common.h"
#include "libs/sound/sound.h"
#include "oscill.h"
#include "options.h"

void
Credits (void)
{
#define NUM_CREDITS 20
	COUNT i;
	RES_TYPE rt;
	RES_INSTANCE ri;
	RES_PACKAGE rp;
	RECT r;
	FRAME f[NUM_CREDITS];
	STAMP s;
	DWORD TimeIn;
	MUSIC_REF hMusic;
	BYTE fade_buf[1];

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return;
	
	rt = GET_TYPE (CREDIT00_ANIM);
	ri = GET_INSTANCE (CREDIT00_ANIM);
	rp = GET_PACKAGE (CREDIT00_ANIM);
	for (i = 0; i < NUM_CREDITS; ++i, ++rp, ++ri)
	{
		f[i] = CaptureDrawable (LoadGraphic (
				MAKE_RESOURCE (rp, rt, ri)
				));
	}
	
	hMusic = LoadMusicInstance (CREDITS_MUSIC);
	if (hMusic)
		PlayMusic (hMusic, TRUE, 1);

	LockMutex (GraphicsLock);
	SetContext (ScreenContext);
	GetContextClipRect (&r);
	s.origin.x = s.origin.y = 0;
	
	BatchGraphics ();
	s.frame = f[0];
	DrawStamp (&s);
	UnbatchGraphics ();
	
	fade_buf[0] = FadeAllToColor;
	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)fade_buf, ONE_SECOND / 2));
	
	TimeIn = GetTimeCounter ();
	for (i = 1; (i < NUM_CREDITS) &&
			!(GLOBAL (CurrentActivity) & CHECK_ABORT); ++i)
	{
		SetTransitionSource (&r);
		BatchGraphics ();
		s.frame = f[0];
		DrawStamp (&s);
		s.frame = f[i];
		DrawStamp (&s);
		ScreenTransition (3, &r);
		UnbatchGraphics ();
		DestroyDrawable (ReleaseDrawable (f[i]));
		
		UnlockMutex (GraphicsLock);
		while ((GetTimeCounter () - TimeIn < ONE_SECOND * 5) &&
			!(GLOBAL (CurrentActivity) & CHECK_ABORT))
		{
			UpdateInputState ();
			SleepThreadUntil (ONE_SECOND / 20);
		}
		LockMutex (GraphicsLock);
		TimeIn = GetTimeCounter ();
	}
	
	DestroyDrawable (ReleaseDrawable (f[0]));
	UnlockMutex (GraphicsLock);

	WaitAnyButtonOrQuit (FALSE);
	
	fade_buf[0] = FadeAllToBlack;
	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)fade_buf, ONE_SECOND / 2));
	FlushColorXForms ();
	
	if (hMusic)
	{
		StopMusic ();
		DestroyMusic (hMusic);
	}
}

void
OutTakes (void)
{
#define NUM_OUTTAKES 15
	static long outtake_list[NUM_OUTTAKES] =
	{
		ZOQFOTPIK_CONVERSATION,
		TALKING_PET_CONVERSATION,
		ORZ_CONVERSATION,
		UTWIG_CONVERSATION,
		THRADD_CONVERSATION,
		SUPOX_CONVERSATION,
		SYREEN_CONVERSATION,
		SHOFIXTI_CONVERSATION,
		PKUNK_CONVERSATION,
		YEHAT_CONVERSATION,
		DRUUGE_CONVERSATION,
		URQUAN_CONVERSATION,
		VUX_CONVERSATION,
		BLACKURQ_CONVERSATION,
		ARILOU_CONVERSATION
	};

	RECT r;
	BOOLEAN oldsubtitles = optSubtitles;
	int i = 0;
	BYTE fade_buf[1];

	optSubtitles = TRUE;
	sliderDisabled = TRUE;

	LockMutex (GraphicsLock);
	SetContext (ScreenContext);
	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x0, 0x0, 0x0), 0x00));
	fade_buf[0] = FadeAllToColor;
	XFormColorMap ((COLORMAPPTR)fade_buf, 0);

	r.corner.x = r.corner.y = 0;
	r.extent.width = SCREEN_WIDTH;
	r.extent.height = SCREEN_HEIGHT;
	DrawFilledRectangle (&r);	
	UnlockMutex (GraphicsLock);

	for (i = 0; (i < NUM_OUTTAKES) &&
			!(GLOBAL (CurrentActivity) & CHECK_ABORT); i++)
	{
		InitCommunication (outtake_list[i]);
	}

	SetContext (ScreenContext);
	fade_buf[0] = FadeAllToBlack;
	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)fade_buf, ONE_SECOND / 2));
	FlushColorXForms ();

	optSubtitles = oldsubtitles;
	sliderDisabled = FALSE;
}
