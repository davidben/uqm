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
#include "vidlib.h"
#include "libs/graphics/gfx_common.h"

//Added by Chris

void FreeHyperData (void);

BOOLEAN InitGameKernel (void);

void LoadMasterShipList (void);

//End Added by Chris

static void
DoFMV (char *name, char *loopname, BOOLEAN uninit)
{
	VidPlay ((MEM_HANDLE)name, loopname, uninit);
	while (VidPlaying ())
		;
	VidStop ();
}

void
DoShipSpin (COUNT index, MUSIC_REF hMusic)
{
	char buf[30];
	BYTE clut_buf[1];
	RECT old_r, r;

	SetGraphicUseOtherExtra (1);
	LoadIntoExtraScreen (0);
	clut_buf[0] = FadeAllToBlack;
	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)clut_buf, ONE_SECOND / 4));
	FlushColorXForms ();
	
	if (hMusic)
		StopMusic ();

	FreeHyperData ();
	
	sprintf (buf, "ship%02d", index);
	DoFMV (buf, "spin", FALSE);

	GetContextClipRect (&old_r);
	r.corner.x = r.corner.y = 0;
	r.extent.width = SCREEN_WIDTH;
	r.extent.height = SCREEN_HEIGHT;
	SetContextClipRect (&r);
	DrawFromExtraScreen (0);
	SetGraphicUseOtherExtra (0);
	SetContextClipRect (&old_r);

	if (hMusic)
		PlayMusic (hMusic, TRUE, 1);
		
	clut_buf[0] = FadeAllToColor;
	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)clut_buf, ONE_SECOND / 4));
	FlushColorXForms ();
}

void
Introduction (void)
{
	BYTE xform_buf[1];
	STAMP s;
	DWORD TimeOut;
	INPUT_STATE InputState;

		int TempInt;

	xform_buf[0] = FadeAllToBlack;
		TempInt = XFormColorMap ((COLORMAPPTR) xform_buf, 1);
	SleepThreadUntil (TempInt);
	SetSemaphore (GraphicsSem);
	SetContext (ScreenContext);
	s.origin.x = s.origin.y = 0;
	s.frame = CaptureDrawable (LoadGraphic (TITLE_ANIM));
	DrawStamp (&s);
	DestroyDrawable (ReleaseDrawable (s.frame));
	ClearSemaphore (GraphicsSem);

	FlushInput ();

	xform_buf[0] = FadeAllToColor;
	TimeOut = XFormColorMap ((COLORMAPPTR)xform_buf, ONE_SECOND / 2);
	LoadMasterShipList ();
	SleepThreadUntil (TimeOut);
	
	GLOBAL (CurrentActivity) |= CHECK_ABORT;
	TimeOut += ONE_SECOND * 3;
	while (!(InputState = AnyButtonPress (FALSE)) &&
				(TaskSwitch (), GetTimeCounter ()) <= TimeOut)
		{
				//
		}
	GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
	xform_buf[0] = FadeAllToBlack;
	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)xform_buf, ONE_SECOND / 2));

	if (InputState == 0)
	{
		static BOOLEAN play_intro = TRUE;
		
		DoFMV (play_intro ? "intro" : "drumall", NULL, TRUE);
		play_intro = !play_intro;
	}
	
	InitGameKernel ();
}

void
Victory (void)
{
	BYTE xform_buf[1];

	xform_buf[0] = FadeAllToBlack;
	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)xform_buf, ONE_SECOND / 2));

	DoFMV ("victory", NULL, TRUE);
		
	xform_buf[0] = FadeAllToBlack;
	XFormColorMap ((COLORMAPPTR)xform_buf, 0);
	
	InitGameKernel ();
}

void
Logo (void)
{
	DoFMV ("logo", NULL, FALSE);
}




