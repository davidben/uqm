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
#include "ships/zoqfot/resinst.h"

#include "libs/mathlib.h"


#define MAX_CREW 10
#define MAX_ENERGY 10
#define ENERGY_REGENERATION 1
#define WEAPON_ENERGY_COST 1
#define SPECIAL_ENERGY_COST (MAX_ENERGY * 3 / 4)
#define ENERGY_WAIT 4
#define MAX_THRUST 40
#define THRUST_INCREMENT 10
#define TURN_WAIT 1
#define THRUST_WAIT 0
#define WEAPON_WAIT 0
#define SPECIAL_WAIT 6

#define SHIP_MASS 5
#define MISSILE_SPEED DISPLAY_TO_WORLD (10)
#define MISSILE_LIFE 10
#define MISSILE_RANGE (MISSILE_SPEED * MISSILE_LIFE)

static RACE_DESC zoqfotpik_desc =
{
	{
		FIRES_FORE,
		6, /* Super Melee cost */
		320 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			3761, 5333,
		},
		(STRING)ZOQFOTPIK_RACE_STRINGS,
		(FRAME)ZOQFOTPIK_ICON_MASK_PMAP_ANIM,
		(FRAME)ZOQFOTPIK_MICON_MASK_PMAP_ANIM,
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
			(FRAME)ZOQFOTPIK_BIG_MASK_PMAP_ANIM,
			(FRAME)ZOQFOTPIK_MED_MASK_PMAP_ANIM,
			(FRAME)ZOQFOTPIK_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)SPIT_BIG_MASK_PMAP_ANIM,
			(FRAME)SPIT_MED_MASK_PMAP_ANIM,
			(FRAME)SPIT_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)STINGER_BIG_MASK_PMAP_ANIM,
			(FRAME)STINGER_MED_MASK_PMAP_ANIM,
			(FRAME)STINGER_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)ZOQFOTPIK_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)ZOQFOTPIK_VICTORY_SONG,
		(SOUND)ZOQFOTPIK_SHIP_SOUNDS,
	},
	{
		0,
		MISSILE_RANGE,
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};

#define ZOQFOTPIK_OFFSET 13
#define SPIT_WAIT 2

static void
spit_preprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		COUNT index, angle, speed;

		ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->next.image.frame);
		angle = GetVelocityTravelAngle (&ElementPtr->velocity);
		if ((index = GetFrameIndex (ElementPtr->next.image.frame)) == 1)
			angle = angle + (((COUNT)TFB_Random () % 3) - 1);

		speed = WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (
				GetFrameCount (ElementPtr->next.image.frame) - index) << 1);
		SetVelocityComponents (&ElementPtr->velocity,
				(SIZE)COSINE (angle, speed),
				(SIZE)SINE (angle, speed));

		ElementPtr->turn_wait = SPIT_WAIT;
		ElementPtr->state_flags |= CHANGING;
	}
}

static COUNT
initialize_spit (PELEMENT ShipPtr, HELEMENT SpitArray[])
{
#define MISSILE_DAMAGE 1
#define MISSILE_HITS 1
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
	MissileBlock.pixoffs = ZOQFOTPIK_OFFSET;
	MissileBlock.speed = DISPLAY_TO_WORLD (
			GetFrameCount (StarShipPtr->RaceDescPtr->ship_data.weapon[0])) << 1;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = spit_preprocess;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	SpitArray[0] = initialize_missile (&MissileBlock);

	return (1);
}

static void spawn_tongue (PELEMENT ElementPtr);

static void
tongue_postprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->turn_wait)
		spawn_tongue (ElementPtr);
}

static void
tongue_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr0, &StarShipPtr);
	if (StarShipPtr->special_counter ==
			StarShipPtr->RaceDescPtr->characteristics.special_wait)
		weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);

	StarShipPtr->special_counter -= ElementPtr0->turn_wait;
	ElementPtr0->turn_wait = 0;
	ElementPtr0->state_flags |= NONSOLID;
}

static void
spawn_tongue (ElementPtr)
PELEMENT ElementPtr;
{
#define TONGUE_SPEED 0
#define TONGUE_HITS 1
#define TONGUE_DAMAGE 12
#define TONGUE_OFFSET 4
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK TongueBlock;
	HELEMENT Tongue;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	TongueBlock.cx = ElementPtr->next.location.x;
	TongueBlock.cy = ElementPtr->next.location.y;
	TongueBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	TongueBlock.face = TongueBlock.index = StarShipPtr->ShipFacing;
	TongueBlock.sender = (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	TongueBlock.pixoffs = 0;
	TongueBlock.speed = TONGUE_SPEED;
	TongueBlock.hit_points = TONGUE_HITS;
	TongueBlock.damage = TONGUE_DAMAGE;
	TongueBlock.life = 1;
	TongueBlock.preprocess_func = 0;
	TongueBlock.blast_offs = TONGUE_OFFSET;
	Tongue = initialize_missile (&TongueBlock);
	if (Tongue)
	{
		ELEMENTPTR TonguePtr;

		LockElement (Tongue, &TonguePtr);
		TonguePtr->postprocess_func = tongue_postprocess;
		TonguePtr->collision_func = tongue_collision;
		if (ElementPtr->state_flags & PLAYER_SHIP)
			TonguePtr->turn_wait = StarShipPtr->special_counter;
		else
		{
			COUNT angle;
			RECT r;
			SIZE x_offs, y_offs;

			TonguePtr->turn_wait = ElementPtr->turn_wait - 1;

			GetFrameRect (TonguePtr->current.image.frame, &r);
			x_offs = DISPLAY_TO_WORLD (r.extent.width >> 1);
			y_offs = DISPLAY_TO_WORLD (r.extent.height >> 1);

			angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
			if (angle > HALF_CIRCLE && angle < FULL_CIRCLE)
				TonguePtr->current.location.x -= x_offs;
			else if (angle > 0 && angle < HALF_CIRCLE)
				TonguePtr->current.location.x += x_offs;
			if (angle < QUADRANT || angle > FULL_CIRCLE - QUADRANT)
				TonguePtr->current.location.y -= y_offs;
			else if (angle > QUADRANT && angle < FULL_CIRCLE - QUADRANT)
				TonguePtr->current.location.y += y_offs;
		}

		SetElementStarShip (TonguePtr, StarShipPtr);
		UnlockElement (Tongue);
		PutElement (Tongue);
	}
}

static void
zoqfotpik_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern, COUNT ConcernCounter)
{
	BOOLEAN GiveTongueJob;
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	GiveTongueJob = FALSE;
	if (StarShipPtr->special_counter == 0)
	{
			PEVALUATE_DESC lpEnemyEvalDesc;

		StarShipPtr->ship_input_state &= ~SPECIAL;

		lpEnemyEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
		if (lpEnemyEvalDesc->ObjectPtr
				&& lpEnemyEvalDesc->which_turn <= 4
#ifdef NEVER
				&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= SPECIAL_ENERGY_COST
#endif /* NEVER */
				)
		{
			SIZE delta_x, delta_y;

			GiveTongueJob = TRUE;

			lpEnemyEvalDesc->MoveState = PURSUE;
			delta_x = lpEnemyEvalDesc->ObjectPtr->next.location.x
					- ShipPtr->next.location.x;
			delta_y = lpEnemyEvalDesc->ObjectPtr->next.location.y
					- ShipPtr->next.location.y;
			if (StarShipPtr->ShipFacing == NORMALIZE_FACING (
					ANGLE_TO_FACING (ARCTAN (delta_x, delta_y))
					))
				StarShipPtr->ship_input_state |= SPECIAL;
		}
	}

	++StarShipPtr->weapon_counter;
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
	--StarShipPtr->weapon_counter;

	if (StarShipPtr->weapon_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~WEAPON;
		if (!GiveTongueJob)
		{
			ObjectsOfConcern += ConcernCounter;
			while (ConcernCounter--)
			{
				--ObjectsOfConcern;
				if (ObjectsOfConcern->ObjectPtr
						&& (ConcernCounter == ENEMY_SHIP_INDEX
						|| (ConcernCounter == ENEMY_WEAPON_INDEX
						&& ObjectsOfConcern->MoveState != AVOID
#ifdef NEVER
						&& !(PlayerControl[cur_player] & STANDARD_RATING)
#endif /* NEVER */
						))
						&& ship_weapons (ShipPtr,
						ObjectsOfConcern->ObjectPtr, DISPLAY_TO_WORLD (20)))
				{
					StarShipPtr->ship_input_state |= WEAPON;
					break;
				}
			}
		}
	}
}

static void
zoqfotpik_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		ProcessSound (SetAbsSoundIndex (
					/* STICK_OUT_TONGUE */
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);

		StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}

	if (StarShipPtr->special_counter)
		spawn_tongue (ElementPtr);
}

RACE_DESCPTR
init_zoqfotpik (void)
{
	RACE_DESCPTR RaceDescPtr;

	zoqfotpik_desc.postprocess_func = zoqfotpik_postprocess;
	zoqfotpik_desc.init_weapon_func = initialize_spit;
	zoqfotpik_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) zoqfotpik_intelligence;

	RaceDescPtr = &zoqfotpik_desc;

	return (RaceDescPtr);
}

