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
#include "libs/vidlib.h"

//Added by Chris

BOOLEAN InitGameKernel (void);

void LoadMasterShipList (void);

//End Added by Chris

void
DoFMV (char *name, char *loopname, BOOLEAN uninit)
{
	VIDEO_REF VidRef;

	VidRef = LoadVideoFile (name);
	VidPlay (VidRef, loopname, uninit);
	VidDoInput ();
	VidStop ();
	DestroyVideo (VidRef);
}

/* //Already defined in fmv.c
void
Introduction (void)
{
	BYTE xform_buf[1];
	STAMP s;
	DWORD TimeOut;
	BOOLEAN InputState;

	xform_buf[0] = FadeAllToBlack;
	SleepThreadUntil (XFormColorMap (
			(COLORMAPPTR)xform_buf, ONE_SECOND / 120));
	LockMutex (GraphicsLock);
	SetContext (ScreenContext);
	s.origin.x = s.origin.y = 0;
	s.frame = CaptureDrawable (LoadGraphic (TITLE_ANIM));
	DrawStamp (&s);
	DestroyDrawable (ReleaseDrawable (s.frame));
	UnlockMutex (GraphicsLock);

	FlushInput ();

	xform_buf[0] = FadeAllToColor;
	TimeOut = XFormColorMap ((COLORMAPPTR)xform_buf, ONE_SECOND / 2);
	LoadMasterShipList ();
	SleepThreadUntil (TimeOut);
	
	GLOBAL (CurrentActivity) |= CHECK_ABORT;
	TimeOut += ONE_SECOND * 3;
	while (!(InputState = AnyButtonPress (FALSE)) && TaskSwitch () <= TimeOut)
		;
	GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
	xform_buf[0] = FadeAllToBlack;
	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)xform_buf, ONE_SECOND / 2));

	if (InputState == 0)
		DoFMV ("intro", NULL, FALSE);
	
	InitGameKernel ();
}
*/

/* //Already defined in fmv.c
void
Victory (void)
{
	BYTE xform_buf[1];

	xform_buf[0] = FadeAllToBlack;
	SleepThreadUntil (XFormColorMap ((COLORMAPPTR)xform_buf, ONE_SECOND / 2));

	DoFMV ("victory", NULL, TRUE);
		
	InitGameKernel ();
}
*/



