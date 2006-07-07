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
#include "ships/melnorme/resinst.h"

#include "globdata.h"
#include "libs/mathlib.h"


#define MAX_CREW 20
#define MAX_ENERGY MAX_ENERGY_SIZE
#define ENERGY_REGENERATION 1
#define WEAPON_ENERGY_COST 5
#define SPECIAL_ENERGY_COST 20
#define ENERGY_WAIT 4
#define MAX_THRUST 36
#define THRUST_INCREMENT 6
#define TURN_WAIT 4
#define THRUST_WAIT 4
#define WEAPON_WAIT 1
#define SPECIAL_WAIT 20

#define SHIP_MASS 7
#define PUMPUP_SPEED DISPLAY_TO_WORLD (45)
#define PUMPUP_LIFE 10

static RACE_DESC melnorme_desc =
{
	{
		FIRES_FORE,
		18, /* Super Melee cost */
		~0, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			MAX_X_UNIVERSE >> 1, MAX_Y_UNIVERSE >> 1,
		},
		(STRING)MELNORME_RACE_STRINGS,
		(FRAME)MELNORME_ICON_MASK_PMAP_ANIM,
		(FRAME)MELNORME_MICON_MASK_PMAP_ANIM,
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
			(FRAME)MELNORME_BIG_MASK_PMAP_ANIM,
			(FRAME)MELNORME_MED_MASK_PMAP_ANIM,
			(FRAME)MELNORME_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)PUMPUP_BIG_MASK_PMAP_ANIM,
			(FRAME)PUMPUP_MED_MASK_PMAP_ANIM,
			(FRAME)PUMPUP_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)CONFUSE_BIG_MASK_PMAP_ANIM,
			(FRAME)CONFUSE_MED_MASK_PMAP_ANIM,
			(FRAME)CONFUSE_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)MELNORME_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)MELNORME_VICTORY_SONG,
		(SOUND)MELNORME_SHIP_SOUNDS,
	},
	{
		0,
		PUMPUP_SPEED * PUMPUP_LIFE,
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};

#define NUM_PUMP_ANIMS 5
#define REVERSE_DIR (BYTE)(1 << 7)

static void
pump_up_preprocess (PELEMENT ElementPtr)
{
	if (--ElementPtr->thrust_wait & 1)
	{
		COUNT frame_index;

		frame_index = GetFrameIndex (ElementPtr->current.image.frame);
		if (((ElementPtr->turn_wait & REVERSE_DIR)
				&& (frame_index % NUM_PUMP_ANIMS) != 0)
				|| (!(ElementPtr->turn_wait & REVERSE_DIR)
				&& ((frame_index + 1) % NUM_PUMP_ANIMS) == 0))
		{
			--frame_index;
			ElementPtr->turn_wait |= REVERSE_DIR;
		}
		else
		{
			++frame_index;
			ElementPtr->turn_wait &= ~REVERSE_DIR;
		}

		ElementPtr->next.image.frame = SetAbsFrameIndex (
				ElementPtr->current.image.frame, frame_index);

		ElementPtr->state_flags |= CHANGING;
	}
}

static COUNT initialize_pump_up (PELEMENT ShipPtr, HELEMENT
		PumpUpArray[]);

#define MELNORME_OFFSET 24
#define LEVEL_COUNTER 72
#define MAX_PUMP 4
#define PUMPUP_DAMAGE 2

static void
pump_up_postprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->state_flags & APPEARING)
	{
		ZeroVelocityComponents (&ElementPtr->velocity);
	}
	else
	{
		HELEMENT hPumpUp;
		ELEMENTPTR EPtr, ShipPtr;
		STARSHIPPTR StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		LockElement (StarShipPtr->hShip, &ShipPtr);
		initialize_pump_up (ShipPtr, &hPumpUp);
		DeltaEnergy (ShipPtr, 0);
		UnlockElement (StarShipPtr->hShip);

		LockElement (hPumpUp, &EPtr);

		EPtr->current.image.frame = ElementPtr->current.image.frame;
		EPtr->turn_wait = ElementPtr->turn_wait;
		EPtr->thrust_wait = ElementPtr->thrust_wait;
		if (--EPtr->thrust_wait == 0)
		{
			if ((EPtr->turn_wait & ~REVERSE_DIR) < MAX_PUMP - 1)
			{
				++EPtr->turn_wait;
				EPtr->current.image.frame = SetRelFrameIndex (
						EPtr->current.image.frame, NUM_PUMP_ANIMS);
				ProcessSound (SetAbsSoundIndex (
						StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2),
						EPtr);
			}
			EPtr->thrust_wait = LEVEL_COUNTER;
		}

		EPtr->mass_points = EPtr->hit_points =
				(PUMPUP_DAMAGE << (ElementPtr->turn_wait & ~REVERSE_DIR));
		SetElementStarShip (EPtr, StarShipPtr);

		if (EPtr->thrust_wait & 1)
		{
			COUNT frame_index;

			frame_index = GetFrameIndex (EPtr->current.image.frame);
			if (((EPtr->turn_wait & REVERSE_DIR)
					&& (frame_index % NUM_PUMP_ANIMS) != 0)
					|| (!(EPtr->turn_wait & REVERSE_DIR)
					&& ((frame_index + 1) % NUM_PUMP_ANIMS) == 0))
			{
				--frame_index;
				EPtr->turn_wait |= REVERSE_DIR;
			}
			else
			{
				++frame_index;
				EPtr->turn_wait &= ~REVERSE_DIR;
			}

			EPtr->current.image.frame = SetAbsFrameIndex (
					EPtr->current.image.frame, frame_index);
		}

		if (StarShipPtr->cur_status_flags & StarShipPtr->old_status_flags
				& WEAPON)
		{
			StarShipPtr->weapon_counter = WEAPON_WAIT;
		}
		else
		{
			COUNT angle;

			EPtr->life_span = PUMPUP_LIFE;
			EPtr->preprocess_func = pump_up_preprocess;
			EPtr->postprocess_func = 0;

			angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
			SetVelocityComponents (&EPtr->velocity,
					COSINE (angle, WORLD_TO_VELOCITY (PUMPUP_SPEED)),
					SINE (angle, WORLD_TO_VELOCITY (PUMPUP_SPEED)));

			ProcessSound (SetAbsSoundIndex (
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 3), EPtr);
		}

		UnlockElement (hPumpUp);
		PutElement (hPumpUp);

		SetPrimType (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],
				NO_PRIM);
		ElementPtr->state_flags |= NONSOLID;
	}
}

static void
animate (PELEMENT ElementPtr)
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
pump_up_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	RECT r;
	BYTE old_thrust_wait;
	HELEMENT hBlastElement;

	GetFrameRect (ElementPtr0->next.image.frame, &r);

	old_thrust_wait = ElementPtr0->thrust_wait;
	ElementPtr0->blast_offset = r.extent.width >> 1;
	hBlastElement = weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
	ElementPtr0->thrust_wait = old_thrust_wait;

	if (hBlastElement)
	{
		ELEMENTPTR BlastElementPtr;

#define MIN_PUMPITUDE_ANIMS 3
		LockElement (hBlastElement, &BlastElementPtr);

		BlastElementPtr->life_span =
				MIN_PUMPITUDE_ANIMS
				+ (ElementPtr0->turn_wait & ~REVERSE_DIR);
		BlastElementPtr->turn_wait = BlastElementPtr->next_turn = 0;
		{
			BlastElementPtr->preprocess_func = animate;
		}

		BlastElementPtr->current.image.farray = ElementPtr0->next.image.farray;
		BlastElementPtr->current.image.frame =
				SetAbsFrameIndex (BlastElementPtr->current.image.farray[0],
				MAX_PUMP * NUM_PUMP_ANIMS);

		UnlockElement (hBlastElement);
	}
}

static COUNT
initialize_pump_up (PELEMENT ShipPtr, HELEMENT PumpUpArray[])
{
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
	MissileBlock.pixoffs = MELNORME_OFFSET;
	MissileBlock.speed = DISPLAY_TO_WORLD (MELNORME_OFFSET);
	MissileBlock.hit_points = PUMPUP_DAMAGE;
	MissileBlock.damage = PUMPUP_DAMAGE;
	MissileBlock.life = 2;
	MissileBlock.preprocess_func = 0;
	MissileBlock.blast_offs = 0;
	PumpUpArray[0] = initialize_missile (&MissileBlock);

	if (PumpUpArray[0])
	{
		ELEMENTPTR PumpUpPtr;

		LockElement (PumpUpArray[0], &PumpUpPtr);
		PumpUpPtr->postprocess_func = pump_up_postprocess;
		PumpUpPtr->collision_func = pump_up_collision;
		PumpUpPtr->thrust_wait = LEVEL_COUNTER;
		UnlockElement (PumpUpArray[0]);
	}

	return (1);
}

static void
confuse_preprocess (PELEMENT ElementPtr)
{
	if (!(ElementPtr->state_flags & NONSOLID))
	{
		ElementPtr->next.image.frame = SetAbsFrameIndex (
				ElementPtr->current.image.frame,
				(GetFrameIndex (ElementPtr->current.image.frame) + 1) & 7);
		ElementPtr->state_flags |= CHANGING;
	}
	else if (ElementPtr->hTarget == 0)
	{
		ElementPtr->life_span = 0;
		ElementPtr->state_flags |= DISAPPEARING;
	}
	else
	{
		ELEMENTPTR eptr;

		LockElement (ElementPtr->hTarget, &eptr);

		ElementPtr->next.location = eptr->next.location;

		if (ElementPtr->turn_wait)
		{
			HELEMENT hEffect;
			STARSHIPPTR StarShipPtr;

			if (GetFrameIndex (ElementPtr->next.image.frame =
					IncFrameIndex (ElementPtr->current.image.frame)) == 0)
				ElementPtr->next.image.frame =
						SetRelFrameIndex (ElementPtr->next.image.frame, -8);

			GetElementStarShip (eptr, &StarShipPtr);
			StarShipPtr->ship_input_state =
					(StarShipPtr->ship_input_state
					& ~(LEFT | RIGHT | SPECIAL))
					| ElementPtr->turn_wait;

			hEffect = AllocElement ();
			if (hEffect)
			{
				LockElement (hEffect, &eptr);

				eptr->state_flags = FINITE_LIFE | NONSOLID | CHANGING
						| (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY));
				eptr->life_span = 1;
				eptr->current = eptr->next = ElementPtr->next;
				eptr->preprocess_func = confuse_preprocess;
				SetPrimType (&(GLOBAL (DisplayArray))[eptr->PrimIndex],
						STAMP_PRIM);

				GetElementStarShip (ElementPtr, &StarShipPtr);
				SetElementStarShip (eptr, StarShipPtr);
				eptr->hTarget = ElementPtr->hTarget;

				UnlockElement (hEffect);
				PutElement (hEffect);
			}
		}

		UnlockElement (ElementPtr->hTarget);
	}
}

static void
confusion_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	if (ElementPtr1->state_flags & PLAYER_SHIP)
	{
		HELEMENT hConfusionElement, hNextElement;
		ELEMENTPTR ConfusionPtr;
		STARSHIPPTR StarShipPtr;

		GetElementStarShip (ElementPtr0, &StarShipPtr);
		for (hConfusionElement = GetHeadElement ();
				hConfusionElement; hConfusionElement = hNextElement)
		{
			LockElement (hConfusionElement, &ConfusionPtr);
			if ((ConfusionPtr->state_flags & (GOOD_GUY | BAD_GUY)) ==
					(ElementPtr0->state_flags & (GOOD_GUY | BAD_GUY))
					&& ConfusionPtr->current.image.farray ==
					StarShipPtr->RaceDescPtr->ship_data.special
					&& (ConfusionPtr->state_flags & NONSOLID))
			{
				UnlockElement (hConfusionElement);
				break;
			}
			hNextElement = GetSuccElement (ConfusionPtr);
			UnlockElement (hConfusionElement);
		}

		if (hConfusionElement || (hConfusionElement = AllocElement ()))
		{
			LockElement (hConfusionElement, &ConfusionPtr);

			if (ConfusionPtr->state_flags == 0) /* not allocated before */
			{
				InsertElement (hConfusionElement, GetHeadElement ());

				ConfusionPtr->current = ElementPtr0->next;
				ConfusionPtr->current.image.frame = SetAbsFrameIndex (
						ConfusionPtr->current.image.frame, 8
						);
				ConfusionPtr->next = ConfusionPtr->current;
				ConfusionPtr->state_flags = FINITE_LIFE | NONSOLID | CHANGING
						| (ElementPtr0->state_flags & (GOOD_GUY | BAD_GUY));
				ConfusionPtr->preprocess_func = confuse_preprocess;
				SetPrimType (
						&(GLOBAL (DisplayArray))[ConfusionPtr->PrimIndex],
						NO_PRIM
						);

				SetElementStarShip (ConfusionPtr, StarShipPtr);
				GetElementStarShip (ElementPtr1, &StarShipPtr);
				ConfusionPtr->hTarget = StarShipPtr->hShip;
			}

			ConfusionPtr->life_span = 400;
			ConfusionPtr->turn_wait =
					(BYTE)(1 << ((BYTE)TFB_Random () & 1)); /* LEFT or RIGHT */

			UnlockElement (hConfusionElement);
		}

		ElementPtr0->hit_points = 0;
		ElementPtr0->life_span = 0;
		ElementPtr0->state_flags |= DISAPPEARING | COLLISION | NONSOLID;
	}
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static COUNT
initialize_confusion (PELEMENT ShipPtr, HELEMENT ConfusionArray[])
{
#define CMISSILE_SPEED DISPLAY_TO_WORLD (30)
#define CMISSILE_HITS 200
#define CMISSILE_DAMAGE 0
#define CMISSILE_LIFE 20
#define CMISSILE_OFFSET 4
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK ConfusionBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	ConfusionBlock.cx = ShipPtr->next.location.x;
	ConfusionBlock.cy = ShipPtr->next.location.y;
	ConfusionBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	ConfusionBlock.index = 0;
	ConfusionBlock.face = StarShipPtr->ShipFacing;
	ConfusionBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	ConfusionBlock.pixoffs = MELNORME_OFFSET;
	ConfusionBlock.speed = CMISSILE_SPEED;
	ConfusionBlock.hit_points = CMISSILE_HITS;
	ConfusionBlock.damage = CMISSILE_DAMAGE;
	ConfusionBlock.life = CMISSILE_LIFE;
	ConfusionBlock.preprocess_func = confuse_preprocess;
	ConfusionBlock.blast_offs = CMISSILE_OFFSET;
	ConfusionArray[0] = initialize_missile (&ConfusionBlock);

	if (ConfusionArray[0])
	{
		ELEMENTPTR CMissilePtr;

		LockElement (ConfusionArray[0], &CMissilePtr);
		CMissilePtr->collision_func = confusion_collision;
		SetElementStarShip (CMissilePtr, StarShipPtr);
		UnlockElement (ConfusionArray[0]);
	}
	return (1);
}

static COUNT
initialize_test_pump_up (PELEMENT ShipPtr, HELEMENT PumpUpArray[])
{
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	//ELEMENTPTR PumpUpPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = 0;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = MELNORME_OFFSET;
	MissileBlock.speed = PUMPUP_SPEED;
	MissileBlock.hit_points = PUMPUP_DAMAGE;
	MissileBlock.damage = PUMPUP_DAMAGE;
	MissileBlock.life = PUMPUP_LIFE;
	MissileBlock.preprocess_func = 0;
	MissileBlock.blast_offs = 0;
	PumpUpArray[0] = initialize_missile (&MissileBlock);

	return (1);
}

static void
melnorme_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern, COUNT ConcernCounter)
{
	BYTE old_count;
	STARSHIPPTR StarShipPtr;
	PEVALUATE_DESC lpEvalDesc;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	StarShipPtr->RaceDescPtr->init_weapon_func = initialize_test_pump_up;
	old_count = StarShipPtr->weapon_counter;

	if (StarShipPtr->weapon_counter == WEAPON_WAIT)
		StarShipPtr->weapon_counter = 0;

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		if (StarShipPtr->RaceDescPtr->ship_info.energy_level < SPECIAL_ENERGY_COST
				+ WEAPON_ENERGY_COST
				&& !(StarShipPtr->old_status_flags & WEAPON))
			lpEvalDesc->MoveState = ENTICE;
		else
		{
			STARSHIPPTR EnemyStarShipPtr;

			GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
			if (!(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags
					& IMMEDIATE_WEAPON))
				lpEvalDesc->MoveState = PURSUE;
		}
	}
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	if (StarShipPtr->weapon_counter == 0
			&& (old_count != 0
			|| ((StarShipPtr->special_counter
			|| StarShipPtr->RaceDescPtr->ship_info.energy_level >= SPECIAL_ENERGY_COST
			+ WEAPON_ENERGY_COST)
			&& !(StarShipPtr->ship_input_state & WEAPON))))
		StarShipPtr->ship_input_state ^= WEAPON;

	StarShipPtr->ship_input_state &= ~SPECIAL;
	if (StarShipPtr->special_counter == 0
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= SPECIAL_ENERGY_COST)
	{
		BYTE old_input_state;

		old_input_state = StarShipPtr->ship_input_state;

		StarShipPtr->RaceDescPtr->init_weapon_func = initialize_confusion;

		++ShipPtr->turn_wait;
		++ShipPtr->thrust_wait;
		ship_intelligence (ShipPtr, ObjectsOfConcern, ENEMY_SHIP_INDEX + 1);
		--ShipPtr->thrust_wait;
		--ShipPtr->turn_wait;

		if (StarShipPtr->ship_input_state & WEAPON)
		{
			StarShipPtr->ship_input_state &= ~WEAPON;
			StarShipPtr->ship_input_state |= SPECIAL;
		}

		StarShipPtr->ship_input_state = (unsigned char)(old_input_state
				| (StarShipPtr->ship_input_state & SPECIAL));
	}

	StarShipPtr->weapon_counter = old_count;

	StarShipPtr->RaceDescPtr->init_weapon_func = initialize_pump_up;
}

static void
melnorme_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		HELEMENT Confusion;

		initialize_confusion (ElementPtr, &Confusion);
		if (Confusion)
		{
			ELEMENTPTR CMissilePtr;
			LockElement (Confusion, &CMissilePtr);
			
			ProcessSound (SetAbsSoundIndex (
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), CMissilePtr);
			
			UnlockElement (Confusion);
			PutElement (Confusion);
			StarShipPtr->special_counter =
					StarShipPtr->RaceDescPtr->characteristics.special_wait;
		}
	}
}

RACE_DESCPTR
init_melnorme (void)
{
	RACE_DESCPTR RaceDescPtr;

	melnorme_desc.postprocess_func = melnorme_postprocess;
	melnorme_desc.init_weapon_func = initialize_pump_up;
	melnorme_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) melnorme_intelligence;

	RaceDescPtr = &melnorme_desc;

	return (RaceDescPtr);
}

