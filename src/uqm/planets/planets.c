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

#include "scan.h"
#include "lander.h"
#include "../colors.h"
#include "../element.h"
#include "../settings.h"
#include "../controls.h"
#include "../sounds.h"
#include "../gameopt.h"
#include "../shipcont.h"
#include "../setup.h"
#include "../uqmdebug.h"
#include "../resinst.h"
#include "../nameref.h"
#include "options.h"
#include "libs/graphics/gfx_common.h"


extern int rotate_planet_task (void *data);


void
DrawScannedObjects (BOOLEAN Reversed)
{
	HELEMENT hElement, hNextElement;

	for (hElement = Reversed ? GetTailElement () : GetHeadElement ();
			hElement; hElement = hNextElement)
	{
		ELEMENT *ElementPtr;

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

void
DrawPlanetSurfaceBorder (void)
{
#define BORDER_HEIGHT  5
	CONTEXT oldContext;
	RECT oldClipRect;
	RECT clipRect;
	RECT r;

	oldContext = SetContext (SpaceContext);
	GetContextClipRect (&oldClipRect);

	// Expand the context clip-rect so that we can tweak the existing border
	clipRect = oldClipRect;
	clipRect.corner.x -= 1;
	clipRect.extent.width += 2;
	clipRect.extent.height += 1;
	SetContextClipRect (&clipRect);

	BatchGraphics ();

	// Border bulk
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08));
	r.corner.x = 0;
	r.corner.y = clipRect.extent.height - MAP_HEIGHT - BORDER_HEIGHT;
	r.extent.width = clipRect.extent.width;
	r.extent.height = BORDER_HEIGHT - 2;
	DrawFilledRectangle (&r);

	SetContextForeGroundColor (SIS_BOTTOM_RIGHT_BORDER_COLOR);
	
	// Border top shadow line
	r.extent.width -= 1;
	r.extent.height = 1;
	r.corner.x = 1;
	r.corner.y -= 1;
	DrawFilledRectangle (&r);
	
	// XXX: We will need bulk left and right rects here if MAP_WIDTH changes

	// Right shadow line
	r.extent.width = 1;
	r.extent.height = MAP_HEIGHT + 2;
	r.corner.y += BORDER_HEIGHT - 1;
	r.corner.x = clipRect.extent.width - 1;
	DrawFilledRectangle (&r);

	SetContextForeGroundColor (SIS_LEFT_BORDER_COLOR);
	
	// Left shadow line
	r.corner.x -= MAP_WIDTH + 1;
	DrawFilledRectangle (&r);

	// Border bottom shadow line
	r.extent.width = MAP_WIDTH + 2;
	r.extent.height = 1;
	DrawFilledRectangle (&r);
	
	UnbatchGraphics ();

	SetContextClipRect (&oldClipRect);
	SetContext (oldContext);
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
		DrawSISMessage (NULL);
		DrawSISTitle (GLOBAL_SIS (PlanetName));
		DrawStarBackGround ();
		DrawPlanetSurfaceBorder ();
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
				SIS_SCREEN_HEIGHT - MAP_HEIGHT, 0, BLACK_COLOR);
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
// Called from the GenerateFunctions.generateOribital() function
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
		// This means the call to LoadPlanet is made from some
		// GenerateFunctions.generateOribital() function.
		PLANET_DESC *pPlanetDesc;

		StopMusic ();

		TaskContext = CreateContext ("TaskContext");

		pPlanetDesc = pSolarSysState->pOrbitalDesc;

		GeneratePlanetSurface (pPlanetDesc, SurfDefFrame);
		SetPlanetMusic (pPlanetDesc->data_index & ~PLANET_SHIELDED);

		if (pPlanetDesc->pPrevDesc != &pSolarSysState->SunDesc[0])
			pPlanetDesc = pPlanetDesc->pPrevDesc;

		GeneratePlanetSide ();
	}

	LockMutex (GraphicsLock);
	DrawOrbitalDisplay (WaitMode ? DRAW_ORBITAL_UPDATE : DRAW_ORBITAL_FULL);
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
	pSolarSysState->Tint_rgb = BLACK_COLOR;

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

	DestroyContext (TaskContext);
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
	info->LanderFont = LoadFont (LANDER_FONT);
	info->LanderFontEff = CaptureDrawable (
			LoadGraphic (LANDER_FONTEFF_PMAP_ANIM));
}

void
FreeLanderFont (PLANET_INFO *info)
{
	DestroyFont (info->LanderFont);
	info->LanderFont = NULL;
	DestroyDrawable (ReleaseDrawable (info->LanderFontEff));
	info->LanderFontEff = NULL;
}

static BOOLEAN
DoPlanetOrbit (MENU_STATE *pMS)
{
	BOOLEAN select = PulsedInputState.menu[KEY_MENU_SELECT];
	BOOLEAN handled;

	if ((GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
			|| GLOBAL_SIS (CrewEnlisted) == (COUNT)~0)
		return FALSE;

	// XXX: pMS actually refers to pSolarSysState->MenuState
	handled = DoMenuChooser (pMS, PM_SCAN);
	if (handled)
		return TRUE;

	if (!select)
		return TRUE;

	LockMutex (GraphicsLock);
	SetFlashRect (NULL);
	UnlockMutex (GraphicsLock);

	switch (pMS->CurState)
	{
		case SCAN:
			ScanSystem ();
			break;
		case EQUIP_DEVICE:
			select = DevicesMenu ();
			if (GLOBAL (CurrentActivity) & START_ENCOUNTER)
			{	// Invoked Talking Pet, a Caster or Sun Device over Chmmr,
				// or a Caster for Ilwrath
				// Going into conversation
				return FALSE;
			}
			break;
		case CARGO:
			CargoMenu ();
			break;
		case ROSTER:
			select = RosterMenu ();
			break;
		case GAME_MENU:
			if (!GameOptions ())
				return FALSE; // abort or load
			break;
		case STARMAP:
		{
			BOOLEAN AutoPilotSet;

			LockMutex (GraphicsLock);
			pSolarSysState->PauseRotate = 1;
			RepairSISBorder ();
			UnlockMutex (GraphicsLock);
			TaskSwitch ();

			AutoPilotSet = StarMap ();

			if (GLOBAL (CurrentActivity) & CHECK_ABORT)
				return FALSE;

			if (!AutoPilotSet)
			{	// Redraw the orbital display
				LoadPlanet (NULL);
				LockMutex (GraphicsLock);
				pSolarSysState->PauseRotate = 0;
				UnlockMutex (GraphicsLock);
				break;
			}
			// Fall through !!!
		}
		case NAVIGATION:
			return FALSE;
	}

	if (!(GLOBAL (CurrentActivity) & CHECK_ABORT))
	{
		if (select)
		{	// 3DO menu jumps to NAVIGATE after a successful submenu run
			if (optWhichMenu != OPT_PC)
				pMS->CurState = NAVIGATION;
			DrawMenuStateStrings (PM_SCAN, pMS->CurState);
		}
		LockMutex (GraphicsLock);
		SetFlashRect (SFR_MENU_3DO);
		UnlockMutex (GraphicsLock);
	}

	return TRUE;
}

void
PlanetOrbitMenu (void)
{
	void *oldInputFunc = pSolarSysState->MenuState.InputFunc;

	DrawMenuStateStrings (PM_SCAN, SCAN);
	LockMutex (GraphicsLock);
	SetFlashRect (SFR_MENU_3DO);
	UnlockMutex (GraphicsLock);

	pSolarSysState->MenuState.CurState = SCAN;
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	// XXX: temporary; will have an own MENU_STATE
	pSolarSysState->MenuState.InputFunc = DoPlanetOrbit;
	DoInput (&pSolarSysState->MenuState, TRUE);
	pSolarSysState->MenuState.InputFunc = oldInputFunc;

	LockMutex (GraphicsLock);
	SetFlashRect (NULL);
	UnlockMutex (GraphicsLock);
	DrawMenuStateStrings (PM_STARMAP, -NAVIGATION);
}
