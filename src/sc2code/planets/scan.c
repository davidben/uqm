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
#include "lifeform.h"
#include "libs/graphics/gfx_common.h"
#include "options.h"
#include "controls.h"
//Added by Chris

void
DrawPlanet(int x, int y, int dy, unsigned int rgb);

void WaitForNoInput (SIZE Duration);

//End Added by Chris

// define SPIN_ON_SCAN to allow the planet to spin 
// while scaning  is going on
#undef SPIN_ON_SCAN

#define FLASH_INDEX 105
#define FLASH_WIDTH 9
#define FLASH_HEIGHT 9
extern FRAME misc_data;

extern void DrawScannedObjects (BOOLEAN Reversed);

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
	extern FRAME SpaceJunkFrame;
	const int leftScanWidth   = 80;
	const int rightScanWidth  = 80;
	const int leftScanOffset  = 5;
	const int rightScanOffset = 50;

	LockMutex (GraphicsLock);
	SetContext (SpaceContext);

	r.corner.x = (SIS_SCREEN_WIDTH >> 1) - (160 >> 1);
	r.corner.y = 13 - 7;
	r.extent.width = 160;
	r.extent.height = 10;
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
PrintScanTitlePC (TEXT *t, RECT *r, char *txt, int xpos)
{
	t->baseline.x = xpos;
	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x15), 0x3B));
	wsprintf (t->pStr, txt);
	t->CharCount = (COUNT)~0;
	font_DrawText (t);
	TextRect (t, r, NULL_PTR);
	t->baseline.x += r->extent.width;
	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0xF, 0x00, 0x19), 0x3B));
}

static void
PrintCoarseScanPC (void)
{
#define SCAN_LEADING_PC 14
	SIZE temp;
	TEXT t;
	RECT r;
	UNICODE buf[40];

	LockMutex (GraphicsLock);
	SetContext (SpaceContext);

	if (CurStarDescPtr->Index == SOL_DEFINED)
	{
		if (pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->SunDesc[0])
			wstrcpy (buf, GLOBAL_SIS (PlanetName));
		else
		{
			switch (pSolarSysState->pOrbitalDesc->pPrevDesc
					- pSolarSysState->PlanetDesc)
			{
				case 2: /* EARTH */
					wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 9));
					break;
				case 4: /* JUPITER */
					switch (pSolarSysState->pOrbitalDesc
							- pSolarSysState->MoonDesc)
					{
						case 0:
							wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 10));
							break;
						case 1:
							wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 11));
							break;
						case 2:
							wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 12));
							break;
						case 3:
							wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 13));
							break;
					}
					break;
				case 5:
					wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 14));
					break;
				case 7:
					wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 15));
					break;
			}
		}
	}
	else
	{
		temp = pSolarSysState->pOrbitalDesc->data_index & ~PLANET_SHIELDED;
		if (temp >= FIRST_GAS_GIANT)
			wsprintf (buf, "%s", GAME_STRING (SCAN_STRING_BASE + 4 + 51));
		else
			wsprintf (buf, "%s %s",
					GAME_STRING (SCAN_STRING_BASE + 4 + temp),
					GAME_STRING (SCAN_STRING_BASE + 4 + 50));
	}

	t.align = ALIGN_CENTER;
	t.baseline.x = SIS_SCREEN_WIDTH >> 1;
	t.baseline.y = 13;
	t.pStr = buf;
	t.CharCount = (COUNT)~0;

	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x15), 0x3B));
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
	t.pStr = buf;
	PrintScanTitlePC (&t, &r, "Orbit: ", LEFT_SIDE_BASELINE_X_PC);
	temp = (SIZE)((pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist * 100L
			+ (EARTH_RADIUS >> 1)) / EARTH_RADIUS);
	if (temp >= 10 * 100)
		wsprintf (buf, "%u.%u a.u.", temp / 100, (temp / 10) % 10);
	else
		wsprintf (buf, "%u.%02u a.u.", temp / 100, temp % 100);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	PrintScanTitlePC (&t, &r, "Atmo: ", LEFT_SIDE_BASELINE_X_PC);
	if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == GAS_GIANT_ATMOSPHERE)
		wsprintf (buf, "Super Thick");
	else if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == 0)
		wsprintf (buf, "Vacuum");
	else
	{
		temp = (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity * 100
			+ (EARTH_ATMOSPHERE >> 1)) / EARTH_ATMOSPHERE;
		if (temp >= 10 * 100)
			wsprintf (buf, "%u.%u atm", temp / 100, (temp / 10) % 10);
		else
			wsprintf (buf, "%u.%02u atm", temp / 100, temp % 100);
	}
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	PrintScanTitlePC (&t, &r, "Temp: ", LEFT_SIDE_BASELINE_X_PC);
	wsprintf (buf, "%d^ c", pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	PrintScanTitlePC (&t, &r, "Weather: ", LEFT_SIDE_BASELINE_X_PC);
	if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == 0)
		wsprintf (buf, "None");
	else
		wsprintf (buf, "Class %u", pSolarSysState->SysInfo.PlanetInfo.Weather + 1);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	PrintScanTitlePC (&t, &r, "Tectonics: ", LEFT_SIDE_BASELINE_X_PC);
	if (PLANSIZE (pSolarSysState->SysInfo.PlanetInfo.PlanDataPtr->Type) == GAS_GIANT)
		wsprintf (buf, "None");
	else
	wsprintf (buf, "Class %u", pSolarSysState->SysInfo.PlanetInfo.Tectonics + 1);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	UnlockMutex (GraphicsLock);

	t.baseline.y = SCAN_BASELINE_Y_PC;

	LockMutex (GraphicsLock);
	t.pStr = buf;
	PrintScanTitlePC (&t, &r, "Mass: ", RIGHT_SIDE_BASELINE_X_PC);
	{
		DWORD tr;

		tr = pSolarSysState->SysInfo.PlanetInfo.PlanetRadius;
		if ((tr = (tr * tr * tr / 100L
				* (DWORD)pSolarSysState->SysInfo.PlanetInfo.PlanetDensity
				+ (DWORD)((100L * 100L) >> 1))
				/ (DWORD)(100L * 100L)) == 0)
			tr = 1;
		if (tr >= 10 * 100)
			wsprintf (buf, "%lu.%lu e.s.", tr / 100, (tr / 10) % 10);
		else
			wsprintf (buf, "%lu.%02lu e.s.", tr / 100, tr % 100);
	}
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	PrintScanTitlePC (&t, &r, "Radius: ", RIGHT_SIDE_BASELINE_X_PC);
	if ((temp = pSolarSysState->SysInfo.PlanetInfo.PlanetRadius) >= 10 * 100)
		wsprintf (buf, "%u.%u e.s.", temp / 100, (temp / 10) % 10);
	else
		wsprintf (buf, "%u.%02u e.s.", temp / 100, temp % 100);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	PrintScanTitlePC (&t, &r, "Gravity: ", RIGHT_SIDE_BASELINE_X_PC);
	if ((temp = pSolarSysState->SysInfo.PlanetInfo.SurfaceGravity) >= 10 * 100)
		wsprintf (buf, "%u.%u g.", temp / 100, (temp / 10) % 10);
	else
	{
		if (temp == 0)
			temp = 1;
		wsprintf (buf, "%u.%02u g.", temp / 100, temp % 100);
	}
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	PrintScanTitlePC (&t, &r, "Day: ", RIGHT_SIDE_BASELINE_X_PC);
	if (pSolarSysState->SysInfo.PlanetInfo.RotationPeriod < 240 * 10)
	{
		temp = (SIZE)(pSolarSysState->SysInfo.PlanetInfo.RotationPeriod * 10 / 24);
		wsprintf (buf, "%u.%02u days", temp / 100, temp % 100);
	}
	else
	{
		temp = (SIZE)((pSolarSysState->SysInfo.PlanetInfo.RotationPeriod
				+ (24 >> 1)) / 24);
		wsprintf (buf, "%u.%u days", temp / 10, temp % 10);
	}
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING_PC;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	PrintScanTitlePC (&t, &r, "Tilt: ", RIGHT_SIDE_BASELINE_X_PC);
	if ((temp = pSolarSysState->SysInfo.PlanetInfo.AxialTilt) < 0)
		temp = -temp;
	wsprintf (buf, "%u^", temp);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	UnlockMutex (GraphicsLock);
}

static void
PrintCoarseScan3DO (void)
{
#define SCAN_LEADING 19
	SIZE temp;
	TEXT t;
	STAMP s;
	UNICODE buf[40];
	extern FRAME SpaceJunkFrame;

	LockMutex (GraphicsLock);
	SetContext (SpaceContext);

	if (CurStarDescPtr->Index == SOL_DEFINED)
	{
		if (pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->SunDesc[0])
			wstrcpy (buf, GLOBAL_SIS (PlanetName));
		else
		{
			switch (pSolarSysState->pOrbitalDesc->pPrevDesc
					- pSolarSysState->PlanetDesc)
			{
				case 2: /* EARTH */
					wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 9));
					break;
				case 4: /* JUPITER */
					switch (pSolarSysState->pOrbitalDesc
							- pSolarSysState->MoonDesc)
					{
						case 0:
							wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 10));
							break;
						case 1:
							wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 11));
							break;
						case 2:
							wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 12));
							break;
						case 3:
							wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 13));
							break;
					}
					break;
				case 5:
					wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 14));
					break;
				case 7:
					wstrcpy (buf, GAME_STRING (PLANET_NUMBER_BASE + 15));
					break;
			}
		}
	}
	else
	{
		temp = pSolarSysState->pOrbitalDesc->data_index & ~PLANET_SHIELDED;
		if (temp >= FIRST_GAS_GIANT)
			wsprintf (buf, "%s", GAME_STRING (SCAN_STRING_BASE + 4 + 51));
		else
			wsprintf (buf, "%s %s",
					GAME_STRING (SCAN_STRING_BASE + 4 + temp),
					GAME_STRING (SCAN_STRING_BASE + 4 + 50));
	}

	t.align = ALIGN_CENTER;
	t.baseline.x = SIS_SCREEN_WIDTH >> 1;
	t.baseline.y = 13;
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0xF, 0x00, 0x19), 0x3B));
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
	temp = (SIZE)((pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist * 100L
			+ (EARTH_RADIUS >> 1)) / EARTH_RADIUS);
	if (temp >= 10 * 100)
		wsprintf (buf, "%u.%u&", temp / 100, (temp / 10) % 10);
	else
		wsprintf (buf, "%u.%02u&", temp / 100, temp % 100);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == GAS_GIANT_ATMOSPHERE)
		wsprintf (buf, "\x7f");
	else
	{
		temp = (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity * 100
				+ (EARTH_ATMOSPHERE >> 1)) / EARTH_ATMOSPHERE;
		if (temp >= 10 * 100)
			wsprintf (buf, "%u.%u&", temp / 100, (temp / 10) % 10);
		else
			wsprintf (buf, "%u.%02u&", temp / 100, temp % 100);
	}
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	wsprintf (buf, "%d^", pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	wsprintf (buf, "<%u>", pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == 0
			? 0 : (pSolarSysState->SysInfo.PlanetInfo.Weather + 1));
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	wsprintf (buf, "<%u>",
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
	{
		DWORD tr;

		tr = pSolarSysState->SysInfo.PlanetInfo.PlanetRadius;
		if ((tr = (tr * tr * tr / 100L
				* (DWORD)pSolarSysState->SysInfo.PlanetInfo.PlanetDensity
				+ (DWORD)((100L * 100L) >> 1))
				/ (DWORD)(100L * 100L)) == 0)
			tr = 1;
		if (tr >= 10 * 100)
			wsprintf (buf, "%lu.%lu&", tr / 100, (tr / 10) % 10);
		else
			wsprintf (buf, "%lu.%02lu&", tr / 100, tr % 100);
	}
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	if ((temp = pSolarSysState->SysInfo.PlanetInfo.PlanetRadius) >= 10 * 100)
		wsprintf (buf, "%u.%u&", temp / 100, (temp / 10) % 10);
	else
		wsprintf (buf, "%u.%02u&", temp / 100, temp % 100);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	if ((temp = pSolarSysState->SysInfo.PlanetInfo.SurfaceGravity) >= 10 * 100)
		wsprintf (buf, "%u.%u&", temp / 100, (temp / 10) % 10);
	else
	{
		if (temp == 0)
			temp = 1;
		wsprintf (buf, "%u.%02u&", temp / 100, temp % 100);
	}
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	if ((temp = pSolarSysState->SysInfo.PlanetInfo.AxialTilt) < 0)
		temp = -temp;
	wsprintf (buf, "%u^", temp);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	UnlockMutex (GraphicsLock);

	LockMutex (GraphicsLock);
	t.pStr = buf;
	if (pSolarSysState->SysInfo.PlanetInfo.RotationPeriod < 240 * 10)
	{
		temp = (SIZE)(pSolarSysState->SysInfo.PlanetInfo.RotationPeriod * 10 / 24);
		wsprintf (buf, "%u.%02u&", temp / 100, temp % 100);
	}
	else
	{
		temp = (SIZE)((pSolarSysState->SysInfo.PlanetInfo.RotationPeriod
				+ (24 >> 1)) / 24);
		wsprintf (buf, "%u.%u&", temp / 10, temp % 10);
	}
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
	p.Object.Stamp.frame = SetAbsFrameIndex (misc_data, FLASH_INDEX);
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
	select = CurrentMenuState.select;
	cancel = CurrentMenuState.cancel;
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
			extern void PlanetSide (PMENU_STATE pMS);

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
			s.frame = SetAbsFrameIndex (misc_data, FLASH_INDEX);
			DrawStamp (&s);
			UnlockMutex (GraphicsLock);

			PlanetSide (pMS);
			if (GLOBAL (CurrentActivity) & CHECK_ABORT)
				goto ExitPlanetSide;

			if (GET_GAME_STATE (FOUND_PLUTO_SPATHI) == 1)
			{
				HSTARSHIP hStarShip;
				extern ACTIVITY NextActivity;
				extern void SaveFlagshipState (void);

				if (pMenuState->flash_task)
				{
					ConcludeTask (pMenuState->flash_task);
					pMenuState->flash_task = 0;
				}

				NextActivity |= CHECK_LOAD; /* fake a load game */
				GLOBAL (CurrentActivity) |= START_ENCOUNTER;

				battle_counter = 0;
				PutGroupInfo (0L, (BYTE)~0);
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
		SetFlashRect ((PRECT)~0L, (FRAME)0);
		UnlockMutex (GraphicsLock);

ExitPlanetSide:
		SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

		pMS->InputFunc = DoScan;
		pMS->CurState = DISPATCH_SHUTTLE;
		SetDefaultMenuRepeatDelay ();
		return (FALSE);
	}
	else
	{
		SIZE dx = 0, dy = 0;
		POINT new_pt;

		new_pt = pSolarSysState->MenuState.first_item;

		if (CurrentMenuState.left) dx = -1;
		if (CurrentMenuState.right) dx = 1;
		if (CurrentMenuState.up) dy = -1;
		if (CurrentMenuState.down) dy = 1;

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

	for (hElement = GetHeadElement ();
			hElement; hElement = hNextElement)
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
	select = CurrentMenuState.select;
	cancel = CurrentMenuState.cancel;
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
			UNICODE buf[20];

			fuel_required = (COUNT)(
					pSolarSysState->SysInfo.PlanetInfo.SurfaceGravity << 1
					);
			if (fuel_required > 3 * FUEL_TANK_SCALE)
				fuel_required = 3 * FUEL_TANK_SCALE;

			if (GLOBAL_SIS (FuelOnBoard) < (DWORD)fuel_required
					|| GLOBAL_SIS (NumLanders) == 0
					|| GLOBAL_SIS (CrewEnlisted) == 0)
			{
				PlaySoundEffect (SetAbsSoundIndex (MenuSounds, 2),
						0, NotPositional (), NULL, GAME_SOUND_PRIORITY);
				return (TRUE);
			}

			wsprintf (buf, "%s%u.%u",
					GAME_STRING (NAVIGATION_STRING_BASE + 5),
					fuel_required / FUEL_TANK_SCALE,
					((fuel_required % FUEL_TANK_SCALE) + 5) / 10);
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

		pSolarSysState->MenuState.Initialized += 4;
#ifndef SPIN_ON_SCAN
		pSolarSysState->PauseRotate = 1;
#endif
		if ((min_scan = pMS->CurState) != AUTO_SCAN)
			max_scan = min_scan;
		else
			min_scan = MINERAL_SCAN, max_scan = BIOLOGICAL_SCAN;

		do
		{
		TEXT t;
		SWORD i;

		t.baseline.x = SIS_SCREEN_WIDTH >> 1;
		t.baseline.y = SIS_SCREEN_HEIGHT - MAP_HEIGHT - 7;
		t.align = ALIGN_CENTER;
		t.CharCount = (COUNT)~0;

		pSolarSysState->CurNode = (COUNT)~0;
		(*pSolarSysState->GenFunc) (
				(BYTE)(min_scan + GENERATE_MINERAL)
				);
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
						BUILD_COLOR (MAKE_RGB15 (0x13, 0x00, 0x00), 0x2C)
						);
				break;
			case ENERGY_SCAN:
				SetContextForeGroundColor (
						BUILD_COLOR (MAKE_RGB15 (0xC, 0xC, 0xC), 0x1C)
						);
				break;
			case BIOLOGICAL_SCAN:
				SetContextForeGroundColor (
						BUILD_COLOR (MAKE_RGB15 (0x00, 0xE, 0x00), 0x6C)
						);
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
				rgb = 0x1f << 10;
				break;
			case ENERGY_SCAN:
				rgb = (0x1f << 10) | (0x1f << 5) | 0x1f;
				break;
			case BIOLOGICAL_SCAN:
				rgb = 0x1f << 5;
				break;
		}

		LockMutex (GraphicsLock);
		BatchGraphics ();
		DrawPlanet (0, 0, 0, 0);
		UnbatchGraphics ();
		UnlockMutex (GraphicsLock);

		PressState = AnyButtonPress (TRUE);
		WaitTime = (ONE_SECOND << 1) / MAP_HEIGHT;
//		LockMutex (GraphicsLock);
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
//			FlushGraphics ();
			SleepThreadUntil (TimeIn + WaitTime);
			TimeIn = GetTimeCounter ();
		}
//		UnlockMutex (GraphicsLock);
		pSolarSysState->Tint_rgb = 0;

	}

		} while (++min_scan <= max_scan);

		LockMutex (GraphicsLock);
		SetContext (SpaceContext);
		r.corner.x = 0;
		r.corner.y = (SIS_SCREEN_HEIGHT - MAP_HEIGHT - 7) - 10;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = (SIS_SCREEN_HEIGHT - MAP_HEIGHT - 7) - r.corner.y + 1;
		RepairBackRect (&r);

		SetContext (ScanContext);
		if (pMS->CurState == AUTO_SCAN)
		{
			DrawPlanet (0, 0, 0, 0);
			DrawScannedObjects (FALSE);
			UnlockMutex (GraphicsLock);

			DrawMenuStateStrings (PM_MIN_SCAN, pMS->CurState = DISPATCH_SHUTTLE);
		}
		else
			UnlockMutex (GraphicsLock);
			
		pSolarSysState->MenuState.Initialized -= 4;
		pSolarSysState->PauseRotate = 0;
		FlushInput ();
	}
	else if (!(pSolarSysState->pOrbitalDesc->data_index & PLANET_SHIELDED)
			&& pSolarSysState->SysInfo.PlanetInfo.AtmoDensity !=
			GAS_GIANT_ATMOSPHERE)
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

	if ((pSolarSysState->pOrbitalDesc->data_index & PLANET_SHIELDED)
			|| pSolarSysState->SysInfo.PlanetInfo.AtmoDensity ==
			GAS_GIANT_ATMOSPHERE)
	{
		MenuState.CurState = EXIT_SCAN;
		ScanContext = 0;
	}
	else
	{
		MenuState.CurState = AUTO_SCAN;
		pSolarSysState->MenuState.first_item.x = (MAP_WIDTH >> 1) << MAG_SHIFT;
		pSolarSysState->MenuState.first_item.y = (MAP_HEIGHT >> 1) << MAG_SHIFT;

		LockMutex (GraphicsLock);
		ScanContext = CaptureContext (CreateContext ());
		SetContext (ScanContext);
		MenuState.flash_rect0.extent.width = FLASH_WIDTH;
		MenuState.flash_rect0.extent.height = FLASH_HEIGHT;
		MenuState.flash_frame0 = CaptureDrawable (
				CreateDrawable (WANT_PIXMAP | MAPPED_TO_DISPLAY,
				FLASH_WIDTH, FLASH_HEIGHT, 1)
				);
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

		f = SetAbsFrameIndex (
				misc_data,
				NUM_SCANDOT_TRANSITIONS * (scan - ENERGY_SCAN)
				);

		pSolarSysState->CurNode = (COUNT)~0;
		(*pSolarSysState->GenFunc) (
				(BYTE)(scan + GENERATE_MINERAL)
				);
		num_nodes = pSolarSysState->CurNode;
		if (num_nodes)
		{
			while (num_nodes--)
			{
				HELEMENT hNodeElement;

				if (!(pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[scan]
						& (1L << num_nodes))
						&& (hNodeElement = AllocElement ()))
				{
					ELEMENTPTR NodeElementPtr;

					LockElement (hNodeElement, &NodeElementPtr);

					pSolarSysState->CurNode = num_nodes;
					(*pSolarSysState->GenFunc) (
							(BYTE)(scan + GENERATE_MINERAL)
							);

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
						NodeElementPtr->mass_points =
								HIBYTE (
								pSolarSysState->SysInfo.PlanetInfo.CurDensity
								);
						NodeElementPtr->current.image.frame =
								SetAbsFrameIndex (
								misc_data,
								(NUM_SCANDOT_TRANSITIONS << 1)
								+ ElementCategory (EType) * 5
								);
						NodeElementPtr->next.image.frame =
								SetRelFrameIndex (
								NodeElementPtr->current.image.frame,
								LOBYTE (
								pSolarSysState->SysInfo.PlanetInfo.CurDensity
								) + 1
								);
						DisplayArray[NodeElementPtr->PrimIndex].Object.Stamp.frame =
								IncFrameIndex (
								NodeElementPtr->next.image.frame
								);
					}
					else
					{
						extern void
								object_animation
								(PELEMENT
								ElementPtr);

						NodeElementPtr->current.image.frame = f;
						NodeElementPtr->next.image.frame =
								SetRelFrameIndex (
								f, NUM_SCANDOT_TRANSITIONS - 1
								);
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
										CANNISTER_MASK_PMAP_ANIM
										));
							for (i = 0; i < MAX_LIFE_VARIATION
									&& life_init_tab[i] != (BYTE)(which_node + 1);
									++i)
							{
								if (life_init_tab[i] == 0)
								{
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
							}

							NodeElementPtr->mass_points = (BYTE)which_node;
							NodeElementPtr->hit_points = HINIBBLE (
									CreatureData[which_node].ValueAndHitPoints
									);
							DisplayArray[NodeElementPtr->PrimIndex].Object.Stamp.frame =
									SetAbsFrameIndex (
									pSolarSysState->PlanetSideFrame[i + 3],
									(COUNT)TFB_Random ()
									);
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
	}
}

