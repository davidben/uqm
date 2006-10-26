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
#include "coderes.h"
#include "globdata.h"
#include "nameref.h"
#include "races.h"


MEM_HANDLE
load_ship (STARSHIPPTR StarShipPtr, BOOLEAN LoadBattleData)
{
	BOOLEAN retval;
	MEM_HANDLE h;

	h = OpenResourceIndexInstance (StarShipPtr->RaceResIndex);

	retval = FALSE;
	if (h)
	{
#define INITIAL_CODE_RES MAKE_RESOURCE (1, CODE, 0)
		BYTE captains_name_index;
		PVOID CodeRef;
		MEM_HANDLE hOldIndex;
		COUNT which_player;

		captains_name_index = StarShipCaptain (StarShipPtr);
		which_player = StarShipPlayer (StarShipPtr);
		hOldIndex = SetResourceIndex (h);

		CodeRef = CaptureCodeRes (LoadCodeRes (INITIAL_CODE_RES),
				(PVOID)&GlobData, (PVOID)&StarShipPtr->RaceDescPtr);
		if (CodeRef == 0)
			goto BadLoad;
		StarShipPtr->RaceDescPtr->CodeRef = CodeRef;

		StarShipPtr->RaceDescPtr->ship_info.icons =
				CaptureDrawable (LoadGraphic (
				(RESOURCE)StarShipPtr->RaceDescPtr->ship_info.icons));
		if (StarShipPtr->RaceDescPtr->ship_info.icons == 0)
		{
			/* goto BadLoad */
		}
			
		StarShipPtr->RaceDescPtr->ship_info.melee_icon =
				CaptureDrawable (LoadGraphic (
				(RESOURCE)StarShipPtr->RaceDescPtr->ship_info.melee_icon));
		if (StarShipPtr->RaceDescPtr->ship_info.melee_icon == 0)
		{
			/* goto BadLoad */
		}

		StarShipPtr->RaceDescPtr->ship_info.race_strings =
				CaptureStringTable (LoadStringTable (
				(RESOURCE)StarShipPtr->RaceDescPtr->ship_info.race_strings));
		if (StarShipPtr->RaceDescPtr->ship_info.race_strings == 0)
		{
			/* goto BadLoad */
		}

		if (LoadBattleData)
		{
			DATA_STUFFPTR RawPtr;

			StarShipPtr->captains_name_index = captains_name_index;
			StarShipPtr->RaceDescPtr->ship_info.ship_flags |= which_player;

			RawPtr = &StarShipPtr->RaceDescPtr->ship_data;
			if (!load_animation (RawPtr->ship,
					(RESOURCE)RawPtr->ship[0],
					(RESOURCE)RawPtr->ship[1],
					(RESOURCE)RawPtr->ship[2]))
				goto BadLoad;

			if (RawPtr->weapon[0] != 0)
			{
				if (!load_animation (RawPtr->weapon,
						(RESOURCE)RawPtr->weapon[0],
						(RESOURCE)RawPtr->weapon[1],
						(RESOURCE)RawPtr->weapon[2]))
					goto BadLoad;
			}

			if (RawPtr->special[0] != 0)
			{
				if (!load_animation (RawPtr->special,
						(RESOURCE)RawPtr->special[0],
						(RESOURCE)RawPtr->special[1],
						(RESOURCE)RawPtr->special[2]))
					goto BadLoad;
			}

			if (RawPtr->captain_control.background != 0)
			{
				RawPtr->captain_control.background =
						CaptureDrawable (LoadGraphic (
						(RESOURCE)RawPtr->captain_control.background));
				if (RawPtr->captain_control.background == 0)
					goto BadLoad;
			}

			if (RawPtr->victory_ditty != 0)
			{
				RawPtr->victory_ditty =
						LoadMusic ((RESOURCE)RawPtr->victory_ditty);
				if (RawPtr->victory_ditty == 0)
					goto BadLoad;
			}

			if (RawPtr->ship_sounds != 0)
			{
				RawPtr->ship_sounds = CaptureSound (
						LoadSound ((RESOURCE)RawPtr->ship_sounds));
				if (RawPtr->ship_sounds == 0)
					goto BadLoad;
			}

			if (StarShipPtr->RaceDescPtr->ship_info.icons)
				StarShipPtr->silhouette = IncFrameIndex (
						StarShipPtr->RaceDescPtr->ship_info.icons);
		}

		retval = TRUE;
BadLoad:
		SetResourceIndex (hOldIndex);
		CloseResourceIndex (h);
	}

	return (retval);
}

void
free_ship (STARSHIPPTR StarShipPtr, BOOLEAN FreeBattleData)
{
	RACE_DESCPTR raceDescPtr = StarShipPtr->RaceDescPtr;
	SHIP_INFO *shipInfo = &raceDescPtr->ship_info;

	if (FreeBattleData)
	{
		DATA_STUFF *shipData = &raceDescPtr->ship_data;

		free_image (shipData->special);
		free_image (shipData->weapon);
		free_image (shipData->ship);

		DestroyDrawable (
				ReleaseDrawable (shipData->captain_control.background));
		DestroyMusic ((MUSIC_REF)shipData->victory_ditty);
		DestroySound (ReleaseSound (shipData->ship_sounds));
	}

	DestroyDrawable (ReleaseDrawable (shipInfo->melee_icon));
	DestroyDrawable (ReleaseDrawable (shipInfo->icons));
	DestroyStringTable (ReleaseStringTable (shipInfo->race_strings));

	DestroyCodeRes (ReleaseCodeRes (raceDescPtr->CodeRef));

	StarShipPtr->RaceDescPtr = 0;
}


