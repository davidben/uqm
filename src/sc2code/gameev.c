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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "gameev.h"

#include "build.h"
#include "clock.h"
#include "encount.h"
#include "globdata.h"
#include "hyper.h"
#include "libs/compiler.h"
#include "libs/mathlib.h"


static void arilou_entrance_event (void);
static void arilou_exit_event (void);
static void check_race_growth (void);
static void black_urquan_genocide (void);
static void pkunk_mission (void);
static void thradd_mission (void);
static void ilwrath_mission (void);
static void utwig_supox_mission (void);
static void mycon_mission (void);


void
AddInitialGameEvents (void) {
	AddEvent (RELATIVE_EVENT, 0, 1, 0, HYPERSPACE_ENCOUNTER_EVENT);
	AddEvent (ABSOLUTE_EVENT, 3, 17, START_YEAR, ARILOU_ENTRANCE_EVENT);
	AddEvent (RELATIVE_EVENT, 0, 0, YEARS_TO_KOHRAH_VICTORY,
			KOHR_AH_VICTORIOUS_EVENT);
	AddEvent (RELATIVE_EVENT, 0, 0, 0, SLYLANDRO_RAMP_UP);
}

void
EventHandler (BYTE selector)
{
	switch (selector)
	{
		case ARILOU_ENTRANCE_EVENT:
			arilou_entrance_event ();
			break;
		case ARILOU_EXIT_EVENT:
			arilou_exit_event ();
			break;
		case HYPERSPACE_ENCOUNTER_EVENT:
			check_race_growth ();
			if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE
					&& GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
				check_hyperspace_encounter ();

			AddEvent (RELATIVE_EVENT, 0, 1, 0, HYPERSPACE_ENCOUNTER_EVENT);
			break;
		case KOHR_AH_VICTORIOUS_EVENT:
			if (GET_GAME_STATE (UTWIG_SUPOX_MISSION))
			{
				AddEvent (RELATIVE_EVENT, 0, 0, 1, KOHR_AH_GENOCIDE_EVENT);
				break;
			}
			/* FALLTHROUGH */
		case KOHR_AH_GENOCIDE_EVENT:
			if (!GET_GAME_STATE (KOHR_AH_FRENZY)
					&& LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
			                && CurStarDescPtr
					&& CurStarDescPtr->Index == SAMATRA_DEFINED)
				AddEvent (RELATIVE_EVENT, 0, 7, 0, KOHR_AH_GENOCIDE_EVENT);
			else
				black_urquan_genocide ();
			break;
		case ADVANCE_PKUNK_MISSION:
			pkunk_mission ();
			break;
		case ADVANCE_THRADD_MISSION:
			thradd_mission ();
			break;
		case ZOQFOT_DISTRESS_EVENT:
			if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
			                && CurStarDescPtr
					&& CurStarDescPtr->Index == ZOQFOT_DEFINED)
				AddEvent (RELATIVE_EVENT, 0, 7, 0, ZOQFOT_DISTRESS_EVENT);
			else
			{
				SET_GAME_STATE (ZOQFOT_DISTRESS, 1);
				AddEvent (RELATIVE_EVENT, 6, 0, 0, ZOQFOT_DEATH_EVENT);
			}
			break;
		case ZOQFOT_DEATH_EVENT:
			if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
			                && CurStarDescPtr
					&& CurStarDescPtr->Index == ZOQFOT_DEFINED)
				AddEvent (RELATIVE_EVENT, 0, 7, 0, ZOQFOT_DEATH_EVENT);
			else if (GET_GAME_STATE (ZOQFOT_DISTRESS))
			{
				HSTARSHIP hZoqFot;
				EXTENDED_SHIP_FRAGMENTPTR ZoqFotPtr;

				hZoqFot = GetStarShipFromIndex (&GLOBAL (avail_race_q),
						ZOQFOTPIK_SHIP);
				ZoqFotPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
						&GLOBAL (avail_race_q), hZoqFot);
				ZoqFotPtr->ShipInfo.actual_strength = 0;
				ZoqFotPtr->ShipInfo.ship_flags &= ~(GOOD_GUY | BAD_GUY);
				UnlockStarShip ( &GLOBAL (avail_race_q), hZoqFot);

				SET_GAME_STATE (ZOQFOT_DISTRESS, 2);
			}
			break;
		case SHOFIXTI_RETURN_EVENT:
			ActivateStarShip (SHOFIXTI_SHIP, 0);
			GLOBAL (CrewCost) -= 2;
					/* crew is not an issue anymore */
			SET_GAME_STATE (CREW_PURCHASED0, 0);
			SET_GAME_STATE (CREW_PURCHASED1, 0);
			break;
		case ADVANCE_UTWIG_SUPOX_MISSION:
			utwig_supox_mission ();
			break;
		case SPATHI_SHIELD_EVENT:
			if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
			                && CurStarDescPtr
					&& CurStarDescPtr->Index == SPATHI_DEFINED)
				AddEvent (RELATIVE_EVENT, 0, 7, 0, SPATHI_SHIELD_EVENT);
			else
			{
				HSTARSHIP hSpathi;
				EXTENDED_SHIP_FRAGMENTPTR SpathiPtr;

				hSpathi = GetStarShipFromIndex (
						&GLOBAL (avail_race_q), SPATHI_SHIP);
				SpathiPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
						&GLOBAL (avail_race_q), hSpathi);

				if (SpathiPtr->ShipInfo.actual_strength)
				{
					ActivateStarShip (SPATHI_SHIP, -1);
					SET_GAME_STATE (SPATHI_SHIELDED_SELVES, 1);
					SpathiPtr->ShipInfo.actual_strength = 0;
				}

				UnlockStarShip (&GLOBAL (avail_race_q), hSpathi);
			}
			break;
		case ADVANCE_ILWRATH_MISSION:
			ilwrath_mission ();
			break;
		case ADVANCE_MYCON_MISSION:
			mycon_mission ();
			break;
		case ARILOU_UMGAH_CHECK:
			SET_GAME_STATE (ARILOU_CHECKED_UMGAH, 2);
			break;
		case YEHAT_REBEL_EVENT:
		{
			HSTARSHIP hRebel, hRoyalist;
			EXTENDED_SHIP_FRAGMENTPTR RebelPtr, RoyalistPtr;

			hRebel = GetStarShipFromIndex (&GLOBAL (avail_race_q),
					YEHAT_REBEL_SHIP);
			RebelPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
					&GLOBAL (avail_race_q), hRebel);
			hRoyalist = GetStarShipFromIndex (&GLOBAL (avail_race_q),
					YEHAT_SHIP);
			RoyalistPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
					&GLOBAL (avail_race_q), hRoyalist);
			RebelPtr->ShipInfo.actual_strength =
					RoyalistPtr->ShipInfo.actual_strength =
					RoyalistPtr->ShipInfo.actual_strength * 2 / 3;
			RebelPtr->ShipInfo.loc.x = 5150;
			RebelPtr->ShipInfo.loc.y = 0;
			UnlockStarShip (&GLOBAL (avail_race_q), hRoyalist);
			UnlockStarShip (&GLOBAL (avail_race_q), hRebel);
			ActivateStarShip (YEHAT_REBEL_SHIP, SPHERE_TRACKING);
			break;
		}
		case SLYLANDRO_RAMP_UP:
			if (!GET_GAME_STATE (DESTRUCT_CODE_ON_SHIP))
			{
				BYTE ramp_factor;

				ramp_factor = GET_GAME_STATE (SLYLANDRO_MULTIPLIER);
				if (++ramp_factor <= 4)
				{
					SET_GAME_STATE (SLYLANDRO_MULTIPLIER, ramp_factor);
					AddEvent (RELATIVE_EVENT, 0, 182, 0, SLYLANDRO_RAMP_UP);
				}
			}
			break;
		case SLYLANDRO_RAMP_DOWN:
		{
			BYTE ramp_factor;

			ramp_factor = GET_GAME_STATE (SLYLANDRO_MULTIPLIER);
			if (--ramp_factor)
				AddEvent (RELATIVE_EVENT, 0, 23, 0, SLYLANDRO_RAMP_DOWN);
			SET_GAME_STATE (SLYLANDRO_MULTIPLIER, ramp_factor);
			break;
		}
	}
}

void
SetRaceDest (BYTE which_race, COORD x, COORD y, BYTE days_left, BYTE
		func_index)
{
	HSTARSHIP hStarShip;
	EXTENDED_SHIP_FRAGMENTPTR TemplatePtr;

	hStarShip = GetStarShipFromIndex (&GLOBAL (avail_race_q), which_race);
	TemplatePtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (avail_race_q), hStarShip);

	TemplatePtr->ShipInfo.dest_loc.x = x;
	TemplatePtr->ShipInfo.dest_loc.y = y;
	TemplatePtr->ShipInfo.days_left = days_left;
	TemplatePtr->ShipInfo.func_index = func_index;

	UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);
}



static void
arilou_entrance_event (void)
{
	SET_GAME_STATE (ARILOU_SPACE, OPENING);
	AddEvent (RELATIVE_EVENT, 0, 3, 0, ARILOU_EXIT_EVENT);
}

static void
arilou_exit_event (void)
{
	COUNT month_index, year_index;

	year_index = GLOBAL (GameClock.year_index);
	if ((month_index = GLOBAL (GameClock.month_index) % 12) == 0)
		++year_index;
	++month_index;

	SET_GAME_STATE (ARILOU_SPACE, CLOSING);
	AddEvent (ABSOLUTE_EVENT,
			month_index, 17, year_index, ARILOU_ENTRANCE_EVENT);
}

static void
check_race_growth (void)
{
	HSTARSHIP hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip; hStarShip = hNextShip)
	{
		EXTENDED_SHIP_FRAGMENTPTR TemplatePtr;

		TemplatePtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (avail_race_q), hStarShip);
		hNextShip = _GetSuccLink (TemplatePtr);

		if (TemplatePtr->ShipInfo.actual_strength
				&& TemplatePtr->ShipInfo.actual_strength != (COUNT)~0)
		{
			SIZE delta_strength;

			delta_strength = (SBYTE)TemplatePtr->ShipInfo.energy_level;
			if (TemplatePtr->ShipInfo.growth_err_term <=
					TemplatePtr->ShipInfo.growth_fract)
			{
				if (delta_strength <= 0)
					--delta_strength;
				else
					++delta_strength;
			}
			TemplatePtr->ShipInfo.growth_err_term -=
					TemplatePtr->ShipInfo.growth_fract;

			delta_strength += TemplatePtr->ShipInfo.actual_strength;
			if (delta_strength <= 0)
			{
				delta_strength = 0;
				TemplatePtr->ShipInfo.ship_flags &= ~(GOOD_GUY | BAD_GUY);
			}
			else if (delta_strength > MAX_FLEET_STRENGTH)
				delta_strength = MAX_FLEET_STRENGTH;
				
			TemplatePtr->ShipInfo.actual_strength = (COUNT)delta_strength;
			if (TemplatePtr->ShipInfo.actual_strength &&
					TemplatePtr->ShipInfo.days_left)
			{
				TemplatePtr->ShipInfo.loc.x +=
						(TemplatePtr->ShipInfo.dest_loc.x
						- TemplatePtr->ShipInfo.loc.x)
						/ TemplatePtr->ShipInfo.days_left;
				TemplatePtr->ShipInfo.loc.y +=
						(TemplatePtr->ShipInfo.dest_loc.y
						- TemplatePtr->ShipInfo.loc.y)
						/ TemplatePtr->ShipInfo.days_left;

				if (--TemplatePtr->ShipInfo.days_left == 0
						&& TemplatePtr->ShipInfo.func_index != (BYTE) ~0)
					EventHandler (TemplatePtr->ShipInfo.func_index);
			}
		}

		UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);
	}
}

static void
black_urquan_genocide (void)
{
	BYTE Index;
	long best_dist;
	SIZE best_dx, best_dy;
	HSTARSHIP hStarShip, hNextShip;
	HSTARSHIP hBlackUrquan;
	EXTENDED_SHIP_FRAGMENTPTR BlackUrquanPtr;

	hBlackUrquan = GetStarShipFromIndex (
			&GLOBAL (avail_race_q), BLACK_URQUAN_SHIP);
	BlackUrquanPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (avail_race_q), hBlackUrquan);

	best_dist = -1;
	best_dx = SOL_X - BlackUrquanPtr->ShipInfo.loc.x;
	best_dy = SOL_Y - BlackUrquanPtr->ShipInfo.loc.y;
	for (Index = 0, hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip; ++Index, hStarShip = hNextShip)
	{
		EXTENDED_SHIP_FRAGMENTPTR TemplatePtr;

		TemplatePtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (avail_race_q), hStarShip);
		hNextShip = _GetSuccLink (TemplatePtr);

		if (Index != BLACK_URQUAN_SHIP
				&& Index != URQUAN_SHIP
				&& TemplatePtr->ShipInfo.actual_strength != (COUNT)~0)
		{
			SIZE dx, dy;

			dx = TemplatePtr->ShipInfo.loc.x - BlackUrquanPtr->ShipInfo.loc.x;
			dy = TemplatePtr->ShipInfo.loc.y - BlackUrquanPtr->ShipInfo.loc.y;
			if (dx == 0 && dy == 0)
			{
				// Arrived at the victim's home world. Cleanse it.
				TemplatePtr->ShipInfo.ship_flags &= ~(GOOD_GUY | BAD_GUY);
				TemplatePtr->ShipInfo.actual_strength = 0;
			}
			else if (TemplatePtr->ShipInfo.actual_strength)
			{
				long dist;

				dist = (long)dx * dx + (long)dy * dy;
				if (best_dist < 0 || dist < best_dist || Index == DRUUGE_SHIP)
				{
					best_dist = dist;
					best_dx = dx;
					best_dy = dy;

					if (Index == DRUUGE_SHIP)
						hNextShip = 0;
				}
			}
		}

		UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);
	}

	if (best_dist < 0 && best_dx == 0 && best_dy == 0)
	{
		// All spheres of influence are gone - game over.
		GLOBAL (CurrentActivity) &= ~IN_BATTLE;
		GLOBAL_SIS (CrewEnlisted) = (COUNT)~0;

		SET_GAME_STATE (KOHR_AH_KILLED_ALL, 1);
	}
	else
	{
		// Moving towards new race to cleanse.
		COUNT speed;

		if (best_dist < 0)
			best_dist = (long)best_dx * best_dx + (long)best_dy * best_dy;

		speed = square_root (best_dist) / 158;
		if (speed == 0)
			speed = 1;
		else if (speed > 255)
			speed = 255;

		SET_GAME_STATE (KOHR_AH_FRENZY, 1);
		SET_GAME_STATE (KOHR_AH_VISITS, 0);
		SET_GAME_STATE (KOHR_AH_REASONS, 0);
		SET_GAME_STATE (KOHR_AH_PLEAD, 0);
		SET_GAME_STATE (KOHR_AH_INFO, 0);
		SET_GAME_STATE (URQUAN_VISITS, 0);
		SetRaceDest (BLACK_URQUAN_SHIP,
				BlackUrquanPtr->ShipInfo.loc.x + best_dx,
				BlackUrquanPtr->ShipInfo.loc.y + best_dy,
				(BYTE)speed, KOHR_AH_GENOCIDE_EVENT);
	}

	UnlockStarShip (&GLOBAL (avail_race_q), hBlackUrquan);
}

static void
pkunk_mission (void)
{
	HSTARSHIP hPkunk;
	EXTENDED_SHIP_FRAGMENTPTR PkunkPtr;

	hPkunk = GetStarShipFromIndex (&GLOBAL (avail_race_q), PKUNK_SHIP);
	PkunkPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (avail_race_q), hPkunk);

	if (PkunkPtr->ShipInfo.actual_strength)
	{
		BYTE MissionState;

		MissionState = GET_GAME_STATE (PKUNK_MISSION);
		if (PkunkPtr->ShipInfo.days_left == 0 && MissionState)
		{
			if ((MissionState & 1)
							/* made it to Yehat space */
					|| (PkunkPtr->ShipInfo.loc.x == 4970
					&& PkunkPtr->ShipInfo.loc.y == 400))
				PkunkPtr->ShipInfo.actual_strength = 0;
			else if (PkunkPtr->ShipInfo.loc.x == 502
					&& PkunkPtr->ShipInfo.loc.y == 401
					&& GET_GAME_STATE (PKUNK_ON_THE_MOVE))
			{
				SET_GAME_STATE (PKUNK_ON_THE_MOVE, 0);
				AddEvent (RELATIVE_EVENT, 3, 0, 0, ADVANCE_PKUNK_MISSION);
				UnlockStarShip (&GLOBAL (avail_race_q), hPkunk);
				return;
			}
		}

		if (PkunkPtr->ShipInfo.actual_strength == 0)
		{
			SET_GAME_STATE (YEHAT_ABSORBED_PKUNK, 1);
			PkunkPtr->ShipInfo.ship_flags &= ~(GOOD_GUY | BAD_GUY);
			ActivateStarShip (YEHAT_SHIP, SPHERE_TRACKING);
		}
		else
		{
			COORD x, y;

			if (!(MissionState & 1))
			{
				x = 4970;
				y = 400;
			}
			else
			{
				x = 502;
				y = 401;
			}
			SET_GAME_STATE (PKUNK_ON_THE_MOVE, 1);
			SET_GAME_STATE (PKUNK_SWITCH, 0);
			SetRaceDest (PKUNK_SHIP, x, y,
					(BYTE)((365 >> 1) - PkunkPtr->ShipInfo.days_left),
					ADVANCE_PKUNK_MISSION);
		}
		SET_GAME_STATE (PKUNK_MISSION, MissionState + 1);
	}

	UnlockStarShip (&GLOBAL (avail_race_q), hPkunk);
}

static void
thradd_mission (void)
{
	BYTE MissionState;
	HSTARSHIP hThradd;
	EXTENDED_SHIP_FRAGMENTPTR ThraddPtr;

	hThradd = GetStarShipFromIndex (&GLOBAL (avail_race_q), THRADDASH_SHIP);
	ThraddPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (avail_race_q), hThradd);

	MissionState = GET_GAME_STATE (THRADD_MISSION);
	if (ThraddPtr->ShipInfo.actual_strength && MissionState < 3)
	{
		COORD x, y;

		if (MissionState < 2)
		{	/* attacking */
			x = 4879;
			y = 7201;
		}
		else
		{	/* returning */
			x = 2535;
			y = 8358;
		}

		if (MissionState == 1)
		{	/* arrived at Kohr-Ah, engaging */
			SIZE strength_loss;

			strength_loss = (SIZE)(ThraddPtr->ShipInfo.actual_strength >> 1);
			ThraddPtr->ShipInfo.energy_level =
					(BYTE)(-strength_loss / 14);
			ThraddPtr->ShipInfo.growth_fract =
					(BYTE)(((strength_loss % 14) << 8) / 14);
			ThraddPtr->ShipInfo.growth_err_term = 255 >> 1;
		}
		else
		{
			if (MissionState != 0)
			{	/* stop losses */
				ThraddPtr->ShipInfo.energy_level = 0;
				ThraddPtr->ShipInfo.growth_fract = 0;
			}
		}
		SetRaceDest (THRADDASH_SHIP, x, y, 14, ADVANCE_THRADD_MISSION);
	}
	++MissionState;
	SET_GAME_STATE (THRADD_MISSION, MissionState);

	if (MissionState == 4 && GET_GAME_STATE (ILWRATH_FIGHT_THRADDASH))
	{	/* returned home - notify the Ilwrath */
		AddEvent (RELATIVE_EVENT, 0, 0, 0, ADVANCE_ILWRATH_MISSION);
	}

	UnlockStarShip (&GLOBAL (avail_race_q), hThradd);
}

static void
ilwrath_mission (void)
{
	BYTE ThraddState;
	HSTARSHIP hIlwrath, hThradd;
	EXTENDED_SHIP_FRAGMENTPTR IlwrathPtr, ThraddPtr;

	hIlwrath = GetStarShipFromIndex (&GLOBAL (avail_race_q), ILWRATH_SHIP);
	IlwrathPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (avail_race_q), hIlwrath);
	hThradd = GetStarShipFromIndex (&GLOBAL (avail_race_q), THRADDASH_SHIP);
	ThraddPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (avail_race_q), hThradd);

	if (IlwrathPtr->ShipInfo.loc.x == ((2500 + 2535) >> 1)
			&& IlwrathPtr->ShipInfo.loc.y == ((8070 + 8358) >> 1))
	{
		IlwrathPtr->ShipInfo.actual_strength =
				ThraddPtr->ShipInfo.actual_strength = 0;
		IlwrathPtr->ShipInfo.ship_flags &= ~(GOOD_GUY | BAD_GUY);
		ThraddPtr->ShipInfo.ship_flags &= ~(GOOD_GUY | BAD_GUY);
	}
	else if (IlwrathPtr->ShipInfo.actual_strength)
	{
		if (!GET_GAME_STATE (ILWRATH_FIGHT_THRADDASH)
				&& (IlwrathPtr->ShipInfo.dest_loc.x != 2500
				|| IlwrathPtr->ShipInfo.dest_loc.y != 8070))
		{
			SetRaceDest (ILWRATH_SHIP, 2500, 8070, 90,
					ADVANCE_ILWRATH_MISSION);
		}
		else
		{
#define MADD_LENGTH 128
			SIZE strength_loss;

			if (IlwrathPtr->ShipInfo.days_left == 0)
			{	/* arrived for battle */
				SET_GAME_STATE (ILWRATH_FIGHT_THRADDASH, 1);
				SET_GAME_STATE (HELIX_UNPROTECTED, 1);
				strength_loss = (SIZE)IlwrathPtr->ShipInfo.actual_strength;
				IlwrathPtr->ShipInfo.energy_level =
						(BYTE)(-strength_loss / MADD_LENGTH);
				IlwrathPtr->ShipInfo.growth_fract =
						(BYTE)(((strength_loss % MADD_LENGTH) << 8) / MADD_LENGTH);
				SetRaceDest (ILWRATH_SHIP,
						(2500 + 2535) >> 1, (8070 + 8358) >> 1,
						MADD_LENGTH - 1, ADVANCE_ILWRATH_MISSION);

				strength_loss = (SIZE)ThraddPtr->ShipInfo.actual_strength;
				ThraddPtr->ShipInfo.energy_level =
						(BYTE)(-strength_loss / MADD_LENGTH);
				ThraddPtr->ShipInfo.growth_fract =
						(BYTE)(((strength_loss % MADD_LENGTH) << 8) / MADD_LENGTH);

				SET_GAME_STATE (THRADD_VISITS, 0);
				if (ThraddPtr->ShipInfo.ship_flags & GOOD_GUY)
					ActivateStarShip (THRADDASH_SHIP, -1);
			}

			ThraddState = GET_GAME_STATE (THRADD_MISSION);
			if (ThraddState == 0 || ThraddState > 3)
			{	/* never went to Kohr-Ah or returned */
				SetRaceDest (THRADDASH_SHIP,
						(2500 + 2535) >> 1, (8070 + 8358) >> 1,
						IlwrathPtr->ShipInfo.days_left + 1, (BYTE)~0);
			}
			else if (ThraddState < 3)
			{	/* recall on the double */
				SetRaceDest (THRADDASH_SHIP, 2535, 8358, 10,
						ADVANCE_THRADD_MISSION);
				SET_GAME_STATE (THRADD_MISSION, 3);
			}
		}
	}

	UnlockStarShip (&GLOBAL (avail_race_q), hThradd);
	UnlockStarShip (&GLOBAL (avail_race_q), hIlwrath);
}

static void
utwig_supox_mission (void)
{
	BYTE MissionState;
	HSTARSHIP hUtwig, hSupox;
	EXTENDED_SHIP_FRAGMENTPTR UtwigPtr, SupoxPtr;

	hUtwig = GetStarShipFromIndex (&GLOBAL (avail_race_q), UTWIG_SHIP);
	UtwigPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (avail_race_q), hUtwig);
	hSupox = GetStarShipFromIndex (&GLOBAL (avail_race_q), SUPOX_SHIP);
	SupoxPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (avail_race_q), hSupox);

	MissionState = GET_GAME_STATE (UTWIG_SUPOX_MISSION);
	if (UtwigPtr->ShipInfo.actual_strength && MissionState < 5)
	{
		if (MissionState == 1)
		{
			SIZE strength_loss;

			AddEvent (RELATIVE_EVENT, 0, (160 >> 1), 0,
					ADVANCE_UTWIG_SUPOX_MISSION);

			strength_loss = (SIZE)(UtwigPtr->ShipInfo.actual_strength >> 1);
			UtwigPtr->ShipInfo.energy_level =
					(BYTE)(-strength_loss / 160);
			UtwigPtr->ShipInfo.growth_fract =
					(BYTE)(((strength_loss % 160) << 8) / 160);
			UtwigPtr->ShipInfo.growth_err_term = 255 >> 1;

			strength_loss = (SIZE)(SupoxPtr->ShipInfo.actual_strength >> 1);
			if (strength_loss)
			{
				SupoxPtr->ShipInfo.energy_level =
						(BYTE)(-strength_loss / 160);
				SupoxPtr->ShipInfo.growth_fract =
						(BYTE)(((strength_loss % 160) << 8) / 160);
				SupoxPtr->ShipInfo.growth_err_term = 255 >> 1;
			}

			SET_GAME_STATE (UTWIG_WAR_NEWS, 0);
			SET_GAME_STATE (SUPOX_WAR_NEWS, 0);
		}
		else if (MissionState == 2)
		{
			AddEvent (RELATIVE_EVENT, 0, (160 >> 1), 0,
					ADVANCE_UTWIG_SUPOX_MISSION);
			++MissionState;
		}
		else
		{
			COORD ux, uy, sx, sy;

			if (MissionState == 0)
			{
				ux = 7208;
				uy = 7000;

				sx = 6479;
				sy = 7541;
			}
			else
			{
				ux = 8534;
				uy = 8797;

				sx = 7468;
				sy = 9246;

				UtwigPtr->ShipInfo.energy_level = 0;
				UtwigPtr->ShipInfo.growth_fract = 0;
				SupoxPtr->ShipInfo.energy_level = 0;
				SupoxPtr->ShipInfo.growth_fract = 0;

				SET_GAME_STATE (UTWIG_WAR_NEWS, 0);
				SET_GAME_STATE (SUPOX_WAR_NEWS, 0);
			}
			SET_GAME_STATE (UTWIG_VISITS, 0);
			SET_GAME_STATE (UTWIG_INFO, 0);
			SET_GAME_STATE (SUPOX_VISITS, 0);
			SET_GAME_STATE (SUPOX_INFO, 0);
			SetRaceDest (UTWIG_SHIP, ux, uy, 21, ADVANCE_UTWIG_SUPOX_MISSION);
			SetRaceDest (SUPOX_SHIP, sx, sy, 21, (BYTE)~0);
		}
	}
	SET_GAME_STATE (UTWIG_SUPOX_MISSION, MissionState + 1);

	UnlockStarShip (&GLOBAL (avail_race_q), hSupox);
	UnlockStarShip (&GLOBAL (avail_race_q), hUtwig);
}

static void
mycon_mission (void)
{
	HSTARSHIP hMycon;
	EXTENDED_SHIP_FRAGMENTPTR MyconPtr;

	hMycon = GetStarShipFromIndex (&GLOBAL (avail_race_q), MYCON_SHIP);
	MyconPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
			&GLOBAL (avail_race_q), hMycon);

	if (MyconPtr->ShipInfo.actual_strength)
	{
		if (MyconPtr->ShipInfo.energy_level)
		{
			// Head back.
			SET_GAME_STATE (MYCON_KNOW_AMBUSH, 1);
			SetRaceDest (MYCON_SHIP, 6392, 2200, 30, (BYTE)~0);

			MyconPtr->ShipInfo.energy_level = 0;
			MyconPtr->ShipInfo.growth_fract = 0;
		}
		else if (MyconPtr->ShipInfo.loc.x != 6858
				|| MyconPtr->ShipInfo.loc.y != 577)
			SetRaceDest (MYCON_SHIP, 6858, 577, 30, ADVANCE_MYCON_MISSION);
					// To Organon.
		else
		{
			// Endure losses at Organon.
			SIZE strength_loss;

			AddEvent (RELATIVE_EVENT, 0, 14, 0, ADVANCE_MYCON_MISSION);
			strength_loss = (SIZE)(MyconPtr->ShipInfo.actual_strength >> 1);
			MyconPtr->ShipInfo.energy_level =
					(BYTE)(-strength_loss / 14);
			MyconPtr->ShipInfo.growth_fract =
					(BYTE)(((strength_loss % 14) << 8) / 14);
			MyconPtr->ShipInfo.growth_err_term = 255 >> 1;
		}
	}

	UnlockStarShip (&GLOBAL (avail_race_q), hMycon);
}

