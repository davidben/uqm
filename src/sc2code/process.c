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
#include "libs/graphics/gfx_common.h"

#define GSCALE_SHIFT 2 // controls the number of discrete scaling steps

COUNT DisplayFreeList;
PRIMITIVE DisplayArray[MAX_DISPLAY_PRIMS];
extern POINT SpaceOrg;

#ifndef OLD_ZOOM
SIZE zoom_out = 1 << ZOOM_SHIFT;
#endif

#if 0
#define CALC_ZOOM_STUFF(idx,sc) \
	do \
	{ \
		int i, z; \
		\
		z = 1 << ZOOM_SHIFT; \
		for (i = 0; (z <<= 1) <= zoom_out; i++) \
			; \
		*(idx) = i; \
		*(sc) = ((1 << i) << (ZOOM_SHIFT + 8)) / zoom_out; \
	} while (0)
#else
#define CALC_ZOOM_STUFF(idx,sc) \
	do \
	{ \
		int i; \
		\
		if (zoom_out < 2 << ZOOM_SHIFT) \
			i = 0; \
		else if (zoom_out < 4 << ZOOM_SHIFT) \
			i = 1; \
		else \
			i = 2; \
		*(idx) = i; \
		*(sc) = (1 << (i + ZOOM_SHIFT + 8)) / zoom_out; \
	} while (0)
#endif

HELEMENT
AllocElement (void)
{
	HELEMENT hElement;

	hElement = AllocLink (&disp_q);
	if (hElement)
	{
		ELEMENTPTR ElementPtr;

		LockElement (hElement, &ElementPtr);
		memset (ElementPtr, 0, sizeof (*ElementPtr));
		ElementPtr->PrimIndex = AllocDisplayPrim ();
		SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
		UnlockElement (hElement);
	}

	return (hElement);
}

void
FreeElement (HELEMENT hElement)
{
	if (hElement)
	{
		ELEMENTPTR ElementPtr;

		LockElement (hElement, &ElementPtr);
		FreeDisplayPrim (ElementPtr->PrimIndex);
		UnlockElement (hElement);

		FreeLink (&disp_q, hElement);
	}
}

void
SetUpElement (register ELEMENTPTR ElementPtr)
{
	ElementPtr->next = ElementPtr->current;
	if (CollidingElement (ElementPtr))
	{
		InitIntersectStartPoint (ElementPtr);
		InitIntersectEndPoint (ElementPtr);
		InitIntersectFrame (ElementPtr);
	}
}

static void
PreProcess (register ELEMENTPTR ElementPtr)
{
	register ELEMENT_FLAGS state_flags;

	if (ElementPtr->life_span == 0)
	{
		if (ElementPtr->pParent) /* untarget this dead element */
			Untarget (ElementPtr);

		ElementPtr->state_flags |= DISAPPEARING;
		if (ElementPtr->death_func)
			(*ElementPtr->death_func) (ElementPtr);
	}

	state_flags = ElementPtr->state_flags;
	if (!(state_flags & DISAPPEARING))
	{
		if (state_flags & APPEARING)
		{
			SetUpElement (ElementPtr);

			if (state_flags & PLAYER_SHIP)
				state_flags &= ~APPEARING; /* want to preprocess ship */
		}

		if (ElementPtr->preprocess_func && !(state_flags & APPEARING))
		{
			(*ElementPtr->preprocess_func) (ElementPtr);

			if (((state_flags = ElementPtr->state_flags) & CHANGING)
					&& CollidingElement (ElementPtr))
				InitIntersectFrame (ElementPtr);
		}

		if (!(state_flags & IGNORE_VELOCITY))
		{
			SIZE delta_x, delta_y;

			GetNextVelocityComponents (&ElementPtr->velocity,
					&delta_x, &delta_y, 1);
			if (delta_x != 0 || delta_y != 0)
			{
				state_flags |= CHANGING;
				ElementPtr->next.location.x += delta_x;
				ElementPtr->next.location.y += delta_y;
			}
		}

		if (CollidingElement (ElementPtr))
			InitIntersectEndPoint (ElementPtr);

		if (state_flags & FINITE_LIFE)
			--ElementPtr->life_span;
	}

	ElementPtr->state_flags = (state_flags
			& ~(POST_PROCESS | COLLISION))
			| PRE_PROCESS;
}

static void
PostProcess (register ELEMENTPTR ElementPtr)
{
	if (ElementPtr->postprocess_func)
		(*ElementPtr->postprocess_func) (ElementPtr);
	ElementPtr->current = ElementPtr->next;

	if (CollidingElement (ElementPtr))
	{
		InitIntersectStartPoint (ElementPtr);
		InitIntersectEndPoint (ElementPtr);
	}

	ElementPtr->state_flags = (ElementPtr->state_flags
			& ~(PRE_PROCESS | CHANGING | APPEARING))
			| POST_PROCESS;
}

#ifdef OLD_ZOOM
static BYTE
CalcReduction (SIZE dx, SIZE dy)
#else
static COUNT
CalcReduction (SIZE dx, SIZE dy)
#endif
{
#ifdef OLD_ZOOM
	BYTE next_reduction;
	SIZE sdx, sdy;

	if (LOBYTE (GLOBAL (CurrentActivity)) > IN_ENCOUNTER)
		return (0);

	sdx = dx;
	sdy = dy;
	for (next_reduction = MAX_VIS_REDUCTION;
			(dx <<= REDUCTION_SHIFT) <= TRANSITION_WIDTH
			&& (dy <<= REDUCTION_SHIFT) <= TRANSITION_HEIGHT
			&& next_reduction > 0;
			next_reduction -= REDUCTION_SHIFT)
		;

			/* check for "real" zoom in */
	if (next_reduction < GLOBAL (cur_state)
			&& GLOBAL (cur_state) <= MAX_VIS_REDUCTION)
	{
#define HYSTERESIS_X DISPLAY_TO_WORLD(24)
#define HYSTERESIS_Y DISPLAY_TO_WORLD(20)
		if (((sdx + HYSTERESIS_X)
				<< (MAX_VIS_REDUCTION - next_reduction)) > TRANSITION_WIDTH
				|| ((sdy + HYSTERESIS_Y)
				<< (MAX_VIS_REDUCTION - next_reduction)) > TRANSITION_HEIGHT)
		   /* if we don't zoom in, we want to stay at next+1 */
		   next_reduction += REDUCTION_SHIFT;
	}

	if (next_reduction == 0
			&& LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
		next_reduction += REDUCTION_SHIFT;
#else
	COUNT next_reduction;
#ifdef KDEBUG
	fprintf (stderr, "CalcReduction:\n");
#endif
	if (LOBYTE (GLOBAL (CurrentActivity)) > IN_ENCOUNTER)
		return (1 << ZOOM_SHIFT);
		
	dx = (dx * MAX_ZOOM_OUT) / (LOG_SPACE_WIDTH >> 2);
	if (dx < (1 << ZOOM_SHIFT))
		dx = 1 << ZOOM_SHIFT;
	else if (dx > MAX_ZOOM_OUT)
		dx = MAX_ZOOM_OUT;
		
	dy = (dy * MAX_ZOOM_OUT) / (LOG_SPACE_HEIGHT >> 2);
	if (dy < (1 << ZOOM_SHIFT))
		dy = 1 << ZOOM_SHIFT;
	else if (dy > MAX_ZOOM_OUT)
		dy = MAX_ZOOM_OUT;
		
	if (dy > dx)
		next_reduction = dy;
	else
		next_reduction = dx;

	if (next_reduction < (2 << ZOOM_SHIFT)
			&& LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
		next_reduction = (2 << ZOOM_SHIFT);
#ifdef KDEBUG
	fprintf (stderr, "CalcReduction: exit\n");
#endif
#endif

	return (next_reduction);
}

#ifdef OLD_ZOOM
static VIEW_STATE
CalcView (PPOINT pNewScrollPt, register BYTE next_reduction,
		PSIZE pdx, PSIZE pdy)
#else
static VIEW_STATE
CalcView (PPOINT pNewScrollPt, register SIZE next_reduction,
		PSIZE pdx, PSIZE pdy, COUNT ships_alive)
#endif
{
	SIZE dx, dy;
	VIEW_STATE view_state;

#ifdef KDEBUG
	fprintf (stderr, "CalcView:\n");
#endif
	dx = ((COORD)(LOG_SPACE_WIDTH >> 1) - pNewScrollPt->x);
	dy = ((COORD)(LOG_SPACE_HEIGHT >> 1) - pNewScrollPt->y);
	dx = WRAP_DELTA_X (dx);
	dy = WRAP_DELTA_Y (dy);
	if (ships_alive == 1)
	{
#define ORG_JUMP_X (LOG_SPACE_WIDTH / 75)
#define ORG_JUMP_Y (LOG_SPACE_HEIGHT / 75)
		if (dx > ORG_JUMP_X)
			dx = ORG_JUMP_X;
		else if (dx < -ORG_JUMP_X)
			dx = -ORG_JUMP_X;
		if (dy > ORG_JUMP_Y)
			dy = ORG_JUMP_Y;
		else if (dy < -ORG_JUMP_Y)
			dy = -ORG_JUMP_Y;
	}

	if ((dx || dy) && LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE)
		MoveSIS (&dx, &dy);

#ifdef OLD_ZOOM
	if (GLOBAL (cur_state) == next_reduction)
#else
	if (zoom_out == next_reduction)
#endif
		view_state = dx == 0 && dy == 0
				&& LOBYTE (GLOBAL (CurrentActivity)) != IN_HYPERSPACE
				? VIEW_STABLE : VIEW_SCROLL;
	else
	{
#ifdef OLD_ZOOM
		GLOBAL (cur_state) = next_reduction;

		SpaceOrg.x = (COORD)(LOG_SPACE_WIDTH >> 1)
				- (LOG_SPACE_WIDTH >> ((MAX_REDUCTION + 1) - next_reduction));
		SpaceOrg.y = (COORD)(LOG_SPACE_HEIGHT >> 1)
				- (LOG_SPACE_HEIGHT >> ((MAX_REDUCTION + 1) - next_reduction));
#else
#define ZOOM_JUMP ((1 << ZOOM_SHIFT) >> 3)
		if (ships_alive == 1
				&& zoom_out > next_reduction
				&& zoom_out <= MAX_ZOOM_OUT
				&& zoom_out - next_reduction > ZOOM_JUMP)
			next_reduction = zoom_out - ZOOM_JUMP;
			
		zoom_out = next_reduction;

		SpaceOrg.x = (int)(LOG_SPACE_WIDTH >> 1)
				- (LOG_SPACE_WIDTH * next_reduction / (MAX_ZOOM_OUT << 2));
		SpaceOrg.y = (int)(LOG_SPACE_HEIGHT >> 1)
				- (LOG_SPACE_HEIGHT * next_reduction / (MAX_ZOOM_OUT << 2));
#endif

		view_state = VIEW_CHANGE;
	}

	if (LOBYTE (GLOBAL (CurrentActivity)) <= IN_HYPERSPACE)
		MoveGalaxy (view_state, dx, dy);

	*pdx = dx;
	*pdy = dy;

#ifdef KDEBUG
	fprintf (stderr, "CalcView: exit\n");
#endif
	return (view_state);
}


static ELEMENT_FLAGS
ProcessCollisions (HELEMENT hSuccElement, register ELEMENTPTR ElementPtr,
		TIME_VALUE min_time, ELEMENT_FLAGS process_flags)
{
	HELEMENT hTestElement;

	while ((hTestElement = hSuccElement) != 0)
	{
		ELEMENTPTR TestElementPtr;

		LockElement (hTestElement, &TestElementPtr);
		if (!(TestElementPtr->state_flags & process_flags))
			PreProcess (TestElementPtr);
		hSuccElement = GetSuccElement (TestElementPtr);

		if (TestElementPtr == ElementPtr)
		{
			UnlockElement (hTestElement);

			continue;
		}

		if (CollisionPossible (TestElementPtr, ElementPtr))
		{
			register ELEMENT_FLAGS state_flags, test_state_flags;
			TIME_VALUE time_val;

			state_flags = ElementPtr->state_flags;
			test_state_flags = TestElementPtr->state_flags;
			if (((state_flags | test_state_flags) & FINITE_LIFE)
					&& (((state_flags & APPEARING)
					&& ElementPtr->life_span > 1)
					|| ((test_state_flags & APPEARING)
					&& TestElementPtr->life_span > 1)))
				time_val = 0;
			else
			{
				while ((time_val = DrawablesIntersect (&ElementPtr->IntersectControl,
						&TestElementPtr->IntersectControl, min_time)) == 1
						&& !((state_flags | test_state_flags) & FINITE_LIFE))
				{
#ifdef DEBUG
					fprintf (stderr, "BAD NEWS 0x%x <--> 0x%x\n", ElementPtr,
							TestElementPtr);
#endif /* DEBUG */
					if (state_flags & COLLISION)
					{
						InitIntersectEndPoint (TestElementPtr);
						TestElementPtr->IntersectControl.IntersectStamp.origin =
								TestElementPtr->IntersectControl.EndPoint;
						time_val = DrawablesIntersect (&ElementPtr->IntersectControl,
								&TestElementPtr->IntersectControl, 1);
						InitIntersectStartPoint (TestElementPtr);
					}

					if (time_val == 1)
					{
						FRAME CurFrame, NextFrame,
								TestCurFrame, TestNextFrame;

						CurFrame = ElementPtr->current.image.frame;
						NextFrame = ElementPtr->next.image.frame;
						TestCurFrame = TestElementPtr->current.image.frame;
						TestNextFrame = TestElementPtr->next.image.frame;
						if (NextFrame == CurFrame
								&& TestNextFrame == TestCurFrame)
						{
							if (test_state_flags & APPEARING)
							{
								do_damage (TestElementPtr, TestElementPtr->hit_points);
								if (TestElementPtr->pParent) /* untarget this dead element */
									Untarget (TestElementPtr);

								TestElementPtr->state_flags |= (COLLISION | DISAPPEARING);
								if (TestElementPtr->death_func)
									(*TestElementPtr->death_func) (TestElementPtr);
							}
							if (state_flags & APPEARING)
							{
								do_damage (ElementPtr, ElementPtr->hit_points);
								if (ElementPtr->pParent) /* untarget this dead element */
									Untarget (ElementPtr);

								ElementPtr->state_flags |= (COLLISION | DISAPPEARING);
								if (ElementPtr->death_func)
									(*ElementPtr->death_func) (ElementPtr);

								UnlockElement (hTestElement);
								return (COLLISION);
							}

							time_val = 0;
						}
						else
						{
							if (GetFrameIndex (CurFrame) !=
									GetFrameIndex (NextFrame))
								ElementPtr->next.image.frame =
										SetEquFrameIndex (NextFrame,
										CurFrame);
							else if (NextFrame != CurFrame)
							{
								ElementPtr->next.image =
										ElementPtr->current.image;
								if (ElementPtr->life_span > NORMAL_LIFE)
									ElementPtr->life_span = NORMAL_LIFE;
							}

							if (GetFrameIndex (TestCurFrame) !=
									GetFrameIndex (TestNextFrame))
								TestElementPtr->next.image.frame =
										SetEquFrameIndex (TestNextFrame,
										TestCurFrame);
							else if (TestNextFrame != TestCurFrame)
							{
								TestElementPtr->next.image =
										TestElementPtr->current.image;
								if (TestElementPtr->life_span > NORMAL_LIFE)
									TestElementPtr->life_span = NORMAL_LIFE;
							}

							InitIntersectStartPoint (ElementPtr);
							InitIntersectEndPoint (ElementPtr);
							InitIntersectFrame (ElementPtr);
							if (state_flags & PLAYER_SHIP)
							{
								STARSHIPPTR StarShipPtr;

								GetElementStarShip (ElementPtr, &StarShipPtr);
								StarShipPtr->ShipFacing =
										GetFrameIndex (
										ElementPtr->next.image.frame);
							}

							InitIntersectStartPoint (TestElementPtr);
							InitIntersectEndPoint (TestElementPtr);
							InitIntersectFrame (TestElementPtr);
							if (test_state_flags & PLAYER_SHIP)
							{
								STARSHIPPTR StarShipPtr;

								GetElementStarShip (TestElementPtr, &StarShipPtr);
								StarShipPtr->ShipFacing =
										GetFrameIndex (
										TestElementPtr->next.image.frame);
							}
						}
					}

					if (time_val == 0)
					{
						InitIntersectEndPoint (ElementPtr);
						InitIntersectEndPoint (TestElementPtr);

						break;
					}
				}
			}

			if (time_val > 0)
			{
				POINT SavePt, TestSavePt;

#ifdef DEBUG
				fprintf (stderr, "0x%x <--> 0x%x at %u\n", ElementPtr,
						TestElementPtr, time_val);
#endif /* DEBUG */
				SavePt = ElementPtr->IntersectControl.EndPoint;
				TestSavePt = TestElementPtr->IntersectControl.EndPoint;
				InitIntersectEndPoint (ElementPtr);
				InitIntersectEndPoint (TestElementPtr);
				if (time_val == 1
						|| (((state_flags & COLLISION)
						|| !ProcessCollisions (hSuccElement, ElementPtr,
						time_val - 1, process_flags))
						&& ((test_state_flags & COLLISION)
						|| !ProcessCollisions (
						!(TestElementPtr->state_flags & APPEARING) ?
						GetSuccElement (ElementPtr) :
						GetHeadElement (), TestElementPtr,
						time_val - 1, process_flags))))
				{
					state_flags = ElementPtr->state_flags;
					test_state_flags = TestElementPtr->state_flags;

#ifdef DEBUG
					fprintf (stderr, "PROCESSING 0x%x <--> 0x%x at %u\n",
							ElementPtr, TestElementPtr, time_val);
#endif /* DEBUG */
					if (test_state_flags & PLAYER_SHIP)
					{
						(*TestElementPtr->collision_func) (
								TestElementPtr, &TestSavePt,
								ElementPtr, &SavePt
								);
						(*ElementPtr->collision_func) (
								ElementPtr, &SavePt,
								TestElementPtr, &TestSavePt
								);
					}
					else
					{
						(*ElementPtr->collision_func) (
								ElementPtr, &SavePt,
								TestElementPtr, &TestSavePt
								);
						(*TestElementPtr->collision_func) (
								TestElementPtr, &TestSavePt,
								ElementPtr, &SavePt
								);
					}

					if (TestElementPtr->state_flags & COLLISION)
					{
						if (!(test_state_flags & COLLISION))
						{
							TestElementPtr->IntersectControl.IntersectStamp.origin =
									TestSavePt;
							TestElementPtr->next.location.x =
									DISPLAY_TO_WORLD (TestSavePt.x);
							TestElementPtr->next.location.y =
									DISPLAY_TO_WORLD (TestSavePt.y);
							InitIntersectEndPoint (TestElementPtr);
						}
					}

					if (ElementPtr->state_flags & COLLISION)
					{
						if (!(state_flags & COLLISION))
						{
							ElementPtr->IntersectControl.IntersectStamp.origin =
									SavePt;
							ElementPtr->next.location.x =
									DISPLAY_TO_WORLD (SavePt.x);
							ElementPtr->next.location.y =
									DISPLAY_TO_WORLD (SavePt.y);
							InitIntersectEndPoint (ElementPtr);

							if (!(state_flags & FINITE_LIFE) &&
									!(test_state_flags & FINITE_LIFE))
							{
								collide (ElementPtr, TestElementPtr);

								ProcessCollisions (GetHeadElement (), ElementPtr,
										MAX_TIME_VALUE, process_flags);
								ProcessCollisions (GetHeadElement (), TestElementPtr,
										MAX_TIME_VALUE, process_flags);
							}
						}
						UnlockElement (hTestElement);
						return (COLLISION);
					}

					if (!CollidingElement (ElementPtr))
					{
						ElementPtr->state_flags |= COLLISION;
						UnlockElement (hTestElement);
						return (COLLISION);
					}
				}
			}
		}

		UnlockElement (hTestElement);
	}

	return (ElementPtr->state_flags & COLLISION);
}

#define MAX_SOUNDS 8
static BYTE num_sounds;
static SOUND sound_buf[MAX_SOUNDS];

static VIEW_STATE
PreProcessQueue (PSIZE pscroll_x, PSIZE pscroll_y)
{
#ifdef OLD_ZOOM
	BYTE min_reduction, max_reduction;
#else
	SIZE min_reduction, max_reduction;
#endif
	COUNT num_ships;
	POINT Origin;
	register HELEMENT hElement;
	COUNT ships_alive;

#ifdef KDEBUG
	fprintf (stderr, "PreProcess:\n");
#endif
	num_ships = (LOBYTE (battle_counter) ? 1 : 0)
			+ (HIBYTE (battle_counter) ? 1 : 0);

#ifdef OLD_ZOOM
	min_reduction = max_reduction = MAX_VIS_REDUCTION + 1;
#else
	min_reduction = max_reduction = MAX_ZOOM_OUT + (1 << ZOOM_SHIFT);
#endif
	Origin.x = (COORD)(LOG_SPACE_WIDTH >> 1);
	Origin.y = (COORD)(LOG_SPACE_HEIGHT >> 1);

	hElement = GetHeadElement ();
	ships_alive = 0;
	while (hElement != 0)
	{
		ELEMENTPTR ElementPtr;
		HELEMENT hNextElement;

		LockElement (hElement, &ElementPtr);

		if (!(ElementPtr->state_flags & PRE_PROCESS))
			PreProcess (ElementPtr);
		hNextElement = GetSuccElement (ElementPtr);

		if (CollidingElement (ElementPtr)
				&& !(ElementPtr->state_flags & COLLISION))
			ProcessCollisions (hNextElement, ElementPtr,
					MAX_TIME_VALUE, PRE_PROCESS);

		if (ElementPtr->state_flags & PLAYER_SHIP)
		{
			SIZE dx, dy;

			ships_alive++;
#ifdef OLD_ZOOM
			if (max_reduction > MAX_VIS_REDUCTION
					&& min_reduction > MAX_VIS_REDUCTION)
#else
			if (max_reduction > MAX_ZOOM_OUT
					&& min_reduction > MAX_ZOOM_OUT)
#endif
			{
				Origin.x = DISPLAY_ALIGN (ElementPtr->next.location.x);
				Origin.y = DISPLAY_ALIGN (ElementPtr->next.location.y);
			}

			dx = DISPLAY_ALIGN (ElementPtr->next.location.x) - Origin.x;
			dx = WRAP_DELTA_X (dx);
			dy = DISPLAY_ALIGN (ElementPtr->next.location.y) - Origin.y;
			dy = WRAP_DELTA_Y (dy);

			if (num_ships <= 2 || !(ElementPtr->state_flags & BAD_GUY))
			{
				Origin.x = DISPLAY_ALIGN (Origin.x + (dx >> 1));
				Origin.y = DISPLAY_ALIGN (Origin.y + (dy >> 1));

				if (dx < 0)
					dx = -dx;
				if (dy < 0)
					dy = -dy;
				max_reduction = CalcReduction (dx, dy);
			}
#ifdef OLD_ZOOM
			else if (max_reduction > MAX_VIS_REDUCTION
					&& min_reduction <= MAX_VIS_REDUCTION)
#else
			else if (max_reduction > MAX_ZOOM_OUT
					&& min_reduction <= MAX_ZOOM_OUT)
#endif
			{
				Origin.x = DISPLAY_ALIGN (Origin.x + (dx >> 1));
				Origin.y = DISPLAY_ALIGN (Origin.y + (dy >> 1));

				if (dx < 0)
					dx = -dx;
				if (dy < 0)
					dy = -dy;
				min_reduction = CalcReduction (dx, dy);
			}
			else
			{
#ifdef OLD_ZOOM
				BYTE reduction;
#else
				SIZE reduction;
#endif

				if (dx < 0)
					dx = -dx;
				if (dy < 0)
					dy = -dy;
				reduction = CalcReduction (dx << 1, dy << 1);

#ifdef OLD_ZOOM
				if (min_reduction > MAX_VIS_REDUCTION
						|| reduction < min_reduction)
#else
				if (min_reduction > MAX_ZOOM_OUT
						|| reduction < min_reduction)
#endif
					min_reduction = reduction;
			}
//			fprintf (stderr, "dx = %d dy = %d min_red = %d max_red = %d\n",
//					dx, dy, min_reduction, max_reduction);
		}

		UnlockElement (hElement);
		hElement = hNextElement;
	}

#ifdef OLD_ZOOM
	if ((min_reduction > MAX_VIS_REDUCTION || min_reduction <= max_reduction)
			&& (min_reduction = max_reduction) > MAX_VIS_REDUCTION
			&& (min_reduction = GLOBAL (cur_state)) > MAX_VIS_REDUCTION)
		min_reduction = 0;
#else
	if ((min_reduction > MAX_ZOOM_OUT || min_reduction <= max_reduction)
			&& (min_reduction = max_reduction) > MAX_ZOOM_OUT
			&& (min_reduction = zoom_out) > MAX_ZOOM_OUT)
		min_reduction = 1 << ZOOM_SHIFT;
#endif

#ifdef KDEBUG
	fprintf (stderr, "PreProcess: exit\n");
#endif
	return (CalcView (&Origin, min_reduction, pscroll_x, pscroll_y, ships_alive));
}

void
InsertPrim (PRIM_LINKS *pLinks, COUNT primIndex, COUNT iPI)
{
	COUNT Link;
	PRIM_LINKS PL;

	if (iPI == END_OF_LIST)
	{
		Link = GetSuccLink (*pLinks); /* get tail */
		if (Link == END_OF_LIST)
			*pLinks = MakeLinks (primIndex, primIndex);
		else
			*pLinks = MakeLinks (GetPredLink (*pLinks), primIndex);
	}
	else
	{
		PL = GetPrimLinks (&DisplayArray[iPI]);
		if (iPI != GetPredLink (*pLinks)) /* if not the head */
			Link = GetPredLink (PL);
		else
		{
			Link = END_OF_LIST;
			*pLinks = MakeLinks (primIndex, GetSuccLink (*pLinks));
		}
		SetPrimLinks (&DisplayArray[iPI], primIndex, GetSuccLink (PL));
	}

	if (Link != END_OF_LIST)
	{
		PL = GetPrimLinks (&DisplayArray[Link]);
		SetPrimLinks (&DisplayArray[Link], GetPredLink (PL), primIndex);
	}
	SetPrimLinks (&DisplayArray[primIndex], Link, iPI);
}

PRIM_LINKS DisplayLinks;

static void
PostProcessQueue (register VIEW_STATE view_state, register SIZE scroll_x,
		register SIZE scroll_y)
{
	POINT delta;
#ifdef OLD_ZOOM
	register BYTE reduction;
#else
	register SIZE reduction;
#endif
	register HELEMENT hElement;

#ifdef KDEBUG
	fprintf (stderr, "PostProcess:\n");
#endif
#ifdef OLD_ZOOM
	reduction = (BYTE)(GLOBAL (cur_state) + ONE_SHIFT);
#else
	reduction = zoom_out << ONE_SHIFT;
#endif

	hElement = GetHeadElement ();
	while (hElement != 0)
	{
		register ELEMENT_FLAGS state_flags;
		ELEMENTPTR ElementPtr;
		HELEMENT hNextElement;

		LockElement (hElement, &ElementPtr);

		state_flags = ElementPtr->state_flags;
		if (state_flags & PRE_PROCESS)
		{
			if (!(state_flags & COLLISION))
				ElementPtr->state_flags &= ~DEFY_PHYSICS;
			else
				ElementPtr->state_flags &= ~COLLISION;

			if (state_flags & POST_PROCESS)
				delta.x = delta.y = 0;
			else
			{
				delta.x = scroll_x;
				delta.y = scroll_y;
			}
		}
		else
		{
			HELEMENT hPostElement;

			hPostElement = hElement;
			do
			{
				ELEMENTPTR PostElementPtr;

				LockElement (hPostElement, &PostElementPtr);
				if (!(PostElementPtr->state_flags & PRE_PROCESS))
					PreProcess (PostElementPtr);
				hNextElement = GetSuccElement (PostElementPtr);

				if (CollidingElement (PostElementPtr)
						&& !(PostElementPtr->state_flags & COLLISION))
					ProcessCollisions (GetHeadElement (), PostElementPtr,
							MAX_TIME_VALUE, PRE_PROCESS | POST_PROCESS);
				UnlockElement (hPostElement);
			} while ((hPostElement = hNextElement) != 0);

			scroll_x = scroll_y = 0;
			delta.x = delta.y = 0; /* because these are
										  * newly added elements
										  * that are already in
										  * adjusted coordinates
										  */
			state_flags = ElementPtr->state_flags;
		}

		if (state_flags & DISAPPEARING)
		{
			hNextElement = GetSuccElement (ElementPtr);
			UnlockElement (hElement);

			RemoveElement (hElement);
			FreeElement (hElement);
		}
		else
		{
			GRAPHICS_PRIM ObjType;

			ObjType = GetPrimType (&DisplayArray[ElementPtr->PrimIndex]);
			if (view_state != VIEW_STABLE
					|| (state_flags & (APPEARING | CHANGING)))
			{
				POINT next;

				if (ObjType == LINE_PRIM)
				{
					SIZE dx, dy;

					dx = ElementPtr->next.location.x
							- ElementPtr->current.location.x;
					dy = ElementPtr->next.location.y
							- ElementPtr->current.location.y;

					next.x = WRAP_X (ElementPtr->current.location.x + delta.x);
					next.y = WRAP_Y (ElementPtr->current.location.y + delta.y);
#ifdef OLD_ZOOM
					DisplayArray[ElementPtr->PrimIndex].Object.Line.first.x =
							(next.x - SpaceOrg.x) >> reduction;
					DisplayArray[ElementPtr->PrimIndex].Object.Line.first.y =
							(next.y - SpaceOrg.y) >> reduction;
#else
					DisplayArray[ElementPtr->PrimIndex].Object.Line.first.x =
							((next.x - SpaceOrg.x) << ZOOM_SHIFT) / reduction;
					DisplayArray[ElementPtr->PrimIndex].Object.Line.first.y =
							((next.y - SpaceOrg.y) << ZOOM_SHIFT) / reduction;
#endif
					next.x += dx;
					next.y += dy;
#ifdef OLD_ZOOM
					DisplayArray[ElementPtr->PrimIndex].Object.Line.second.x =
							(next.x - SpaceOrg.x) >> reduction;
					DisplayArray[ElementPtr->PrimIndex].Object.Line.second.y =
							(next.y - SpaceOrg.y) >> reduction;
#else
					DisplayArray[ElementPtr->PrimIndex].Object.Line.second.x =
							((next.x - SpaceOrg.x) << ZOOM_SHIFT) / reduction;
					DisplayArray[ElementPtr->PrimIndex].Object.Line.second.y =
							((next.y - SpaceOrg.y) << ZOOM_SHIFT) / reduction;
#endif
				}
				else
				{
					next.x = WRAP_X (ElementPtr->next.location.x + delta.x);
					next.y = WRAP_Y (ElementPtr->next.location.y + delta.y);
#ifdef OLD_ZOOM
					DisplayArray[ElementPtr->PrimIndex].Object.Point.x =
							(next.x - SpaceOrg.x) >> reduction;
					DisplayArray[ElementPtr->PrimIndex].Object.Point.y =
							(next.y - SpaceOrg.y) >> reduction;
#else
					DisplayArray[ElementPtr->PrimIndex].Object.Point.x =
							((next.x - SpaceOrg.x) << ZOOM_SHIFT) / reduction;
					DisplayArray[ElementPtr->PrimIndex].Object.Point.y =
							((next.y - SpaceOrg.y) << ZOOM_SHIFT) / reduction;
#endif

					if (ObjType == STAMP_PRIM
							|| ObjType == STAMPFILL_PRIM)
					{
						if (view_state == VIEW_CHANGE
								|| (state_flags & (APPEARING | CHANGING)))
						{
#ifdef OLD_ZOOM
							ElementPtr->next.image.frame =
#ifdef SAFE
									SetAbsFrameIndex (
									ElementPtr->next.image.farray
									[GLOBAL (cur_state)],
									GetFrameIndex (ElementPtr->next.image.frame));
#else /* SAFE */
									SetEquFrameIndex (
									ElementPtr->next.image.farray
									[GLOBAL (cur_state)],
									ElementPtr->next.image.frame);
#endif /* SAFE */
#else
							COUNT index, scale;
							
							CALC_ZOOM_STUFF (&index, &scale);
							ElementPtr->next.image.frame =
#ifdef SAFE
									SetAbsFrameIndex (
									ElementPtr->next.image.farray
									[index],
									GetFrameIndex (ElementPtr->next.image.frame));
#else /* SAFE */
									SetEquFrameIndex (
									ElementPtr->next.image.farray
									[index],
									ElementPtr->next.image.frame);
#endif /* SAFE */
#endif
						}
						DisplayArray[ElementPtr->PrimIndex].Object.Stamp.frame =
								ElementPtr->next.image.frame;
					}
				}

				ElementPtr->next.location = next;
			}

			PostProcess (ElementPtr);

			if (ObjType < NUM_PRIMS)
				InsertPrim (&DisplayLinks, ElementPtr->PrimIndex, END_OF_LIST);

			hNextElement = GetSuccElement (ElementPtr);
			UnlockElement (hElement);
		}

		hElement = hNextElement;
	}
#ifdef KDEBUG
	fprintf (stderr, "PostProcess: exit\n");
#endif
}

void
InitDisplayList (void)
{
	COUNT i;

#ifdef OLD_ZOOM
	GLOBAL (cur_state) = MAX_VIS_REDUCTION + 1;
#else
	zoom_out = MAX_ZOOM_OUT + (1 << ZOOM_SHIFT);
#endif

	ReinitQueue (&disp_q);

	for (i = 0; i < MAX_DISPLAY_PRIMS; ++i)
		SetPrimLinks (&DisplayArray[i], END_OF_LIST, i + 1);
	SetPrimLinks (&DisplayArray[i - 1], END_OF_LIST, END_OF_LIST);
	DisplayFreeList = 0;
	DisplayLinks = MakeLinks (END_OF_LIST, END_OF_LIST);
}

void
PlaySound (SOUND S, BYTE Priority)
{
#define MIN_FX_CHANNEL 1
#define NUM_FX_CHANNELS 4 /* obviously number of channels
												   is assumed to be at least 5 */
	BYTE chan, c;
	static BYTE lru_channel[NUM_FX_CHANNELS] = {0, 1, 2, 3};
	static SOUND channel[NUM_FX_CHANNELS];

	if (S == 0) return;

	for (chan = 0; chan < NUM_FX_CHANNELS; ++chan)
	{
		if (S == channel[chan])
			break;
	}

	if (chan == NUM_FX_CHANNELS)
	{
		for (chan = 0; chan < NUM_FX_CHANNELS; ++chan)
		{
			if (!ChannelPlaying (chan + MIN_FX_CHANNEL))
				break;
		}

		if (chan == NUM_FX_CHANNELS)
			chan = lru_channel[0];
	}

	channel[chan] = S;

	for (c = 0; c < NUM_FX_CHANNELS - 1; ++c)
	{
		if (lru_channel[c] == chan)
		{
			memmove (
					(PBYTE)&lru_channel[c],
					(PBYTE)&lru_channel[c + 1],
					(NUM_FX_CHANNELS - 1) - c
					);
			break;
		}
	}
	lru_channel[NUM_FX_CHANNELS - 1] = chan;

	PlaySoundEffect (S, chan + MIN_FX_CHANNEL, Priority);
}

UWORD nth_frame;

void
RedrawQueue (BOOLEAN clear)
{
	SIZE scroll_x, scroll_y;
	VIEW_STATE view_state;

	SetContext (StatusContext);

	view_state = PreProcessQueue (&scroll_x, &scroll_y);
	PostProcessQueue (view_state, scroll_x, scroll_y);

	SetContext (SpaceContext);
	if (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE
			|| !(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
	{
		BYTE skip_frames;

		if ((skip_frames = HIBYTE (nth_frame)) != (BYTE)~0
				&& (skip_frames == 0 || (--nth_frame & 0x00FF) == 0))
		{
			nth_frame += skip_frames;
if (clear)
	ClearDrawable (); // this is for BATCH_BUILD_PAGE effect, but not scaled by SetGraphicScale
{
	COUNT index, scale;
	
	CALC_ZOOM_STUFF (&index, &scale);
	SetGraphicScale ((scale >> GSCALE_SHIFT) << GSCALE_SHIFT);
}
			DrawBatch (DisplayArray, DisplayLinks, 0);//BATCH_BUILD_PAGE);
SetGraphicScale (0);
		}

		if (num_sounds > 0)
		{
			SOUND *pSound;

			pSound = sound_buf;
			do
			{
				PlaySound (*pSound++, GAME_SOUND_PRIORITY);
			} while (--num_sounds);
		}
	}
	DisplayLinks = MakeLinks (END_OF_LIST, END_OF_LIST);
}

void
ProcessSound (SOUND Sound)
{
	if (Sound == (SOUND)~0)
	{
		memset (sound_buf, 0, sizeof (sound_buf));
		num_sounds = MAX_SOUNDS;
	}
	else if (num_sounds < MAX_SOUNDS)
		sound_buf[num_sounds++] = Sound;
}

void
Untarget (ELEMENTPTR ElementPtr)
{
	HELEMENT hElement, hNextElement;

	for (hElement = GetHeadElement (); hElement; hElement = hNextElement)
	{
		HELEMENT hTarget;
		ELEMENTPTR ListPtr;

		LockElement (hElement, &ListPtr);
		hNextElement = GetSuccElement (ListPtr);

		hTarget = ListPtr->hTarget;
		if (hTarget)
		{
			ELEMENTPTR TargetElementPtr;

			LockElement (hTarget, &TargetElementPtr);
			if (TargetElementPtr == ElementPtr)
				ListPtr->hTarget = 0;
			UnlockElement (hTarget);
		}

		UnlockElement (hElement);
	}
}

