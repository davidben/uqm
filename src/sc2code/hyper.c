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

#include "hyper.h"

#include "build.h"
#include "collide.h"
#include "colors.h"
#include "controls.h"
#include "encount.h"
#include "globdata.h"
#include "init.h"
#include "nameref.h"
#include "resinst.h"
#include "setup.h"
#include "sounds.h"
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"



#define XOFFS ((RADAR_SCAN_WIDTH + (UNIT_SCREEN_WIDTH << 2)) >> 1)
#define YOFFS ((RADAR_SCAN_HEIGHT + (UNIT_SCREEN_HEIGHT << 2)) >> 1)

static FRAME hyperstars[3];
static COLORMAP hypercmaps[2];
static BYTE fuel_ticks;
static COUNT hyper_dx, hyper_dy, hyper_extra;

void
MoveSIS (PSIZE pdx, PSIZE pdy)
{
	SIZE new_dx, new_dy;

	new_dx = *pdx;
	if ((GLOBAL_SIS (log_x) -= new_dx) < 0)
	{
		new_dx += (SIZE)GLOBAL_SIS (log_x);
		GLOBAL_SIS (log_x) = 0;
	}
	else if (GLOBAL_SIS (log_x) > MAX_X_LOGICAL)
	{
		new_dx += (SIZE)(GLOBAL_SIS (log_x) - MAX_X_LOGICAL);
		GLOBAL_SIS (log_x) = MAX_X_LOGICAL;
	}

	new_dy = *pdy;
	if ((GLOBAL_SIS (log_y) -= new_dy) < 0)
	{
		new_dy += (SIZE)GLOBAL_SIS (log_y);
		GLOBAL_SIS (log_y) = 0;
	}
	else if (GLOBAL_SIS (log_y) > MAX_Y_LOGICAL)
	{
		new_dy += (SIZE)(GLOBAL_SIS (log_y) - MAX_Y_LOGICAL);
		GLOBAL_SIS (log_y) = MAX_Y_LOGICAL;
	}

	if (new_dx != *pdx || new_dy != *pdy)
	{
		HELEMENT hElement, hNextElement;

		*pdx = new_dx;
		*pdy = new_dy;

		for (hElement = GetTailElement ();
				hElement != 0; hElement = hNextElement)
		{
			ELEMENTPTR ElementPtr;

			LockElement (hElement, &ElementPtr);

			if (!(ElementPtr->state_flags & PLAYER_SHIP))
				hNextElement = GetPredElement (ElementPtr);
			else
			{
				ElementPtr->next.location.x = (LOG_SPACE_WIDTH >> 1) - new_dx;
				ElementPtr->next.location.y = (LOG_SPACE_HEIGHT >> 1) - new_dy;
				hNextElement = 0;
			}

			UnlockElement (hElement);
		}
	}

	if (GLOBAL_SIS (FuelOnBoard)
			&& GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
	{
		COUNT cur_fuel_ticks;
		COUNT hyper_dist;
		DWORD adj_dx, adj_dy;

		if (new_dx < 0)
			new_dx = -new_dx;
		hyper_dx += new_dx;
		if (new_dy < 0)
			new_dy = -new_dy;
		hyper_dy += new_dy;

		/* These macros are also used in the fuel estimate on the starmap. */
		adj_dx = LOGX_TO_UNIVERSE(16 * hyper_dx);
		adj_dy = MAX_Y_UNIVERSE - LOGY_TO_UNIVERSE(16 * hyper_dy);

		hyper_dist = square_root (adj_dx * adj_dx + adj_dy * adj_dy) 
					+ hyper_extra;
		cur_fuel_ticks = hyper_dist >> 4;

		if (cur_fuel_ticks > (COUNT)fuel_ticks)
		{
#ifndef TESTING
			DeltaSISGauges (0, fuel_ticks - cur_fuel_ticks, 0);
#endif /* TESTING */
			if (cur_fuel_ticks > 0x00FF)
			{
				hyper_dx = 0;
				hyper_extra = hyper_dist & ((1 << 4) - 1);
				hyper_dy = 0;
				cur_fuel_ticks = 0;
			}

			fuel_ticks = (BYTE)cur_fuel_ticks;
		}
	}
}

void
check_hyperspace_encounter (void)
{
	BYTE Type;
	POINT universe;
	HSTARSHIP hStarShip, hNextShip;
	COUNT EncounterPercent[] =
	{
		RACE_HYPERSPACE_PERCENT
	};
	
	universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
	universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q)), Type = 0;
			hStarShip && (GLOBAL (CurrentActivity) & IN_BATTLE);
			hStarShip = hNextShip, ++Type)
	{
		COUNT encounter_radius;
		EXTENDED_SHIP_FRAGMENTPTR TemplatePtr;

		TemplatePtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (avail_race_q),
				hStarShip
				);
		hNextShip = _GetSuccLink (TemplatePtr);

		encounter_radius = TemplatePtr->ShipInfo.actual_strength;
		if (encounter_radius)
		{
			BYTE encounter_flags;
			SIZE dx, dy;
			COUNT percent;
			HENCOUNTER hEncounter;
			ENCOUNTERPTR EncounterPtr;

			encounter_flags = 0;
			percent = EncounterPercent[Type];
			if (encounter_radius != (COUNT)~0)
				encounter_radius =
						(encounter_radius * SPHERE_RADIUS_INCREMENT) >> 1;
			else
			{
				HENCOUNTER hNextEncounter;

				encounter_radius = (MAX_X_UNIVERSE + 1) << 1;
				if (Type == SLYLANDRO_SHIP)
				{
					encounter_flags = ONE_SHOT_ENCOUNTER;
					if (!GET_GAME_STATE (STARBASE_AVAILABLE))
						percent = 100;
					else
						percent *= GET_GAME_STATE (SLYLANDRO_MULTIPLIER);
				}
				else if (Type == MELNORME_SHIP
						&& (GLOBAL_SIS (FuelOnBoard) == 0
						|| GET_GAME_STATE (USED_BROADCASTER))
						&& GET_GAME_STATE (MELNORME_ANGER) < 3)
				{
					if (!GET_GAME_STATE (USED_BROADCASTER))
						percent = 30;
					else
						percent = 100;
					encounter_flags = ONE_SHOT_ENCOUNTER;
				}


				for (hEncounter = GetHeadEncounter ();
						hEncounter; hEncounter = hNextEncounter)
				{
					LockEncounter (hEncounter, &EncounterPtr);
					hNextEncounter = GetSuccEncounter (EncounterPtr);
					if (EncounterPtr->SD.Type == Type)
					{
						percent = 0;
						hNextEncounter = 0;
					}
					UnlockEncounter (hEncounter);
				}


				if (percent == 100 && Type == MELNORME_SHIP)
				{
					SET_GAME_STATE (BROADCASTER_RESPONSE, 1);
				}
			}

			if ((dx = universe.x - TemplatePtr->ShipInfo.loc.x) < 0)
				dx = -dx;
			if ((dy = universe.y - TemplatePtr->ShipInfo.loc.y) < 0)
				dy = -dy;
			if ((COUNT)dx < encounter_radius
					&& (COUNT)dy < encounter_radius
					&& (DWORD)dx * dx + (DWORD)dy * dy <
					(DWORD)encounter_radius * encounter_radius
					&& ((COUNT)TFB_Random () % 100) < percent)
			{
				// Ship spawned for encounter.
				hEncounter = AllocEncounter ();
				if (hEncounter)
				{
					LockEncounter (hEncounter, &EncounterPtr);
					memset (EncounterPtr, 0, sizeof (*EncounterPtr));
					EncounterPtr->origin = TemplatePtr->ShipInfo.loc;
					EncounterPtr->radius = encounter_radius;
					EncounterPtr->SD.Index = encounter_flags;
					EncounterPtr->SD.Type = Type;
					UnlockEncounter (hEncounter);

					PutEncounter (hEncounter);
				}
			}
		}

		UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);
	}

	SET_GAME_STATE (USED_BROADCASTER, 0);
}

void
FreeHyperData (void)
{
	DestroyDrawable (ReleaseDrawable (hyperstars[0]));
	hyperstars[0] = 0;
	DestroyDrawable (ReleaseDrawable (hyperstars[1]));
	hyperstars[1] = 0;
	DestroyDrawable (ReleaseDrawable (hyperstars[2]));
	hyperstars[2] = 0;

	DestroyColorMap (ReleaseColorMap (hypercmaps[0]));
	hypercmaps[0] = 0;
	DestroyColorMap (ReleaseColorMap (hypercmaps[1]));
	hypercmaps[1] = 0;
}

static void
LoadHyperData (void)
{
	if (hyperstars[0] == 0)
	{
		hyperstars[0] = CaptureDrawable (
				LoadGraphic (AMBIENT_MASK_PMAP_ANIM));
		hyperstars[1] = CaptureDrawable (
				LoadGraphic (HYPERSTARS_MASK_PMAP_ANIM));
		hypercmaps[0] = CaptureColorMap (LoadColorMap (HYPER_COLOR_TAB));

		hyperstars[2] = CaptureDrawable (
				LoadGraphic (ARISPACE_MASK_PMAP_ANIM));
		hypercmaps[1] = CaptureColorMap (LoadColorMap (ARISPACE_COLOR_TAB));
	}
}

BOOLEAN
LoadHyperspace (void)
{
	hyper_dx = hyper_dy = hyper_extra = 0;
	fuel_ticks = 1;

	GLOBAL (ShipStamp.origin.x) = -MAX_X_UNIVERSE;
	GLOBAL (ShipStamp.origin.y) = -MAX_Y_UNIVERSE;

	LoadHyperData ();
	{
		FRAME F;
		
		F = hyperstars[0];
		hyperstars[0] = stars_in_space;
		stars_in_space = F;
	}

	if (!(LastActivity & CHECK_LOAD))
		RepairSISBorder ();
	else
	{
		if (LOBYTE (LastActivity) == 0)
		{
			DrawSISFrame ();
		}
		else
		{
			ClearSISRect (DRAW_SIS_DISPLAY);
			RepairSISBorder ();
		}
	}
	DrawSISMessage (NULL_PTR);

	SetContext (RadarContext);
	SetContextBackGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x00, 0x0E, 0x00), 0x6C));

	SetContext (SpaceContext);
	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
	{
		SetContextBackGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x07, 0x00, 0x00), 0x2F));
		SetColorMap (GetColorMapAddress (hypercmaps[0]));
	}
	else
	{
		SetContextBackGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x1A, 0x00), 0x2F));
		SetColorMap (GetColorMapAddress (hypercmaps[1]));
		SET_GAME_STATE (USED_BROADCASTER, 0);
		SET_GAME_STATE (BROADCASTER_RESPONSE, 0);
	}
//    ClearDrawable ();

	ClearSISRect (CLEAR_SIS_RADAR);

	return (TRUE);
}

BOOLEAN
FreeHyperspace (void)
{
	UnlockMutex (GraphicsLock);
	SuspendGameClock ();
	LockMutex (GraphicsLock);

	{
		FRAME F;
		
		F = hyperstars[0];
		hyperstars[0] = stars_in_space;
		stars_in_space = F;
	}
//    FreeHyperData ();

	return (TRUE);
}

static void
ElementToUniverse (ELEMENTPTR ElementPtr, PPOINT pPt)
{
	SDWORD log_x, log_y;

	log_x = GLOBAL_SIS (log_x)
			+ (ElementPtr->next.location.x - (LOG_SPACE_WIDTH >> 1));
	log_y = GLOBAL_SIS (log_y)
			+ (ElementPtr->next.location.y - (LOG_SPACE_HEIGHT >> 1));
	pPt->x = LOGX_TO_UNIVERSE (log_x);
	pPt->y = LOGY_TO_UNIVERSE (log_y);
}

static void
cleanup_hyperspace (void)
{
	HENCOUNTER hEncounter, hNextEncounter;

	for (hEncounter = GetHeadEncounter ();
			hEncounter != 0; hEncounter = hNextEncounter)
	{
		ENCOUNTERPTR EncounterPtr;

		LockEncounter (hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter (EncounterPtr);
		if (EncounterPtr->hElement)
		{
			ELEMENTPTR ElementPtr;

			LockElement (EncounterPtr->hElement, &ElementPtr);

			if (ElementPtr->state_flags & BAD_GUY)
			{
				RemoveEncounter (hEncounter);
				InsertEncounter (hEncounter, GetHeadEncounter ());
			}

			UnlockElement (EncounterPtr->hElement);
		}
		EncounterPtr->hElement = 0;
		UnlockEncounter (hEncounter);
	}
}

typedef enum
{
	RANDOM_ENCOUNTER_TRANSITION,
	INTERPLANETARY_TRANSITION,
	ARILOU_SPACE_TRANSITION
} TRANSITION_TYPE;

static void
unhyper_transition (PELEMENT ElementPtr)
{
	COUNT frame_index;

	ElementPtr->state_flags |= CHANGING;

	frame_index = GetFrameIndex (ElementPtr->current.image.frame);
	if (frame_index == 0)
		frame_index += ANGLE_TO_FACING (FULL_CIRCLE);
	else if (frame_index < ANGLE_TO_FACING (FULL_CIRCLE))
		frame_index = NORMALIZE_FACING (frame_index + 1);
	else if (++frame_index == GetFrameCount (ElementPtr->current.image.frame))
	{
		cleanup_hyperspace ();

		GLOBAL (CurrentActivity) &= ~IN_BATTLE;
		switch ((TRANSITION_TYPE) ElementPtr->turn_wait)
		{
			case RANDOM_ENCOUNTER_TRANSITION:
				SaveFlagshipState ();
				GLOBAL (CurrentActivity) |= START_ENCOUNTER;
				break;
			case INTERPLANETARY_TRANSITION:
				GLOBAL (ShipStamp.frame) = 0;
				SET_GAME_STATE (USED_BROADCASTER, 0);
				if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
				{
					GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
					SET_GAME_STATE (ESCAPE_COUNTER, 0);
				}
				else
				{
					POINT pt;

					GLOBAL (autopilot.x) =
							GLOBAL (autopilot.y) = ~0;

					ElementToUniverse (ElementPtr, &pt);
					CurStarDescPtr = FindStar (NULL_PTR, &pt, 5, 5);
					if (CurStarDescPtr->star_pt.x == ARILOU_HOME_X
							&& CurStarDescPtr->star_pt.y == ARILOU_HOME_Y)
					{
						GLOBAL (CurrentActivity) |= START_ENCOUNTER;
					}
					else
					{
						COUNT index;
						POINT portal_pt[] =
						{
							{4091, 7748},
							{3184, 4906},
							{9211, 6104},
							{5673, 1207},
							{1910,  926},
							{8607,  151},
							{  50, 1647},
							{6117, 4131},
							{5658, 9712},
							{2302, 3988},
							{ 112, 9409},
							{7752, 8906},
							{ 368, 6332},
							{9735, 3153},
							{5850, 6213},
						};

						index = CurStarDescPtr
								- ((STAR_DESCPTR)&star_array[NUM_SOLAR_SYSTEMS + 1]);
						GLOBAL_SIS (log_x) =
								UNIVERSE_TO_LOGX (portal_pt[index].x);
						GLOBAL_SIS (log_y) =
								UNIVERSE_TO_LOGY (portal_pt[index].y);

						SET_GAME_STATE (ARILOU_SPACE_SIDE, 0);
					}
				}
				break;
			case ARILOU_SPACE_TRANSITION:
				/* Enter QuasiSpace from HyperSpace by any portal,
				 * or HyperSpace from QuasiSpace through the periodically
				 * opening portal.
				 */
				GLOBAL (ShipStamp.frame) = 0;
				SET_GAME_STATE (USED_BROADCASTER, 0);
				GLOBAL (autopilot.x) =
						GLOBAL (autopilot.y) = ~0;
				if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
				{
					// From HyperSpace to QuasiSpace.
					GLOBAL_SIS (log_x) =
							UNIVERSE_TO_LOGX (QUASI_SPACE_X);
					GLOBAL_SIS (log_y) =
							UNIVERSE_TO_LOGY (QUASI_SPACE_Y);
					if (GET_GAME_STATE (PORTAL_COUNTER) == 0)
					{
						// Periodically appearing portal.
						SET_GAME_STATE (ARILOU_SPACE_SIDE, 3);
					}
					else
					{
						// Player-induced portal.
						SET_GAME_STATE (PORTAL_COUNTER, 0);
						SET_GAME_STATE (ARILOU_SPACE_SIDE, 3);
					}
				}
				else
				{
					// From QuasiSpace to HyperSpace through the
					// periodically appearing portal.
					GLOBAL_SIS (log_x) =
							UNIVERSE_TO_LOGX (ARILOU_SPACE_X);
					GLOBAL_SIS (log_y) =
							UNIVERSE_TO_LOGY (ARILOU_SPACE_Y);
					SET_GAME_STATE (ARILOU_SPACE_SIDE, 0);
				}
				break;
		}

		ZeroVelocityComponents (&ElementPtr->velocity);
		SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
		return;
	}
	ElementPtr->next.image.frame =
			SetAbsFrameIndex (ElementPtr->current.image.frame, frame_index);
}

static void
init_transition (ELEMENTPTR ElementPtr0, ELEMENTPTR ElementPtr1,
		TRANSITION_TYPE which_transition)
{
	SIZE dx, dy, num_turns;
	STARSHIPPTR StarShipPtr;

	dx = WORLD_TO_VELOCITY (ElementPtr0->next.location.x
			- ElementPtr1->next.location.x);
	dy = WORLD_TO_VELOCITY (ElementPtr0->next.location.y
			- ElementPtr1->next.location.y);

	ElementPtr1->state_flags |= NONSOLID;
	ElementPtr1->preprocess_func = unhyper_transition;
	ElementPtr1->postprocess_func = NULL_PTR;
	ElementPtr1->turn_wait = (BYTE) which_transition;

	GetElementStarShip (ElementPtr1, &StarShipPtr);
	if ((num_turns = GetFrameCount (ElementPtr1->next.image.frame)
			- ANGLE_TO_FACING (FULL_CIRCLE)
			+ NORMALIZE_FACING (ANGLE_TO_FACING (FULL_CIRCLE)
			- StarShipPtr->ShipFacing)) == 0)
		num_turns = 1;

	SetVelocityComponents (&ElementPtr1->velocity,
			dx / num_turns, dy / num_turns);
}

BOOLEAN
hyper_transition (PELEMENT ElementPtr)
{
	if (ElementPtr->state_flags & APPEARING)
	{
		if (LastActivity & CHECK_LOAD)
		{
			LastActivity &= ~CHECK_LOAD;

			ElementPtr->current = ElementPtr->next;
			SetUpElement (ElementPtr);

			ElementPtr->state_flags |= DEFY_PHYSICS;

			return (FALSE);
		}
		else
		{
			ElementPtr->preprocess_func =
						(void (*) (struct element
								*ElementPtr))hyper_transition;
			ElementPtr->postprocess_func = NULL_PTR;
			ElementPtr->state_flags |= NONSOLID;
			ElementPtr->next.image.frame =
					SetAbsFrameIndex (ElementPtr->current.image.frame,
					GetFrameCount (ElementPtr->current.image.frame) - 1);
		}
	}
	else
	{
		COUNT frame_index;

		frame_index = GetFrameIndex (ElementPtr->current.image.frame);
		if (frame_index-- <= ANGLE_TO_FACING (FULL_CIRCLE))
		{
			STARSHIPPTR StarShipPtr;

			if (frame_index == ANGLE_TO_FACING (FULL_CIRCLE) - 1)
				frame_index = 0;
			else
				frame_index = NORMALIZE_FACING (frame_index);

			GetElementStarShip (ElementPtr, &StarShipPtr);
			if (frame_index == StarShipPtr->ShipFacing)
			{
				ElementPtr->preprocess_func = ship_preprocess;
				ElementPtr->postprocess_func = ship_postprocess;
				ElementPtr->state_flags &= ~NONSOLID;
			}
		}

		ElementPtr->state_flags |= CHANGING;
		ElementPtr->next.image.frame =
				SetAbsFrameIndex (ElementPtr->current.image.frame,
				frame_index);

		if (!(ElementPtr->state_flags & NONSOLID))
		{
			ElementPtr->current = ElementPtr->next;
			SetUpElement (ElementPtr);

			ElementPtr->state_flags |= DEFY_PHYSICS;
		}
	}

	return (TRUE);
}

static void
hyper_collision (PELEMENT ElementPtr0, PPOINT
		pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	if ((ElementPtr1->state_flags & PLAYER_SHIP)
			&& GET_GAME_STATE (PORTAL_COUNTER) == 0)
	{
		SIZE dx, dy;
		POINT pt;
		STAR_DESCPTR SDPtr;
		STARSHIPPTR StarShipPtr;

		ElementToUniverse (ElementPtr0, &pt);

		SDPtr = FindStar (NULL_PTR, &pt, 5, 5);

		GetElementStarShip (ElementPtr1, &StarShipPtr);
		GetCurrentVelocityComponents (&ElementPtr1->velocity, &dx, &dy);
		if (SDPtr == CurStarDescPtr
				|| (ElementPtr1->state_flags & APPEARING)
				|| !(dx || dy || (StarShipPtr->cur_status_flags
				& (LEFT | RIGHT | THRUST | WEAPON | SPECIAL))))
		{
			CurStarDescPtr = SDPtr;
			ElementPtr0->state_flags |= DEFY_PHYSICS | COLLISION;
		}
		else if ((GLOBAL (CurrentActivity) & IN_BATTLE)
				&& (GLOBAL (autopilot.x) == ~0
				|| GLOBAL (autopilot.y) == ~0
				|| (GLOBAL (autopilot.x) == SDPtr->star_pt.x
				&& GLOBAL (autopilot.y) == SDPtr->star_pt.y)))
		{
			CurStarDescPtr = SDPtr;
			ElementPtr0->state_flags |= COLLISION;

			init_transition (ElementPtr0, ElementPtr1,
					INTERPLANETARY_TRANSITION);
		}
	}
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static void
hyper_death (PELEMENT ElementPtr)
{
	if (!(ElementPtr->state_flags & DEFY_PHYSICS)
			&& (GLOBAL (CurrentActivity) & IN_BATTLE))
		CurStarDescPtr = 0;
}

static void
arilou_space_death (PELEMENT ElementPtr)
{
	if (!(ElementPtr->state_flags & DEFY_PHYSICS)
			|| GET_GAME_STATE (ARILOU_SPACE_COUNTER) == 0)
	{
		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		{
			SET_GAME_STATE (ARILOU_SPACE_SIDE, 0);
		}
		else
		{
			SET_GAME_STATE (ARILOU_SPACE_SIDE, 3);
		}
	}
}

static void
arilou_space_collision (PELEMENT ElementPtr0,
		PPOINT pPt0, PELEMENT ElementPtr1, PPOINT
		pPt1)
{
	COUNT which_side;

	if (!(ElementPtr1->state_flags & PLAYER_SHIP))
		return;

	which_side = GET_GAME_STATE (ARILOU_SPACE_SIDE);
	if (which_side == 0 || which_side == 3)
	{
		if (ElementPtr1->state_flags & DEFY_PHYSICS)
		{
			SET_GAME_STATE (ARILOU_SPACE_SIDE, which_side ^ 1);
		}
		else
		{
			init_transition (ElementPtr0, ElementPtr1, ARILOU_SPACE_TRANSITION);
		}
	}

	ElementPtr0->state_flags |= DEFY_PHYSICS | COLLISION;
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static HELEMENT
AllocHyperElement (STAR_DESCPTR SDPtr)
{
	HELEMENT hHyperSpaceElement;

	hHyperSpaceElement = AllocElement ();
	if (hHyperSpaceElement)
	{
		ELEMENTPTR HyperSpaceElementPtr;

		LockElement (hHyperSpaceElement, &HyperSpaceElementPtr);
		HyperSpaceElementPtr->state_flags = CHANGING | FINITE_LIFE;
		HyperSpaceElementPtr->life_span = 1;
		HyperSpaceElementPtr->mass_points = 1;

		{
			long lx, ly;

			lx = UNIVERSE_TO_LOGX (SDPtr->star_pt.x)
					+ (LOG_SPACE_WIDTH >> 1)
					- GLOBAL_SIS (log_x);
			HyperSpaceElementPtr->current.location.x = WRAP_X (lx);

			ly = UNIVERSE_TO_LOGY (SDPtr->star_pt.y)
					+ (LOG_SPACE_HEIGHT >> 1)
					- GLOBAL_SIS (log_y);
			HyperSpaceElementPtr->current.location.y = WRAP_Y (ly);
		}

		SetPrimType (&DisplayArray[HyperSpaceElementPtr->PrimIndex], STAMP_PRIM);
		HyperSpaceElementPtr->current.image.farray =
				&hyperstars[1 + (GET_GAME_STATE (ARILOU_SPACE_SIDE) >> 1)];

		UnlockElement (hHyperSpaceElement);
	}

	return (hHyperSpaceElement);
}

static void
AddAmbientElement (void)
{
	HELEMENT hHyperSpaceElement;

	hHyperSpaceElement = AllocElement ();
	if (hHyperSpaceElement)
	{
		SIZE dx, dy;
		DWORD rand_val;
		ELEMENTPTR HyperSpaceElementPtr;

		LockElement (hHyperSpaceElement, &HyperSpaceElementPtr);
		HyperSpaceElementPtr->state_flags =
				APPEARING | FINITE_LIFE | NONSOLID;
		SetPrimType (&DisplayArray[HyperSpaceElementPtr->PrimIndex], STAMP_PRIM);
		HyperSpaceElementPtr->preprocess_func = animation_preprocess;

		rand_val = TFB_Random ();
		dy = LOWORD (rand_val);
		dx = (SIZE)(LOBYTE (dy) % SPACE_WIDTH) - (SPACE_WIDTH >> 1);
		dy = (SIZE)(HIBYTE (dy) % SPACE_HEIGHT) - (SPACE_HEIGHT >> 1);
		HyperSpaceElementPtr->current.location.x = (LOG_SPACE_WIDTH >> 1)
				+ DISPLAY_TO_WORLD (dx);
		HyperSpaceElementPtr->current.location.y = (LOG_SPACE_HEIGHT >> 1)
				+ DISPLAY_TO_WORLD (dy);
		HyperSpaceElementPtr->current.image.farray = &stars_in_space;

		if (HIWORD (rand_val) & 7)
		{
			HyperSpaceElementPtr->life_span = 14;
			HyperSpaceElementPtr->current.image.frame = stars_in_space;
		}
		else
		{
			HyperSpaceElementPtr->life_span = 12;
			HyperSpaceElementPtr->current.image.frame =
					SetAbsFrameIndex (stars_in_space, 14);
		}

		UnlockElement (hHyperSpaceElement);

		InsertElement (hHyperSpaceElement, GetHeadElement ());
	}
}

#define NUM_VORTEX_TRANSITIONS 9
#define VORTEX_WAIT 1

static void
encounter_transition (PELEMENT ElementPtr)
{
	ElementPtr->state_flags &= ~DISAPPEARING;
	ElementPtr->life_span = 1;
	if (ElementPtr->turn_wait)
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		FRAME f;

		if (ElementPtr->hit_points)
		{
			f = DecFrameIndex (ElementPtr->current.image.frame);
			ElementPtr->next.image.frame = f;
		}
		else
		{
			f = IncFrameIndex (ElementPtr->current.image.frame);
			if (f != ElementPtr->current.image.farray[0])
				ElementPtr->next.image.frame = f;
			else
				ElementPtr->death_func = NULL_PTR;
		}

		ElementPtr->turn_wait = VORTEX_WAIT;
	}
}

static void
encounter_collision (PELEMENT ElementPtr0, PPOINT
		pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{

	if ((ElementPtr1->state_flags & PLAYER_SHIP)
			&& (GLOBAL (CurrentActivity) & IN_BATTLE))
	{
		HENCOUNTER hEncounter, hNextEncounter;

		init_transition (ElementPtr0, ElementPtr1, RANDOM_ENCOUNTER_TRANSITION);

		for (hEncounter = GetHeadEncounter ();
				hEncounter != 0; hEncounter = hNextEncounter)
		{
			ENCOUNTERPTR EncounterPtr;

			LockEncounter (hEncounter, &EncounterPtr);
			hNextEncounter = GetSuccEncounter (EncounterPtr);
			if (EncounterPtr->hElement)
			{
				ELEMENTPTR ElementPtr;

				LockElement (EncounterPtr->hElement, &ElementPtr);

				ElementPtr->state_flags |= NONSOLID | IGNORE_SIMILAR;

				UnlockElement (EncounterPtr->hElement);
			}
			UnlockEncounter (hEncounter);
		}

		ElementPtr0->state_flags |= BAD_GUY;
		ZeroVelocityComponents (&ElementPtr0->velocity);
	}
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static HELEMENT
AddEncounterElement (ENCOUNTERPTR EncounterPtr,
		PPOINT puniverse)
{
	BOOLEAN NewEncounter;
	HELEMENT hElement;
	STAR_DESC SD;
	

	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) >= 2)
		return (0);

	if (EncounterPtr->SD.Index & ENCOUNTER_REFORMING)
	{
		EncounterPtr->SD.Index &= ~ENCOUNTER_REFORMING;

		EncounterPtr->transition_state = 100;
		if ((EncounterPtr->SD.Index & ONE_SHOT_ENCOUNTER)
				|| LONIBBLE (EncounterPtr->SD.Index) == 0)
			return (0);
	}

	if (LONIBBLE (EncounterPtr->SD.Index))
	{
		NewEncounter = FALSE;
		SD.star_pt = EncounterPtr->SD.star_pt;
	}
	else
	{
		BYTE Type;
		SIZE dx, dy;
		COUNT i, NumShips;
		DWORD radius_squared;
		BYTE EncounterMakeup[] =
		{
			RACE_ENCOUNTER_MAKEUP
		};

		NewEncounter = TRUE;

		radius_squared = (DWORD)EncounterPtr->radius * EncounterPtr->radius;

		Type = EncounterPtr->SD.Type;
		NumShips = LONIBBLE (EncounterMakeup[Type]);
		for (i = HINIBBLE (EncounterMakeup[Type]) - NumShips; i; --i)
		{
			if ((COUNT)TFB_Random () % 100 < 50)
				++NumShips;
		}

		if (NumShips > MAX_HYPER_SHIPS)
			NumShips = MAX_HYPER_SHIPS;


		EncounterPtr->SD.Index = MAKE_BYTE (
				NumShips, HINIBBLE (EncounterPtr->SD.Index));
		for (i = 0; i < NumShips; ++i)
		{
			HSTARSHIP hStarShip;
			SHIP_FRAGMENTPTR TemplatePtr;

			hStarShip = GetStarShipFromIndex (&GLOBAL (avail_race_q), Type);
			TemplatePtr = (SHIP_FRAGMENTPTR)LockStarShip (
					&GLOBAL (avail_race_q), hStarShip);
			EncounterPtr->SD.ShipList[i] = TemplatePtr->ShipInfo;
			EncounterPtr->SD.ShipList[i].var1 = Type;
			UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);
		}


		do
		{
			DWORD rand_val;

			rand_val = TFB_Random ();

			SD.star_pt.x = puniverse->x
					+ (LOWORD (rand_val) % (XOFFS << 1)) - XOFFS;
			if (SD.star_pt.x < 0)
				SD.star_pt.x = 0;
			else if (SD.star_pt.x > MAX_X_UNIVERSE)
				SD.star_pt.x = MAX_X_UNIVERSE;
			SD.star_pt.y = puniverse->y
					+ (HIWORD (rand_val) % (YOFFS << 1)) - YOFFS;
			if (SD.star_pt.y < 0)
				SD.star_pt.y = 0;
			else if (SD.star_pt.y > MAX_Y_UNIVERSE)
				SD.star_pt.y = MAX_Y_UNIVERSE;

			dx = SD.star_pt.x - EncounterPtr->origin.x;
			dy = SD.star_pt.y - EncounterPtr->origin.y;
		} while ((DWORD)((long)dx * dx + (long)dy * dy) > radius_squared);

		EncounterPtr->SD.star_pt = SD.star_pt;
		EncounterPtr->log_x = UNIVERSE_TO_LOGX (SD.star_pt.x);
		EncounterPtr->log_y = UNIVERSE_TO_LOGY (SD.star_pt.y);
	}

	hElement = AllocHyperElement (&SD);
	if (hElement)
	{
		SIZE i;
		ELEMENTPTR ElementPtr;


		LockElement (hElement, &ElementPtr);
		
		if ((i = EncounterPtr->transition_state) || NewEncounter)
		{
			if (i < 0)
			{
				i = -i;
				ElementPtr->hit_points = 1;
			}
			if (i == 0 || i > NUM_VORTEX_TRANSITIONS)
				i = NUM_VORTEX_TRANSITIONS;

			ElementPtr->current.image.frame = SetRelFrameIndex (
					ElementPtr->current.image.farray[0], -i);
			ElementPtr->death_func = encounter_transition;
		}
		else
		{
			ElementPtr->current.image.frame =
					DecFrameIndex (ElementPtr->current.image.farray[0]);
		}

		ElementPtr->turn_wait = VORTEX_WAIT;
		ElementPtr->preprocess_func = NULL_PTR;
		ElementPtr->postprocess_func = NULL_PTR;
		ElementPtr->collision_func = encounter_collision;

		SetUpElement (ElementPtr);

		ElementPtr->IntersectControl.IntersectStamp.frame =
				DecFrameIndex (stars_in_space);
		SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
		ElementPtr->state_flags |= NONSOLID | IGNORE_VELOCITY;

		UnlockElement (hElement);

		InsertElement (hElement, GetTailElement ());
	}
	
	return (EncounterPtr->hElement = hElement);
}

#define GRID_OFFSET 200

static void
DrawHyperGrid (COORD ux, COORD uy, COORD ox,
		COORD oy)
{
	COORD sx, sy, ex, ey;
	RECT r;

	ClearDrawable ();
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x00, 0x10, 0x00), 0x6B));

	if ((sx = ux - (RADAR_SCAN_WIDTH >> 1)) < 0)
		sx = 0;
	else
		sx -= sx % GRID_OFFSET;
	if ((ex = ux + (RADAR_SCAN_WIDTH >> 1)) > MAX_X_UNIVERSE + 1)
		ex = MAX_X_UNIVERSE + 1;

	if ((sy = uy - (RADAR_SCAN_HEIGHT >> 1)) < 0)
		sy = 0;
	else
		sy -= sy % GRID_OFFSET;
	if ((ey = uy + (RADAR_SCAN_HEIGHT >> 1)) > MAX_Y_UNIVERSE + 1)
		ey = MAX_Y_UNIVERSE + 1;

	r.corner.y = (COORD) ((long)(MAX_Y_UNIVERSE - ey)
			* RADAR_HEIGHT / RADAR_SCAN_HEIGHT) - oy;
	r.extent.width = 1;
	r.extent.height = ((COORD) ((long)(MAX_Y_UNIVERSE - sy)
			* RADAR_HEIGHT / RADAR_SCAN_HEIGHT) - oy) - r.corner.y + 1;
	for (ux = sx; ux <= ex; ux += GRID_OFFSET)
	{
		r.corner.x = (COORD) ((long)ux
				* RADAR_WIDTH / RADAR_SCAN_WIDTH) - ox;
		DrawFilledRectangle (&r);
	} while ((ux += GRID_OFFSET) < ex);

	r.corner.x = (COORD) ((long)sx * RADAR_WIDTH / RADAR_SCAN_WIDTH) - ox;
	r.extent.width = ((COORD) ((long)ex
			* RADAR_WIDTH / RADAR_SCAN_WIDTH) - ox)
			- r.corner.x + 1;
	r.extent.height = 1;
	for (uy = sy; uy <= ey; uy += GRID_OFFSET)
	{
		r.corner.y = (COORD)((long)(MAX_Y_UNIVERSE - uy)
				* RADAR_HEIGHT / RADAR_SCAN_HEIGHT) - oy;
		DrawFilledRectangle (&r);
	} while ((uy += GRID_OFFSET) < ey);
}


static void
ProcessEncounters (PPOINT puniverse, COORD ox,
		COORD oy)
{
	STAMP s;
	HENCOUNTER hEncounter, hNextEncounter;

	s.frame = SetAbsFrameIndex (stars_in_space, 91);
	for (hEncounter = GetHeadEncounter ();
			hEncounter; hEncounter = hNextEncounter)
	{
		COORD ex, ey;
		ENCOUNTERPTR EncounterPtr;

		LockEncounter (hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter (EncounterPtr);

		if (EncounterPtr->hElement == 0
				&& AddEncounterElement (EncounterPtr, puniverse) == 0)
		{
DeleteEncounter:
			UnlockEncounter (hEncounter);

			RemoveEncounter (hEncounter);
			FreeEncounter (hEncounter);
			continue;
		}

		{
			ELEMENTPTR ElementPtr;

			LockElement (EncounterPtr->hElement, &ElementPtr);

			if (ElementPtr->death_func)
			{
				if (EncounterPtr->transition_state
						&& ElementPtr->turn_wait == 0)
				{
					--EncounterPtr->transition_state;
					if (EncounterPtr->transition_state >=
							NUM_VORTEX_TRANSITIONS)
						++ElementPtr->turn_wait;
					else if (EncounterPtr->transition_state ==
							-NUM_VORTEX_TRANSITIONS)
					{
						ElementPtr->death_func = NULL_PTR;
						UnlockElement (EncounterPtr->hElement);
						goto DeleteEncounter;
					}
					else
						SetPrimType (&DisplayArray[ElementPtr->PrimIndex], STAMP_PRIM);
				}
			}
			else
			{
				SIZE delta_x, delta_y;
				COUNT encounter_radius;

				ElementPtr->life_span = 1;
				GetNextVelocityComponents (&ElementPtr->velocity,
						&delta_x, &delta_y, 1);
				if (ElementPtr->thrust_wait)
					--ElementPtr->thrust_wait;
				else if (!(ElementPtr->state_flags & BAD_GUY))
				{
					COUNT cur_facing, delta_facing;

					cur_facing = ANGLE_TO_FACING (
							GetVelocityTravelAngle (&ElementPtr->velocity));
					delta_facing = NORMALIZE_FACING (cur_facing
							- ANGLE_TO_FACING (ARCTAN (
							puniverse->x - EncounterPtr->SD.star_pt.x,
							puniverse->y - EncounterPtr->SD.star_pt.y)));
					if (delta_facing || (delta_x == 0 && delta_y == 0))
					{
						SIZE s, RaceHyperSpeed[] =
						{
							RACE_HYPER_SPEED
						};

#define ENCOUNTER_TRACK_WAIT 3
						s = RaceHyperSpeed[EncounterPtr->SD.Type];
						if (delta_facing < ANGLE_TO_FACING (HALF_CIRCLE))
							--cur_facing;
						else
							++cur_facing;
						if (NORMALIZE_FACING (delta_facing
								+ ANGLE_TO_FACING (OCTANT)) > ANGLE_TO_FACING (QUADRANT))
						{
							if (delta_facing < ANGLE_TO_FACING (HALF_CIRCLE))
								--cur_facing;
							else
								++cur_facing;
							s >>= 1;
						}
						cur_facing = FACING_TO_ANGLE (cur_facing);
						SetVelocityComponents (
								&ElementPtr->velocity,
								COSINE (cur_facing, s),
								SINE (cur_facing, s)
								);
						GetNextVelocityComponents (&ElementPtr->velocity,
								&delta_x, &delta_y, 1);

						ElementPtr->thrust_wait = ENCOUNTER_TRACK_WAIT;
					}
				}
				EncounterPtr->log_x += delta_x;
				EncounterPtr->log_y -= delta_y;
				EncounterPtr->SD.star_pt.x =
						LOGX_TO_UNIVERSE (EncounterPtr->log_x);
				EncounterPtr->SD.star_pt.y =
						LOGY_TO_UNIVERSE (EncounterPtr->log_y);

				encounter_radius = EncounterPtr->radius + (GRID_OFFSET >> 1);
				if ((delta_x = EncounterPtr->SD.star_pt.x
						- EncounterPtr->origin.x) < 0)
					delta_x = -delta_x;
				if ((delta_y = EncounterPtr->SD.star_pt.y
						- EncounterPtr->origin.y) < 0)
					delta_y = -delta_y;
				if ((COUNT)delta_x >= encounter_radius
						|| (COUNT)delta_y >= encounter_radius
						|| (DWORD)delta_x * delta_x + (DWORD)delta_y * delta_y >=
						(DWORD)encounter_radius * encounter_radius)
				{
					ElementPtr->state_flags |= NONSOLID;
					ElementPtr->life_span = 0;

					if (EncounterPtr->transition_state == 0)
					{
						ElementPtr->death_func = encounter_transition;
						EncounterPtr->transition_state = -1;
						ElementPtr->hit_points = 1;
					}
					else
					{
						ElementPtr->death_func = NULL_PTR;
						UnlockElement (EncounterPtr->hElement);
						goto DeleteEncounter;
					}
				}
			}

			ex = EncounterPtr->SD.star_pt.x;
			ey = EncounterPtr->SD.star_pt.y;
			if (ex - puniverse->x >= -UNIT_SCREEN_WIDTH
					&& ex - puniverse->x <= UNIT_SCREEN_WIDTH
					&& ey - puniverse->y >= -UNIT_SCREEN_HEIGHT
					&& ey - puniverse->y <= UNIT_SCREEN_HEIGHT)
			{
				ElementPtr->next.location.x = (SIZE)(
						EncounterPtr->log_x - GLOBAL_SIS (log_x)
						) + (LOG_SPACE_WIDTH >> 1);
				ElementPtr->next.location.y = (SIZE)(
						EncounterPtr->log_y - GLOBAL_SIS (log_y)
						) + (LOG_SPACE_HEIGHT >> 1);
				if ((ElementPtr->state_flags & NONSOLID)
						&& EncounterPtr->transition_state == 0)
				{
					ElementPtr->current.location =
							ElementPtr->next.location;
					SetPrimType (&DisplayArray[ElementPtr->PrimIndex], STAMP_PRIM);
					if (ElementPtr->death_func == 0)
					{
						InitIntersectStartPoint (ElementPtr);
						ElementPtr->state_flags &= ~NONSOLID;
					}
				}
			}
			else
			{
				ElementPtr->state_flags |= NONSOLID;
				if (ex - puniverse->x < -XOFFS
						|| ex - puniverse->x > XOFFS
						|| ey - puniverse->y < -YOFFS
						|| ey - puniverse->y > YOFFS)
				{
					ElementPtr->life_span = 0;
					ElementPtr->death_func = NULL_PTR;
					UnlockElement (EncounterPtr->hElement);

					goto DeleteEncounter;
				}

				SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
			}

			UnlockElement (EncounterPtr->hElement);
		}

		s.origin.x = (COORD)((long)ex * RADAR_WIDTH
				/ RADAR_SCAN_WIDTH) - ox;
		s.origin.y = (COORD)((long)(MAX_Y_UNIVERSE - ey) * RADAR_HEIGHT
				/ RADAR_SCAN_HEIGHT) - oy;
		DrawStamp (&s);

		UnlockEncounter (hEncounter);
	}
}

void
SeedUniverse (void)
{
	COORD ox, oy;
	COORD sx, sy, ex, ey;
	SWORD portalCounter, arilouSpaceCounter, arilouSpaceSide;
	POINT universe;
	FRAME blip_frame;
	STAMP s;
	STAR_DESCPTR SDPtr;
	HELEMENT hHyperSpaceElement;
	ELEMENTPTR HyperSpaceElementPtr;

	ClockTick ();

	universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
	universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));

	blip_frame = SetAbsFrameIndex (stars_in_space, 90);

	SetContext (RadarContext);
	BatchGraphics ();
	
	ox = (COORD)((long)universe.x
			* RADAR_WIDTH / RADAR_SCAN_WIDTH)
			- (RADAR_WIDTH >> 1);
	oy = (COORD)((long)(MAX_Y_UNIVERSE - universe.y)
			* RADAR_HEIGHT / RADAR_SCAN_HEIGHT)
			- (RADAR_HEIGHT >> 1);

	ex = (COORD)((long)GLOBAL (ShipStamp.origin.x)
			* RADAR_WIDTH / RADAR_SCAN_WIDTH)
			- (RADAR_WIDTH >> 1);
	ey = (COORD)((long)(MAX_Y_UNIVERSE
			- GLOBAL (ShipStamp.origin.y))
			* RADAR_HEIGHT / RADAR_SCAN_HEIGHT)
			- (RADAR_HEIGHT >> 1);

	arilouSpaceCounter = GET_GAME_STATE (ARILOU_SPACE_COUNTER);
	arilouSpaceSide = GET_GAME_STATE (ARILOU_SPACE_SIDE);

//    if (ox != ex || oy != ey)
	{
		DrawHyperGrid (universe.x, universe.y, ox, oy);

		{
			SDPtr = 0;
			while ((SDPtr = FindStar (SDPtr, &universe, XOFFS, YOFFS)))
			{
				BYTE star_type;

				ex = SDPtr->star_pt.x;
				ey = SDPtr->star_pt.y;
				star_type = STAR_TYPE (SDPtr->Type);
				if (arilouSpaceSide >= 2 &&
						ex == ARILOU_HOME_X && ey == ARILOU_HOME_Y)
					star_type = SUPER_GIANT_STAR;

				s.origin.x = (COORD)((long)ex * RADAR_WIDTH
						/ RADAR_SCAN_WIDTH) - ox;
				s.origin.y = (COORD)((long)(MAX_Y_UNIVERSE - ey) * RADAR_HEIGHT
						/ RADAR_SCAN_HEIGHT) - oy;
				s.frame = SetRelFrameIndex (blip_frame,
						star_type + 2);
				DrawStamp (&s);
			}
		}
	}

	portalCounter = GET_GAME_STATE (PORTAL_COUNTER);
	if (portalCounter || arilouSpaceCounter)
	{
		COUNT i;
		STAR_DESC SD[2];
				// This array is filled with the STAR_DESC's of
				// QuasiSpace portals that need to be taken into account.
				// i is set to the number of active portals (max 2).

		i = 0;
		if (portalCounter)
		{
			// A player-created QuasiSpace portal is opening.
			static POINT portal_pt;

			SD[i].Index = ((portalCounter - 1) >> 1) + 18;
			if (portalCounter == 1)
				portal_pt = universe;
			SD[i].star_pt = portal_pt;
			++i;

			if (++portalCounter == (10 + 1))
				portalCounter = (9 + 1);

			SET_GAME_STATE (PORTAL_COUNTER, portalCounter);
		}

		if (arilouSpaceCounter)
		{
			// The periodically appearing QuasiSpace portal is open.
			SD[i].Index = arilouSpaceCounter >> 1;
			if (arilouSpaceSide <= 1)
			{
				// The player is in HyperSpace
				SD[i].Index += 18;
				SD[i].star_pt.x = ARILOU_SPACE_X;
				SD[i].star_pt.y = ARILOU_SPACE_Y;
			}
			else
			{
				// The player is in QuasiSpace
				SD[i].star_pt.x = QUASI_SPACE_X;
				SD[i].star_pt.y = QUASI_SPACE_Y;
			}
			++i;
		}

		// Process the i portals from SD.
		do
		{
			--i;
			sx = SD[i].star_pt.x - universe.x + XOFFS;
			sy = SD[i].star_pt.y - universe.y + YOFFS;
			if (sx >= 0 && sy >= 0
					&& sx < (XOFFS << 1) && sy < (YOFFS << 1))
			{
				ex = SD[i].star_pt.x;
				ey = SD[i].star_pt.y;
				s.origin.x = (COORD)((long)ex * RADAR_WIDTH
						/ RADAR_SCAN_WIDTH) - ox;
				s.origin.y = (COORD)((long)(MAX_Y_UNIVERSE - ey)
						* RADAR_HEIGHT
						/ RADAR_SCAN_HEIGHT) - oy;
				s.frame = SetAbsFrameIndex (stars_in_space, 95);
				DrawStamp (&s);

				if ((ex -= universe.x) < 0)
					ex = -ex;
				if ((ey -= universe.y) < 0)
					ey = -ey;
				if (ex <= (XOFFS / NUM_RADAR_SCREENS)
						&& ey <= (YOFFS / NUM_RADAR_SCREENS)
						&& (hHyperSpaceElement =
						AllocHyperElement ((STAR_DESCPTR)&SD[i])) != 0)
				{
					LockElement (hHyperSpaceElement, &HyperSpaceElementPtr);
					HyperSpaceElementPtr->current.image.frame = SetAbsFrameIndex (
							hyperstars[1 + (GET_GAME_STATE (ARILOU_SPACE_SIDE) >> 1)],
							SD[i].Index
							);
					HyperSpaceElementPtr->preprocess_func =
							HyperSpaceElementPtr->postprocess_func = NULL_PTR;
					HyperSpaceElementPtr->collision_func = arilou_space_collision;

					SetUpElement (HyperSpaceElementPtr);

					if (arilouSpaceSide == 1 || arilouSpaceSide == 2)
						HyperSpaceElementPtr->death_func = arilou_space_death;
					else
					{
						HyperSpaceElementPtr->death_func = NULL_PTR;
						HyperSpaceElementPtr->IntersectControl.IntersectStamp.frame =
								DecFrameIndex (stars_in_space);
					}

					UnlockElement (hHyperSpaceElement);

					InsertElement (hHyperSpaceElement, GetHeadElement ());
				}
			}
		} while (i);
	}

	{
		SDPtr = 0;
		while ((SDPtr = FindStar (SDPtr, &universe, XOFFS, YOFFS)))
		{
			ex = SDPtr->star_pt.x;
			ey = SDPtr->star_pt.y;
			if ((ex -= universe.x) < 0)
				ex = -ex;
			if ((ey -= universe.y) < 0)
				ey = -ey;
			if (ex <= (XOFFS / NUM_RADAR_SCREENS)
					&& ey <= (YOFFS / NUM_RADAR_SCREENS)
					&& (hHyperSpaceElement = AllocHyperElement (SDPtr)) != 0)
			{
				BYTE star_type;

				star_type = SDPtr->Type;

				LockElement (hHyperSpaceElement, &HyperSpaceElementPtr);
				HyperSpaceElementPtr->current.image.frame = SetAbsFrameIndex (
						hyperstars[1 + (GET_GAME_STATE (ARILOU_SPACE_SIDE) >> 1)],
						STAR_TYPE (star_type)
						* NUM_STAR_COLORS
						+ STAR_COLOR (star_type)
						);
				HyperSpaceElementPtr->preprocess_func =
						HyperSpaceElementPtr->postprocess_func = NULL_PTR;
				HyperSpaceElementPtr->collision_func = hyper_collision;

				SetUpElement (HyperSpaceElementPtr);

				if (SDPtr == CurStarDescPtr
						&& GET_GAME_STATE (PORTAL_COUNTER) == 0)
					HyperSpaceElementPtr->death_func = hyper_death;
				else
				{
					HyperSpaceElementPtr->death_func = NULL_PTR;
					HyperSpaceElementPtr->IntersectControl.IntersectStamp.frame =
							DecFrameIndex (stars_in_space);
				}
				UnlockElement (hHyperSpaceElement);

				InsertElement (hHyperSpaceElement, GetHeadElement ());
			}
		}
		ProcessEncounters (&universe, ox, oy);
	}

	s.origin.x = RADAR_WIDTH >> 1;
	s.origin.y = RADAR_HEIGHT >> 1;
	s.frame = blip_frame;
	DrawStamp (&s);

	{
		// draws borders to mini-map
		
		RECT r;
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x0E, 0x0E, 0x0E), 0x00));
		r.corner.x = 0;
		r.corner.y = 0;
		r.extent.width = RADAR_WIDTH - 1;
		r.extent.height = 1;
		DrawFilledRectangle (&r);
		r.extent.width = 1;
		r.extent.height = RADAR_HEIGHT - 1;
		DrawFilledRectangle (&r);

		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x06, 0x06, 0x06), 0x00));
		r.corner.x = RADAR_WIDTH - 1;
		r.corner.y = 1;
		r.extent.height = RADAR_HEIGHT - 1;
		DrawFilledRectangle (&r);
		r.corner.x = 1;
		r.corner.y = RADAR_HEIGHT - 1;
		r.extent.width = RADAR_WIDTH - 2;
		r.extent.height = 1;
		DrawFilledRectangle (&r);

		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x08, 0x08, 0x08), 0x00));
		r.corner.x = 0;
		DrawPoint (&r.corner);
		r.corner.x = RADAR_WIDTH - 1;
		r.corner.y = 0;
		DrawPoint (&r.corner);
	}

	UnbatchGraphics ();

	SetContext (StatusContext);

	if (!(LOWORD (TFB_Random ()) & 7))
		AddAmbientElement ();

	if (universe.x != GLOBAL (ShipStamp.origin.x)
			|| universe.y != GLOBAL (ShipStamp.origin.y))
	{
		GLOBAL (ShipStamp.origin) = universe;
		DrawHyperCoords (universe);
	}
}

void
DoMenuOptions (void)
{
	COLOR OldColor;
	CONTEXT OldContext;
	MENU_STATE MenuState;

UnbatchGraphics ();

	OldContext = SetContext (SpaceContext);
	OldColor = SetContextBackGroundColor (BLACK_COLOR);

	UnlockMutex (GraphicsLock);
	SuspendGameClock ();

	memset ((PMENU_STATE)&MenuState, 0, sizeof (MenuState));
	MenuState.InputFunc = DoFlagshipCommands;
	MenuState.Initialized = 1;
	MenuState.CurState = STARMAP + 1;

	DrawMenuStateStrings (PM_STARMAP, STARMAP);
	LockMutex (GraphicsLock);
	SetFlashRect (SFR_MENU_3DO, (FRAME)0);
	UnlockMutex (GraphicsLock);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	DoInput ((PVOID)&MenuState, TRUE);

	LockMutex (GraphicsLock);
	SetFlashRect (NULL_PTR, (FRAME)0);

	SetContext (SpaceContext);

	if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
	{
		ClearSISRect (CLEAR_SIS_RADAR);
		UnlockMutex (GraphicsLock);
		WaitForNoInput (ONE_SECOND / 2);
		ResumeGameClock ();
		LockMutex (GraphicsLock);
	}

	SetContextBackGroundColor (OldColor);
	SetContext (OldContext);
	if (!(GLOBAL (CurrentActivity) & IN_BATTLE))
		cleanup_hyperspace ();

BatchGraphics ();
}

