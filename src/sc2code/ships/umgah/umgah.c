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
#include "ships/umgah/resinst.h"

#include "libs/mathlib.h"


#define MAX_CREW 10
#define MAX_ENERGY 30
#define ENERGY_REGENERATION MAX_ENERGY
#define WEAPON_ENERGY_COST 0
#define SPECIAL_ENERGY_COST 1
#define ENERGY_WAIT 150
#define MAX_THRUST /* DISPLAY_TO_WORLD (5) */ 18
#define THRUST_INCREMENT /* DISPLAY_TO_WORLD (2) */ 6
#define TURN_WAIT 4
#define THRUST_WAIT 3
#define WEAPON_WAIT 0
#define SPECIAL_WAIT 2

#define SHIP_MASS 1

static FRAME LastShipFrame[2];

static RACE_DESC umgah_desc =
{
	{
		FIRES_FORE | IMMEDIATE_WEAPON,
		7, /* Super Melee cost */
		833 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			1798, 6000,
		},
		(STRING)UMGAH_RACE_STRINGS,
		(FRAME)UMGAH_ICON_MASK_PMAP_ANIM,
		(FRAME)UMGAH_MICON_MASK_PMAP_ANIM,
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
			(FRAME)UMGAH_BIG_MASK_PMAP_ANIM,
			(FRAME)UMGAH_MED_MASK_PMAP_ANIM,
			(FRAME)UMGAH_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)SPRITZ_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
		},
		{
			(FRAME)CONE_BIG_MASK_ANIM,
			(FRAME)CONE_MED_MASK_ANIM,
			(FRAME)CONE_SML_MASK_ANIM,
		},
		{
			(FRAME)UMGAH_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)UMGAH_VICTORY_SONG,
		(SOUND)UMGAH_SHIP_SOUNDS,
	},
	{
		0,
		(LONG_RANGE_WEAPON << 2),
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};

static void
cone_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	StarShipPtr->RaceDescPtr->ship_data.special[0] =
			SetRelFrameIndex (StarShipPtr->RaceDescPtr->ship_data.special[0],
			ANGLE_TO_FACING (FULL_CIRCLE));

	ElementPtr->state_flags |= APPEARING;
}

static void
cone_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	HELEMENT hBlastElement;

	hBlastElement = weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
	if (hBlastElement)
	{
		RemoveElement (hBlastElement);
		FreeElement (hBlastElement);

		ElementPtr0->state_flags &= ~DISAPPEARING;
	}
}

#define JUMP_DIST DISPLAY_TO_WORLD (40)

static void
umgah_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern, COUNT ConcernCounter)
{
	PEVALUATE_DESC lpEvalDesc;
	STARSHIPPTR StarShipPtr, EnemyStarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == ENTICE)
	{
		if (lpEvalDesc->which_turn > 3
				|| (StarShipPtr->old_status_flags & SPECIAL))
			lpEvalDesc->ObjectPtr = 0;
		else if ((lpEvalDesc->ObjectPtr->state_flags & FINITE_LIFE)
				&& !(lpEvalDesc->ObjectPtr->state_flags & CREW_OBJECT))
			lpEvalDesc->MoveState = AVOID;
		else
			lpEvalDesc->MoveState = PURSUE;
	}

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (StarShipPtr->special_counter
			|| ObjectsOfConcern[GRAVITY_MASS_INDEX].ObjectPtr
			|| lpEvalDesc->ObjectPtr == 0)
	{
		StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = CLOSE_RANGE_WEAPON;
		ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

		if (lpEvalDesc->which_turn < 16)
			StarShipPtr->ship_input_state |= WEAPON;
		StarShipPtr->ship_input_state &= ~SPECIAL;
	}
	else
	{
		BYTE this_turn;
		SIZE delta_x, delta_y;
		BOOLEAN EnemyBehind, EnoughJuice;

		if (lpEvalDesc->which_turn >= 0xFF + 1)
			this_turn = 0xFF;
		else
			this_turn = (BYTE)lpEvalDesc->which_turn;

		EnoughJuice = (BOOLEAN)(WORLD_TO_TURN (
				JUMP_DIST * StarShipPtr->RaceDescPtr->ship_info.energy_level
				/ SPECIAL_ENERGY_COST
				) > this_turn);
		delta_x = lpEvalDesc->ObjectPtr->next.location.x -
				ShipPtr->next.location.x;
		delta_y = lpEvalDesc->ObjectPtr->next.location.y -
				ShipPtr->next.location.y;
		EnemyBehind = (BOOLEAN)(NORMALIZE_ANGLE (
				ARCTAN (delta_x, delta_y)
				- (FACING_TO_ANGLE (StarShipPtr->ShipFacing)
				+ HALF_CIRCLE) + (OCTANT + (OCTANT >> 2))
				) <= ((OCTANT + (OCTANT >> 2)) << 1));
		
		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if (EnoughJuice
				&& ((StarShipPtr->old_status_flags & SPECIAL)
				|| EnemyBehind
				|| (this_turn > 6
				&& MANEUVERABILITY (
				&EnemyStarShipPtr->RaceDescPtr->cyborg_control
				) <= SLOW_SHIP)
				|| (this_turn >= 16 && this_turn <= 24)))
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = (LONG_RANGE_WEAPON << 3);
		else
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = CLOSE_RANGE_WEAPON;

		ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

		if (StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange == CLOSE_RANGE_WEAPON)
			StarShipPtr->ship_input_state &= ~SPECIAL;
		else
		{
			BOOLEAN LinedUp;

			StarShipPtr->ship_input_state &= ~THRUST;
			LinedUp = (BOOLEAN)(ShipPtr->turn_wait == 0
					&& !(StarShipPtr->old_status_flags & (LEFT | RIGHT)));
			if (((StarShipPtr->old_status_flags & SPECIAL)
					&& this_turn <= StarShipPtr->RaceDescPtr->characteristics.special_wait)
					|| (!(StarShipPtr->old_status_flags & SPECIAL)
					&& EnemyBehind && (LinedUp || this_turn < 16)))
			{
				StarShipPtr->ship_input_state |= SPECIAL;
				StarShipPtr->RaceDescPtr->characteristics.special_wait = this_turn;

				/* don't want him backing straight into ship */
				if (this_turn <= 8 && LinedUp)
				{
					if (TFB_Random () & 1)
						StarShipPtr->ship_input_state |= LEFT;
					else
						StarShipPtr->ship_input_state |= RIGHT;
				}
			}
			else if (StarShipPtr->old_status_flags & SPECIAL)
			{
				StarShipPtr->ship_input_state &= ~(SPECIAL | LEFT | RIGHT);
				StarShipPtr->ship_input_state |= THRUST;
			}
		}

		if (this_turn < 16 && !EnemyBehind)
			StarShipPtr->ship_input_state |= WEAPON;
	}

	if (!(StarShipPtr->ship_input_state & SPECIAL))
		StarShipPtr->RaceDescPtr->characteristics.special_wait = 0xFF;
}
static COUNT
initialize_cone (PELEMENT ShipPtr, HELEMENT ConeArray[])
{
#define UMGAH_OFFSET 0
#define MISSILE_SPEED 0
#define MISSILE_HITS 100
#define MISSILE_DAMAGE 1
#define MISSILE_LIFE 1
#define MISSILE_OFFSET 0
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = UMGAH_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = cone_preprocess;
	MissileBlock.blast_offs = MISSILE_OFFSET;

	if (ShipPtr->next.image.frame != LastShipFrame[WHICH_SIDE(ShipPtr->state_flags)])
	{
		LastShipFrame[WHICH_SIDE(ShipPtr->state_flags)] = ShipPtr->next.image.frame;

		StarShipPtr->RaceDescPtr->ship_data.special[0] =
				SetAbsFrameIndex (
				StarShipPtr->RaceDescPtr->ship_data.special[0],
				StarShipPtr->ShipFacing
				);
	}
	
	MissileBlock.index = GetFrameIndex (StarShipPtr->RaceDescPtr->ship_data.special[0]);
	ConeArray[0] = initialize_missile (&MissileBlock);

	if (ConeArray[0])
	{
		ELEMENTPTR ConePtr;

		LockElement (ConeArray[0], &ConePtr);
		ConePtr->collision_func = cone_collision;
		ConePtr->state_flags &= ~APPEARING;
		ConePtr->next = ConePtr->current;
		InitIntersectStartPoint (ConePtr);
		InitIntersectEndPoint (ConePtr);
		ConePtr->IntersectControl.IntersectStamp.frame =
				StarShipPtr->RaceDescPtr->ship_data.special[0];
		UnlockElement (ConeArray[0]);
	}

	return (1);
}

static void
umgah_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->special_counter > 0)
	{
		StarShipPtr->special_counter = 0;

		ZeroVelocityComponents (&ElementPtr->velocity);
	}
}

static void
umgah_preprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->state_flags & APPEARING)
		LastShipFrame[WHICH_SIDE(ElementPtr->state_flags)] = 0;
	else
	{
		STARSHIPPTR StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		if (ElementPtr->thrust_wait == 0
				&& (StarShipPtr->cur_status_flags & SPECIAL)
				&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
		{
			COUNT facing;

			ProcessSound (SetAbsSoundIndex (
							/* ZIP_BACKWARDS */
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
			facing = FACING_TO_ANGLE (StarShipPtr->ShipFacing) + HALF_CIRCLE;
			DeltaVelocityComponents (&ElementPtr->velocity,
					COSINE (facing, WORLD_TO_VELOCITY (JUMP_DIST)),
					SINE (facing, WORLD_TO_VELOCITY (JUMP_DIST)));
			StarShipPtr->cur_status_flags &=
					~(SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);

			StarShipPtr->special_counter = SPECIAL_WAIT;
		}
	}
}

RACE_DESCPTR
init_umgah (void)
{
	RACE_DESCPTR RaceDescPtr;

	umgah_desc.preprocess_func = umgah_preprocess;
	umgah_desc.postprocess_func = umgah_postprocess;
	umgah_desc.init_weapon_func = initialize_cone;
	umgah_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) umgah_intelligence;

	RaceDescPtr = &umgah_desc;

	return (RaceDescPtr);
}

