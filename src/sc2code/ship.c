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

void
animation_preprocess (PELEMENT ElementPtr)
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

UWORD
inertial_thrust (ELEMENTPTR ElementPtr)
{
	register COUNT CurrentAngle, TravelAngle;
	COUNT max_thrust, thrust_increment;
	register VELOCITYPTR VelocityPtr;
	STARSHIPPTR StarShipPtr;

	VelocityPtr = &ElementPtr->velocity;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	CurrentAngle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
	thrust_increment = StarShipPtr->RaceDescPtr->characteristics.thrust_increment;
	max_thrust = StarShipPtr->RaceDescPtr->characteristics.max_thrust;
	if (thrust_increment == max_thrust)
	{
		SetVelocityVector (VelocityPtr,
				max_thrust, StarShipPtr->ShipFacing);
		return (SHIP_AT_MAX_SPEED);
	}
	else if ((TravelAngle =
			GetVelocityTravelAngle (VelocityPtr)) == CurrentAngle
			&& (StarShipPtr->cur_status_flags
			& (SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED))
			&& !(StarShipPtr->cur_status_flags & SHIP_IN_GRAVITY_WELL))
		return (StarShipPtr->cur_status_flags
				& (SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED));
	else
	{
		register SIZE delta_x, delta_y;
		SIZE cur_delta_x, cur_delta_y;
		DWORD desired_speed, max_speed;

		thrust_increment = WORLD_TO_VELOCITY (thrust_increment);
		GetCurrentVelocityComponents (VelocityPtr, &cur_delta_x, &cur_delta_y);
		delta_x = cur_delta_x
				+ COSINE (CurrentAngle, thrust_increment);
		delta_y = cur_delta_y
				+ SINE (CurrentAngle, thrust_increment);
		desired_speed = (DWORD)((long)delta_x * delta_x)
				+ (DWORD)((long)delta_y * delta_y);
		max_speed = (DWORD)WORLD_TO_VELOCITY (max_thrust)
				* WORLD_TO_VELOCITY (max_thrust);
		if (desired_speed <= max_speed)
			SetVelocityComponents (VelocityPtr, delta_x, delta_y);
		else
		{
#define MAX_ALLOWED_SPEED WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (18))
			DWORD current_speed;

			if (((StarShipPtr->cur_status_flags & SHIP_IN_GRAVITY_WELL)
					&& desired_speed <=
					(DWORD)MAX_ALLOWED_SPEED * (DWORD)MAX_ALLOWED_SPEED)
					|| (current_speed =
					(DWORD)((long)cur_delta_x * (long)cur_delta_x)
					+ (DWORD)((long)cur_delta_y * (long)cur_delta_y)) > desired_speed)
			{
				SetVelocityComponents (VelocityPtr, delta_x, delta_y);
				return (SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);
			}
			else if (TravelAngle == CurrentAngle)
			{
				if (current_speed <= max_speed)
					SetVelocityVector (VelocityPtr,
							max_thrust, StarShipPtr->ShipFacing);
				return (SHIP_AT_MAX_SPEED);
			}
			else
			{
				VELOCITY_DESC v;

				v = *VelocityPtr;

				DeltaVelocityComponents (&v,
						COSINE (CurrentAngle, thrust_increment >> 1)
						- COSINE (TravelAngle, thrust_increment),
						SINE (CurrentAngle, thrust_increment >> 1)
						- SINE (TravelAngle, thrust_increment));
				GetCurrentVelocityComponents (&v, &cur_delta_x, &cur_delta_y);
				desired_speed = (long)cur_delta_x * (long)cur_delta_x
						+ (long)cur_delta_y * (long)cur_delta_y;
				if (desired_speed > max_speed)
				{
					if (desired_speed < current_speed)
						*VelocityPtr = v;
					return (SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);
				}

				*VelocityPtr = v;
			}
		}

		return (0);
	}
}

void
ship_preprocess (register PELEMENT ElementPtr)
{
	register ELEMENT_FLAGS cur_status_flags;
	STARSHIPPTR StarShipPtr;
	RACE_DESCPTR RDPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	RDPtr = StarShipPtr->RaceDescPtr;

	cur_status_flags =
			StarShipPtr->cur_status_flags
			& ~(LEFT | RIGHT | THRUST | WEAPON | SPECIAL);
	if (!(ElementPtr->state_flags & APPEARING))
		cur_status_flags |= StarShipPtr->ship_input_state
				& (LEFT | RIGHT | THRUST | WEAPON | SPECIAL);
	else
	{
		ElementPtr->crew_level =
				(BYTE)RDPtr->ship_info.crew_level;

		if ((ElementPtr->state_flags & BAD_GUY)
				&& LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
		{
			STAMP s;
			CONTEXT OldContext;

			OldContext = SetContext (StatusContext);
			s.origin.x = s.origin.y = 0;
			s.frame = RDPtr->ship_data.captain_control.background;
			DrawStamp (&s);
			DestroyDrawable (ReleaseDrawable (s.frame));
			RDPtr->ship_data.captain_control.background = 0;
			SetContext (OldContext);
		}
		else if (LOBYTE (GLOBAL (CurrentActivity)) <= IN_ENCOUNTER)
		{
			CONTEXT OldContext;

			InitShipStatus (StarShipPtr, NULL_PTR);
			OldContext = SetContext (StatusContext);
			DrawCaptainsWindow (StarShipPtr);
			SetContext (OldContext);
			if (RDPtr->preprocess_func)
				(*RDPtr->preprocess_func) (ElementPtr);

			if (ElementPtr->hTarget == 0)
			{
				extern void ship_transition (PELEMENT ElementPtr);

				ship_transition (ElementPtr);
			}
			else
			{
				extern BOOLEAN OpponentAlive (STARSHIPPTR
						TestStarShipPtr);

				ElementPtr->hTarget = 0;
				if (!PLRPlaying ((MUSIC_REF)~0) && OpponentAlive (StarShipPtr))
					BattleSong (TRUE);
			}
			return;
		}
		else
		{
			ElementPtr->current.location.x = LOG_SPACE_WIDTH >> 1;
			ElementPtr->current.location.y = LOG_SPACE_HEIGHT >> 1;
			ElementPtr->next.location = ElementPtr->current.location;
			InitIntersectStartPoint (ElementPtr);
			InitIntersectEndPoint (ElementPtr);

			ClearSemaphore (GraphicsSem);
			ResumeGameClock ();
			SetGameClockRate (HYPERSPACE_CLOCK_RATE);
			SetSemaphore (GraphicsSem);

			if (hyper_transition (ElementPtr))
				return;
		}
	}
	StarShipPtr->cur_status_flags = cur_status_flags;

	if (StarShipPtr->energy_counter)
		--StarShipPtr->energy_counter;
	else if (RDPtr->ship_info.energy_level < (BYTE)RDPtr->ship_info.max_energy
			|| (SBYTE)RDPtr->characteristics.energy_regeneration < 0)
		DeltaEnergy ((ELEMENTPTR)ElementPtr,
				(SBYTE)RDPtr->characteristics.energy_regeneration);

	if (RDPtr->preprocess_func)
	{
		(*RDPtr->preprocess_func) (ElementPtr);
		cur_status_flags = StarShipPtr->cur_status_flags;
	}

	if (ElementPtr->turn_wait)
		--ElementPtr->turn_wait;
	else if (cur_status_flags & (LEFT | RIGHT))
	{
		if (cur_status_flags & LEFT)
			StarShipPtr->ShipFacing =
					NORMALIZE_FACING (StarShipPtr->ShipFacing - 1);
		else
			StarShipPtr->ShipFacing =
					NORMALIZE_FACING (StarShipPtr->ShipFacing + 1);
		ElementPtr->next.image.frame =
				SetAbsFrameIndex (ElementPtr->next.image.frame,
				StarShipPtr->ShipFacing);
		ElementPtr->state_flags |= CHANGING;

		ElementPtr->turn_wait = RDPtr->characteristics.turn_wait;
	}

	if (ElementPtr->thrust_wait)
		--ElementPtr->thrust_wait;
	else if (cur_status_flags & THRUST)
	{
		UWORD thrust_status;

		thrust_status = inertial_thrust ((ELEMENTPTR)ElementPtr);
		StarShipPtr->cur_status_flags &=
				~(SHIP_AT_MAX_SPEED
				| SHIP_BEYOND_MAX_SPEED
				| SHIP_IN_GRAVITY_WELL);
		StarShipPtr->cur_status_flags |= thrust_status;

		ElementPtr->thrust_wait = RDPtr->characteristics.thrust_wait;

		if (!OBJECT_CLOAKED (ElementPtr)
				&& LOBYTE (GLOBAL (CurrentActivity)) <= IN_ENCOUNTER)
		{
			extern void spawn_ion_trail
					(PELEMENT
					ElementPtr);

			spawn_ion_trail (ElementPtr);
		}
	}

	if (LOBYTE (GLOBAL (CurrentActivity)) <= IN_ENCOUNTER)
		PreProcessStatus (ElementPtr);
}

void
ship_postprocess (register
		PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;
	RACE_DESCPTR RDPtr;

	if (ElementPtr->crew_level == 0)
		return;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	RDPtr = StarShipPtr->RaceDescPtr;

	if (StarShipPtr->weapon_counter)
		--StarShipPtr->weapon_counter;
	else if ((StarShipPtr->cur_status_flags
			& WEAPON) && DeltaEnergy ((ELEMENTPTR)ElementPtr,
			-RDPtr->characteristics.weapon_energy_cost))
	{
		COUNT num_weapons;
		HELEMENT Weapon[6];

		num_weapons = (*RDPtr->init_weapon_func) (ElementPtr, Weapon);

		if (num_weapons)
		{
			HELEMENT *WeaponPtr;
			STARSHIPPTR StarShipPtr;

			GetElementStarShip (ElementPtr, &StarShipPtr);
			WeaponPtr = &Weapon[0];
			do
			{
				HELEMENT w;

				w = *WeaponPtr++;
				if (w)
				{
					ELEMENTPTR EPtr;

					EPtr = LockElement (w, &EPtr);
					SetElementStarShip (EPtr, StarShipPtr);
					UnlockElement (w);

					PutElement (w);
				}
			} while (--num_weapons);
			ProcessSound (RDPtr->ship_data.ship_sounds);
		}

		StarShipPtr->weapon_counter =
				RDPtr->characteristics.weapon_wait;
	}

	if (StarShipPtr->special_counter)
		--StarShipPtr->special_counter;

	if (RDPtr->postprocess_func)
		(*RDPtr->postprocess_func) (ElementPtr);

	if (LOBYTE (GLOBAL (CurrentActivity)) <= IN_ENCOUNTER)
		PostProcessStatus (ElementPtr);
}

void
collision (PELEMENT
		ElementPtr0, PPOINT
		pPt0, PELEMENT
		ElementPtr1, PPOINT
		pPt1)
{
	if (!(ElementPtr1->state_flags & FINITE_LIFE))
	{
		ElementPtr0->state_flags |= COLLISION;
		if (GRAVITY_MASS (ElementPtr1->mass_points))
		{
			SIZE damage;

			if ((damage = ElementPtr0->hit_points >> 2) == 0)
				damage = 1;
			do_damage ((ELEMENTPTR)ElementPtr0, damage);

			damage = TARGET_DAMAGED_FOR_1_PT + (damage >> 1);
			if (damage > TARGET_DAMAGED_FOR_6_PLUS_PT)
				damage = TARGET_DAMAGED_FOR_6_PLUS_PT;
			ProcessSound (SetAbsSoundIndex (GameSounds, damage));
		}
	}
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static BOOLEAN
spawn_ship (STARSHIPPTR
		StarShipPtr)
{
	HELEMENT hShip;
	RACE_DESCPTR RDPtr;

	if (!load_ship (StarShipPtr, TRUE))
		return (FALSE);

	RDPtr = StarShipPtr->RaceDescPtr;
	RDPtr->ship_info.var2 = (BYTE)StarShipPtr->ShipFacing;

	StarShipPtr->ship_input_state = 0;
	StarShipPtr->cur_status_flags =
			StarShipPtr->old_status_flags = 0;

	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_ENCOUNTER
			|| LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
	{
		if ((RDPtr->ship_info.crew_level = StarShipPtr->special_counter) >
				RDPtr->ship_info.max_crew)
			RDPtr->ship_info.crew_level = RDPtr->ship_info.max_crew;
	}

	StarShipPtr->energy_counter =
			StarShipPtr->weapon_counter =
			StarShipPtr->special_counter = 0;

	if ((hShip = StarShipPtr->hShip) == 0
			&& (hShip = AllocElement ()) != 0)
		InsertElement (hShip, GetHeadElement ());

	if ((StarShipPtr->hShip = hShip) != 0)
	{
		ELEMENTPTR ShipElementPtr;

		LockElement (hShip, &ShipElementPtr);

		ShipElementPtr->crew_level = 0;
		ShipElementPtr->mass_points = RDPtr->characteristics.ship_mass;
		ShipElementPtr->state_flags = APPEARING | PLAYER_SHIP | IGNORE_SIMILAR
				| (RDPtr->ship_info.ship_flags & (GOOD_GUY | BAD_GUY));
		ShipElementPtr->turn_wait = ShipElementPtr->thrust_wait = 0;
		ShipElementPtr->life_span = NORMAL_LIFE;

		SetPrimType (&DisplayArray[ShipElementPtr->PrimIndex], STAMP_PRIM);
		ShipElementPtr->current.image.farray = RDPtr->ship_data.ship;

		if ((ShipElementPtr->state_flags & BAD_GUY)
				&& LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
		{
			StarShipPtr->ShipFacing = 0;
			ShipElementPtr->current.image.frame =
					SetAbsFrameIndex (RDPtr->ship_data.ship[0],
					StarShipPtr->ShipFacing);
			ShipElementPtr->current.location.x = LOG_SPACE_WIDTH >> 1;
			ShipElementPtr->current.location.y = LOG_SPACE_HEIGHT >> 1;
			++ShipElementPtr->life_span;
		}
		else
		{
			StarShipPtr->ShipFacing = NORMALIZE_FACING (Random ());
			if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE)
			{
				COUNT facing;

				if ((facing = LOWORD (GLOBAL (ShipStamp.frame))) > 0)
					--facing;

				GLOBAL (ShipStamp.frame) = (FRAME)MAKE_DWORD (
						StarShipPtr->ShipFacing + 1, 0
						);
				StarShipPtr->ShipFacing = facing;
			}
			ShipElementPtr->current.image.frame =
					SetAbsFrameIndex (RDPtr->ship_data.ship[0],
					StarShipPtr->ShipFacing);
			do
			{
				ShipElementPtr->current.location.x =
						WRAP_X (DISPLAY_ALIGN_X (Random ()));
				ShipElementPtr->current.location.y =
						WRAP_Y (DISPLAY_ALIGN_Y (Random ()));
			} while (CalculateGravity (ShipElementPtr)
					|| TimeSpaceMatterConflict (ShipElementPtr));
		}

		ShipElementPtr->preprocess_func = ship_preprocess;
		ShipElementPtr->postprocess_func = ship_postprocess;
		ShipElementPtr->death_func = ship_death;
		ShipElementPtr->collision_func = collision;
		ZeroVelocityComponents (&ShipElementPtr->velocity);

		SetElementStarShip (ShipElementPtr, StarShipPtr);
		ShipElementPtr->hTarget = 0;

		UnlockElement (hShip);
	}

	return (hShip != 0);
}

BOOLEAN
GetNextStarShip (STARSHIPPTR
		LastStarShipPtr,
		COUNT which_side)
{
	HSTARSHIP hBattleShip;

	cur_player = which_side;
	CyborgDescPtr = 0;


	hBattleShip = GetEncounterStarShip (LastStarShipPtr, which_side);
	if (hBattleShip)
	{
		STARSHIPPTR StarShipPtr;

		StarShipPtr = LockStarShip (&race_q[which_side], hBattleShip);
		if (LastStarShipPtr)
		{
			if (StarShipPtr == LastStarShipPtr)
				LastStarShipPtr = 0;
			else
				StarShipPtr->hShip = LastStarShipPtr->hShip;
		}

		if (!spawn_ship (StarShipPtr))
		{
			UnlockStarShip (&race_q[which_side], hBattleShip);
			return (FALSE);
		}
		UnlockStarShip (&race_q[which_side], hBattleShip);
	}

	if (LastStarShipPtr)
		LastStarShipPtr->hShip = 0;

	return (hBattleShip != 0);
}

