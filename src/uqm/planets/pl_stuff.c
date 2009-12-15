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

#include "planets.h"
#include "../colors.h"
#include "../setup.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/mathlib.h"
#include "scan.h"
#include "options.h"

#include <math.h>


// define USE_ADDITIVE_SCAN_BLIT to use additive blittting
// instead of transparency for the planet scans.
// It still doesn't look right though (it is too bright)
#define USE_ADDITIVE_SCAN_BLIT


// XXX: these are currently defined in libs/graphics/sdl/3do_getbody.c
//  they should be sorted out and cleaned up at some point
extern DWORD frame_mapRGBA (FRAME FramePtr, UBYTE r, UBYTE g,
		UBYTE b, UBYTE a);
extern void fill_frame_rgb (FRAME FramePtr, DWORD color, int x0, int y0,
		int x, int y);
extern void arith_frame_blit (FRAME srcFrame, const RECT *rsrc,
		FRAME dstFrame, const RECT *rdst, int num, int denom);

static int rotFrameIndex;
static int rotDirection;
static bool throbShield;
static int rotPointIndex;

// Draw the planet sphere and any extra graphic (like a shield) if present
void
DrawPlanetSphere (int x, int y)
{
	STAMP s;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	s.origin.x = x;
	s.origin.y = y;

	BatchGraphics ();
	s.frame = Orbit->SphereFrame;
	DrawStamp (&s);
	if (Orbit->ObjectFrame)
	{
		s.frame = Orbit->ObjectFrame;
		DrawStamp (&s);
	}
	UnbatchGraphics ();
}

void
DrawDefaultPlanetSphere (void)
{
	CONTEXT oldContext;

	oldContext = SetContext (SpaceContext);
	DrawPlanetSphere (SIS_SCREEN_WIDTH / 2, PLANET_ORG_Y);
	SetContext (oldContext);
}

void
InitSphereRotation (int direction, BOOLEAN shielded)
{
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	rotDirection = direction;
	rotPointIndex = 0;
	throbShield = shielded && optWhichShield == OPT_3DO;

	if (throbShield)
	{
		// ObjectFrame must contain the shield graphic
		Orbit->WorkFrame = Orbit->ObjectFrame;
		// We need a scratch frame so that we can apply throbbing
		// to the shield, so create one
		Orbit->ObjectFrame = CaptureDrawable (CreateDrawable (
				WANT_PIXMAP | WANT_ALPHA,
				GetFrameWidth (Orbit->ObjectFrame),
				GetFrameHeight (Orbit->ObjectFrame), 2));
	}

	// Render the first sphere/shield frame
	// Prepare will set the next one
	rotFrameIndex = 1;
	PrepareNextRotationFrame ();
}

void
UninitSphereRotation (void)
{
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	if (Orbit->WorkFrame)
	{
		DestroyDrawable (ReleaseDrawable (Orbit->ObjectFrame));
		Orbit->ObjectFrame = Orbit->WorkFrame;
		Orbit->WorkFrame = NULL;
	}
}

void
PrepareNextRotationFrame (void)
{
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	// Generate the next rotation frame
	// We alternate between the frames because we do not call FlushGraphics()
	// The frame we just drew may not have made it to the screen yet
	rotFrameIndex ^= 1;

	// Go to next point, taking care of wraparounds
	rotPointIndex += rotDirection;
	if (rotPointIndex < 0)
		rotPointIndex = MAP_WIDTH - 1;
	else if (rotPointIndex >= MAP_WIDTH)
		rotPointIndex = 0;

	// prepare the next sphere frame
	Orbit->SphereFrame = SetAbsFrameIndex (Orbit->SphereFrame, rotFrameIndex);
	RenderPlanetSphere (Orbit->SphereFrame, rotPointIndex, throbShield);
	
	if (throbShield)
	{	// prepare the next shield throb frame
		Orbit->ObjectFrame = SetAbsFrameIndex (Orbit->ObjectFrame,
				rotFrameIndex);
		SetShieldThrobEffect (Orbit->WorkFrame, rotPointIndex,
				Orbit->ObjectFrame);
	}
}

#define ZOOM_RATE  24
#define ZOOM_TIME  (ONE_SECOND * 6 / 5)

// This takes care of zooming the planet sphere into place
// when entering orbit
void
ZoomInPlanetSphere (void)
{
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;
	const int base = GSCALE_IDENTITY;
	int dx, dy;
	int oldScale;
	int oldMode;
	int i;
	int frameCount;
	int zoomCorner;
	RECT frameRect;
	RECT repairRect;
	TimeCount NextTime;

	frameCount = ZOOM_TIME / (ONE_SECOND / ZOOM_RATE);

	// Planet zoom in from a randomly chosen corner
	zoomCorner = TFB_Random ();
	dx = 1 - (zoomCorner & 1) * 2;
	dy = 1 - (zoomCorner & 2);

	if (Orbit->ObjectFrame)
		GetFrameRect (Orbit->ObjectFrame, &frameRect);
	else
		GetFrameRect (Orbit->SphereFrame, &frameRect);
	repairRect = frameRect;

	for (i = 0; i <= frameCount; ++i)
	{
		double scale;
		POINT pt;

		NextTime = GetTimeCounter () + (ONE_SECOND / ZOOM_RATE);

		// Use 1 + e^-2 - e^(-2x / frameCount)) function to get a decelerating
		// zoom like the one 3DO does (supposedly)
		if (i < frameCount)
			scale = 1.134 - exp (-2.0 * i / frameCount);
		else
			scale = 1.0; // final frame

		// start from beyond the screen
		pt.x = SIS_SCREEN_WIDTH / 2 + (int) (dx * (1.0 - scale)
				* (SIS_SCREEN_WIDTH * 6 / 10) + 0.5);
		pt.y = PLANET_ORG_Y + (int) (dy * (1.0 - scale)
				* (SCAN_SCREEN_HEIGHT * 6 / 10) + 0.5);

		LockMutex (GraphicsLock);
		SetContext (SpaceContext);

		BatchGraphics ();
		if (i > 0)
			RepairBackRect (&repairRect);

		oldMode = SetGraphicScaleMode (TFB_SCALE_BILINEAR);
		oldScale = SetGraphicScale ((int)(base * scale + 0.5));
		DrawPlanetSphere (pt.x, pt.y);
		SetGraphicScale (oldScale);
		SetGraphicScaleMode (oldMode);

		UnbatchGraphics ();
		UnlockMutex (GraphicsLock);

		repairRect.corner.x = pt.x + frameRect.corner.x;
		repairRect.corner.y = pt.y + frameRect.corner.y;

		PrepareNextRotationFrame ();

		SleepThreadUntil (NextTime);
	}
}

void
RotatePlanetSphere (BOOLEAN keepRate)
{
	static TimeCount NextTime;
	TimeCount Now = GetTimeCounter ();

	if (keepRate && Now < NextTime)
		return; // not time yet

	NextTime = Now + PLANET_ROTATION_RATE;
	DrawDefaultPlanetSphere ();

	PrepareNextRotationFrame ();
}

// tintColor.a is ignored
void
DrawPlanet (int dy, Color tintColor)
{
	STAMP s;
	UBYTE a = 128;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = pSolarSysState->TopoFrame;
	BatchGraphics ();
	if (sameColor (tintColor, BLACK_COLOR))
	{	// no tint -- just draw the surface
		DrawStamp (&s);
	}
	else
	{	// apply different scan type tints
		UBYTE r, g, b;
		DWORD p;
		COUNT framew, frameh;
		RECT srect, drect, *psrect = NULL, *pdrect = NULL;
		FRAME tintFrame[2];

		tintFrame[0] = SetAbsFrameIndex (Orbit->TintFrame, 0);
		tintFrame[1] = SetAbsFrameIndex (Orbit->TintFrame, 1);

		framew = GetFrameWidth (tintFrame[0]);
		frameh = GetFrameHeight (tintFrame[0]);

		if (!sameColor (tintColor, Orbit->TintColor))
		{
			Orbit->TintColor = tintColor;
			// Buffer the topoMap to the tintFrame;
			arith_frame_blit (s.frame, NULL, tintFrame[0], NULL, 0, 0);
			r = tintColor.r / 2;
			g = tintColor.g / 2;
			b = tintColor.b / 2;
#ifdef USE_ADDITIVE_SCAN_BLIT
			a = 255;
#endif
			p = frame_mapRGBA (tintFrame[1], r, g, b, a);
			fill_frame_rgb (tintFrame[1], p, 0, 0, 0, 0);
		}
		
		drect.corner.x = 0;
		drect.extent.width = framew;
		srect.corner.x = 0;
		srect.corner.y = 0;
		srect.extent.width = framew;
		if (dy >= 0 && dy <= frameh)
		{
			drect.corner.y = dy;
			drect.extent.height = 1;
			pdrect = &drect;
			srect.extent.height = 1;
			psrect = &srect;
		}
		if (dy < 0)
		{
			drect.corner.y = -dy;
			drect.extent.height = frameh + dy;
			pdrect = &drect;
			srect.extent.height = frameh + dy;
			psrect = &srect;
		}

		if (dy <= frameh)
		{
#ifdef USE_ADDITIVE_SCAN_BLIT
			arith_frame_blit (tintFrame[1], psrect, tintFrame[0], pdrect, 0, -1);
#else
			arith_frame_blit (tintFrame[1], psrect, tintFrame[0], pdrect, 0, 0);
#endif
		}
		s.frame = tintFrame[0];
		DrawStamp (&s);
	}
	UnbatchGraphics ();
}

