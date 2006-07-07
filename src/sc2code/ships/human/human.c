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
#include "ships/human/resinst.h"

#include "colors.h"
#include "globdata.h"


#define MAX_CREW 18
#define MAX_ENERGY 18
#define ENERGY_REGENERATION 1
#define WEAPON_ENERGY_COST 9
#define SPECIAL_ENERGY_COST 4
#define ENERGY_WAIT 8
#define MAX_THRUST /* DISPLAY_TO_WORLD (6) */ 24
#define THRUST_INCREMENT /* DISPLAY_TO_WORLD (2) */ 3
#define TURN_WAIT 1
#define THRUST_WAIT 4
#define WEAPON_WAIT 10
#define SPECIAL_WAIT 9

#define SHIP_MASS 6
#define MISSILE_LIFE 60
#define MIN_MISSILE_SPEED DISPLAY_TO_WORLD (10)
#define MAX_MISSILE_SPEED DISPLAY_TO_WORLD (20)

static RACE_DESC human_desc =
{
	{
		FIRES_FORE | SEEKING_WEAPON | POINT_DEFENSE,
		11, /* Super Melee cost */
		0 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			1752, 1450,
		},
		(STRING)HUMAN_RACE_STRINGS,
		(FRAME)HUMAN_ICON_MASK_PMAP_ANIM,
		(FRAME)HUMAN_MICON_MASK_PMAP_ANIM,
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
			(FRAME)HUMAN_BIG_MASK_PMAP_ANIM,
			(FRAME)HUMAN_MED_MASK_PMAP_ANIM,
			(FRAME)HUMAN_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)SATURN_BIG_MASK_PMAP_ANIM,
			(FRAME)SATURN_MED_MASK_PMAP_ANIM,
			(FRAME)SATURN_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		{
			(FRAME)HUMAN_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)HUMAN_VICTORY_SONG,
		(SOUND)HUMAN_SHIP_SOUNDS,
	},
	{
		0,
		LONG_RANGE_WEAPON,
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};

#define MISSILE_SPEED (MAX_THRUST >= MIN_MISSILE_SPEED ? \
										MAX_THRUST : MIN_MISSILE_SPEED)
#define TRACK_WAIT 3

static void
nuke_preprocess (PELEMENT ElementPtr)
{
	COUNT facing;

	facing = GetFrameIndex (ElementPtr->next.image.frame);
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		if (TrackShip (ElementPtr, &facing) > 0)
		{
			ElementPtr->next.image.frame =
					SetAbsFrameIndex (ElementPtr->next.image.frame,
					facing);
			ElementPtr->state_flags |= CHANGING;
		}

		ElementPtr->turn_wait = TRACK_WAIT;
	}

	{
		SIZE speed;

#define THRUST_SCALE DISPLAY_TO_WORLD (1)
		if ((speed = MISSILE_SPEED +
				((MISSILE_LIFE - ElementPtr->life_span) *
				THRUST_SCALE)) > MAX_MISSILE_SPEED)
			speed = MAX_MISSILE_SPEED;
		SetVelocityVector (&ElementPtr->velocity,
				speed, facing);
	}
}

static void
spawn_point_defense (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (ElementPtr->state_flags & PLAYER_SHIP)
	{
		HELEMENT hDefense;

		hDefense = AllocElement ();
		if (hDefense)
		{
			ELEMENTPTR DefensePtr;

			LockElement (hDefense, &DefensePtr);
			DefensePtr->state_flags = APPEARING | NONSOLID | FINITE_LIFE |
					(ElementPtr->state_flags & (GOOD_GUY | BAD_GUY));
			{
				DefensePtr->death_func = spawn_point_defense;
			}
			GetElementStarShip (ElementPtr, &StarShipPtr);
			SetElementStarShip (DefensePtr, StarShipPtr);
			UnlockElement (hDefense);

			PutElement (hDefense);
		}
	}
	else
	{
		BOOLEAN PaidFor;
		HELEMENT hObject, hNextObject;
		ELEMENTPTR ShipPtr;

		PaidFor = FALSE;

		LockElement (StarShipPtr->hShip, &ShipPtr);
		for (hObject = GetTailElement (); hObject; hObject = hNextObject)
		{
			ELEMENTPTR ObjectPtr;

			LockElement (hObject, &ObjectPtr);
			hNextObject = GetPredElement (ObjectPtr);
			if (ObjectPtr != ShipPtr && CollidingElement (ObjectPtr) &&
					!OBJECT_CLOAKED (ObjectPtr))
			{
#define LASER_RANGE (UWORD)100
				SIZE delta_x, delta_y;

				delta_x = ObjectPtr->next.location.x -
						ShipPtr->next.location.x;
				delta_y = ObjectPtr->next.location.y -
						ShipPtr->next.location.y;
				if (delta_x < 0)
					delta_x = -delta_x;
				if (delta_y < 0)
					delta_y = -delta_y;
				delta_x = WORLD_TO_DISPLAY (delta_x);
				delta_y = WORLD_TO_DISPLAY (delta_y);
				if ((UWORD)delta_x <= LASER_RANGE &&
						(UWORD)delta_y <= LASER_RANGE &&
						(UWORD)delta_x * (UWORD)delta_x +
						(UWORD)delta_y * (UWORD)delta_y <=
						LASER_RANGE * LASER_RANGE)
				{
					HELEMENT hPointDefense;
					LASER_BLOCK LaserBlock;

					if (!PaidFor)
					{
						if (!DeltaEnergy (ShipPtr, -SPECIAL_ENERGY_COST))
							break;

						ProcessSound (SetAbsSoundIndex (
										/* POINT_DEFENSE_LASER */
								StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
						StarShipPtr->special_counter =
								StarShipPtr->RaceDescPtr->characteristics.special_wait;
						PaidFor = TRUE;
					}

					LaserBlock.cx = ShipPtr->next.location.x;
					LaserBlock.cy = ShipPtr->next.location.y;
					LaserBlock.face = 0;
					LaserBlock.ex = ObjectPtr->next.location.x
							- ShipPtr->next.location.x;
					LaserBlock.ey = ObjectPtr->next.location.y
							- ShipPtr->next.location.y;
					LaserBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
							| IGNORE_SIMILAR;
					LaserBlock.pixoffs = 0;
					LaserBlock.color = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F);
					hPointDefense = initialize_laser (&LaserBlock);
					if (hPointDefense)
					{
						ELEMENTPTR PDPtr;

						LockElement (hPointDefense, &PDPtr);
						SetElementStarShip (PDPtr, StarShipPtr);
						PDPtr->hTarget = 0;
						UnlockElement (hPointDefense);

						PutElement (hPointDefense);
					}
				}
			}
			UnlockElement (hObject);
		}
		UnlockElement (StarShipPtr->hShip);
	}
}

static COUNT
initialize_nuke (PELEMENT ShipPtr, HELEMENT NukeArray[])
{
#define HUMAN_OFFSET 42
#define MISSILE_DAMAGE 4
#define MISSILE_HITS 1
#define NUKE_OFFSET 8
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->state_flags & (GOOD_GUY | BAD_GUY);
	MissileBlock.pixoffs = HUMAN_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = nuke_preprocess;
	MissileBlock.blast_offs = NUKE_OFFSET;
	NukeArray[0] = initialize_missile (&MissileBlock);

	if (NukeArray[0])
	{
		ELEMENTPTR NukePtr;

		LockElement (NukeArray[0], &NukePtr);
		NukePtr->turn_wait = TRACK_WAIT;
		UnlockElement (NukeArray[0]);
	}

	return (1);
}

static void
human_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern, COUNT ConcernCounter)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter == 0
			&& ((ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr != NULL_PTR
			&& ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn <= 2)
			|| (ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr != NULL_PTR
			&& ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn <= 4)))
		StarShipPtr->ship_input_state |= SPECIAL;
	else
		StarShipPtr->ship_input_state &= ~SPECIAL;
	ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr = NULL_PTR;

	ship_intelligence (ShipPtr,
			ObjectsOfConcern, ConcernCounter);

	if (StarShipPtr->weapon_counter == 0)
	{
		if (ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr
				&& (!(StarShipPtr->ship_input_state & (LEFT | RIGHT /* | THRUST */))
				|| ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn <= 12))
			StarShipPtr->ship_input_state |= WEAPON;
	}
}

static void
human_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0)
	{
		spawn_point_defense (ElementPtr);
	}
}

RACE_DESCPTR
init_human (void)
{
	RACE_DESCPTR RaceDescPtr;

	human_desc.postprocess_func = human_postprocess;
	human_desc.init_weapon_func = initialize_nuke;
	human_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) human_intelligence;

	RaceDescPtr = &human_desc;

	return (RaceDescPtr);
}

