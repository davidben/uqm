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

//Added by Chris

void FreeSC2Data (void);

void FreeLanderData (void);

void FreeIPData (void);

void FreeMasterShipList (void);

void UninitSpace (void);

void FreeHyperData (void);

//End Added by Chris

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
	
FreeSC2Data ();
FreeLanderData ();
FreeIPData ();
FreeHyperData ();
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

	SetSemaphore (GraphicsSem);
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
	for (i = 1; i < NUM_CREDITS; ++i)
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
		
		SleepThreadUntil (TimeIn + ONE_SECOND * 5);
		TimeIn = GetTimeCounter ();
	}
	
	DestroyDrawable (ReleaseDrawable (f[0]));
	ClearSemaphore (GraphicsSem);

	while (!AnyButtonPress (FALSE));
	while (AnyButtonPress (FALSE));
	
	fade_buf[0] = FadeAllToBlack;
	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)fade_buf, ONE_SECOND / 2));
	FlushColorXForms ();
	
	if (hMusic)
	{
		StopMusic ();
		DestroyMusic (hMusic);
	}
}


