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
#include "colors.h"
#include "element.h"
#include "globdata.h"
#include "init.h"
#include "port.h"
#include "resinst.h"
#include "nameref.h"
#include "setup.h"
#include "units.h"


FRAME stars_in_space;
FRAME asteroid[NUM_VIEWS];
FRAME blast[NUM_VIEWS];
FRAME explosion[NUM_VIEWS];


BOOLEAN
load_animation (PFRAME pixarray, DWORD big_res, DWORD med_res, DWORD
		sml_res)
{
	DRAWABLE d;

	if ((d = LoadGraphic (big_res)) == (DRAWABLE)NULL_PTR)
		return (FALSE);
	pixarray[0] = CaptureDrawable (d);

	if (med_res != 0L && (d = LoadGraphic (med_res)) == (DRAWABLE)NULL_PTR)
		return (FALSE);
	pixarray[1] = CaptureDrawable (d);

	if (sml_res != 0L && (d = LoadGraphic (sml_res)) == (DRAWABLE)NULL_PTR)
		return (FALSE);
	pixarray[2] = CaptureDrawable (d);

	return (TRUE);
}

BOOLEAN
free_image (PFRAME pixarray)
{
	BOOLEAN retval;
	COUNT i;

	retval = TRUE;
	for (i = 0; i < NUM_VIEWS; ++i)
	{
		if (pixarray[i] != (FRAME)NULL_PTR)
		{
			if (!DestroyDrawable (ReleaseDrawable (pixarray[i])))
				retval = FALSE;
			pixarray[i] = (FRAME)NULL_PTR;
		}
	}

	return (retval);
}

static BYTE space_ini_cnt;

BOOLEAN
InitSpace (void)
{
	if (space_ini_cnt++ == 0
			&& LOBYTE (GLOBAL (CurrentActivity)) <= IN_ENCOUNTER)
	{
		stars_in_space = CaptureDrawable (
				LoadGraphic (STAR_MASK_PMAP_ANIM));
		if (stars_in_space == NULL)
			return FALSE;

		if (!load_animation (explosion,
				BOOM_BIG_MASK_PMAP_ANIM,
				BOOM_MED_MASK_PMAP_ANIM,
				BOOM_SML_MASK_PMAP_ANIM))
			return FALSE;

		if (!load_animation (blast,
				BLAST_BIG_MASK_PMAP_ANIM,
				BLAST_MED_MASK_PMAP_ANIM,
				BLAST_SML_MASK_PMAP_ANIM))
			return FALSE;

		if (!load_animation (asteroid,
				ASTEROID_BIG_MASK_PMAP_ANIM,
				ASTEROID_MED_MASK_PMAP_ANIM,
				ASTEROID_SML_MASK_PMAP_ANIM))
			return FALSE;
	}

	return TRUE;
}

void
UninitSpace (void)
{
	if (space_ini_cnt && --space_ini_cnt == 0)
	{
		free_image (blast);
		free_image (explosion);
		free_image (asteroid);

		DestroyDrawable (ReleaseDrawable (stars_in_space));
		stars_in_space = 0;
	}
}

SIZE
InitShips (void)
{
	SIZE num_ships;

	InitSpace ();

	SetContext (StatusContext);
	SetContext (SpaceContext);

	InitDisplayList ();
	InitGalaxy ();

	num_ships = NUM_SIDES;
	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE)
	{
		ReinitQueue (&race_q[0]);
		ReinitQueue (&race_q[1]);

		Build (&race_q[0], SIS_RES_INDEX, GOOD_GUY, 0);

		LoadHyperspace ();

		--num_ships;
	}
	else
	{
		COUNT i;
		RECT r;

		SetContextFGFrame (Screen);
		r.corner.x = SAFE_X;
		r.corner.y = SAFE_Y;
		r.extent.width = SPACE_WIDTH;
		r.extent.height = SPACE_HEIGHT;
		SetContextClipRect (&r);

		SetContextBackGroundColor (BLACK_COLOR);
		{
			CONTEXT OldContext;

			OldContext = SetContext (ScreenContext);

			SetContextBackGroundColor (BLACK_COLOR);
			ClearDrawable ();

			SetContext (OldContext);
		}

		if (LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
			free_gravity_well ();
		else
		{
#define NUM_ASTEROIDS 5
			for (i = 0; i < NUM_ASTEROIDS; ++i)
				spawn_asteroid (NULL_PTR);
#define NUM_PLANETS 1
			for (i = 0; i < NUM_PLANETS; ++i)
				spawn_planet ();
		}
	}

	// FlushInput ();

	return (num_ships);
}

void
UninitShips (void)
{
	BYTE crew_retrieved;
	SIZE i;
	HELEMENT hElement, hNextElement;
	STARSHIPPTR SPtr[NUM_PLAYERS];

	StopSound ();

	UninitSpace ();

	for (i = 0; i < NUM_PLAYERS; ++i)
		SPtr[i] = 0;

	// Count the crew floating in space.
	crew_retrieved = 0;
	for (hElement = GetHeadElement ();
			hElement != 0; hElement = hNextElement)
	{
		ELEMENTPTR ElementPtr;
		LockElement (hElement, &ElementPtr);
		hNextElement = GetSuccElement (ElementPtr);
		if (ElementPtr->state_flags & CREW_OBJECT)
			++crew_retrieved;
		UnlockElement (hElement);
	}

	for (hElement = GetHeadElement ();
			hElement != 0; hElement = hNextElement)
	{
		ELEMENTPTR ElementPtr;
		extern void new_ship (PELEMENT ElementPtr);

		LockElement (hElement, &ElementPtr);
		hNextElement = GetSuccElement (ElementPtr);
		if ((ElementPtr->state_flags & PLAYER_SHIP)
				|| ElementPtr->death_func == new_ship)
		{
			STARSHIPPTR StarShipPtr;

			GetElementStarShip (ElementPtr, &StarShipPtr);

			// There should only be one ship left in battle.
			// He gets the crew still floating in space.
			if (StarShipPtr->RaceDescPtr->ship_info.crew_level)
			{
				if (crew_retrieved >=
						StarShipPtr->RaceDescPtr->ship_info.max_crew -
						StarShipPtr->RaceDescPtr->ship_info.crew_level)
					StarShipPtr->RaceDescPtr->ship_info.crew_level =
							StarShipPtr->RaceDescPtr->ship_info.max_crew;
				else
					StarShipPtr->RaceDescPtr->ship_info.crew_level +=
							crew_retrieved;
			}

			if (StarShipPtr->RaceDescPtr->uninit_func != NULL)
				(*StarShipPtr->RaceDescPtr->uninit_func) (
						StarShipPtr->RaceDescPtr);
			StarShipPtr->ShipFacing =
					StarShipPtr->RaceDescPtr->ship_info.var2;
			StarShipPtr->special_counter =
					StarShipPtr->RaceDescPtr->ship_info.crew_level;
			SPtr[WHICH_SIDE (ElementPtr->state_flags)] = StarShipPtr;
			free_ship (StarShipPtr, TRUE);
		}
		UnlockElement (hElement);
	}

	GLOBAL (CurrentActivity) &= ~IN_BATTLE;

	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
	{
	}
	else if (LOBYTE (GLOBAL (CurrentActivity)) <= IN_ENCOUNTER
			&& !(GLOBAL (CurrentActivity) & CHECK_ABORT))
	{
		for (i = NUM_PLAYERS - 1; i >= 0; --i)
		{
			if (SPtr[i])
				GetEncounterStarShip (SPtr[i], i);
		}
	}

	if (LOBYTE (GLOBAL (CurrentActivity)) != IN_ENCOUNTER)
	{
		ReinitQueue (&race_q[0]);
		ReinitQueue (&race_q[1]);

		if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE)
			FreeHyperspace ();
	}
}


