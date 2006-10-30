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

#include "element.h"
#include "scan.h"
#include "lander.h"
#include "settings.h"
#include "setup.h"
#include "uqmdebug.h"
#include "libs/graphics/gfx_common.h"
#include "resinst.h"
#include "nameref.h"


extern int rotate_planet_task (PVOID data);


void
DrawScannedObjects (BOOLEAN Reversed)
{
	HELEMENT hElement, hNextElement;

	for (hElement = Reversed ? GetTailElement () : GetHeadElement ();
			hElement; hElement = hNextElement)
	{
		ELEMENTPTR ElementPtr;

		LockElement (hElement, &ElementPtr);
		hNextElement = Reversed ?
				GetPredElement (ElementPtr) :
				GetSuccElement (ElementPtr);

		if (ElementPtr->state_flags & APPEARING)
		{
			STAMP s;

			s.origin = ElementPtr->current.location;
			s.frame = ElementPtr->next.image.frame;
			DrawStamp (&s);
		}

		UnlockElement (hElement);
	}
}

typedef enum
{
	DRAW_ORBITAL_FULL,
	DRAW_ORBITAL_WAIT,
	DRAW_ORBITAL_UPDATE,

} DRAW_ORBITAL_MODE;

static void
DrawOrbitalDisplay (DRAW_ORBITAL_MODE Mode)
{
	RECT r = { { SIS_ORG_X, SIS_ORG_Y }, 
				{ SIS_SCREEN_WIDTH, SIS_SCREEN_HEIGHT } };

	BatchGraphics ();
	
	if (Mode != DRAW_ORBITAL_UPDATE)
	{
		SetTransitionSource (NULL);

		DrawSISFrame ();
		DrawSISMessage (NULL_PTR);
		DrawSISTitle (GLOBAL_SIS (PlanetName));
		DrawStarBackGround (TRUE);
	}

	SetContext (SpaceContext);
	
	if (Mode == DRAW_ORBITAL_WAIT)
	{
		STAMP s;

		s.frame = CaptureDrawable (
				LoadGraphic (ORBENTER_PMAP_ANIM));
		s.origin.x = -SAFE_X;
		s.origin.y = SIS_SCREEN_HEIGHT - MAP_HEIGHT;
		DrawStamp (&s);
		DestroyDrawable (ReleaseDrawable (s.frame));
	}
	else
	{
		DrawPlanet (SIS_SCREEN_WIDTH - MAP_WIDTH,
				SIS_SCREEN_HEIGHT - MAP_HEIGHT, 0, 0);
	}

	if (Mode != DRAW_ORBITAL_UPDATE)
	{
		ScreenTransition (3, &r);
	}

	UnbatchGraphics ();

	if (Mode != DRAW_ORBITAL_WAIT)
	{
		LoadIntoExtraScreen (&r);
	}
}

// Initialise the surface graphics, and start the planet music.
// Called from the GENERATE_ORBITAL case of an IP generation function
// (when orbit is entered; either from IP, or from loading a saved game)
// and when "starmap" is selected from orbit and then cancelled;
// also after in-orbit comm and after defeating planet guards in combat.
// SurfDefFrame contains surface definition images when a planet comes
// with its own bitmap (currently only for Earth)
void
LoadPlanet (FRAME SurfDefFrame)
{
	BOOLEAN WaitMode;

#ifdef DEBUG
	if (disableInteractivity)
		return;
#endif

	WaitMode = !(LastActivity & CHECK_LOAD) &&
			(pSolarSysState->MenuState.Initialized <= 2);

	if (WaitMode)
	{
		LockMutex (GraphicsLock);
		DrawOrbitalDisplay (DRAW_ORBITAL_WAIT);
		UnlockMutex (GraphicsLock);
	}

	if (pSolarSysState->MenuState.flash_task == 0)
	{
		// The "rotate planets" task is not initialised yet.
		// This means the call to LoadPlanet is made from a
		// GENERATE_ORBITAL case of an IP generation function.
		PPLANET_DESC pPlanetDesc;

		StopMusic ();

		TaskContext = CaptureContext (CreateContext ());

		pPlanetDesc = pSolarSysState->pOrbitalDesc;


		/* 
		if (pPlanetDesc->data_index & PLANET_SHIELDED)
			pSolarSysState->PlanetSideFrame[2] = CaptureDrawable (
					LoadGraphic (PLANET_SHIELDED_MASK_PMAP_ANIM)
					);
		else if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity != GAS_GIANT_ATMOSPHERE)
			LoadLanderData ();
		*/

		GeneratePlanetMask (pPlanetDesc, SurfDefFrame);
		SetPlanetMusic ((UBYTE)(pPlanetDesc->data_index & ~PLANET_SHIELDED));

		if (pPlanetDesc->pPrevDesc != &pSolarSysState->SunDesc[0])
			pPlanetDesc = pPlanetDesc->pPrevDesc;

		GeneratePlanetSide ();
	}

	LockMutex (GraphicsLock);
	DrawOrbitalDisplay (WaitMode ? DRAW_ORBITAL_UPDATE : DRAW_ORBITAL_FULL);
#if 0
	// this used to draw the static slave shield graphic
	SetContext (SpaceContext);
	s.frame = pSolarSysState->PlanetSideFrame[2];
	if (s.frame)
	{
		s.origin.x = SIS_SCREEN_WIDTH >> 1;
		s.origin.y = ((116 - SIS_ORG_Y) >> 1) + 2;
		DrawStamp (&s);
	}
#endif
	UnlockMutex (GraphicsLock);

	if (!PLRPlaying ((MUSIC_REF)~0))
		PlayMusic (LanderMusic, TRUE, 1);

	if (pSolarSysState->MenuState.flash_task == 0)
	{
		pSolarSysState->MenuState.flash_task =
				AssignTask (rotate_planet_task, 4096,
				"rotate planets");

		while (pSolarSysState->MenuState.Initialized == 2)
			TaskSwitch ();
	}
}

void
FreePlanet (void)
{
	COUNT i;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	if (pSolarSysState->MenuState.flash_task)
	{
		ConcludeTask (pSolarSysState->MenuState.flash_task);
//		Task_SetState (pSolarSysState->MenuState.flash_task, TASK_EXIT);
		pSolarSysState->MenuState.flash_task = 0;
	}

	StopMusic ();
	LockMutex (GraphicsLock);

	for (i = 0; i < sizeof (pSolarSysState->PlanetSideFrame)
			/ sizeof (pSolarSysState->PlanetSideFrame[0]); ++i)
	{
		DestroyDrawable (ReleaseDrawable (pSolarSysState->PlanetSideFrame[i]));
		pSolarSysState->PlanetSideFrame[i] = 0;
	}

//    FreeLanderData ();

	DestroyStringTable (ReleaseStringTable (pSolarSysState->XlatRef));
	pSolarSysState->XlatRef = 0;
	DestroyDrawable (ReleaseDrawable (pSolarSysState->TopoFrame));
	pSolarSysState->TopoFrame = 0;
	DestroyColorMap (ReleaseColorMap (pSolarSysState->OrbitalCMap));
	pSolarSysState->OrbitalCMap = 0;

	HFree (Orbit->lpTopoData);
	Orbit->lpTopoData = 0;
	DestroyDrawable (ReleaseDrawable (Orbit->TopoZoomFrame));
	Orbit->TopoZoomFrame = 0;
	DestroyDrawable (ReleaseDrawable (Orbit->PlanetFrameArray));
	Orbit->PlanetFrameArray = 0;

	DestroyDrawable (ReleaseDrawable (Orbit->TintFrame));
	Orbit->TintFrame = 0;
	pSolarSysState->Tint_rgb = 0;

	DestroyDrawable (ReleaseDrawable (Orbit->ObjectFrame));
	Orbit->ObjectFrame = 0;
	DestroyDrawable (ReleaseDrawable (Orbit->WorkFrame));
	Orbit->WorkFrame = 0;

	if (Orbit->lpTopoMap != 0)
	{
		HFree (Orbit->lpTopoMap);
		Orbit->lpTopoMap = 0;
	}
	if (Orbit->ScratchArray != 0)
	{
		HFree (Orbit->ScratchArray);
		Orbit->ScratchArray = 0;
	}

	DestroyContext (ReleaseContext (TaskContext));
	TaskContext = 0;
	
	DestroyStringTable (ReleaseStringTable (
			pSolarSysState->SysInfo.PlanetInfo.DiscoveryString
			));
	pSolarSysState->SysInfo.PlanetInfo.DiscoveryString = 0;
	FreeLanderFont (&pSolarSysState->SysInfo.PlanetInfo);
	pSolarSysState->PauseRotate = 0;

	UnlockMutex (GraphicsLock);
}

void
LoadStdLanderFont (PLANET_INFO *info)
{
	info->LanderFont = CaptureFont (LoadFont (LANDER_FONT));
	info->LanderFontEff = CaptureDrawable (
			LoadGraphic (LANDER_FONTEFF_PMAP_ANIM));
}

void
FreeLanderFont (PLANET_INFO *info)
{
	DestroyFont (ReleaseFont (info->LanderFont));
	info->LanderFont = 0;
	DestroyDrawable (ReleaseDrawable (info->LanderFontEff));
	info->LanderFontEff = 0;
}
