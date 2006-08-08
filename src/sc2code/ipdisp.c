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

#include "collide.h"
#include "globdata.h"
#include "init.h"
#include "races.h"
#include "encount.h"
#include "libs/mathlib.h"


void
NotifyOthers (COUNT which_race, BYTE target_loc)
{
	HSTARSHIP hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q));
			hStarShip; hStarShip = hNextShip)
	{
		SHIP_FRAGMENTPTR StarShipPtr;

		StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (npc_built_ship_q), hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);

		if (GET_RACE_ID (StarShipPtr) == which_race)
		{
			BYTE task;

			task = GET_GROUP_MISSION (StarShipPtr) | IGNORE_FLAGSHIP;

			if (target_loc == 0)
			{
				task &= ~IGNORE_FLAGSHIP;
				SET_ORBIT_LOC (StarShipPtr, GET_GROUP_DEST (StarShipPtr));
/* task = FLEE | IGNORE_FLAGSHIP; */
			}
			else if ((target_loc = GET_GROUP_DEST (StarShipPtr)) == 0)
			{
				target_loc = GET_ORBIT_LOC (StarShipPtr);
				SET_ORBIT_LOC (StarShipPtr, NORMALIZE_FACING (TFB_Random ()));
#ifdef OLD
				target_loc = (BYTE)((
						(COUNT)TFB_Random ()
						% pSolarSysState->SunDesc[0].NumPlanets) + 1);
#endif /* OLD */
				if (!(task & REFORM_GROUP))
				{
					if ((task & ~IGNORE_FLAGSHIP) != EXPLORE)
						StarShipPtr->ShipInfo.group_counter = 0;
					else
						StarShipPtr->ShipInfo.group_counter =
								((COUNT) TFB_Random () % MAX_REVOLUTIONS)
								<< FACING_SHIFT;
				}
			}

			SET_GROUP_MISSION (StarShipPtr, task);
			SET_GROUP_DEST (StarShipPtr, target_loc);
		}

		UnlockStarShip (&GLOBAL (npc_built_ship_q), hStarShip);
	}
}

static void
ip_group_preprocess (PELEMENT ElementPtr)
{
#define TRACK_WAIT 5
	BYTE task;
	BYTE target_loc, group_loc, flagship_loc;
	SIZE radius;
	POINT dest_pt;
	SIZE vdx, vdy;
	ELEMENTPTR EPtr;
	SHIP_FRAGMENTPTR StarShipPtr;

	EPtr = (ELEMENTPTR)ElementPtr;
	EPtr->state_flags &=
			~(DISAPPEARING | NONSOLID); /* "I'm not quite dead." */
	++EPtr->life_span; /* so that it will 'die'
										 * again next time.
										 */
	GetElementStarShip (EPtr, &StarShipPtr);
	group_loc = GET_GROUP_LOC (StarShipPtr);
			/* save old location */
	DisplayArray[EPtr->PrimIndex].Object.Point = StarShipPtr->ShipInfo.loc;

	if (group_loc != 0)
		radius = MAX_ZOOM_RADIUS;
	else
		radius = pSolarSysState->SunDesc[0].radius;

	dest_pt.x = (SIS_SCREEN_WIDTH >> 1)
			+ (SIZE)((long)StarShipPtr->ShipInfo.loc.x
			* (DISPLAY_FACTOR >> 1) / radius);
	dest_pt.y = (SIS_SCREEN_HEIGHT >> 1)
			+ (SIZE)((long)StarShipPtr->ShipInfo.loc.y
			* (DISPLAY_FACTOR >> 1) / radius);
	EPtr->current.location.x = DISPLAY_TO_WORLD (dest_pt.x)
			+ (COORD)(LOG_SPACE_WIDTH >> 1)
			- (LOG_SPACE_WIDTH >> (MAX_REDUCTION + 1));
	EPtr->current.location.y = DISPLAY_TO_WORLD (dest_pt.y)
			+ (COORD)(LOG_SPACE_HEIGHT >> 1)
			- (LOG_SPACE_HEIGHT >> (MAX_REDUCTION + 1));

	InitIntersectStartPoint (EPtr);

	if (pSolarSysState->pBaseDesc == pSolarSysState->PlanetDesc)
		flagship_loc = 0;
	else
		flagship_loc = (BYTE)(pSolarSysState->pBaseDesc->pPrevDesc
				- pSolarSysState->PlanetDesc + 1);

	task = GET_GROUP_MISSION (StarShipPtr);
	if (pSolarSysState->MenuState.CurState)
		goto ExitIPProcess;

	if ((task & REFORM_GROUP)
			&& --StarShipPtr->ShipInfo.group_counter == 0)
	{
		task &= ~REFORM_GROUP;
		SET_GROUP_MISSION (StarShipPtr, task);
		if ((task & ~IGNORE_FLAGSHIP) != EXPLORE)
			StarShipPtr->ShipInfo.group_counter = 0;
		else
			StarShipPtr->ShipInfo.group_counter =
					((COUNT)TFB_Random ()
					% MAX_REVOLUTIONS)
					<< FACING_SHIFT;
	}

	if (!(task & REFORM_GROUP))
	{
		if ((task & ~(IGNORE_FLAGSHIP | REFORM_GROUP)) != FLEE)
		{
			if (EPtr->state_flags & BAD_GUY)
				EPtr->state_flags &= ~GOOD_GUY;
			else
				EPtr->state_flags |= BAD_GUY;
		}
		else if (!(task & IGNORE_FLAGSHIP)
				&& !(EPtr->state_flags & (GOOD_GUY | BAD_GUY)))
		{	// fleeing yehat ship collisions after menu fix
			EPtr->state_flags |= BAD_GUY;
		}
	}

	target_loc = GET_GROUP_DEST (StarShipPtr);
	if (!(task & (IGNORE_FLAGSHIP | REFORM_GROUP)))
	{
		if (target_loc == 0 && task != FLEE)
		{
			/* if intercepting flagship */
			target_loc = flagship_loc;
			if (EPtr->thrust_wait > TRACK_WAIT)
			{
				EPtr->thrust_wait = 0;
				ZeroVelocityComponents (&EPtr->velocity);
			}
		}
		else if (group_loc == flagship_loc)
		{
			long detect_dist;

			detect_dist = 1200;
			if (group_loc != 0) /* if in planetary views */
			{
				detect_dist *= (MAX_ZOOM_RADIUS / MIN_ZOOM_RADIUS);
				if (GET_RACE_ID (StarShipPtr) == URQUAN_PROBE_SHIP)
					detect_dist <<= 1;
			}
			vdx = GLOBAL (ip_location.x)
					- StarShipPtr->ShipInfo.loc.x;
			vdy = GLOBAL (ip_location.y)
					- StarShipPtr->ShipInfo.loc.y;
			if ((long)vdx * vdx
					+ (long)vdy * vdy < (long)detect_dist * detect_dist)
			{
				EPtr->thrust_wait = 0;
				ZeroVelocityComponents (&EPtr->velocity);

				NotifyOthers (GET_RACE_ID (StarShipPtr), 0);
				task = GET_GROUP_MISSION (StarShipPtr);
				if ((target_loc = GET_GROUP_DEST (StarShipPtr)) == 0)
					target_loc = flagship_loc;
			}
		}
	}

	GetCurrentVelocityComponents (&EPtr->velocity, &vdx, &vdy);

	task &= ~IGNORE_FLAGSHIP;
	if (task <= ON_STATION)
#ifdef NEVER
	if (task <= FLEE || (task == ON_STATION
			&& GET_GROUP_DEST (StarShipPtr) == 0))
#endif /* NEVER */
	{
		BOOLEAN Transition;
		SIZE dx, dy;
		SIZE delta_x, delta_y;
		COUNT angle;

		Transition = FALSE;
		if (task == FLEE)
		{
			dest_pt.x = StarShipPtr->ShipInfo.loc.x << 1;
			dest_pt.y = StarShipPtr->ShipInfo.loc.y << 1;
		}
		else if (((task != ON_STATION || GET_GROUP_DEST (StarShipPtr) == 0)
				&& group_loc == target_loc)
				|| (task == ON_STATION
				&& GET_GROUP_DEST (StarShipPtr)
				&& group_loc == 0))
		{
			if (GET_GROUP_DEST (StarShipPtr) == 0)
				dest_pt = GLOBAL (ip_location);
			else
			{
				COUNT orbit_dist;
				POINT org;

				if (task != ON_STATION)
				{
					orbit_dist = ORBIT_RADIUS;
					org.x = org.y = 0;
				}
				else
				{
					orbit_dist = STATION_RADIUS;
					XFormIPLoc (
							&pSolarSysState->PlanetDesc[
								target_loc - 1
							].image.origin,
							&org,
							FALSE
							);
				}

				angle = FACING_TO_ANGLE (GET_ORBIT_LOC (StarShipPtr) + 1);
				dest_pt.x = org.x + COSINE (angle, orbit_dist);
				dest_pt.y = org.y + SINE (angle, orbit_dist);
				if (StarShipPtr->ShipInfo.loc.x == dest_pt.x
						&& StarShipPtr->ShipInfo.loc.y == dest_pt.y)
				{
					BYTE next_loc;

					SET_ORBIT_LOC (StarShipPtr,
							NORMALIZE_FACING (ANGLE_TO_FACING (angle)));
					angle += FACING_TO_ANGLE (1);
					dest_pt.x = org.x + COSINE (angle, orbit_dist);
					dest_pt.y = org.y + SINE (angle, orbit_dist);

					EPtr->thrust_wait = (BYTE)~0;
					if (StarShipPtr->ShipInfo.group_counter)
						--StarShipPtr->ShipInfo.group_counter;
					else if (task == EXPLORE
							&& (next_loc = (BYTE)(((COUNT)TFB_Random ()
							% pSolarSysState->SunDesc[0].NumPlanets)
							+ 1)) != target_loc)
					{
						EPtr->thrust_wait = 0;
						SET_GROUP_DEST (StarShipPtr, target_loc = next_loc);
					}
				}
			}
		}
		else if (group_loc == 0)
		{
			if (GET_GROUP_DEST (StarShipPtr) == 0)
				dest_pt = pSolarSysState->SunDesc[0].location;
			else
				XFormIPLoc (&pSolarSysState->PlanetDesc[
						target_loc - 1].image.origin, &dest_pt, FALSE);
		}
		else
		{
			if (task == ON_STATION)
				target_loc = 0;

			dest_pt.x = StarShipPtr->ShipInfo.loc.x << 1;
			dest_pt.y = StarShipPtr->ShipInfo.loc.y << 1;
		}

		delta_x = dest_pt.x - StarShipPtr->ShipInfo.loc.x;
		delta_y = dest_pt.y - StarShipPtr->ShipInfo.loc.y;
		angle = ARCTAN (delta_x, delta_y);

		if (EPtr->thrust_wait && EPtr->thrust_wait != (BYTE)~0)
			--EPtr->thrust_wait;
		else if ((vdx == 0 && vdy == 0)
				|| angle != GetVelocityTravelAngle (&EPtr->velocity))
		{
			SIZE speed;

			if (EPtr->thrust_wait && GET_GROUP_DEST (StarShipPtr) != 0)
			{
#define ORBIT_SPEED 60
				speed = ORBIT_SPEED;
				if (task == ON_STATION)
					speed >>= 1;
			}
			else
			{
				SIZE RaceIPSpeed[] =
				{
					RACE_IP_SPEED
				};

				speed = RaceIPSpeed[GET_RACE_ID (StarShipPtr)];
				EPtr->thrust_wait = TRACK_WAIT;
			}

			SetVelocityComponents (&EPtr->velocity,
					vdx = COSINE (angle, speed),
					vdy = SINE (angle, speed));
		}

		dx = vdx, dy = vdy;
		if (group_loc == target_loc)
		{
			if (target_loc == 0)
			{
				if (task == FLEE)
					goto CheckGetAway;
			}
			else if (target_loc == GET_GROUP_DEST (StarShipPtr))
			{
PartialRevolution:
				if ((long)((COUNT)(dx * dx) + (COUNT)(dy * dy))
						>= (long)delta_x * delta_x + (long)delta_y * delta_y)
				{
					StarShipPtr->ShipInfo.loc = dest_pt;
					vdx = vdy = 0;
					ZeroVelocityComponents (&EPtr->velocity);
				}
			}
		}
		else
		{
			if (group_loc == 0)
			{
				if (pSolarSysState->SunDesc[0].radius < MAX_ZOOM_RADIUS)
				{
					dx >>= 1;
					dy >>= 1;
					if (pSolarSysState->SunDesc[0].radius == MIN_ZOOM_RADIUS)
					{
						dx >>= 1;
						dy >>= 1;
					}
				}

				if (task == ON_STATION
						&& GET_GROUP_DEST (StarShipPtr))
					goto PartialRevolution;
				else if ((long)((COUNT)(dx * dx) + (COUNT)(dy * dy))
						>= (long)delta_x * delta_x + (long)delta_y * delta_y)
					Transition = TRUE;
			}
			else
			{
CheckGetAway:
				dest_pt.x = (SIS_SCREEN_WIDTH >> 1)
						+ (SIZE)((long)StarShipPtr->ShipInfo.loc.x
						* (DISPLAY_FACTOR >> 1) / MAX_ZOOM_RADIUS);
				dest_pt.y = (SIS_SCREEN_HEIGHT >> 1)
						+ (SIZE)((long)StarShipPtr->ShipInfo.loc.y
						* (DISPLAY_FACTOR >> 1) / MAX_ZOOM_RADIUS);
				if (dest_pt.x < 0
						|| dest_pt.x >= SIS_SCREEN_WIDTH
						|| dest_pt.y < 0
						|| dest_pt.y >= SIS_SCREEN_HEIGHT)
					Transition = TRUE;
			}

			if (Transition)
			{
						/* no collisions during transition */
				EPtr->state_flags |= NONSOLID;

				vdx = vdy = 0;
				ZeroVelocityComponents (&EPtr->velocity);
				if (group_loc != 0)
				{
					PPLANET_DESC pCurDesc;

					pCurDesc = &pSolarSysState->PlanetDesc[group_loc - 1];
					XFormIPLoc (
							&pCurDesc->image.origin,
							&StarShipPtr->ShipInfo.loc,
							FALSE
							);
					SET_GROUP_LOC (StarShipPtr, group_loc = 0);
				}
				else if (target_loc == 0)
				{
					EPtr->life_span = 0;
					EPtr->state_flags |= DISAPPEARING | NONSOLID;
					StarShipPtr->ShipInfo.crew_level = 0;
					return;
				}
				else
				{
					if (target_loc == GET_GROUP_DEST (StarShipPtr))
					{
						SET_ORBIT_LOC (StarShipPtr,
								NORMALIZE_FACING (
								ANGLE_TO_FACING (angle + HALF_CIRCLE)
								));
						StarShipPtr->ShipInfo.group_counter =
								((COUNT)TFB_Random () % MAX_REVOLUTIONS)
								<< FACING_SHIFT;
					}

					StarShipPtr->ShipInfo.loc.x = -(SIZE)((long)COSINE (
							angle, SIS_SCREEN_WIDTH * 9 / 16
							) * MAX_ZOOM_RADIUS / (DISPLAY_FACTOR >> 1));
					StarShipPtr->ShipInfo.loc.y = -(SIZE)((long)SINE (
							angle, SIS_SCREEN_WIDTH * 9 / 16
							) * MAX_ZOOM_RADIUS / (DISPLAY_FACTOR >> 1));

					SET_GROUP_LOC (StarShipPtr, group_loc = target_loc);
				}
			}
		}
	}

	if (group_loc != 0)
		radius = MAX_ZOOM_RADIUS;
	else
	{
		radius = pSolarSysState->SunDesc[0].radius;
		if (radius < MAX_ZOOM_RADIUS)
		{
			vdx >>= 1;
			vdy >>= 1;
			if (radius == MIN_ZOOM_RADIUS)
			{
				vdx >>= 1;
				vdy >>= 1;
			}
		}
	}
	StarShipPtr->ShipInfo.loc.x += vdx;
	StarShipPtr->ShipInfo.loc.y += vdy;

	dest_pt.x = (SIS_SCREEN_WIDTH >> 1)
			+ (SIZE)((long)StarShipPtr->ShipInfo.loc.x
			* (DISPLAY_FACTOR >> 1) / radius);
	dest_pt.y = (SIS_SCREEN_HEIGHT >> 1)
			+ (SIZE)((long)StarShipPtr->ShipInfo.loc.y
			* (DISPLAY_FACTOR >> 1) / radius);

ExitIPProcess:
	EPtr->next.location.x = DISPLAY_TO_WORLD (dest_pt.x)
			+ (COORD)(LOG_SPACE_WIDTH >> 1)
			- (LOG_SPACE_WIDTH >> (MAX_REDUCTION + 1));
	EPtr->next.location.y = DISPLAY_TO_WORLD (dest_pt.y)
			+ (COORD)(LOG_SPACE_HEIGHT >> 1)
			- (LOG_SPACE_HEIGHT >> (MAX_REDUCTION + 1));

	if (group_loc != flagship_loc
			|| ((task & REFORM_GROUP)
			&& (StarShipPtr->ShipInfo.group_counter & 1)))
	{
		SetPrimType (&DisplayArray[EPtr->PrimIndex], NO_PRIM);
		EPtr->state_flags |= NONSOLID;
	}
	else
	{
		SetPrimType (&DisplayArray[EPtr->PrimIndex], STAMP_PRIM);
		if (task & REFORM_GROUP)
			 EPtr->state_flags |= NONSOLID;
	}

	EPtr->state_flags |= CHANGING;
}

static void
flag_ship_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT
		ElementPtr1, PPOINT pPt1)
{
	if ((GLOBAL (CurrentActivity) & START_ENCOUNTER)
			|| pSolarSysState->MenuState.CurState
			|| (ElementPtr1->state_flags & GOOD_GUY))
		return;

	if (!(ElementPtr1->state_flags & COLLISION)) /* not processed yet */
		ElementPtr0->state_flags |= COLLISION | NONSOLID;
	else
	{
		ElementPtr1->state_flags &= ~COLLISION;
		GLOBAL (CurrentActivity) |= START_ENCOUNTER;
	}
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static void
ip_group_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT
		ElementPtr1, PPOINT pPt1)
{
	SHIP_FRAGMENTPTR StarShipPtr;

	if ((GLOBAL (CurrentActivity) & START_ENCOUNTER)
			|| pSolarSysState->MenuState.CurState
			|| (ElementPtr0->state_flags & GOOD_GUY))
	{
		ElementPtr0->state_flags &= ~BAD_GUY;
		return;
	}

	GetElementStarShip (ElementPtr0, &StarShipPtr);
	if (ElementPtr0->state_flags & ElementPtr1->state_flags & BAD_GUY)
	{
		if ((ElementPtr0->state_flags & COLLISION)
				|| (ElementPtr1->current.location.x == ElementPtr1->next.location.x
				&& ElementPtr1->current.location.y == ElementPtr1->next.location.y))
			ElementPtr0->state_flags &= ~COLLISION;
		else
		{
			ElementPtr1->state_flags |= COLLISION;

			StarShipPtr->ShipInfo.loc =
					DisplayArray[ElementPtr0->PrimIndex].Object.Point;
			ElementPtr0->next.location = ElementPtr0->current.location;
			InitIntersectEndPoint (ElementPtr0);
		}
	}
	else
	{
		EncounterGroup = GET_GROUP_ID (StarShipPtr);

		if (GET_RACE_ID (StarShipPtr) == URQUAN_PROBE_SHIP)
		{
			SET_GROUP_MISSION (StarShipPtr, FLEE | IGNORE_FLAGSHIP);
			SET_GROUP_DEST (StarShipPtr, 0);
		}
		else
		{
			SET_GROUP_MISSION (StarShipPtr,
					GET_GROUP_MISSION (StarShipPtr) | REFORM_GROUP);
			StarShipPtr->ShipInfo.group_counter = 100;
			NotifyOthers (GET_RACE_ID (StarShipPtr), (BYTE)~0);
		}

		if (!(ElementPtr1->state_flags & COLLISION)) /* not processed yet */
			ElementPtr0->state_flags |= COLLISION | NONSOLID;
		else
		{
			ElementPtr1->state_flags &= ~COLLISION;
			GLOBAL (CurrentActivity) |= START_ENCOUNTER;
		}
	}
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static void
spawn_ip_group (SHIP_FRAGMENTPTR StarShipPtr)
{
	HELEMENT hIPSHIPElement;

	hIPSHIPElement = AllocElement ();
	if (hIPSHIPElement)
	{
		BYTE task;
		ELEMENTPTR IPSHIPElementPtr;

		LockElement (hIPSHIPElement, &IPSHIPElementPtr);
		IPSHIPElementPtr->turn_wait = GET_GROUP_ID (StarShipPtr);
		IPSHIPElementPtr->mass_points = 1;
		IPSHIPElementPtr->hit_points = 1;
		IPSHIPElementPtr->state_flags =
				CHANGING | FINITE_LIFE | IGNORE_VELOCITY;

		task = GET_GROUP_MISSION (StarShipPtr);
		if (!(task & IGNORE_FLAGSHIP))
			IPSHIPElementPtr->state_flags |= BAD_GUY;
		else
		{
			IPSHIPElementPtr->state_flags |= GOOD_GUY;
			if (GET_RACE_ID (StarShipPtr) == YEHAT_SHIP
					&& GET_GAME_STATE (YEHAT_CIVIL_WAR))
			{
				SET_GROUP_MISSION (StarShipPtr, FLEE | (task & REFORM_GROUP));
				SET_GROUP_DEST (StarShipPtr, 0);
			}
		}

		SetPrimType (&DisplayArray[IPSHIPElementPtr->PrimIndex], STAMP_PRIM);
		IPSHIPElementPtr->current.image.farray =
				&StarShipPtr->ShipInfo.melee_icon;
		IPSHIPElementPtr->current.image.frame =
				SetAbsFrameIndex (StarShipPtr->ShipInfo.melee_icon, 1);
			/* preprocessing has a side effect
			 * we wish to avoid.  So death_func
			 * is used instead, but will achieve
			 * same result without the side
			 * effect (InitIntersectFrame)
			 */
		IPSHIPElementPtr->death_func = ip_group_preprocess;
		IPSHIPElementPtr->collision_func = ip_group_collision;

		{
			SIZE radius;
			POINT pt;

			if (GET_GROUP_LOC (StarShipPtr) != 0)
				radius = MAX_ZOOM_RADIUS;
			else
				radius = pSolarSysState->SunDesc[0].radius;

			pt.x = (SIS_SCREEN_WIDTH >> 1)
					+ (SIZE)((long)StarShipPtr->ShipInfo.loc.x
					* DISPLAY_FACTOR / radius);
			pt.y = (SIS_SCREEN_HEIGHT >> 1)
					+ (SIZE)((long)StarShipPtr->ShipInfo.loc.y
					* (DISPLAY_FACTOR >> 1) / radius);

			IPSHIPElementPtr->current.location.x =
					DISPLAY_TO_WORLD (pt.x)
					+ (COORD)(LOG_SPACE_WIDTH >> 1)
					- (LOG_SPACE_WIDTH >> (MAX_REDUCTION + 1));
			IPSHIPElementPtr->current.location.y =
					DISPLAY_TO_WORLD (pt.y)
					+ (COORD)(LOG_SPACE_HEIGHT >> 1)
					- (LOG_SPACE_HEIGHT >> (MAX_REDUCTION + 1));
		}

		SetElementStarShip (IPSHIPElementPtr, StarShipPtr);

		SetUpElement (IPSHIPElementPtr);
		IPSHIPElementPtr->IntersectControl.IntersectStamp.frame =
				DecFrameIndex (stars_in_space);

		UnlockElement (hIPSHIPElement);

		PutElement (hIPSHIPElement);
	}
}

#define FLIP_WAIT 42

static void
flag_ship_preprocess (PELEMENT ElementPtr)
{
	if (--ElementPtr->thrust_wait == 0)
		/* juggle list after flagship */
	{
		HELEMENT hSuccElement;

		if ((hSuccElement = GetSuccElement (ElementPtr))
				&& hSuccElement != GetTailElement ())
		{
			HELEMENT hPredElement;
			ELEMENTPTR TailPtr;

			LockElement (GetTailElement (), &TailPtr);
			hPredElement = _GetPredLink (TailPtr);
			UnlockElement (GetTailElement ());

			RemoveElement (hSuccElement);
			PutElement (hSuccElement);
		}

		ElementPtr->thrust_wait = FLIP_WAIT;
	}

	if (pSolarSysState->MenuState.CurState == 0)
	{
		BYTE flagship_loc, ec;
		SIZE vdx, vdy, radius;
		POINT pt;

		GetCurrentVelocityComponents (&GLOBAL (velocity), &vdx, &vdy);

		if (pSolarSysState->pBaseDesc == pSolarSysState->MoonDesc)
		{
			flagship_loc = (BYTE)(pSolarSysState->pBaseDesc->pPrevDesc
						- pSolarSysState->PlanetDesc + 2);
			radius = MAX_ZOOM_RADIUS;
		}
		else
		{
			flagship_loc = 1;
			radius = pSolarSysState->SunDesc[0].radius;
			if (radius < MAX_ZOOM_RADIUS)
			{
				vdx >>= 1;
				vdy >>= 1;
				if (radius == MIN_ZOOM_RADIUS)
				{
					vdx >>= 1;
					vdy >>= 1;
				}
			}
		}

		pt.x = (SIS_SCREEN_WIDTH >> 1)
				+ (SIZE)((long)GLOBAL (ip_location.x)
				* (DISPLAY_FACTOR >> 1) / radius);
		pt.y = (SIS_SCREEN_HEIGHT >> 1)
				+ (SIZE)((long)GLOBAL (ip_location.y)
				* (DISPLAY_FACTOR >> 1) / radius);
		ElementPtr->current.location.x = DISPLAY_TO_WORLD (pt.x)
				+ (COORD)(LOG_SPACE_WIDTH >> 1)
				- (LOG_SPACE_WIDTH >> (MAX_REDUCTION + 1));
		ElementPtr->current.location.y = DISPLAY_TO_WORLD (pt.y)
				+ (COORD)(LOG_SPACE_HEIGHT >> 1)
				- (LOG_SPACE_HEIGHT >> (MAX_REDUCTION + 1));
		InitIntersectStartPoint (ElementPtr);

		GLOBAL (ip_location.x) += vdx;
		GLOBAL (ip_location.y) += vdy;

		pt.x = (SIS_SCREEN_WIDTH >> 1)
				+ (SIZE)((long)GLOBAL (ip_location.x)
				* (DISPLAY_FACTOR >> 1) / radius);
		pt.y = (SIS_SCREEN_HEIGHT >> 1)
				+ (SIZE)((long)GLOBAL (ip_location.y)
				* (DISPLAY_FACTOR >> 1) / radius);
		ElementPtr->next.location.x = DISPLAY_TO_WORLD (pt.x)
				+ (COORD)(LOG_SPACE_WIDTH >> 1)
				- (LOG_SPACE_WIDTH >> (MAX_REDUCTION + 1));
		ElementPtr->next.location.y = DISPLAY_TO_WORLD (pt.y)
				+ (COORD)(LOG_SPACE_HEIGHT >> 1)
				- (LOG_SPACE_HEIGHT >> (MAX_REDUCTION + 1));

		GLOBAL (ShipStamp.origin) = pt;
		ElementPtr->next.image.frame = GLOBAL (ShipStamp.frame);

		if (ElementPtr->mass_points == flagship_loc)
		{
			if (ElementPtr->state_flags & NONSOLID)
				ElementPtr->state_flags &= ~NONSOLID;
		}
		else /* no collisions during transition */
		{
			ElementPtr->state_flags |= NONSOLID;
			ElementPtr->mass_points = flagship_loc;
		}

		if ((ec = GET_GAME_STATE (ESCAPE_COUNTER))
				&& !(GLOBAL (CurrentActivity) & START_ENCOUNTER))
		{
			ElementPtr->state_flags |= NONSOLID;

			--ec;
			SET_GAME_STATE (ESCAPE_COUNTER, ec);
		}

		ElementPtr->state_flags |= CHANGING;
	}
}

static void
spawn_flag_ship (void)
{
	HELEMENT hFlagShipElement;

	hFlagShipElement = AllocElement ();
	if (hFlagShipElement)
	{
		ELEMENTPTR FlagShipElementPtr;

		LockElement (hFlagShipElement, &FlagShipElementPtr);
		FlagShipElementPtr->hit_points = 1;
		if (pSolarSysState->pBaseDesc == pSolarSysState->PlanetDesc)
			FlagShipElementPtr->mass_points = 1;
		else
			FlagShipElementPtr->mass_points =
					(BYTE)(pSolarSysState->pBaseDesc->pPrevDesc
					- pSolarSysState->PlanetDesc + 2);
		FlagShipElementPtr->state_flags =
				APPEARING | GOOD_GUY | IGNORE_VELOCITY;
		if (GET_GAME_STATE (ESCAPE_COUNTER))
			FlagShipElementPtr->state_flags |= NONSOLID;
		FlagShipElementPtr->life_span = NORMAL_LIFE;
		FlagShipElementPtr->thrust_wait = FLIP_WAIT;
		SetPrimType (&DisplayArray[FlagShipElementPtr->PrimIndex], STAMP_PRIM);
		FlagShipElementPtr->current.image.farray =
				&GLOBAL (ShipStamp.frame);
		FlagShipElementPtr->current.image.frame =
				GLOBAL (ShipStamp.frame);
		FlagShipElementPtr->preprocess_func = flag_ship_preprocess;
		FlagShipElementPtr->collision_func = flag_ship_collision;

		FlagShipElementPtr->current.location.x =
				DISPLAY_TO_WORLD (GLOBAL (ShipStamp.origin.x))
				+ (COORD)(LOG_SPACE_WIDTH >> 1)
				- (LOG_SPACE_WIDTH >> (MAX_REDUCTION + 1));
		FlagShipElementPtr->current.location.y =
				DISPLAY_TO_WORLD (GLOBAL (ShipStamp.origin.y))
				+ (COORD)(LOG_SPACE_HEIGHT >> 1)
				- (LOG_SPACE_HEIGHT >> (MAX_REDUCTION + 1));

		UnlockElement (hFlagShipElement);

		PutElement (hFlagShipElement);
	}
}

void
DoMissions (void)
{
	HSTARSHIP hStarShip, hNextShip;

	spawn_flag_ship ();

	if (EncounterRace >= 0)
	{
		NotifyOthers (EncounterRace, 0);
		EncounterRace = -1;
	}

	for (hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q));
			hStarShip; hStarShip = hNextShip)
	{
		SHIP_FRAGMENTPTR StarShipPtr;

		StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (npc_built_ship_q), hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);

		if (StarShipPtr->ShipInfo.crew_level)
			spawn_ip_group (StarShipPtr);

		UnlockStarShip (&GLOBAL (npc_built_ship_q), hStarShip);
	}
}

