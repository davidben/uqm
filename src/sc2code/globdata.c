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

#include "globdata.h"

#include "coderes.h"
#include "encount.h"
#include "setup.h"
#include "resinst.h"
#include "build.h"
#include "state.h"
#include "gamestr.h"

#include <stdlib.h>

//Added by Chris

void InitGlobData (void);

//End Added by Chris

CONTEXT RadarContext;
FRAME PlayFrame;

GLOBDATA GlobData;

extern FRAME flagship_status, misc_data;
BOOLEAN initedSIS = 0;

void
CreateRadar (void)
{
	if (RadarContext == 0)
	{
		RECT r;
		CONTEXT OldContext;

		RadarContext = CaptureContext (CreateContext ());
		OldContext = SetContext (RadarContext);
		SetContextFGFrame (Screen);
		r.corner.x = RADAR_X;
		r.corner.y = RADAR_Y;
		r.extent.width = RADAR_WIDTH;
		r.extent.height = RADAR_HEIGHT;
		SetContextClipRect (&r);
		SetContext (OldContext);
	}
}

BOOLEAN
LoadSC2Data (void)
{
	if (flagship_status == 0)
	{
		MEM_HANDLE hOldIndex;

		hOldIndex = SetResourceIndex (hResIndex);

		flagship_status = CaptureDrawable (
				LoadGraphic (FLAGSTAT_MASK_PMAP_ANIM));
		if (flagship_status == NULL)
			return FALSE;

		misc_data = CaptureDrawable (
				LoadGraphic (MISCDATA_MASK_PMAP_ANIM));
		if (misc_data == NULL)
			return FALSE;

		SetResourceIndex (hOldIndex);
	}

	CreateRadar ();

	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE)
	{
		GLOBAL (ShipStamp.origin.x) =
				GLOBAL (ShipStamp.origin.y) = -1;
	}

	return TRUE;
}

BOOLEAN
InitSIS (void)
{
	COUNT i;

	InitGlobData ();

	PlayFrame = CaptureDrawable (LoadGraphic (PLAYMENU_ANIM));
	
	{
		COUNT num_ships;
		RES_TYPE rt;
		RES_INSTANCE ri;
		RES_PACKAGE rp;

		rt = GET_TYPE (ARILOU_SHIP_INDEX);
		ri = GET_INSTANCE (ARILOU_SHIP_INDEX);
		rp = GET_PACKAGE (ARILOU_SHIP_INDEX);

		num_ships = (GET_PACKAGE (BLACKURQ_SHIP_INDEX) - rp + 1)
				+ 2; /* Yehat Rebels and Ur-Quan probe */

		InitQueue (&GLOBAL (avail_race_q),
				num_ships, sizeof (EXTENDED_SHIP_FRAGMENT));
		for (i = 0; i < num_ships; ++i)
		{
			DWORD ship_ref;
			HSTARSHIP hStarShip;

			if (i < num_ships - 2)
				ship_ref = MAKE_RESOURCE (rp++, rt, ri++);
			else if (i == num_ships - 2)
				ship_ref = YEHAT_SHIP_INDEX;
			else
				ship_ref = PROBE_RES_INDEX;
			hStarShip = Build (&GLOBAL (avail_race_q), ship_ref, 0, 0);
			if (hStarShip)
			{
				SHIP_FRAGMENTPTR FragPtr;
				EXTENDED_SHIP_FRAGMENTPTR ExtFragPtr;
				extern HSTARSHIP FindMasterShip (DWORD ship_ref);

				FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (
						&GLOBAL (avail_race_q), hStarShip
						);
				if (i < num_ships - 1)
				{
					HSTARSHIP hMasterShip;
					STARSHIPPTR MasterShipPtr;
					
					hMasterShip = FindMasterShip (ship_ref);
					MasterShipPtr = LockStarShip (&master_q, hMasterShip);
					FragPtr->ShipInfo = ((SHIP_FRAGMENTPTR)MasterShipPtr)->ShipInfo;
					UnlockStarShip (&master_q, hMasterShip);
				}
				else
				{
					load_ship ((STARSHIPPTR)FragPtr, FALSE);
					FragPtr->ShipInfo = FragPtr->RaceDescPtr->ship_info;
					FragPtr->RaceDescPtr->ship_info.melee_icon = 0;
					FragPtr->RaceDescPtr->ship_info.icons = 0;
					FragPtr->RaceDescPtr->ship_info.race_strings = 0;
					free_ship ((STARSHIPPTR)FragPtr, FALSE);
				}

				FragPtr->ShipInfo.ship_flags = BAD_GUY;
				ExtFragPtr = (EXTENDED_SHIP_FRAGMENTPTR)FragPtr;
				ExtFragPtr->ShipInfo.known_strength = 0;
				ExtFragPtr->ShipInfo.known_loc = ExtFragPtr->ShipInfo.loc;
				if (FragPtr->ShipInfo.var2 == (BYTE)~0)
					ExtFragPtr->ShipInfo.actual_strength = (COUNT)~0;
				else if (i == YEHAT_REBEL_SHIP)
					ExtFragPtr->ShipInfo.actual_strength = 0;
				else
					ExtFragPtr->ShipInfo.actual_strength =
							(COUNT)FragPtr->ShipInfo.var2 << 1;
				ExtFragPtr->ShipInfo.growth_fract = 0;
				ExtFragPtr->ShipInfo.growth_err_term = 255 >> 1;
				ExtFragPtr->ShipInfo.energy_level = 0;
				ExtFragPtr->ShipInfo.days_left = 0;
				FragPtr->RaceDescPtr = (RACE_DESCPTR)&ExtFragPtr->ShipInfo;

				UnlockStarShip (
						&GLOBAL (avail_race_q), hStarShip
						);
			}
		}
	}

	InitSISContexts ();
	LoadSC2Data ();

	InitPlanetInfo ();
	InitGroupInfo (TRUE);

	GLOBAL (glob_flags) = NUM_READ_SPEEDS >> 1;

	GLOBAL (ElementWorth[COMMON]) = 1;
	GLOBAL_SIS (ElementAmounts[COMMON]) = 0;
	GLOBAL (ElementWorth[CORROSIVE]) = 2;
	GLOBAL_SIS (ElementAmounts[CORROSIVE]) = 0;
	GLOBAL (ElementWorth[BASE_METAL]) = 3;
	GLOBAL_SIS (ElementAmounts[BASE_METAL]) = 0;
	GLOBAL (ElementWorth[NOBLE]) = 4;
	GLOBAL_SIS (ElementAmounts[NOBLE]) = 0;
	GLOBAL (ElementWorth[RARE_EARTH]) = 5;
	GLOBAL_SIS (ElementAmounts[RARE_EARTH]) = 0;
	GLOBAL (ElementWorth[PRECIOUS]) = 6;
	GLOBAL_SIS (ElementAmounts[PRECIOUS]) = 0;
	GLOBAL (ElementWorth[RADIOACTIVE]) = 8;
	GLOBAL_SIS (ElementAmounts[RADIOACTIVE]) = 0;
	GLOBAL (ElementWorth[EXOTIC]) = 25;
	GLOBAL_SIS (ElementAmounts[EXOTIC]) = 0;

	for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
		GLOBAL_SIS (DriveSlots[i]) = EMPTY_SLOT + 0;
	GLOBAL_SIS (DriveSlots[5]) =
			GLOBAL_SIS (DriveSlots[6]) = FUSION_THRUSTER;
	for (i = 0; i < NUM_JET_SLOTS; ++i)
		GLOBAL_SIS (JetSlots[i]) = EMPTY_SLOT + 1;
	GLOBAL_SIS (JetSlots[0]) =
			GLOBAL_SIS (JetSlots[6]) = TURNING_JETS;
	for (i = 0; i < NUM_MODULE_SLOTS; ++i)
		GLOBAL_SIS (ModuleSlots[i]) = EMPTY_SLOT + 2;
	GLOBAL_SIS (ModuleSlots[15]) = GUN_WEAPON;
	GLOBAL_SIS (ModuleSlots[2]) = CREW_POD;
	GLOBAL_SIS (CrewEnlisted) = CREW_POD_CAPACITY;
	GLOBAL_SIS (ModuleSlots[8]) = STORAGE_BAY;
	GLOBAL_SIS (ModuleSlots[1]) = FUEL_TANK;
	GLOBAL_SIS (FuelOnBoard) = 10 * FUEL_TANK_SCALE;

	InitQueue (&GLOBAL (built_ship_q),
			MAX_BUILT_SHIPS, sizeof (SHIP_FRAGMENT));
	InitQueue (&GLOBAL (npc_built_ship_q),
			(MAX_BATTLE_GROUPS >> 1), sizeof (SHIP_FRAGMENT));
	InitQueue (&GLOBAL (encounter_q), 16, sizeof (ENCOUNTER));

	GLOBAL (CurrentActivity) = IN_INTERPLANETARY | START_INTERPLANETARY;

	GLOBAL_SIS (ResUnits) = 0;
	GLOBAL (CrewCost) = 3;
	GLOBAL (FuelCost) = 20;
	GLOBAL (ModuleCost[PLANET_LANDER]) = 500 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[FUSION_THRUSTER]) = 500 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[TURNING_JETS]) = 500 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[CREW_POD]) = 2000 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[STORAGE_BAY]) = 750 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[FUEL_TANK]) = 500 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[DYNAMO_UNIT]) = 2000 / MODULE_COST_SCALE;
	GLOBAL (ModuleCost[GUN_WEAPON]) = 2000 / MODULE_COST_SCALE;

	GLOBAL_SIS (NumLanders) = 1;

	wstrcpy (GLOBAL_SIS (ShipName), GAME_STRING (NAMING_STRING_BASE + 2));
	wstrcpy (GLOBAL_SIS (CommanderName), GAME_STRING (NAMING_STRING_BASE + 3));

	ActivateStarShip (HUMAN_SHIP, 0);
	CloneShipFragment (HUMAN_SHIP, &GLOBAL (built_ship_q), 0);

	GLOBAL_SIS (log_x) = UNIVERSE_TO_LOGX (SOL_X);
	GLOBAL_SIS (log_y) = UNIVERSE_TO_LOGY (SOL_Y);
	CurStarDescPtr = 0;
	GLOBAL (autopilot.x) = GLOBAL (autopilot.y) = ~0;

	/* In case the program is exited before the full game is terminated,
	 * make sure that the temporary files are deleted.
	 * This can be removed if we make sure if the full game is terminated
	 * before the game is exited.
	 * The initedSIS variable is added so the uninit won't happen more
	 * than once, as you can't remove the atexit function (when the full game
	 * ends).
	 */
	initedSIS = TRUE;
	atexit (UninitSIS);

	return (TRUE);
}

void
FreeSC2Data (void)
{
	DestroyContext (ReleaseContext (RadarContext));
	RadarContext = 0;
	DestroyDrawable (ReleaseDrawable (misc_data));
	misc_data = 0;
	DestroyDrawable (ReleaseDrawable (flagship_status));
	flagship_status = 0;
}

void
UninitSIS (void)
{
	HSTARSHIP hStarShip;

	if (!initedSIS)
		return;

	UninitQueue (&GLOBAL (encounter_q));
	UninitQueue (&GLOBAL (npc_built_ship_q));
	UninitQueue (&GLOBAL (built_ship_q));
	UninitGroupInfo ();
	UninitPlanetInfo ();

//    FreeSC2Data ();

	hStarShip = GetTailLink (&GLOBAL (avail_race_q));
	if (hStarShip)
	{
		STARSHIPPTR StarShipPtr;

		StarShipPtr = LockStarShip (&GLOBAL (avail_race_q), hStarShip);
		DestroyDrawable (ReleaseDrawable (
				StarShipPtr->RaceDescPtr->ship_info.melee_icon));
		DestroyDrawable (ReleaseDrawable (
				StarShipPtr->RaceDescPtr->ship_info.icons));
		DestroyStringTable (ReleaseStringTable (
				StarShipPtr->RaceDescPtr->ship_info.race_strings));
		UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);
	}

	UninitQueue (&GLOBAL (avail_race_q));
	
	DestroyDrawable (ReleaseDrawable (PlayFrame));
	PlayFrame = 0;
	initedSIS = FALSE;
}

