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

#include "planets/planets.h"
#include "setup.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "planets/scan.h"

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
extern void arith_frame_blit (FRAME srcFrame, RECT *rsrc, FRAME dstFrame,
		RECT *rdst, int num, int denom);


// RotatePlanet
// This will take care of zooming into a planet on orbit, generating the planet frames
// And applying the shield.

// The speed to zoom in.
#define PLANET_ZOOM_SPEED 2

PRECT
RotatePlanet (int x, int dx, int dy, COUNT scale_amt, UBYTE zoom_from, PRECT zoomr)
{
	STAMP s;
	FRAME pFrame[2];
	COUNT i, num_frames, old_scale;
	RECT *rp = NULL;
	CONTEXT OldContext;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;
	int base = GSCALE_IDENTITY;

	num_frames = 1;
	pFrame[0] = Orbit->PlanetFrameArray;
	if (Orbit->ObjectFrame)
	{
		pFrame[1] = Orbit->ObjectFrame;
		num_frames++;
	}

	if (scale_amt)
	{	
		if (zoomr->extent.width)
			rp = zoomr;
		//  we're zooming in, take care of scaling the frames
		//Translate the planet so it comes from the bottom right corner
		dx += ((zoom_from & 0x01) ? 1 : -1) * dx * (base - scale_amt) / base;
		dy += ((zoom_from & 0x02) ? 1 : -1) * dy * (base - scale_amt) / base;
	}

	//LockMutex (GraphicsLock);

	// PauseRotate needs to be checked twice.  It is first checked
	// at the rotate_planet_task function to bypass rendering the
	// planet (and thus slowinng down other parts of te code.  It
	// is checked here because it is possile that PauseRotate was
	// set between then and now, and we don't want too push
	// anything onto the DrawQueue in that case.  If the locking
	// operation is moved before the RenderLevelMasks call, one of
	// the two PauseRotate checks can be removed.

	//if (((PSOLARSYS_STATE volatile)pSolarSysState)->PauseRotate !=1)
	{
		OldContext = SetContext (SpaceContext);
		BatchGraphics ();
		if (rp)
			RepairBackRect (rp);
		s.origin.x = dx;
		s.origin.y = dy;
		old_scale = GetGraphicScale ();
		SetGraphicScale (scale_amt);
		for (i = 0; i < num_frames; i++)
		{
			s.frame = pFrame[i];
			DrawStamp (&s);
		}
		SetGraphicScale (old_scale);
		UnbatchGraphics ();
		SetContext (OldContext);
	}
	//UnlockMutex (GraphicsLock);
	if (scale_amt && scale_amt != base)
	{
		GetFrameRect (pFrame[num_frames - 1], zoomr);
		zoomr->corner.x += dx;
		zoomr->corner.y += dy;
		return zoomr;
	}
	return NULL;

	(void)x; // unused param
}

void
DrawPlanet (int x, int y, int dy, unsigned int rgb)
{
	STAMP s;
	UBYTE a = 128;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	s.origin.x = x;
	s.origin.y = y;
	s.frame = pSolarSysState->TopoFrame;
	BatchGraphics ();
	if (! rgb)
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

		if (rgb != pSolarSysState->Tint_rgb)
		{
			pSolarSysState->Tint_rgb = rgb;
			// Buffer the topoMap to the tintFrame;
			arith_frame_blit (s.frame, NULL, tintFrame[0], NULL, 0, 0);
			r = (rgb & (0x1f << 10)) >> 8;
			g = (rgb & (0x1f << 5)) >> 3;
			b = (rgb & 0x1f) << 2;
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

