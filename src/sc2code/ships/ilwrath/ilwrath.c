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

#include "ships/ship.h"
#include "ships/ilwrath/resinst.h"

#include "colors.h"
#include "globdata.h"


#define MAX_CREW 22
#define MAX_ENERGY 16
#define ENERGY_REGENERATION 4
#define WEAPON_ENERGY_COST 1
#define SPECIAL_ENERGY_COST 3
#define ENERGY_WAIT 4
#define MAX_THRUST 25
#define THRUST_INCREMENT 5
#define TURN_WAIT 2
#define THRUST_WAIT 0
#define WEAPON_WAIT 0
#define SPECIAL_WAIT 13

#define SHIP_MASS 7
#define MISSILE_LIFE 8

static RACE_DESC ilwrath_desc =
{
	{
		FIRES_FORE,
		10, /* Super Melee cost */
		1410 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			48, 1700,
		},
		(STRING)ILWRATH_RACE_STRINGS,
		(FRAME)ILWRATH_ICON_MASK_PMAP_ANIM,
		(FRAME)ILWRATH_MICON_MASK_PMAP_ANIM,
	},
	{
		MAX_THRUST,
		THRUST_INCREMENT,
		ENERGY_REGENERATION,
		WEAPON_ENERGY_COST,
		SPECIAL_ENERGY_COST,
		ENERGY_WAIT,
		TURN_WAIT,
		THRUST_WAIT,
		WEAPON_WAIT,
		SPECIAL_WAIT,
		SHIP_MASS,
	},
	{
		{
			(FRAME)ILWRATH_BIG_MASK_PMAP_ANIM,
			(FRAME)ILWRATH_MED_MASK_PMAP_ANIM,
			(FRAME)ILWRATH_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)FIRE_BIG_MASK_PMAP_ANIM,
			(FRAME)FIRE_MED_MASK_PMAP_ANIM,
			(FRAME)FIRE_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		{
			(FRAME)ILWRATH_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)ILWRATH_VICTORY_SONG,
		(SOUND)ILWRATH_SHIP_SOUNDS,
	},
	{
		0,
		CLOSE_RANGE_WEAPON,
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};

static void
flame_preprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->current.image.frame);
		ElementPtr->state_flags |= CHANGING;

		ElementPtr->turn_wait = ElementPtr->next_turn;
	}
}

static void
flame_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
	ElementPtr0->state_flags &= ~DISAPPEARING;
	ElementPtr0->state_flags |= NONSOLID;
}

static void
ilwrath_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern, COUNT ConcernCounter)
{
	PEVALUATE_DESC lpEvalDesc;
	STARSHIPPTR StarShipPtr;

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	 lpEvalDesc->MoveState = PURSUE;
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->which_turn <= 10)
				/* don't want to dodge when you could be flaming */
		ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr = 0;

	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (lpEvalDesc->ObjectPtr
			&& (lpEvalDesc->which_turn <= 6
			|| (lpEvalDesc->which_turn <= 10
			&& ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn <= 10)))
	{
		StarShipPtr->ship_input_state &= ~SPECIAL;
		if (OBJECT_CLOAKED (ShipPtr))
		{
			StarShipPtr->ship_input_state &= ~LEFT | RIGHT;
			StarShipPtr->ship_input_state |= THRUST;
		}
		StarShipPtr->ship_input_state |= WEAPON;
	}
	else if (StarShipPtr->special_counter == 0
			&& (LOBYTE (GLOBAL (CurrentActivity)) != IN_ENCOUNTER
			|| !GET_GAME_STATE (PROBE_ILWRATH_ENCOUNTER)))
	{
		StarShipPtr->ship_input_state &= ~SPECIAL;
		if (!OBJECT_CLOAKED (ShipPtr)
				&& !(StarShipPtr->ship_input_state & WEAPON))
			StarShipPtr->ship_input_state |= SPECIAL;
	}
}

static COUNT
initialize_flame (PELEMENT ShipPtr, HELEMENT FlameArray[])
{
#define ILWRATH_OFFSET 29
#define MISSILE_SPEED MAX_THRUST
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 1
#define MISSILE_OFFSET 0
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = 0;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = ILWRATH_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = flame_preprocess;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	FlameArray[0] = initialize_missile (&MissileBlock);

	if (FlameArray[0])
	{
		SIZE dx, dy;
		ELEMENTPTR FlamePtr;

		LockElement (FlameArray[0], &FlamePtr);
		GetCurrentVelocityComponents (&ShipPtr->velocity, &dx, &dy);
		DeltaVelocityComponents (&FlamePtr->velocity, dx, dy);
		FlamePtr->current.location.x -= VELOCITY_TO_WORLD (dx);
		FlamePtr->current.location.y -= VELOCITY_TO_WORLD (dy);

		FlamePtr->collision_func = flame_collision;
		FlamePtr->turn_wait = 0;
		UnlockElement (FlameArray[0]);
	}

	return (1);
}

static void
ilwrath_preprocess (PELEMENT ElementPtr)
{
	ELEMENT_FLAGS status_flags;
	STARSHIPPTR StarShipPtr;
	PPRIMITIVE lpPrim;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	status_flags = StarShipPtr->cur_status_flags;
	lpPrim = &(GLOBAL (DisplayArray))[ElementPtr->PrimIndex];
	if (GetPrimType (lpPrim) == STAMPFILL_PRIM)
	{
		COLOR Color;
		BOOLEAN weapon_discharge;

		Color = GetPrimColor (lpPrim);
		weapon_discharge = ((status_flags & WEAPON)
				&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= WEAPON_ENERGY_COST);
		if (weapon_discharge
				|| (StarShipPtr->special_counter == 0
				&& ((status_flags & SPECIAL) || Color != BLACK_COLOR)))
		{
			if (Color == BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F))
				SetPrimType (lpPrim, STAMP_PRIM);
			else if (Color == BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x1F), 0x0B))
				SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));
			else if (Color == BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x14), 0x03))
				SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x1F), 0x0B));
			else if (Color == BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09))
				SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x14), 0x03));
			else if (Color == BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01))
				SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09));
			else
			{
				ProcessSound (SetAbsSoundIndex (
								/* CLOAKING_OFF */
						StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2), ElementPtr);
				SetPrimColor (lpPrim,
						BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01));
				if (weapon_discharge)
				{
					COUNT facing;

					facing = StarShipPtr->ShipFacing;
					if (TrackShip (ElementPtr, &facing) >= 0)
					{
#define LOOK_AHEAD 4
						ELEMENTPTR eptr;
						SIZE dx0, dy0, dx1, dy1;
						VELOCITY_DESC v;

						LockElement (ElementPtr->hTarget, &eptr);
						v = eptr->velocity;
						GetNextVelocityComponents (&v, &dx0, &dy0, LOOK_AHEAD);
						v = ElementPtr->velocity;
						GetNextVelocityComponents (&v, &dx1, &dy1, LOOK_AHEAD);
						dx0 = (eptr->current.location.x + dx0)
								- (ElementPtr->current.location.x + dx1);
						dy0 = (eptr->current.location.y + dy0)
								- (ElementPtr->current.location.y + dy1);
						UnlockElement (ElementPtr->hTarget);

						StarShipPtr->ShipFacing =
								NORMALIZE_FACING (
								ANGLE_TO_FACING (ARCTAN (dx0, dy0))
								);
#ifdef NOTYET
						if (ElementPtr->thrust_wait == 0
								&& (StarShipPtr->cur_status_flags & THRUST))
						{
							COUNT last_facing;

							do
							{
								VELOCITY_DESC temp_v;

								last_facing = StarShipPtr->ShipFacing;
								inertial_thrust (ElementPtr);
								temp_v = ElementPtr->velocity;
								ElementPtr->velocity = v;

								dx0 += dx1;
								dy0 += dy1;
								GetNextVelocityComponents (&temp_v,
										&dx1, &dy1, LOOK_AHEAD);
								dx0 -= dx1;
								dy0 -= dy1;
								StarShipPtr->ShipFacing =
										NORMALIZE_FACING (
										ANGLE_TO_FACING (ARCTAN (dx0, dy0))
										);
							} while (StarShipPtr->ShipFacing != last_facing);
						}
#endif /* NOTYET */
						if (ElementPtr->turn_wait == 0)
							++ElementPtr->turn_wait;
						ElementPtr->next.image.frame =
								SetAbsFrameIndex (ElementPtr->next.image.frame,
								StarShipPtr->ShipFacing);
					}
					ElementPtr->hTarget = 0;
				}
			}

			ElementPtr->state_flags |= CHANGING;
			status_flags &= ~SPECIAL;
			StarShipPtr->special_counter = 0;
		}
		else if (Color != BLACK_COLOR)
		{
			if (Color == BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01))
			{
				SetPrimColor (lpPrim, BLACK_COLOR);
				Untarget (ElementPtr);
			}
			else if (Color == BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09))
				SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01));
			else if (Color == BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x14), 0x03))
				SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09));
			else if (Color == BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x1F), 0x0B))
				SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x14), 0x03));
			else
				SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x1F), 0x0B));

			ElementPtr->state_flags |= CHANGING;
		}
	}

	if ((status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));
		SetPrimType (lpPrim, STAMPFILL_PRIM);

		ProcessSound (SetAbsSoundIndex (
						/* CLOAKING_ON */
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
		StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;

		ElementPtr->state_flags |= CHANGING;
	}
}

RACE_DESCPTR
init_ilwrath (void)
{
	RACE_DESCPTR RaceDescPtr;

	ilwrath_desc.preprocess_func = ilwrath_preprocess;
	ilwrath_desc.init_weapon_func = initialize_flame;
	ilwrath_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) ilwrath_intelligence;

	RaceDescPtr = &ilwrath_desc;

	return (RaceDescPtr);
}

