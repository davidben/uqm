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

#include "build.h"
#include "controls.h"
#include "encount.h"
#include "gamestr.h"
#include "lifeform.h"
#include "nameref.h"
#include "options.h"
#include "resinst.h"
#include "settings.h"
#include "load.h"
#include "util.h"
#include "lander.h"
#include "setup.h"
#include "sounds.h"
#include "state.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "libs/mathlib.h"


extern FRAME SpaceJunkFrame;

// define SPIN_ON_SCAN to allow the planet to spin 
// while scaning  is going on
#undef SPIN_ON_SCAN

#define FLASH_INDEX 105
#define FLASH_WIDTH 9
#define FLASH_HEIGHT 9

CONTEXT ScanContext;

void
RepairBackRect (PRECT pRect)
{
	RECT new_r, old_r;

	GetContextClipRect (&old_r);
	new_r.corner.x = pRect->corner.x + old_r.corner.x;
	new_r.corner.y = pRect->corner.y + old_r.corner.y;
	new_r.extent = pRect->extent;

	new_r.extent.height += new_r.corner.y & 1;
	new_r.corner.y &= ~1;
	DrawFromExtraScreen (&new_r);
}

static void
EraseCoarseScan (void)
{
	RECT r, tr;
	const int leftScanWidth   = 80;
	const int rightScanWidth  = 80;
	const int leftScanOffset  = 5;
	const int rightScanOffset = 50;
	const int nameEraseWidth = SIS_SCREEN_WIDTH - 2;

	LockMutex (GraphicsLock);
	SetContext (SpaceContext);

	r.corner.x = (SIS_SCREEN_WIDTH >> 1) - (nameEraseWidth >> 1);
	r.corner.y = 13 - 10;
	r.extent.width = nameEraseWidth;
	r.extent.height = 14;
	RepairBackRect (&r);

	GetFrameRect (SetAbsFrameIndex (SpaceJunkFrame, 20), &tr);
	r = tr;
	r.corner.x += leftScanOffset;
	r.extent.width = leftScanWidth;
	RepairBackRect (&r);

	r = tr;
	r.corner.x += (r.extent.width - rightScanOffset);
	r.extent.width = rightScanWidth;
	RepairBackRect (&r);

	UnlockMutex (GraphicsLock);
}

static void
PrintScanTitlePC (TEXT *t, RECT *r, const char *txt, int xpos)
{
	t->baseline.x = xpos;
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x15), 0x3B));
	t->pStr = txt;
	t->CharCount = (COUNT)~0;
	font_DrawText (t);
	TextRect (t, r, NULL_PTR);
	t->baseline.x += r->extent.width;
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x0F, 0x00, 0x19), 0x3B));
}

static void
MakeScanValue (UNICODE *buf, long val, const UNICODE *extra)
{
	if (val >= 10 * 100)
	{	// 1 decimal place
		sprintf (buf, "%ld.%ld%s", val / 100, (val / 10) % 10, extra);
	}
	else
	{	// 2 decimal places
		sprintf (buf, "%ld.%02ld%s", val / 100, val % 100, extra);
	}
}

static void
PrintCoarseScanPC (void)
{
#define SCAN_LEADING_PC 14
	SDWORD val;
	TEXT t;
	RECT r;
	UNICODE buf[200];

	LockMutex (GraphicsLock);
	SetContext (SpaceContext);

	if (CurStarDescPtr->Index == SOL_DEFINED)
	{
		if (pSolarSysState->pOrbitalDesc->pPrevDesc ==
				&pSolarSysState->SunDesc[0])
			utf8StringCopy (buf, sizeof (buf), GLOBAL_SIS (PlanetName));
		else
		{
			switch (pSolarSysState->pOrbitalDesc->pPrevDesc
					- pSolarSysState->PlanetDesc)
			{
				case 2: /* EARTH */
					utf8StringCopy (buf, sizeof (buf),
							GAME_STRING (PLANET_NUMBER_BASE + 9));
					break;
				case 4: /* JUPITER */
					switch (pSolarSysState->pOrbitalDesc
							- pSolarSysState->MoonDesc)
					{
						case 0:
							utf8StringCopy (buf, sizeof (buf),
									GAME_STRING (PLANET_NUMBER_BASE + 10));
							break;
						case 1:
							utf8StringCopy (buf, sizeof (buf),
									GAME_STRING (PLANET_NUMBER_BASE + 11));
							break;
						case 2:
							utf8StringCopy (buf, sizeof (buf),
									GAME_STRING (PLANET_NUMBER_BASE + 12));
							break;
						case 3:
							utf8StringCopy (buf, sizeof (buf),
									GAME_STRING (PLANET_NUMBER_BASE + 13));
							break;
					}
					break;
				case 5: /* SATURN */
					utf8StringCopy (buf, sizeof (buf),
							GAME_STRING (PLANET_NUMBER_BASE + 14));
					break;
				case 7: /* NEPTUNE */
					utf8StringCopy (buf, sizeof (buf),
							GAME_STRING (PLANET_NUMBER_BASE + 15));
					break;
			}
		}
	}
	else
	{
		val = pSolarSysState->pOrbitalDesc->data_index & ~PLANET_SHIELDED;
		if (val >= FIRST_GAS_GIANT)
			sprintf (buf, "%s", GAME_STRING (SCAN_STRING_BASE + 4 + 51));
		else
			sprintf (buf, "%s %s",
					GAME_STRING (SCAN_STRING_BASE + 4 + val),
					GAME_STRING (SCAN_STRING_BASE + 4 + 50));
	}

	t.align = ALIGN_CENTER;
	t.baseline.x = SIS_SCREEN_WIDTH >> 1;
	t.baseline.y = 13;
	t.pStr = buf;
	t.CharCount = (COUNT)~0;

	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x15), 0x3B));
	SetContextFont (MicroFont);
	font_DrawText (&t);

	SetContextFont (TinyFont);
	UnlockMutex (GraphicsLock);

#define LEFT_SIDE_BASELINE_X_PC 5
#define RIGHT_SIDE_BASELINE_X_PC (SIS_SCREEN_WIDTH - 75)
#define SCAN_BASELINE_Y_PC 40

	t.baseline.y = SCAN_BASELINE_Y_PC;
	t.align = ALIGN_LEFT;

	LockMutex (GraphicsLock);
	PrintScanTitlePC (&t, &r, GAME_STRING (ORBITSCAN_STRING_BASE),
			LEFT_SIDE_BASELINE_X_PC); // "Orbit: "
	val = ((pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist * 100L
			+ (EARTH_RADIUS >> 1)) / EARTH_RADIUS);
	MakeScanValue (buf, val,
			GAME_STRING (ORBITSCAN_STRING_BASE + 1)); // " a.u."
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	PrintScanTitlePC (&t, &r, GAME_STRING (ORBITSCAN_STRING_BASE + 2),
			LEFT_SIDE_BASELINE_X_PC); // "Atmo: "
	if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == GAS_GIANT_ATMOSPHERE)
		utf8StringCopy (buf, sizeof (buf),
				GAME_STRING (ORBITSCAN_STRING_BASE + 3)); // "Super Thick"
	else if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == 0)
		utf8StringCopy (buf, sizeof (buf),
				GAME_STRING (ORBITSCAN_STRING_BASE + 4)); // "Vacuum"
	else
	{
		val = (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity * 100
			+ (EARTH_ATMOSPHERE >> 1)) / EARTH_ATMOSPHERE;
		MakeScanValue (buf, val,
				GAME_STRING (ORBITSCAN_STRING_BASE + 5)); // " atm"
	}
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	PrintScanTitlePC (&t, &r, GAME_STRING (ORBITSCAN_STRING_BASE + 6),
			LEFT_SIDE_BASELINE_X_PC); // "Temp: "
	sprintf (buf, "%d" STR_DEGREE_SIGN " c",
			pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature);
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	PrintScanTitlePC (&t, &r, GAME_STRING (ORBITSCAN_STRING_BASE + 7),
			LEFT_SIDE_BASELINE_X_PC); // "Weather: "
	if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == 0)
		t.pStr = GAME_STRING (ORBITSCAN_STRING_BASE + 8); // "None"
	else
	{
		sprintf (buf, "%s %u",
				GAME_STRING (ORBITSCAN_STRING_BASE + 9), // "Class"
				pSolarSysState->SysInfo.PlanetInfo.Weather + 1);
		t.pStr = buf;
	}
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	PrintScanTitlePC (&t, &r, GAME_STRING (ORBITSCAN_STRING_BASE + 10),
			LEFT_SIDE_BASELINE_X_PC); // "Tectonics: "
	if (PLANSIZE (pSolarSysState->SysInfo.PlanetInfo.PlanDataPtr->Type) ==
			GAS_GIANT)
		t.pStr = GAME_STRING (ORBITSCAN_STRING_BASE + 8); // "None"
	else
	{
		sprintf (buf, "%s %u",
				GAME_STRING (ORBITSCAN_STRING_BASE + 9), // "Class"
				pSolarSysState->SysInfo.PlanetInfo.Tectonics + 1);
		t.pStr = buf;
	}
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	UnlockMutex (GraphicsLock);

	t.baseline.y = SCAN_BASELINE_Y_PC;

	LockMutex (GraphicsLock);
	PrintScanTitlePC (&t, &r, GAME_STRING (ORBITSCAN_STRING_BASE + 11),
			RIGHT_SIDE_BASELINE_X_PC); // "Mass: "
	val = pSolarSysState->SysInfo.PlanetInfo.PlanetRadius;
	val = (val * val * val / 100L
			* pSolarSysState->SysInfo.PlanetInfo.PlanetDensity
			+ ((100L * 100L) >> 1)) / (100L * 100L);
	if (val == 0)
		val = 1;
	MakeScanValue (buf, val,
			GAME_STRING (ORBITSCAN_STRING_BASE + 12)); // " e.s."
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	PrintScanTitlePC (&t, &r, GAME_STRING (ORBITSCAN_STRING_BASE + 13),
			RIGHT_SIDE_BASELINE_X_PC); // "Radius: "
	val = pSolarSysState->SysInfo.PlanetInfo.PlanetRadius;
	MakeScanValue (buf, val,
			GAME_STRING (ORBITSCAN_STRING_BASE + 12)); // " e.s."
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	PrintScanTitlePC (&t, &r, GAME_STRING (ORBITSCAN_STRING_BASE + 14),
			RIGHT_SIDE_BASELINE_X_PC); // "Gravity: "
	val = pSolarSysState->SysInfo.PlanetInfo.SurfaceGravity;
	if (val == 0)
		val = 1;
	MakeScanValue (buf, val,
			GAME_STRING (ORBITSCAN_STRING_BASE + 15)); // " g."
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	PrintScanTitlePC (&t, &r, GAME_STRING (ORBITSCAN_STRING_BASE + 16),
			RIGHT_SIDE_BASELINE_X_PC); // "Day: "
	val = (SDWORD)pSolarSysState->SysInfo.PlanetInfo.RotationPeriod
			* 10 / 24;
	MakeScanValue (buf, val,
			GAME_STRING (ORBITSCAN_STRING_BASE + 17)); // " days"
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	PrintScanTitlePC (&t, &r, GAME_STRING (ORBITSCAN_STRING_BASE + 18),
			RIGHT_SIDE_BASELINE_X_PC); // "Tilt: "
	val = pSolarSysState->SysInfo.PlanetInfo.AxialTilt;
	if (val < 0)
		val = -val;
	t.pStr = buf;
	sprintf (buf, "%d" STR_DEGREE_SIGN, val);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	UnlockMutex (GraphicsLock);
}

static void
PrintCoarseScan3DO (void)
{
#define SCAN_LEADING 19
	SDWORD val;
	TEXT t;
	STAMP s;
	UNICODE buf[200];

	LockMutex (GraphicsLock);
	SetContext (SpaceContext);

	if (CurStarDescPtr->Index == SOL_DEFINED)
	{
		if (pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->SunDesc[0])
			utf8StringCopy (buf, sizeof (buf), GLOBAL_SIS (PlanetName));
		else
		{
			switch (pSolarSysState->pOrbitalDesc->pPrevDesc
					- pSolarSysState->PlanetDesc)
			{
				case 2: /* EARTH */
					utf8StringCopy (buf, sizeof (buf),
							GAME_STRING (PLANET_NUMBER_BASE + 9));
					break;
				case 4: /* JUPITER */
					switch (pSolarSysState->pOrbitalDesc
							- pSolarSysState->MoonDesc)
					{
						case 0:
							utf8StringCopy (buf, sizeof (buf),
									GAME_STRING (PLANET_NUMBER_BASE + 10));
							break;
						case 1:
							utf8StringCopy (buf, sizeof (buf),
									GAME_STRING (PLANET_NUMBER_BASE + 11));
							break;
						case 2:
							utf8StringCopy (buf, sizeof (buf),
									GAME_STRING (PLANET_NUMBER_BASE + 12));
							break;
						case 3:
							utf8StringCopy (buf, sizeof (buf),
									GAME_STRING (PLANET_NUMBER_BASE + 13));
							break;
					}
					break;
				case 5: /* SATURN */
					utf8StringCopy (buf, sizeof (buf),
							GAME_STRING (PLANET_NUMBER_BASE + 14));
					break;
				case 7: /* NEPTUNE */
					utf8StringCopy (buf, sizeof (buf),
							GAME_STRING (PLANET_NUMBER_BASE + 15));
					break;
			}
		}
	}
	else
	{
		val = pSolarSysState->pOrbitalDesc->data_index & ~PLANET_SHIELDED;
		if (val >= FIRST_GAS_GIANT)
			sprintf (buf, "%s", GAME_STRING (SCAN_STRING_BASE + 4 + 51));
		else
			sprintf (buf, "%s %s",
					GAME_STRING (SCAN_STRING_BASE + 4 + val),
					GAME_STRING (SCAN_STRING_BASE + 4 + 50));
	}

	t.align = ALIGN_CENTER;
	t.baseline.x = SIS_SCREEN_WIDTH >> 1;
	t.baseline.y = 13;
	t.pStr = buf;
	t.CharCount = (COUNT)~0;

	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x0F, 0x00, 0x19), 0x3B));
	SetContextFont (MicroFont);
	font_DrawText (&t);

	s.origin.x = s.origin.y = 0;
	s.origin.x = 16 - SAFE_X;
	s.frame = SetAbsFrameIndex (SpaceJunkFrame, 20);
	DrawStamp (&s);

	UnlockMutex (GraphicsLock);

#define LEFT_SIDE_BASELINE_X (27 + (16 - SAFE_X))
#define RIGHT_SIDE_BASELINE_X (SIS_SCREEN_WIDTH - LEFT_SIDE_BASELINE_X)
#define SCAN_BASELINE_Y 25

	t.baseline.x = LEFT_SIDE_BASELINE_X;
	t.baseline.y = SCAN_BASELINE_Y;
	t.align = ALIGN_LEFT;

	LockMutex (GraphicsLock);
	t.pStr = buf;
	val = ((pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist * 100L
			+ (EARTH_RADIUS >> 1)) / EARTH_RADIUS);
	MakeScanValue (buf, val, STR_EARTH_SIGN);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == GAS_GIANT_ATMOSPHERE)
		strcpy (buf, STR_INFINITY_SIGN);
	else
	{
		val = (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity * 100
				+ (EARTH_ATMOSPHERE >> 1)) / EARTH_ATMOSPHERE;
		MakeScanValue (buf, val, STR_EARTH_SIGN);
	}
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	sprintf (buf, "%d" STR_DEGREE_SIGN,
			pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	sprintf (buf, "<%u>", pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == 0
			? 0 : (pSolarSysState->SysInfo.PlanetInfo.Weather + 1));
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	sprintf (buf, "<%u>",
			PLANSIZE (
			pSolarSysState->SysInfo.PlanetInfo.PlanDataPtr->Type
			) == GAS_GIANT
			? 0 : (pSolarSysState->SysInfo.PlanetInfo.Tectonics + 1));
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	UnlockMutex (GraphicsLock);

	t.baseline.x = RIGHT_SIDE_BASELINE_X;
	t.baseline.y = SCAN_BASELINE_Y;
	t.align = ALIGN_RIGHT;

	LockMutex (GraphicsLock);
	t.pStr = buf;
	val = pSolarSysState->SysInfo.PlanetInfo.PlanetRadius;
	val = (val * val * val / 100L
			* pSolarSysState->SysInfo.PlanetInfo.PlanetDensity
			+ ((100L * 100L) >> 1)) / (100L * 100L);
	if (val == 0)
		val = 1;
	MakeScanValue (buf, val, STR_EARTH_SIGN);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	val = pSolarSysState->SysInfo.PlanetInfo.PlanetRadius;
	MakeScanValue (buf, val, STR_EARTH_SIGN);

	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	val = pSolarSysState->SysInfo.PlanetInfo.SurfaceGravity;
	if (val == 0)
		val = 1;
	MakeScanValue (buf, val, STR_EARTH_SIGN);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	val = pSolarSysState->SysInfo.PlanetInfo.AxialTilt;
	if (val < 0)
		val = -val;
	sprintf (buf, "%d" STR_DEGREE_SIGN, val);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	val = (SDWORD)pSolarSysState->SysInfo.PlanetInfo.RotationPeriod
			* 10 / 24;
	MakeScanValue (buf, val, STR_EARTH_SIGN);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	UnlockMutex (GraphicsLock);
}

static void
SetPlanetLoc (POINT new_pt)
{
	RECT r;
	STAMP s;

	pSolarSysState->MenuState.first_item = new_pt;
	new_pt.x >>= MAG_SHIFT;
	new_pt.y >>= MAG_SHIFT;

	LockMutex (GraphicsLock);
	SetContext (ScanContext);
	s.origin.x = pMenuState->flash_rect0.corner.x - (FLASH_WIDTH >> 1);
	s.origin.y = pMenuState->flash_rect0.corner.y - (FLASH_HEIGHT >> 1);
	s.frame = pMenuState->flash_frame0;
	DrawStamp (&s);

	pMenuState->flash_rect0.corner = new_pt;
	GetContextClipRect (&r);
	r.corner.x += pMenuState->flash_rect0.corner.x - (FLASH_WIDTH >> 1);
	r.corner.y += pMenuState->flash_rect0.corner.y - (FLASH_HEIGHT >> 1);
	r.extent.width = FLASH_WIDTH;
	r.extent.height = FLASH_HEIGHT;
	LoadDisplayPixmap (&r, pMenuState->flash_frame0);
	UnlockMutex (GraphicsLock);
	
	TaskSwitch ();
}

int
flash_planet_loc_func(void *data)
{
	DWORD TimeIn;
	BYTE c, val;
	PRIMITIVE p;
	Task task = (Task) data;

	SetPrimType (&p, STAMPFILL_PRIM);
	SetPrimNextLink (&p, END_OF_LIST);

	p.Object.Stamp.origin.x = p.Object.Stamp.origin.y = -1;
	p.Object.Stamp.frame = SetAbsFrameIndex (MiscDataFrame, FLASH_INDEX);
	c = 0x00;
	val = -0x02;
	TimeIn = 0;
	while (!Task_ReadState(task, TASK_EXIT))
	{
		DWORD T;
		CONTEXT OldContext;

		LockMutex (GraphicsLock);
		T = GetTimeCounter ();
		if (p.Object.Stamp.origin.x != pMenuState->flash_rect0.corner.x
				|| p.Object.Stamp.origin.y != pMenuState->flash_rect0.corner.y)
		{
			c = 0x00;
			val = -0x02;
		}
		else
		{
			if (T < TimeIn)
			{
				UnlockMutex (GraphicsLock);
				TaskSwitch ();

				continue;
			}

			if (c == 0x00 || c == 0x1A)
				val = -val;
			c += val;
		}
		p.Object.Stamp.origin = pMenuState->flash_rect0.corner;
		SetPrimColor (&p, BUILD_COLOR (MAKE_RGB15 (c, c, c), c));

		OldContext = SetContext (ScanContext);
		DrawBatch (&p, 0, 0);
		SetContext (OldContext);

		TimeIn = T + (ONE_SECOND >> 4);
		UnlockMutex (GraphicsLock);
		
		TaskSwitch ();
	}
	FinishTask (task);
	return(0);
}

static BOOLEAN DoScan (PMENU_STATE pMS);

static BOOLEAN
PickPlanetSide (PMENU_STATE pMS)
{
	BOOLEAN select, cancel;
	select = PulsedInputState.menu[KEY_MENU_SELECT];
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL];
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		if (pMenuState->flash_task)
		{
			ConcludeTask(pMenuState->flash_task);
			pMenuState->flash_task = 0;
		}
		goto ExitPlanetSide;
	}

	if (!pMS->Initialized)
	{
		pMS->InputFunc = PickPlanetSide;
		SetMenuRepeatDelay (0, 0, 0, FALSE);
		SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
		if (!select)
		{
			RECT r;

			pMS->Initialized = TRUE;

			LockMutex (GraphicsLock);
			SetContext (ScanContext);
			pMenuState->flash_rect0.corner.x =
					pSolarSysState->MenuState.first_item.x >> MAG_SHIFT;
			pMenuState->flash_rect0.corner.y =
					pSolarSysState->MenuState.first_item.y >> MAG_SHIFT;
			GetContextClipRect (&r);
			r.corner.x += pMenuState->flash_rect0.corner.x - (FLASH_WIDTH >> 1);
			r.corner.y += pMenuState->flash_rect0.corner.y - (FLASH_HEIGHT >> 1);
			r.extent.width = FLASH_WIDTH;
			r.extent.height = FLASH_HEIGHT;
			LoadDisplayPixmap (&r, pMenuState->flash_frame0);

			SetFlashRect (NULL_PTR, (FRAME)0);
			UnlockMutex (GraphicsLock);

			InitLander (0);

			pMenuState->flash_task = AssignTask (flash_planet_loc_func,
					2048, "flash planet location");
		}
	}
	else if (select || cancel)
	{
		STAMP s;

		SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

		LockMutex (GraphicsLock);
		DrawStatusMessage (NULL_PTR);
		UnlockMutex (GraphicsLock);

		FlushInput ();
		if (pMenuState->flash_task)
		{
			ConcludeTask (pMenuState->flash_task);
			pMenuState->flash_task = 0;
		}

		if (!select)
			SetPlanetLoc (pSolarSysState->MenuState.first_item);
		else
		{
			COUNT fuel_required;

			fuel_required = (COUNT)(
					pSolarSysState->SysInfo.PlanetInfo.SurfaceGravity << 1
					);
			if (fuel_required > 3 * FUEL_TANK_SCALE)
				fuel_required = 3 * FUEL_TANK_SCALE;

			EraseCoarseScan ();

			LockMutex (GraphicsLock);
			DeltaSISGauges (0, -(SIZE)fuel_required, 0);
			SetContext (ScanContext);
			s.origin = pMenuState->flash_rect0.corner;
			s.frame = SetAbsFrameIndex (MiscDataFrame, FLASH_INDEX);
			DrawStamp (&s);
			UnlockMutex (GraphicsLock);

			PlanetSide (pMS);
			if (GLOBAL (CurrentActivity) & CHECK_ABORT)
				goto ExitPlanetSide;

			if (GET_GAME_STATE (FOUND_PLUTO_SPATHI) == 1)
			{
				HSTARSHIP hStarShip;

				if (pMenuState->flash_task)
				{
					ConcludeTask (pMenuState->flash_task);
					pMenuState->flash_task = 0;
				}

				NextActivity |= CHECK_LOAD; /* fake a load game */
				GLOBAL (CurrentActivity) |= START_ENCOUNTER;

				EncounterGroup = 0;
				PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
				ReinitQueue (&GLOBAL (npc_built_ship_q));

				hStarShip = CloneShipFragment (SPATHI_SHIP,
						&GLOBAL (npc_built_ship_q), 1);
				if (hStarShip)
				{
					BYTE captains_name_index;
					COUNT which_player;
					STARSHIPPTR StarShipPtr;

					StarShipPtr = LockStarShip (
							&GLOBAL (npc_built_ship_q), hStarShip
							);
					which_player = StarShipPlayer (StarShipPtr);
					captains_name_index = NAME_OFFSET + NUM_CAPTAINS_NAMES;
					OwnStarShip (StarShipPtr, which_player, captains_name_index);
					UnlockStarShip (
							&GLOBAL (npc_built_ship_q), hStarShip
							);
				}

				SaveFlagshipState ();
				return (FALSE);
			}

			if (optWhichCoarseScan == OPT_PC)
				PrintCoarseScanPC ();
			else
				PrintCoarseScan3DO ();
		}

		DrawMenuStateStrings (PM_MIN_SCAN, DISPATCH_SHUTTLE);
		LockMutex (GraphicsLock);
		SetFlashRect (SFR_MENU_3DO, (FRAME)0);
		UnlockMutex (GraphicsLock);

ExitPlanetSide:
		SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

		pMS->InputFunc = DoScan;
		pMS->CurState = DISPATCH_SHUTTLE;
		SetDefaultMenuRepeatDelay ();
	}
	else
	{
		SIZE dx = 0, dy = 0;
		POINT new_pt;

		new_pt = pSolarSysState->MenuState.first_item;

		if (PulsedInputState.menu[KEY_MENU_LEFT]) dx = -1;
		if (PulsedInputState.menu[KEY_MENU_RIGHT]) dx = 1;
		if (PulsedInputState.menu[KEY_MENU_UP]) dy = -1;
		if (PulsedInputState.menu[KEY_MENU_DOWN]) dy = 1;

		dx = dx << MAG_SHIFT;
		if (dx)
		{
			new_pt.x += dx;
			if (new_pt.x < 0)
				new_pt.x += (MAP_WIDTH << MAG_SHIFT);
			else if (new_pt.x >= (MAP_WIDTH << MAG_SHIFT))
				new_pt.x -= (MAP_WIDTH << MAG_SHIFT);
		}
		dy = dy << MAG_SHIFT;
		if (dy)
		{
			new_pt.y += dy;
			if (new_pt.y < 0 || new_pt.y >= (MAP_HEIGHT << MAG_SHIFT))
				new_pt.y = pSolarSysState->MenuState.first_item.y;
		}

		if (new_pt.x != pSolarSysState->MenuState.first_item.x
				|| new_pt.y != pSolarSysState->MenuState.first_item.y)
		{
			DWORD TimeIn;

			TimeIn = GetTimeCounter ();
			SetPlanetLoc (new_pt);
			SleepThreadUntil (TimeIn + ONE_SECOND / 40);
		}
	}

	return (TRUE);
}

#define NUM_FLASH_COLORS 8

static void
DrawScannedStuff (COUNT y, BYTE CurState)
{
	HELEMENT hElement, hNextElement;
	COLOR OldColor;

	OldColor = SetContextForeGroundColor (0);

	for (hElement = GetHeadElement (); hElement; hElement = hNextElement)
	{
		ELEMENTPTR ElementPtr;
		//COLOR OldColor;
		SIZE dy;
		
		LockElement (hElement, &ElementPtr);
		hNextElement = GetSuccElement (ElementPtr);

		dy = y - ElementPtr->current.location.y;
		if (LOBYTE (ElementPtr->life_span) == CurState
				&& dy >= 0)// && dy <= 3)
		{
			COUNT i;
			//DWORD Time;
			STAMP s;

			ElementPtr->state_flags |= APPEARING;

			s.origin = ElementPtr->current.location;
			s.frame = ElementPtr->current.image.frame;
			
			if (dy >= NUM_FLASH_COLORS)
			{
				i = (COUNT)(GetFrameIndex (ElementPtr->next.image.frame)
						- GetFrameIndex (ElementPtr->current.image.frame)
						+ 1);
				do
				{
					DrawStamp (&s);
					s.frame = IncFrameIndex (s.frame);
				} while (--i);
			}
			else
			{
				BYTE r, g, b;
				COLOR c;
				
				// mineral -- white --> turquoise?? (contrasts with red)
				// energy -- white --> red (contrasts with white)
				// bio -- white --> violet (contrasts with green)
				b = (BYTE)(0x1f - 0x1f * dy / (NUM_FLASH_COLORS - 1));
				switch (CurState)
				{
					case (MINERAL_SCAN):
						r = b;
						g = 0x1f;
						b = 0x1f;
						break;
					case (ENERGY_SCAN):
						r = 0x1f;
						g = b;
						break;
					case (BIOLOGICAL_SCAN):
						r = 0x1f;
						g = b;
						b = 0x1f;
						break;
				}
				
				c = BUILD_COLOR (MAKE_RGB15 (r, g, b), 0);
								
				SetContextForeGroundColor (c);
				i = (COUNT)(GetFrameIndex (ElementPtr->next.image.frame)
						- GetFrameIndex (ElementPtr->current.image.frame)
						+ 1);
				do
				{
					DrawFilledStamp (&s);
					s.frame = IncFrameIndex (s.frame);
				} while (--i);
			}
		}

		UnlockElement (hElement);
	}
	
	SetContextForeGroundColor (OldColor);
}

static BOOLEAN
DoScan (PMENU_STATE pMS)
{
	DWORD TimeIn, WaitTime;
	BOOLEAN select, cancel;

	select = PulsedInputState.menu[KEY_MENU_SELECT];
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL];
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	if (!pMS->Initialized)
	{
		pMS->Initialized = TRUE;
		pMS->InputFunc = DoScan;
	}
	else if (cancel || (select && pMS->CurState == EXIT_SCAN))
	{
		LockMutex (GraphicsLock);
		SetContext (SpaceContext);
		BatchGraphics ();
		DrawPlanet (SIS_SCREEN_WIDTH - MAP_WIDTH, SIS_SCREEN_HEIGHT - MAP_HEIGHT, 0, 0);
		UnbatchGraphics ();
		UnlockMutex (GraphicsLock);

		EraseCoarseScan ();
// DrawMenuStateStrings (PM_SCAN, SCAN);

		return (FALSE);
	}
	else if (select)
	{
		BYTE min_scan, max_scan;
		RECT r;
		BOOLEAN PressState, ButtonState;

		if (pMS->CurState == DISPATCH_SHUTTLE)
		{
			COUNT fuel_required;
			UNICODE buf[100];

			if ((pSolarSysState->pOrbitalDesc->data_index & PLANET_SHIELDED)
					|| (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity ==
						GAS_GIANT_ATMOSPHERE))
			{	// cannot dispatch to shielded planets or gas giants
				PlayMenuSound (MENU_SOUND_FAILURE);
				return (TRUE);
			}

			fuel_required = (COUNT)(
					pSolarSysState->SysInfo.PlanetInfo.SurfaceGravity << 1
					);
			if (fuel_required > 3 * FUEL_TANK_SCALE)
				fuel_required = 3 * FUEL_TANK_SCALE;

			if (GLOBAL_SIS (FuelOnBoard) < (DWORD)fuel_required
					|| GLOBAL_SIS (NumLanders) == 0
					|| GLOBAL_SIS (CrewEnlisted) == 0)
			{
				PlayMenuSound (MENU_SOUND_FAILURE);
				return (TRUE);
			}

			sprintf (buf, "%s%1.1f",
					GAME_STRING (NAVIGATION_STRING_BASE + 5),
					(float) fuel_required / FUEL_TANK_SCALE);
			LockMutex (GraphicsLock);
			ClearSISRect (CLEAR_SIS_RADAR);
			DrawStatusMessage (buf);
			UnlockMutex (GraphicsLock);

			LockMutex (GraphicsLock);
			SetContext (ScanContext);
			BatchGraphics ();
			DrawPlanet (0, 0, 0, 0);
			DrawScannedObjects (FALSE);
			UnbatchGraphics ();
			UnlockMutex (GraphicsLock);
		
			pMS->Initialized = FALSE;
			pMS->CurFrame = 0;
			return PickPlanetSide (pMS);
		}

		// Various scans
		if (pSolarSysState->pOrbitalDesc->data_index & PLANET_SHIELDED)
		{	// cannot scan shielded planets
			PlayMenuSound (MENU_SOUND_FAILURE);
			return (TRUE);
		}

		pSolarSysState->MenuState.Initialized += 4;
#ifndef SPIN_ON_SCAN
		pSolarSysState->PauseRotate = 1;
#endif
	
		min_scan = pMS->CurState;
		if (min_scan != AUTO_SCAN)
			max_scan = min_scan;
		else
		{
			min_scan = MINERAL_SCAN;
			max_scan = BIOLOGICAL_SCAN;
		}

		do
		{
			TEXT t;
			SWORD i;

			t.baseline.x = SIS_SCREEN_WIDTH >> 1;
			t.baseline.y = SIS_SCREEN_HEIGHT - MAP_HEIGHT - 7;
			t.align = ALIGN_CENTER;
			t.CharCount = (COUNT)~0;

			pSolarSysState->CurNode = (COUNT)~0;
			(*pSolarSysState->GenFunc) ((BYTE)(min_scan + GENERATE_MINERAL));
			pMS->delta_item = (SIZE)pSolarSysState->CurNode;
			t.pStr = GAME_STRING (SCAN_STRING_BASE + min_scan);

			LockMutex (GraphicsLock);
			SetContext (SpaceContext);
			r.corner.x = 0;
			r.corner.y = t.baseline.y - 10;
			r.extent.width = SIS_SCREEN_WIDTH;
			r.extent.height = t.baseline.y - r.corner.y + 1;
			RepairBackRect (&r);

			SetContextFont (MicroFont);
			switch (min_scan)
			{
				case MINERAL_SCAN:
					SetContextForeGroundColor (
							BUILD_COLOR (MAKE_RGB15 (0x13, 0x00, 0x00), 0x2C));
					break;
				case ENERGY_SCAN:
					SetContextForeGroundColor (
							BUILD_COLOR (MAKE_RGB15 (0x0C, 0x0C, 0x0C), 0x1C));
					break;
				case BIOLOGICAL_SCAN:
					SetContextForeGroundColor (
							BUILD_COLOR (MAKE_RGB15 (0x00, 0x0E, 0x00), 0x6C));
					break;
			}
			font_DrawText (&t);

			SetContext (ScanContext);
			UnlockMutex (GraphicsLock);

			{
				DWORD rgb;
				
				switch (min_scan)
				{
					case MINERAL_SCAN:
						rgb = MAKE_RGB15 (0x1f, 0x00, 0x00);
						break;
					case ENERGY_SCAN:
						rgb = MAKE_RGB15 (0x1f, 0x1f, 0x1f);
						break;
					case BIOLOGICAL_SCAN:
						rgb = MAKE_RGB15 (0x00, 0x1f, 0x00);
						break;
				}

				LockMutex (GraphicsLock);
				BatchGraphics ();
				DrawPlanet (0, 0, 0, 0);
				UnbatchGraphics ();
				UnlockMutex (GraphicsLock);

				PressState = AnyButtonPress (TRUE);
				WaitTime = (ONE_SECOND << 1) / MAP_HEIGHT;
//				LockMutex (GraphicsLock);
				TimeIn = GetTimeCounter ();
				for (i = 0; i < MAP_HEIGHT + NUM_FLASH_COLORS + 1; i++)
				{
					ButtonState = AnyButtonPress (TRUE);
					if (PressState)
					{
						PressState = ButtonState;
						ButtonState = FALSE;
					}
					if (ButtonState)
						i = -i;
					LockMutex (GraphicsLock);
					BatchGraphics ();
					DrawPlanet (0, 0, i, rgb);
					if (i < 0)
						i = MAP_HEIGHT + NUM_FLASH_COLORS;
					if (pMS->delta_item)
						DrawScannedStuff (i, min_scan);
					UnbatchGraphics ();
					UnlockMutex (GraphicsLock);
//					FlushGraphics ();
					SleepThreadUntil (TimeIn + WaitTime);
					TimeIn = GetTimeCounter ();
				}
//				UnlockMutex (GraphicsLock);
				pSolarSysState->Tint_rgb = 0;

			}

		} while (++min_scan <= max_scan);

		LockMutex (GraphicsLock);
		SetContext (SpaceContext);
		r.corner.x = 0;
		r.corner.y = (SIS_SCREEN_HEIGHT - MAP_HEIGHT - 7) - 10;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = (SIS_SCREEN_HEIGHT - MAP_HEIGHT - 7)
				- r.corner.y + 1;
		RepairBackRect (&r);

		SetContext (ScanContext);
		if (pMS->CurState == AUTO_SCAN)
		{
			DrawPlanet (0, 0, 0, 0);
			DrawScannedObjects (FALSE);
			UnlockMutex (GraphicsLock);

			pMS->CurState = DISPATCH_SHUTTLE;
			DrawMenuStateStrings (PM_MIN_SCAN, pMS->CurState);
		}
		else
			UnlockMutex (GraphicsLock);
			
		pSolarSysState->MenuState.Initialized -= 4;
		pSolarSysState->PauseRotate = 0;
		FlushInput ();
	}
	else if (optWhichMenu == OPT_PC ||
			(!(pSolarSysState->pOrbitalDesc->data_index & PLANET_SHIELDED)
			&& pSolarSysState->SysInfo.PlanetInfo.AtmoDensity !=
				GAS_GIANT_ATMOSPHERE))
		DoMenuChooser (pMS, PM_MIN_SCAN);

	return (TRUE);
}

void
ScanSystem (void)
{
	RECT r;
	MENU_STATE MenuState;

	MenuState.InputFunc = DoScan;
	MenuState.Initialized = FALSE;
	MenuState.flash_task = 0;

{
#define REPAIR_SCAN (1 << 6)
	extern BYTE draw_sys_flags;
	
	if (draw_sys_flags & REPAIR_SCAN)
	{
		RECT r;

		r.corner.x = SIS_ORG_X;
		r.corner.y = SIS_ORG_Y;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = SIS_SCREEN_HEIGHT;
		LoadIntoExtraScreen (&r);
		draw_sys_flags &= ~REPAIR_SCAN;
	}
}

	if (optWhichMenu == OPT_3DO &&
			((pSolarSysState->pOrbitalDesc->data_index & PLANET_SHIELDED)
			|| pSolarSysState->SysInfo.PlanetInfo.AtmoDensity ==
				GAS_GIANT_ATMOSPHERE))
	{
		MenuState.CurState = EXIT_SCAN;
		ScanContext = 0;
	}
	else
	{
		MenuState.CurState = AUTO_SCAN;
		pSolarSysState->MenuState.first_item.x =
				(MAP_WIDTH >> 1) << MAG_SHIFT;
		pSolarSysState->MenuState.first_item.y =
				(MAP_HEIGHT >> 1) << MAG_SHIFT;

		LockMutex (GraphicsLock);
		ScanContext = CaptureContext (CreateContext ());
		SetContext (ScanContext);
		MenuState.flash_rect0.extent.width = FLASH_WIDTH;
		MenuState.flash_rect0.extent.height = FLASH_HEIGHT;
		MenuState.flash_frame0 = CaptureDrawable (
				CreateDrawable (WANT_PIXMAP | MAPPED_TO_DISPLAY,
				FLASH_WIDTH, FLASH_HEIGHT, 1));
		SetContextFGFrame (Screen);
		r.corner.x = (SIS_ORG_X + SIS_SCREEN_WIDTH) - MAP_WIDTH;
		r.corner.y = (SIS_ORG_Y + SIS_SCREEN_HEIGHT) - MAP_HEIGHT;
		r.extent.width = MAP_WIDTH;
		r.extent.height = MAP_HEIGHT;
		SetContextClipRect (&r);
		DrawScannedObjects (FALSE);
		UnlockMutex (GraphicsLock);
	}

	DrawMenuStateStrings (PM_MIN_SCAN, MenuState.CurState);

	if (optWhichCoarseScan == OPT_PC)
		PrintCoarseScanPC ();
	else
		PrintCoarseScan3DO ();

	pMenuState = &MenuState;
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	DoInput ((PVOID)&MenuState, FALSE);
	pMenuState = 0;

	if (ScanContext)
	{
		LockMutex (GraphicsLock);
		SetContext (SpaceContext);
		DestroyDrawable (ReleaseDrawable (MenuState.flash_frame0));
		DestroyContext (ReleaseContext (ScanContext));
		ScanContext = 0;
		UnlockMutex (GraphicsLock);
	}
}

void
GeneratePlanetSide (void)
{
	SIZE scan;
	BYTE life_init_tab[MAX_LIFE_VARIATION];

	InitDisplayList ();
	if (pSolarSysState->pOrbitalDesc->data_index & PLANET_SHIELDED)
		return;

	memset (life_init_tab, 0, sizeof (life_init_tab));
	for (scan = BIOLOGICAL_SCAN; scan >= MINERAL_SCAN; --scan)
	{
		COUNT num_nodes;
		FRAME f;

		f = SetAbsFrameIndex (MiscDataFrame,
				NUM_SCANDOT_TRANSITIONS * (scan - ENERGY_SCAN));

		pSolarSysState->CurNode = (COUNT)~0;
		(*pSolarSysState->GenFunc) ((BYTE)(scan + GENERATE_MINERAL));

		num_nodes = pSolarSysState->CurNode;
		while (num_nodes--)
		{
			HELEMENT hNodeElement;
			ELEMENTPTR NodeElementPtr;

			if (pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[scan]
					& (1L << num_nodes))
				continue;

			hNodeElement = AllocElement ();
			if (!hNodeElement)
				continue;

			LockElement (hNodeElement, &NodeElementPtr);

			pSolarSysState->CurNode = num_nodes;
			(*pSolarSysState->GenFunc) ((BYTE)(scan + GENERATE_MINERAL));

			NodeElementPtr->life_span = MAKE_WORD (scan, num_nodes + 1);
			NodeElementPtr->state_flags = BAD_GUY;
			NodeElementPtr->current.location.x =
					pSolarSysState->SysInfo.PlanetInfo.CurPt.x;
			NodeElementPtr->current.location.y =
					pSolarSysState->SysInfo.PlanetInfo.CurPt.y;

			SetPrimType (&DisplayArray[NodeElementPtr->PrimIndex], STAMP_PRIM);
			if (scan == MINERAL_SCAN)
			{
				COUNT EType;

				EType = pSolarSysState->SysInfo.PlanetInfo.CurType;
				NodeElementPtr->turn_wait = (BYTE)EType;
				NodeElementPtr->mass_points = HIBYTE (
						pSolarSysState->SysInfo.PlanetInfo.CurDensity);
				NodeElementPtr->current.image.frame = SetAbsFrameIndex (
						MiscDataFrame, (NUM_SCANDOT_TRANSITIONS << 1)
						+ ElementCategory (EType) * 5);
				NodeElementPtr->next.image.frame = SetRelFrameIndex (
						NodeElementPtr->current.image.frame, LOBYTE (
						pSolarSysState->SysInfo.PlanetInfo.CurDensity) + 1);
				DisplayArray[NodeElementPtr->PrimIndex].Object.Stamp.frame =
						IncFrameIndex (NodeElementPtr->next.image.frame);
			}
			else
			{
				extern void object_animation (PELEMENT ElementPtr);

				NodeElementPtr->current.image.frame = f;
				NodeElementPtr->next.image.frame = SetRelFrameIndex (
						f, NUM_SCANDOT_TRANSITIONS - 1);
				NodeElementPtr->turn_wait = MAKE_BYTE (4, 4);
				NodeElementPtr->preprocess_func = object_animation;
				if (scan == ENERGY_SCAN)
				{
					if (pSolarSysState->SysInfo.PlanetInfo.CurType == 1)
						NodeElementPtr->mass_points = 0;
					else if (pSolarSysState->SysInfo.PlanetInfo.CurType == 2)
						NodeElementPtr->mass_points = 1;
					else
						NodeElementPtr->mass_points = MAX_SCROUNGED;
					DisplayArray[NodeElementPtr->PrimIndex].Object.Stamp.frame =
							pSolarSysState->PlanetSideFrame[1];
				}
				else
				{
					COUNT i, which_node;

					which_node = pSolarSysState->SysInfo.PlanetInfo.CurType;

					if (CreatureData[which_node].Attributes & SPEED_MASK)
					{
						i = (COUNT)TFB_Random ();
						NodeElementPtr->current.location.x =
								(LOBYTE (i) % (MAP_WIDTH - (8 << 1))) + 8;
						NodeElementPtr->current.location.y =
								(HIBYTE (i) % (MAP_HEIGHT - (8 << 1))) + 8;
					}

					if (pSolarSysState->PlanetSideFrame[0] == 0)
						pSolarSysState->PlanetSideFrame[0] =
								CaptureDrawable (LoadGraphic (
								CANNISTER_MASK_PMAP_ANIM));
					for (i = 0; i < MAX_LIFE_VARIATION
							&& life_init_tab[i] != (BYTE)(which_node + 1);
							++i)
					{
						if (life_init_tab[i] != 0)
							continue;

						life_init_tab[i] = (BYTE)which_node + 1;

						pSolarSysState->PlanetSideFrame[i + 3] =
								CaptureDrawable (LoadGraphic (
								MAKE_RESOURCE (
								GET_PACKAGE (LIFE00_MASK_PMAP_ANIM)
								+ which_node, GFXRES,
								GET_INSTANCE (LIFE00_MASK_PMAP_ANIM)
								+ which_node)));

						break;
					}

					NodeElementPtr->mass_points = (BYTE)which_node;
					NodeElementPtr->hit_points = HINIBBLE (
							CreatureData[which_node].ValueAndHitPoints);
					DisplayArray[NodeElementPtr->PrimIndex].
							Object.Stamp.frame = SetAbsFrameIndex (
							pSolarSysState->PlanetSideFrame[i + 3],
							(COUNT)TFB_Random ());
				}
			}

			NodeElementPtr->next.location.x =
					NodeElementPtr->current.location.x << MAG_SHIFT;
			NodeElementPtr->next.location.y =
					NodeElementPtr->current.location.y << MAG_SHIFT;
			UnlockElement (hNodeElement);

			PutElement (hNodeElement);
		}
	}
}


