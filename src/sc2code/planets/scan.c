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

//Added by Chris

void
DrawPlanet(int x, int y, int dy, unsigned int rgb);

void WaitForNoInput (SIZE Duration);

//End Added by Chris

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

	SetSemaphore (GraphicsSem);
	SetContext (SpaceContext);

	r.corner.x = (SIS_SCREEN_WIDTH >> 1) - (160 >> 1);
	r.corner.y = 13 - 7;
	r.extent.width = 160;
	r.extent.height = 10;
	RepairBackRect (&r);

	GetFrameRect (SetAbsFrameIndex (SpaceJunkFrame, 20), &tr);
	r = tr;
	r.extent.width = 70;
	RepairBackRect (&r);

	r = tr;
	r.corner.x += (r.extent.width - 70);
	r.extent.width = 70;
	RepairBackRect (&r);

	ClearSemaphore (GraphicsSem);
}

static void
PrintCoarseScan (void)
{
#define SCAN_LEADING 19
	SIZE temp;
	TEXT t;
	STAMP s;
	UNICODE buf[40];
	extern FRAME SpaceJunkFrame;

	SetSemaphore (GraphicsSem);
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
	DrawText (&t);

	s.origin.x = s.origin.y = 0;
	s.frame = SetAbsFrameIndex (SpaceJunkFrame, 20);
	DrawStamp (&s);

	ClearSemaphore (GraphicsSem);

#define LEFT_SIDE_BASELINE_X 27
#define RIGHT_SIDE_BASELINE_X (SIS_SCREEN_WIDTH - LEFT_SIDE_BASELINE_X)
#define SCAN_BASELINE_Y 25

	t.baseline.x = LEFT_SIDE_BASELINE_X;
	t.baseline.y = SCAN_BASELINE_Y;
	t.align = ALIGN_LEFT;

	SetSemaphore (GraphicsSem);
	t.pStr = buf;
	temp = (SIZE)((pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist * 100L
			+ (EARTH_RADIUS >> 1)) / EARTH_RADIUS);
	if (temp >= 10 * 100)
		wsprintf (buf, "%u.%u&", temp / 100, (temp / 10) % 10);
	else
		wsprintf (buf, "%u.%02u&", temp / 100, temp % 100);
	t.CharCount = (COUNT)~0;
	DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	ClearSemaphore (GraphicsSem);

	SetSemaphore (GraphicsSem);
	t.pStr = buf;
	if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == GAS_GIANT_ATMOSPHERE)
		wsprintf (buf, "|");
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
	DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	ClearSemaphore (GraphicsSem);

	SetSemaphore (GraphicsSem);
	t.pStr = buf;
	wsprintf (buf, "%d^", pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature);
	t.CharCount = (COUNT)~0;
	DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	ClearSemaphore (GraphicsSem);

	SetSemaphore (GraphicsSem);
	t.pStr = buf;
	wsprintf (buf, "<%u>", pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == 0
			? 0 : (pSolarSysState->SysInfo.PlanetInfo.Weather + 1));
	t.CharCount = (COUNT)~0;
	DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	ClearSemaphore (GraphicsSem);

	SetSemaphore (GraphicsSem);
	t.pStr = buf;
	wsprintf (buf, "<%u>",
			PLANSIZE (
			pSolarSysState->SysInfo.PlanetInfo.PlanDataPtr->Type
			) == GAS_GIANT
			? 0 : (pSolarSysState->SysInfo.PlanetInfo.Tectonics + 1));
	t.CharCount = (COUNT)~0;
	DrawText (&t);
	ClearSemaphore (GraphicsSem);

	t.baseline.x = RIGHT_SIDE_BASELINE_X;
	t.baseline.y = SCAN_BASELINE_Y;
	t.align = ALIGN_RIGHT;

	SetSemaphore (GraphicsSem);
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
	DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	ClearSemaphore (GraphicsSem);

	SetSemaphore (GraphicsSem);
	t.pStr = buf;
	if ((temp = pSolarSysState->SysInfo.PlanetInfo.PlanetRadius) >= 10 * 100)
		wsprintf (buf, "%u.%u&", temp / 100, (temp / 10) % 10);
	else
		wsprintf (buf, "%u.%02u&", temp / 100, temp % 100);
	t.CharCount = (COUNT)~0;
	DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	ClearSemaphore (GraphicsSem);

	SetSemaphore (GraphicsSem);
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
	DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	ClearSemaphore (GraphicsSem);

	SetSemaphore (GraphicsSem);
	t.pStr = buf;
	if ((temp = pSolarSysState->SysInfo.PlanetInfo.AxialTilt) < 0)
		temp = -temp;
	wsprintf (buf, "%u^", temp);
	t.CharCount = (COUNT)~0;
	DrawText (&t);
	t.baseline.y += SCAN_LEADING;
	ClearSemaphore (GraphicsSem);

	SetSemaphore (GraphicsSem);
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
	DrawText (&t);
	ClearSemaphore (GraphicsSem);
}

static void
SetPlanetLoc (POINT new_pt)
{
	RECT r;
	STAMP s;

	pSolarSysState->MenuState.first_item = new_pt;
	new_pt.x >>= MAG_SHIFT;
	new_pt.y >>= MAG_SHIFT;

	SetSemaphore (GraphicsSem);
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
	ClearSemaphore (GraphicsSem);
	
	TaskSwitch ();
}

int
flash_planet_loc_func(void *blah)
{
	DWORD TimeIn;
	BYTE c, val;
	PRIMITIVE p;

	SetPrimType (&p, STAMPFILL_PRIM);
	SetPrimNextLink (&p, END_OF_LIST);

	p.Object.Stamp.origin.x = p.Object.Stamp.origin.y = -1;
	p.Object.Stamp.frame = SetAbsFrameIndex (misc_data, FLASH_INDEX);
	c = 0x00;
	val = -0x02;
	TimeIn = 0;
	for (;;)
	{
		DWORD T;
		CONTEXT OldContext;

		SetSemaphore (GraphicsSem);
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
				ClearSemaphore (GraphicsSem);
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
		ClearSemaphore (GraphicsSem);
		
		TaskSwitch ();
	}
	(void) blah;  /* Satisfying compiler (unused parameter) */
	return(0);
}

static BOOLEAN DoScan (INPUT_STATE InputState, PMENU_STATE pMS);

static BOOLEAN
PickPlanetSide (INPUT_STATE InputState, PMENU_STATE pMS)
{
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		if (pMenuState->flash_task)
		{
			SetSemaphore (GraphicsSem);
			KillThread (pMenuState->flash_task);
			ClearSemaphore (GraphicsSem);
			pMenuState->flash_task = 0;
		}
		goto ExitPlanetSide;
	}

	pMS->MenuRepeatDelay = 0;
	if (!pMS->Initialized)
	{
		pMS->InputFunc = PickPlanetSide;
		if (pMS->CurFrame == 0)
		{
			pMS->CurFrame = (FRAME)MenuSounds;
			MenuSounds = 0;
		}
		if (!(InputState & DEVICE_BUTTON1))
		{
			RECT r;

			pMS->Initialized = TRUE;

			SetSemaphore (GraphicsSem);
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
			ClearSemaphore (GraphicsSem);

			InitLander (0);

			pMenuState->flash_task = CreateThread (flash_planet_loc_func,
					NULL, 2048, "flash planet location");
		}
	}
	else if (InputState & (DEVICE_BUTTON1 | DEVICE_BUTTON2))
	{
		STAMP s;

		MenuSounds = (SOUND)pMS->CurFrame;
		pMS->CurFrame = 0;

		SetSemaphore (GraphicsSem);
		DrawStatusMessage (NULL_PTR);
		if (pMenuState->flash_task)
		{
			KillThread (pMenuState->flash_task);
			pMenuState->flash_task = 0;
		}
		ClearSemaphore (GraphicsSem);

		if (!(InputState & DEVICE_BUTTON1))
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

			SetSemaphore (GraphicsSem);
			DeltaSISGauges (0, -(SIZE)fuel_required, 0);
			SetContext (ScanContext);
			s.origin = pMenuState->flash_rect0.corner;
			s.frame = SetAbsFrameIndex (misc_data, FLASH_INDEX);
			DrawStamp (&s);
			ClearSemaphore (GraphicsSem);

			PlanetSide (pMS);
			if (GLOBAL (CurrentActivity) & CHECK_ABORT)
				goto ExitPlanetSide;

			if (GET_GAME_STATE (FOUND_PLUTO_SPATHI) == 1)
			{
				HSTARSHIP hStarShip;
				extern ACTIVITY NextActivity;
				extern void SaveFlagshipState (void);

				SetSemaphore (GraphicsSem);
				if (pMenuState->flash_task)
				{
					KillThread (pMenuState->flash_task);
					pMenuState->flash_task = 0;
				}
				ClearSemaphore (GraphicsSem);

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

			PrintCoarseScan ();
		}

		DrawMenuStateStrings (PM_MIN_SCAN, DISPATCH_SHUTTLE);
		SetSemaphore (GraphicsSem);
		SetFlashRect ((PRECT)~0L, (FRAME)0);
		ClearSemaphore (GraphicsSem);

ExitPlanetSide:
		if (pMS->CurFrame)
		{
			MenuSounds = (SOUND)pMS->CurFrame;
			pMS->CurFrame = 0;
		}

		pMS->InputFunc = DoScan;
		pMS->CurState = DISPATCH_SHUTTLE;
		pMS->MenuRepeatDelay = MENU_REPEAT_DELAY;
	}
	else
	{
		SIZE dx, dy;
		POINT new_pt;

		new_pt = pSolarSysState->MenuState.first_item;

		dx = GetInputXComponent (InputState) << MAG_SHIFT;
		if (dx)
		{
			new_pt.x += dx;
			if (new_pt.x < 0)
				new_pt.x += (MAP_WIDTH << MAG_SHIFT);
			else if (new_pt.x >= (MAP_WIDTH << MAG_SHIFT))
				new_pt.x -= (MAP_WIDTH << MAG_SHIFT);
		}
		dy = GetInputYComponent (InputState) << MAG_SHIFT;
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
			SleepThreadUntil (TimeIn + 3);
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
DoScan (INPUT_STATE InputState, PMENU_STATE
		pMS)
{
	//STAMP s;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	if (!pMS->Initialized)
	{
		pMS->Initialized = TRUE;
		pMS->InputFunc = DoScan;
	}
	else if ((InputState & DEVICE_BUTTON2)
			|| ((InputState & DEVICE_BUTTON1)
			&& pMS->CurState == EXIT_SCAN))
	{
		SetSemaphore (GraphicsSem);
		SetContext (SpaceContext);
		BatchGraphics ();
		DrawPlanet (SIS_SCREEN_WIDTH - MAP_WIDTH, SIS_SCREEN_HEIGHT - MAP_HEIGHT, 0, 0);
		UnbatchGraphics ();
		ClearSemaphore (GraphicsSem);

		EraseCoarseScan ();
// DrawMenuStateStrings (PM_SCAN, SCAN);

		return (FALSE);
	}
	else if (InputState & DEVICE_BUTTON1)
	{
		BYTE min_scan, max_scan;
		RECT r;
		INPUT_STATE PressState, ButtonState;

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
						0, GAME_SOUND_PRIORITY);
				return (TRUE);
			}

			wsprintf (buf, "%s%u.%u",
					GAME_STRING (NAVIGATION_STRING_BASE + 5),
					fuel_required / FUEL_TANK_SCALE,
					((fuel_required % FUEL_TANK_SCALE) + 5) / 10);
			SetSemaphore (GraphicsSem);
			DrawStatusMessage (buf);
			ClearSemaphore (GraphicsSem);

			SetSemaphore (GraphicsSem);
			SetContext (ScanContext);
			BatchGraphics ();
			DrawPlanet (0, 0, 0, 0);
			DrawScannedObjects (FALSE);
			UnbatchGraphics ();
			ClearSemaphore (GraphicsSem);
		
			pMS->Initialized = FALSE;
			pMS->CurFrame = 0;
			return (PickPlanetSide (InputState, pMS));
		}

pSolarSysState->MenuState.Initialized += 4;

		if ((min_scan = pMS->CurState) != AUTO_SCAN)
			max_scan = min_scan;
		else
			min_scan = MINERAL_SCAN, max_scan = BIOLOGICAL_SCAN;

		do
		{
		TEXT t;
		COUNT i;

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

		SetSemaphore (GraphicsSem);
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
		DrawText (&t);

		SetContext (ScanContext);
		ClearSemaphore (GraphicsSem);

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

		SetSemaphore (GraphicsSem);
		BatchGraphics ();
		DrawPlanet (0, 0, 0, 0);
		UnbatchGraphics ();
		ClearSemaphore (GraphicsSem);

		PressState = AnyButtonPress (TRUE);
		for (i = 0; i < MAP_HEIGHT + NUM_FLASH_COLORS + 1; i++)
		{
			ButtonState = AnyButtonPress (TRUE);
			if (PressState)
			{
				PressState = ButtonState;
				ButtonState = 0;
			}
			if (ButtonState)
				i = MAP_HEIGHT + NUM_FLASH_COLORS;
			SetSemaphore (GraphicsSem);
			BatchGraphics ();
			DrawPlanet (0, 0, i, rgb);
			if (pMS->delta_item)
				DrawScannedStuff (i, min_scan);
			UnbatchGraphics ();
			ClearSemaphore (GraphicsSem);
			
			SleepThread (2);
		}
	}

		} while (++min_scan <= max_scan);

		SetSemaphore (GraphicsSem);
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
			ClearSemaphore (GraphicsSem);

			DrawMenuStateStrings (PM_MIN_SCAN, pMS->CurState = DISPATCH_SHUTTLE);
		}
		else
			ClearSemaphore (GraphicsSem);
			
pSolarSysState->MenuState.Initialized -= 4;
		WaitForNoInput (ONE_SECOND / 2);
	}
	else if (!(pSolarSysState->pOrbitalDesc->data_index & PLANET_SHIELDED)
			&& pSolarSysState->SysInfo.PlanetInfo.AtmoDensity !=
			GAS_GIANT_ATMOSPHERE)
	{
		BYTE NewState;

		NewState = pMS->CurState;
		if (GetInputXComponent (InputState) < 0
				|| GetInputYComponent (InputState) < 0)
		{
			if (NewState-- == MINERAL_SCAN)
				NewState = DISPATCH_SHUTTLE;
		}
		else if (GetInputXComponent (InputState) > 0
				|| GetInputYComponent (InputState) > 0)
		{
			if (NewState++ == DISPATCH_SHUTTLE)
				NewState = MINERAL_SCAN;
		}

		if (NewState != pMS->CurState)
		{
			DrawMenuStateStrings (PM_MIN_SCAN, NewState);
			pMS->CurState = NewState;
		}
	}

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

		SetSemaphore (GraphicsSem);
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
		ClearSemaphore (GraphicsSem);
	}

	DrawMenuStateStrings (PM_MIN_SCAN, MenuState.CurState);

	PrintCoarseScan ();

	pMenuState = &MenuState;
	DoInput ((PVOID)&MenuState);
	pMenuState = 0;

	if (ScanContext)
	{
		SetSemaphore (GraphicsSem);
		SetContext (SpaceContext);
		DestroyDrawable (ReleaseDrawable (MenuState.flash_frame0));
		DestroyContext (ReleaseContext (ScanContext));
		ScanContext = 0;
		ClearSemaphore (GraphicsSem);
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
								i = (COUNT)Random ();
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
									(COUNT)Random ()
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

