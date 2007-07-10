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
#include "grpinfo.h"
#include "encount.h"
#include "libs/mathlib.h"


void
NotifyOthers (COUNT which_race, BYTE target_loc)
{
	HSHIPFRAG hGroup, hNextGroup;

	for (hGroup = GetHeadLink (&GLOBAL (ip_group_q));
			hGroup; hGroup = hNextGroup)
	{
		IP_GROUP *GroupPtr;

		GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
		hNextGroup = _GetSuccLink (GroupPtr);

		if (GroupPtr->race_id == which_race)
		{
			BYTE task;

			task = GroupPtr->task | IGNORE_FLAGSHIP;

			if (target_loc == 0)
			{
				task &= ~IGNORE_FLAGSHIP;
				// XXX: orbit_pos is abused here to store the previous
				//   group destination, before the intercept task.
				//   Returned to dest_loc below.
				GroupPtr->orbit_pos = GroupPtr->dest_loc;
/* task = FLEE | IGNORE_FLAGSHIP; */
			}
			else if ((target_loc = GroupPtr->dest_loc) == 0)
			{
				// XXX: orbit_pos is abused to store the previous
				//   group destination, before the intercept task.
				target_loc = GroupPtr->orbit_pos;
				GroupPtr->orbit_pos = NORMALIZE_FACING (TFB_Random ());
#ifdef OLD
				target_loc = (BYTE)((
						(COUNT)TFB_Random ()
						% pSolarSysState->SunDesc[0].NumPlanets) + 1);
#endif /* OLD */
				if (!(task & REFORM_GROUP))
				{
					if ((task & ~IGNORE_FLAGSHIP) != EXPLORE)
						GroupPtr->group_counter = 0;
					else
						GroupPtr->group_counter =
								((COUNT) TFB_Random () % MAX_REVOLUTIONS)
								<< FACING_SHIFT;
				}
			}

			GroupPtr->task = task;
			GroupPtr->dest_loc = target_loc;
		}

		UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);
	}
}

static void
ip_group_preprocess (ELEMENT *ElementPtr)
{
#define TRACK_WAIT 5
	BYTE task;
	BYTE target_loc, group_loc, flagship_loc;
	SIZE radius;
	POINT dest_pt;
	SIZE vdx, vdy;
	ELEMENT *EPtr;
	IP_GROUP *GroupPtr;

	EPtr = ElementPtr;
	EPtr->state_flags &=
			~(DISAPPEARING | NONSOLID); /* "I'm not quite dead." */
	++EPtr->life_span; /* so that it will 'die'
										 * again next time.
										 */
	GetElementStarShip (EPtr, &GroupPtr);
	group_loc = GroupPtr->sys_loc;
			/* save old location */
	DisplayArray[EPtr->PrimIndex].Object.Point = GroupPtr->loc;

	if (group_loc != 0)
		radius = MAX_ZOOM_RADIUS;
	else
		radius = pSolarSysState->SunDesc[0].radius;

	dest_pt.x = (SIS_SCREEN_WIDTH >> 1)
			+ (SIZE)((long)GroupPtr->loc.x
			* (DISPLAY_FACTOR >> 1) / radius);
	dest_pt.y = (SIS_SCREEN_HEIGHT >> 1)
			+ (SIZE)((long)GroupPtr->loc.y
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

	task = GroupPtr->task;
	if (pSolarSysState->MenuState.CurState)
		goto ExitIPProcess;

	if ((task & REFORM_GROUP) && --GroupPtr->group_counter == 0)
	{
		task &= ~REFORM_GROUP;
		GroupPtr->task = task;
		if ((task & ~IGNORE_FLAGSHIP) != EXPLORE)
			GroupPtr->group_counter = 0;
		else
			GroupPtr->group_counter = ((COUNT)TFB_Random ()
					% MAX_REVOLUTIONS) << FACING_SHIFT;
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

	target_loc = GroupPtr->dest_loc;
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
				if (GroupPtr->race_id == URQUAN_PROBE_SHIP)
					detect_dist <<= 1;
			}
			vdx = GLOBAL (ip_location.x) - GroupPtr->loc.x;
			vdy = GLOBAL (ip_location.y) - GroupPtr->loc.y;
			if ((long)vdx * vdx
					+ (long)vdy * vdy < (long)detect_dist * detect_dist)
			{
				EPtr->thrust_wait = 0;
				ZeroVelocityComponents (&EPtr->velocity);

				NotifyOthers (GroupPtr->race_id, 0);
				task = GroupPtr->task;
				if ((target_loc = GroupPtr->dest_loc) == 0)
					target_loc = flagship_loc;
			}
		}
	}

	GetCurrentVelocityComponents (&EPtr->velocity, &vdx, &vdy);

	task &= ~IGNORE_FLAGSHIP;
	if (task <= ON_STATION)
#ifdef NEVER
	if (task <= FLEE || (task == ON_STATION
			&& GroupPtr->dest_loc == 0))
#endif /* NEVER */
	{
		BOOLEAN Transition;
		SIZE dx, dy;
		SIZE delta_x, delta_y;
		COUNT angle;

		Transition = FALSE;
		if (task == FLEE)
		{
			dest_pt.x = GroupPtr->loc.x << 1;
			dest_pt.y = GroupPtr->loc.y << 1;
		}
		else if (((task != ON_STATION || GroupPtr->dest_loc == 0)
				&& group_loc == target_loc)
				|| (task == ON_STATION && GroupPtr->dest_loc
				&& group_loc == 0))
		{
			if (GroupPtr->dest_loc == 0)
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

				angle = FACING_TO_ANGLE (GroupPtr->orbit_pos + 1);
				dest_pt.x = org.x + COSINE (angle, orbit_dist);
				dest_pt.y = org.y + SINE (angle, orbit_dist);
				if (GroupPtr->loc.x == dest_pt.x
						&& GroupPtr->loc.y == dest_pt.y)
				{
					BYTE next_loc;

					GroupPtr->orbit_pos = NORMALIZE_FACING (
							ANGLE_TO_FACING (angle));
					angle += FACING_TO_ANGLE (1);
					dest_pt.x = org.x + COSINE (angle, orbit_dist);
					dest_pt.y = org.y + SINE (angle, orbit_dist);

					EPtr->thrust_wait = (BYTE)~0;
					if (GroupPtr->group_counter)
						--GroupPtr->group_counter;
					else if (task == EXPLORE
							&& (next_loc = (BYTE)(((COUNT)TFB_Random ()
							% pSolarSysState->SunDesc[0].NumPlanets)
							+ 1)) != target_loc)
					{
						EPtr->thrust_wait = 0;
						target_loc = next_loc;
						GroupPtr->dest_loc = next_loc;
					}
				}
			}
		}
		else if (group_loc == 0)
		{
			if (GroupPtr->dest_loc == 0)
				dest_pt = pSolarSysState->SunDesc[0].location;
			else
				XFormIPLoc (&pSolarSysState->PlanetDesc[
						target_loc - 1].image.origin, &dest_pt, FALSE);
		}
		else
		{
			if (task == ON_STATION)
				target_loc = 0;

			dest_pt.x = GroupPtr->loc.x << 1;
			dest_pt.y = GroupPtr->loc.y << 1;
		}

		delta_x = dest_pt.x - GroupPtr->loc.x;
		delta_y = dest_pt.y - GroupPtr->loc.y;
		angle = ARCTAN (delta_x, delta_y);

		if (EPtr->thrust_wait && EPtr->thrust_wait != (BYTE)~0)
			--EPtr->thrust_wait;
		else if ((vdx == 0 && vdy == 0)
				|| angle != GetVelocityTravelAngle (&EPtr->velocity))
		{
			SIZE speed;

			if (EPtr->thrust_wait && GroupPtr->dest_loc != 0)
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

				speed = RaceIPSpeed[GroupPtr->race_id];
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
			else if (target_loc == GroupPtr->dest_loc)
			{
PartialRevolution:
				if ((long)((COUNT)(dx * dx) + (COUNT)(dy * dy))
						>= (long)delta_x * delta_x + (long)delta_y * delta_y)
				{
					GroupPtr->loc = dest_pt;
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

				if (task == ON_STATION && GroupPtr->dest_loc)
					goto PartialRevolution;
				else if ((long)((COUNT)(dx * dx) + (COUNT)(dy * dy))
						>= (long)delta_x * delta_x + (long)delta_y * delta_y)
					Transition = TRUE;
			}
			else
			{
CheckGetAway:
				dest_pt.x = (SIS_SCREEN_WIDTH >> 1)
						+ (SIZE)((long)GroupPtr->loc.x
						* (DISPLAY_FACTOR >> 1) / MAX_ZOOM_RADIUS);
				dest_pt.y = (SIS_SCREEN_HEIGHT >> 1)
						+ (SIZE)((long)GroupPtr->loc.y
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
					PLANET_DESC *pCurDesc;

					pCurDesc = &pSolarSysState->PlanetDesc[group_loc - 1];
					XFormIPLoc (&pCurDesc->image.origin, &GroupPtr->loc,
							FALSE);
					group_loc = 0;
					GroupPtr->sys_loc = 0;
				}
				else if (target_loc == 0)
				{
					/* Group completely left the star system */
					EPtr->life_span = 0;
					EPtr->state_flags |= DISAPPEARING | NONSOLID;
					GroupPtr->in_system = 0;
					return;
				}
				else
				{
					if (target_loc == GroupPtr->dest_loc)
					{
						GroupPtr->orbit_pos = NORMALIZE_FACING (
								ANGLE_TO_FACING (angle + HALF_CIRCLE));
						GroupPtr->group_counter =
								((COUNT)TFB_Random () % MAX_REVOLUTIONS)
								<< FACING_SHIFT;
					}

					GroupPtr->loc.x = -(SIZE)((long)COSINE (
							angle, SIS_SCREEN_WIDTH * 9 / 16
							) * MAX_ZOOM_RADIUS / (DISPLAY_FACTOR >> 1));
					GroupPtr->loc.y = -(SIZE)((long)SINE (
							angle, SIS_SCREEN_WIDTH * 9 / 16
							) * MAX_ZOOM_RADIUS / (DISPLAY_FACTOR >> 1));

					group_loc = target_loc;
					GroupPtr->sys_loc = target_loc;
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
	GroupPtr->loc.x += vdx;
	GroupPtr->loc.y += vdy;

	dest_pt.x = (SIS_SCREEN_WIDTH >> 1)
			+ (SIZE)((long)GroupPtr->loc.x
			* (DISPLAY_FACTOR >> 1) / radius);
	dest_pt.y = (SIS_SCREEN_HEIGHT >> 1)
			+ (SIZE)((long)GroupPtr->loc.y
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
			&& (GroupPtr->group_counter & 1)))
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
flag_ship_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
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
ip_group_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	IP_GROUP *GroupPtr;

	if ((GLOBAL (CurrentActivity) & START_ENCOUNTER)
			|| pSolarSysState->MenuState.CurState
			|| (ElementPtr0->state_flags & GOOD_GUY))
	{
		ElementPtr0->state_flags &= ~BAD_GUY;
		return;
	}

	GetElementStarShip (ElementPtr0, &GroupPtr);
	if (ElementPtr0->state_flags & ElementPtr1->state_flags & BAD_GUY)
	{
		if ((ElementPtr0->state_flags & COLLISION)
				|| (ElementPtr1->current.location.x == ElementPtr1->next.location.x
				&& ElementPtr1->current.location.y == ElementPtr1->next.location.y))
			ElementPtr0->state_flags &= ~COLLISION;
		else
		{
			ElementPtr1->state_flags |= COLLISION;

			GroupPtr->loc =
					DisplayArray[ElementPtr0->PrimIndex].Object.Point;
			ElementPtr0->next.location = ElementPtr0->current.location;
			InitIntersectEndPoint (ElementPtr0);
		}
	}
	else
	{
		EncounterGroup = GroupPtr->group_id;

		if (GroupPtr->race_id == URQUAN_PROBE_SHIP)
		{
			GroupPtr->task = FLEE | IGNORE_FLAGSHIP;
			GroupPtr->dest_loc = 0;
		}
		else
		{
			GroupPtr->task |= REFORM_GROUP;
			GroupPtr->group_counter = 100;
			NotifyOthers (GroupPtr->race_id, (BYTE)~0);
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
spawn_ip_group (IP_GROUP *GroupPtr)
{
	HELEMENT hIPSHIPElement;

	hIPSHIPElement = AllocElement ();
	if (hIPSHIPElement)
	{
		BYTE task;
		ELEMENT *IPSHIPElementPtr;

		LockElement (hIPSHIPElement, &IPSHIPElementPtr);
		// XXX: turn_wait hack is not actually used anywhere
		//IPSHIPElementPtr->turn_wait = GroupPtr->group_id;
		IPSHIPElementPtr->sys_loc = 1;
		IPSHIPElementPtr->hit_points = 1;
		IPSHIPElementPtr->state_flags =
				CHANGING | FINITE_LIFE | IGNORE_VELOCITY;

		task = GroupPtr->task;
		if (!(task & IGNORE_FLAGSHIP))
			IPSHIPElementPtr->state_flags |= BAD_GUY;
		else
		{
			IPSHIPElementPtr->state_flags |= GOOD_GUY;
			// XXX: Hack: Yehat revolution start, fleeing groups
			if (GroupPtr->race_id == YEHAT_SHIP
					&& GET_GAME_STATE (YEHAT_CIVIL_WAR))
			{
				GroupPtr->task = FLEE | (task & REFORM_GROUP);
				GroupPtr->dest_loc = 0;
			}
		}

		SetPrimType (&DisplayArray[IPSHIPElementPtr->PrimIndex], STAMP_PRIM);
		// XXX: Hack: farray points to FRAME[3] and given FRAME
		IPSHIPElementPtr->current.image.farray = &GroupPtr->melee_icon;
		IPSHIPElementPtr->current.image.frame = SetAbsFrameIndex (
					GroupPtr->melee_icon, 1);
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

			if (GroupPtr->sys_loc != 0)
				radius = MAX_ZOOM_RADIUS;
			else
				radius = pSolarSysState->SunDesc[0].radius;

			pt.x = (SIS_SCREEN_WIDTH >> 1)
					+ (SIZE)((long)GroupPtr->loc.x
					* DISPLAY_FACTOR / radius);
			pt.y = (SIS_SCREEN_HEIGHT >> 1)
					+ (SIZE)((long)GroupPtr->loc.y
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

		SetElementStarShip (IPSHIPElementPtr, GroupPtr);

		SetUpElement (IPSHIPElementPtr);
		IPSHIPElementPtr->IntersectControl.IntersectStamp.frame =
				DecFrameIndex (stars_in_space);

		UnlockElement (hIPSHIPElement);

		PutElement (hIPSHIPElement);
	}
}

#define FLIP_WAIT 42

static void
flag_ship_preprocess (ELEMENT *ElementPtr)
{
	if (--ElementPtr->thrust_wait == 0)
		/* juggle list after flagship */
	{
		HELEMENT hSuccElement;

		if ((hSuccElement = GetSuccElement (ElementPtr))
				&& hSuccElement != GetTailElement ())
		{
			HELEMENT hPredElement;
			ELEMENT *TailPtr;

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

		if (ElementPtr->sys_loc == flagship_loc)
		{
			if (ElementPtr->state_flags & NONSOLID)
				ElementPtr->state_flags &= ~NONSOLID;
		}
		else /* no collisions during transition */
		{
			ElementPtr->state_flags |= NONSOLID;
			ElementPtr->sys_loc = flagship_loc;
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
		ELEMENT *FlagShipElementPtr;

		LockElement (hFlagShipElement, &FlagShipElementPtr);
		FlagShipElementPtr->hit_points = 1;
		if (pSolarSysState->pBaseDesc == pSolarSysState->PlanetDesc)
			FlagShipElementPtr->sys_loc = 1;
		else
			FlagShipElementPtr->sys_loc =
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
	HSHIPFRAG hGroup, hNextGroup;

	spawn_flag_ship ();

	if (EncounterRace >= 0)
	{
		NotifyOthers (EncounterRace, 0);
		EncounterRace = -1;
	}

	for (hGroup = GetHeadLink (&GLOBAL (ip_group_q));
			hGroup; hGroup = hNextGroup)
	{
		IP_GROUP *GroupPtr;

		GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
		hNextGroup = _GetSuccLink (GroupPtr);

		if (GroupPtr->in_system)
			spawn_ip_group (GroupPtr);

		UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);
	}
}

