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
#include "settings.h"
#include "setup.h"
#include "uqmdebug.h"
#include "libs/graphics/gfx_common.h"


//Added by Chris

void
DrawPlanet(int x, int y, int dy, unsigned int rgb);

//End Added by Chris

extern void SetPlanetMusic (BYTE planet_type);
extern int rotate_planet_task (PVOID Blah);

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

// Initialise the surface graphics, and start the planet music.
// Called from the GENERATE_ORBITAL case of an IP generation function
// (when orbit is entered; either from IP, or from loading a saved game)
// and when "starmap" is selected from orbit and then cancelled.
// IsDefined is true only when the planet comes with its own bitmap,
// namely for Earth.
void
LoadPlanet (BOOLEAN IsDefined)
{
	STAMP s;

#ifdef DEBUG
	if (disableInteractivity)
		return;
#endif
	
	LockMutex (GraphicsLock);

	BatchGraphics ();
	SetTransitionSource (NULL);
	if (!(LastActivity & CHECK_LOAD))
		DrawStarBackGround (TRUE);

	if (pSolarSysState->MenuState.flash_task == 0)
	{
		// The "rotate planets" task is not initialised yet.
		// This means the call to LoadPlanet is made from a
		// GENERATE_ORBITAL case of an IP generation function.
		PPLANET_DESC pPlanetDesc;
		extern void GeneratePlanetSide (void);

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

		{
			extern void GeneratePlanetMask (PPLANET_DESC
					pPlanetDesc, BOOLEAN IsEarth);

			GeneratePlanetMask (pPlanetDesc, IsDefined);
		}
		SetPlanetMusic ((UBYTE)(pPlanetDesc->data_index & ~PLANET_SHIELDED));

		if (pPlanetDesc->pPrevDesc != &pSolarSysState->SunDesc[0])
			pPlanetDesc = pPlanetDesc->pPrevDesc;

		GeneratePlanetSide ();
	}

	if (LastActivity & CHECK_LOAD)
	{
		if (LOBYTE (LastActivity) == 0)
		{
			UnlockMutex (GraphicsLock);
			DrawSISFrame ();
			LockMutex (GraphicsLock);
		}
		else
		{
			ClearSISRect (DRAW_SIS_DISPLAY);
			RepairSISBorder ();
		}
		DrawSISMessage (NULL_PTR);
		DrawSISTitle (GLOBAL_SIS (PlanetName));
	}

	SetContext (SpaceContext);

	s.frame = pSolarSysState->PlanetSideFrame[2];
	if (s.frame)
	{
		s.origin.x = SIS_SCREEN_WIDTH >> 1;
		s.origin.y = ((116 - SIS_ORG_Y) >> 1) + 2;
		DrawStamp (&s);
	}

	if (!(LastActivity & CHECK_LOAD))
	{
		RECT r;

		BatchGraphics ();
		DrawPlanet (SIS_SCREEN_WIDTH - MAP_WIDTH, SIS_SCREEN_HEIGHT - MAP_HEIGHT, 0, 0);
		UnbatchGraphics ();

		r.corner.x = SIS_ORG_X;
		r.corner.y = SIS_ORG_Y;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = SIS_SCREEN_HEIGHT;
		ScreenTransition (3, &r);
		UnbatchGraphics ();
		LoadIntoExtraScreen (&r);
	}
	
	UnlockMutex (GraphicsLock);

	if (!PLRPlaying ((MUSIC_REF)~0))
	{
		extern MUSIC_REF LanderMusic;
			
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

	if (LastActivity & CHECK_LOAD)
	{
		RECT r;
		CONTEXT oldContext;

		r.corner.x = SIS_ORG_X;
		r.corner.y = SIS_ORG_Y;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = SIS_SCREEN_HEIGHT;
		LockMutex (GraphicsLock);
		oldContext = SetContext (SpaceContext);
		DrawStarBackGround (TRUE);
		SetContext (oldContext);
		BatchGraphics();
		DrawPlanet (SIS_SCREEN_WIDTH - MAP_WIDTH, SIS_SCREEN_HEIGHT - MAP_HEIGHT, 0, 0);
		UnbatchGraphics();
		ScreenTransition (3, &r);  // How does this work?
		UnbatchGraphics ();
		LoadIntoExtraScreen (&r);
		UnlockMutex (GraphicsLock);
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

	HFree (Orbit->isPFADefined);
	Orbit->isPFADefined = 0;
	DestroyDrawable (ReleaseDrawable (Orbit->TintFrame));
	Orbit->TintFrame = 0;
	pSolarSysState->Tint_rgb = 0;
	DestroyDrawable (ReleaseDrawable (Orbit->ShieldFrame));
	Orbit->ShieldFrame = 0;

	if (Orbit->lpTopoMap!=0)
	{
		HFree (Orbit->lpTopoMap);
		Orbit->lpTopoMap = 0;
	}

	DestroyContext (ReleaseContext (TaskContext));
	TaskContext = 0;

	DestroyStringTable (ReleaseStringTable (
			pSolarSysState->SysInfo.PlanetInfo.DiscoveryString
			));
	pSolarSysState->SysInfo.PlanetInfo.DiscoveryString = 0;
	DestroyFont (ReleaseFont (
			pSolarSysState->SysInfo.PlanetInfo.LanderFont
			));
	pSolarSysState->SysInfo.PlanetInfo.LanderFont = 0;
	pSolarSysState->PauseRotate = 0;

	UnlockMutex (GraphicsLock);
}

