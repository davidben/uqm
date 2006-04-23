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

#if defined(DEBUG) || defined(USE_DEBUG_KEY)

#include "uqmdebug.h"

#include "colors.h"
#include "clock.h"
#include "encount.h"
#include "gamestr.h"
#include "gameev.h"
#include "globdata.h"
#include "planets/lifeform.h"
#include "races.h"
#include "setup.h"
#include "state.h"
#include "libs/mathlib.h"
#include "libs/misc.h"

#include <stdio.h>
#include <errno.h>


static void dumpEventCallback (const EVENTPTR eventPtr, void *arg);

static void starRecurse (STAR_DESC *star, void *arg);
static void planetRecurse (STAR_DESC *star, SOLARSYS_STATE *system,
		PLANET_DESC *planet, void *arg);
static void moonRecurse (STAR_DESC *star, SOLARSYS_STATE *system,
		PLANET_DESC *planet, PLANET_DESC *moon, void *arg);

static void dumpSystemCallback (const STAR_DESC *star,
		const SOLARSYS_STATE *system, void *arg);
static void dumpPlanetCallback (const PLANET_DESC *planet, void *arg);
static void dumpMoonCallback (const PLANET_DESC *moon, void *arg);
static void dumpWorld (FILE *out, const PLANET_DESC *world);

static void dumpPlanetTypeCallback (int index, const PlanetFrame *planet,
		void *arg);


BOOLEAN instantMove = FALSE;
BOOLEAN disableInteractivity = FALSE;
void (* volatile debugHook) (void) = NULL;


void
debugKeyPressed (void)
{
	// State modifying:
	equipShip ();
	resetCrewBattle ();
	resetEnergyBattle ();
	instantMove = !instantMove;
	showSpheres ();
	activateAllShips ();
//	forwardToNextEvent (TRUE);		

	// Tests
//	Scale_PerfTest ();

	// Informational:
//	dumpEvents (stderr);
//	dumpPlanetTypes(stderr);
//	debugHook = dumpUniverseToFile;
			// This will cause dumpUniverseToFile to be called from the
			// main loop. Calling it from here would give threading
			// problems.

	// Interactive:
//	uio_debugInteractive(stdin, stdout, stderr);
}

////////////////////////////////////////////////////////////////////////////

// Fast forwards to the next event.
// If skipHEE is set, HYPERSPACE_ENCOUNTER_EVENTs are skipped.
void
forwardToNextEvent (BOOLEAN skipHEE)
{
	HEVENT hEvent;
	EVENTPTR EventPtr;
	COUNT year, month, day;
			// time of next event
	BOOLEAN done;

	if (!GameClockRunning ())
		return;

	SuspendGameClock ();

	done = !skipHEE;
	do {
		hEvent = GetHeadEvent ();
		if (hEvent == 0)
			return;
		LockEvent (hEvent, &EventPtr);
		if (EventPtr->func_index != HYPERSPACE_ENCOUNTER_EVENT)
			done = TRUE;
		year = EventPtr->year_index;
		month = EventPtr->month_index;
		day = EventPtr->day_index;
		UnlockEvent (hEvent);

		for (;;) {
			if (GLOBAL (GameClock.year_index) > year ||
					(GLOBAL (GameClock.year_index) == year &&
					(GLOBAL (GameClock.month_index) > month ||
					(GLOBAL (GameClock.month_index) == month &&
					GLOBAL (GameClock.day_index) >= day))))
				break;

			while (ClockTick () > 0)
				;

			ResumeGameClock ();
			SleepThread (ONE_SECOND / 60);
			SuspendGameClock ();
		}
	} while (!done);

	ResumeGameClock ();
}

const char *
eventName (BYTE func_index)
{
	switch (func_index) {
	case ARILOU_ENTRANCE_EVENT:
		return "ARILOU_ENTRANCE_EVENT";
	case ARILOU_EXIT_EVENT:
		return "ARILOU_EXIT_EVENT";
	case HYPERSPACE_ENCOUNTER_EVENT:
		return "HYPERSPACE_ENCOUNTER_EVENT";
	case KOHR_AH_VICTORIOUS_EVENT:
		return "KOHR_AH_VICTORIOUS_EVENT";
	case ADVANCE_PKUNK_MISSION:
		return "ADVANCE_PKUNK_MISSION";
	case ADVANCE_THRADD_MISSION:
		return "ADVANCE_THRADD_MISSION";
	case ZOQFOT_DISTRESS_EVENT:
		return "ZOQFOT_DISTRESS";
	case ZOQFOT_DEATH_EVENT:
		return "ZOQFOT_DEATH_EVENT";
	case SHOFIXTI_RETURN_EVENT:
		return "SHOFIXTI_RETURN_EVENT";
	case ADVANCE_UTWIG_SUPOX_MISSION:
		return "ADVANCE_UTWIG_SUPOX_MISSION";
	case KOHR_AH_GENOCIDE_EVENT:
		return "KOHR_AH_GENOCIDE_EVENT";
	case SPATHI_SHIELD_EVENT:
		return "SPATHI_SHIELD_EVENT";
	case ADVANCE_ILWRATH_MISSION:
		return "ADVANCE_ILWRATH_MISSION";
	case ADVANCE_MYCON_MISSION:
		return "ADVANCE_MYCON_MISSION";
	case ARILOU_UMGAH_CHECK:
		return "ARILOU_UMGAH_CHECK";
	case YEHAT_REBEL_EVENT:
		return "YEHAT_REBEL_EVENT";
	case SLYLANDRO_RAMP_UP:
		return "SLYLANDRO_RAMP_UP";
	case SLYLANDRO_RAMP_DOWN:
		return "SLYLANDRO_RAMP_DOWN";
	default:
		// Should not happen
		return "???";
	}
}

static void
dumpEventCallback (const EVENTPTR eventPtr, void *arg)
{
	FILE *out = (FILE *) arg;
	dumpEvent (out, eventPtr);
}

void
dumpEvent (FILE *out, EVENTPTR eventPtr)
{
	fprintf (out, "%4u/%02u/%02u: %s\n",
			eventPtr->year_index,
			eventPtr->month_index,
			eventPtr->day_index,
			eventName (eventPtr->func_index));
}

void
dumpEvents (FILE *out)
{
	BOOLEAN restartClock = FALSE;

	if (GameClockRunning ()) {
		SuspendGameClock ();
		restartClock = TRUE;
	}
	ForAllEvents (dumpEventCallback, out);
	if (restartClock)
		ResumeGameClock ();
}

////////////////////////////////////////////////////////////////////////////

// NB: Ship maximum speed and turning rate aren't updated in
// HyperSpace/QuasiSpace or in melee.
void
equipShip (void)
{
	int i;

	// Don't do anything unless in the full game.
	if (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE)
		return;

	// Thrusters:
	for (i = 0; i < NUM_DRIVE_SLOTS; i++)
		GLOBAL_SIS (DriveSlots[i]) = FUSION_THRUSTER;

	// Turning jets:
	for (i = 0; i < NUM_JET_SLOTS; i++)
		GLOBAL_SIS (JetSlots[i]) = TURNING_JETS;

	// Shields:
	SET_GAME_STATE (LANDER_SHIELDS,
			(1 << EARTHQUAKE_DISASTER) |
			(1 << BIOLOGICAL_DISASTER) |
			(1 << LIGHTNING_DISASTER) |
			(1 << LAVASPOT_DISASTER));

	// Modules:
	if (GET_GAME_STATE (CHMMR_BOMB_STATE) < 2)
	{
		// The Precursor bomb has not been installed.
		// This is the original TFB testing layout.
		i = 0;
		GLOBAL_SIS (ModuleSlots[i++]) = HIGHEFF_FUELSYS;
		GLOBAL_SIS (ModuleSlots[i++]) = HIGHEFF_FUELSYS;
		GLOBAL_SIS (ModuleSlots[i++]) = CREW_POD;
		GLOBAL_SIS (ModuleSlots[i++]) = CREW_POD;
		GLOBAL_SIS (ModuleSlots[i++]) = CREW_POD;
		GLOBAL_SIS (ModuleSlots[i++]) = CREW_POD;
		GLOBAL_SIS (ModuleSlots[i++]) = CREW_POD;
		GLOBAL_SIS (ModuleSlots[i++]) = STORAGE_BAY;
		GLOBAL_SIS (ModuleSlots[i++]) = SHIVA_FURNACE;
		GLOBAL_SIS (ModuleSlots[i++]) = SHIVA_FURNACE;
		GLOBAL_SIS (ModuleSlots[i++]) = DYNAMO_UNIT;
		GLOBAL_SIS (ModuleSlots[i++]) = TRACKING_SYSTEM;
		GLOBAL_SIS (ModuleSlots[i++]) = TRACKING_SYSTEM;
		GLOBAL_SIS (ModuleSlots[i++]) = SHIVA_FURNACE;
		GLOBAL_SIS (ModuleSlots[i++]) = CANNON_WEAPON;
		GLOBAL_SIS (ModuleSlots[i++]) = CANNON_WEAPON;
		
		// Landers:
		GLOBAL_SIS (NumLanders) = MAX_LANDERS;
	}
	else
	{
		// The Precursor bomb has been installed.
		i = NUM_BOMB_MODULES;
		GLOBAL_SIS (ModuleSlots[i++]) = HIGHEFF_FUELSYS;
		GLOBAL_SIS (ModuleSlots[i++]) = CREW_POD;
		GLOBAL_SIS (ModuleSlots[i++]) = SHIVA_FURNACE;
		GLOBAL_SIS (ModuleSlots[i++]) = SHIVA_FURNACE;
		GLOBAL_SIS (ModuleSlots[i++]) = CANNON_WEAPON;
		GLOBAL_SIS (ModuleSlots[i++]) = SHIVA_FURNACE;
	}

	assert (i <= NUM_MODULE_SLOTS);

	// Fill the fuel and crew compartments to the maximum.
	GLOBAL_SIS (FuelOnBoard) = FUEL_RESERVE;
	GLOBAL_SIS (CrewEnlisted) = 0;
	for (i = 0; i < NUM_MODULE_SLOTS; i++)
	{
		switch (GLOBAL_SIS (ModuleSlots[i])) {
			case CREW_POD:
				GLOBAL_SIS (CrewEnlisted) += CREW_POD_CAPACITY;
				break;
			case FUEL_TANK:
				GLOBAL_SIS (FuelOnBoard) += FUEL_TANK_CAPACITY;
				break;
			case HIGHEFF_FUELSYS:
				GLOBAL_SIS (FuelOnBoard) += HEFUEL_TANK_CAPACITY;
				break;
		}
	}

	// Update the maximum speed and turning rate when in interplanetary.
	if (pSolarSysState != NULL)
	{
		// Thrusters:
		pSolarSysState->max_ship_speed = 5 * IP_SHIP_THRUST_INCREMENT;
		for (i = 0; i < NUM_DRIVE_SLOTS; i++)
			if (GLOBAL_SIS (DriveSlots[i] == FUSION_THRUSTER))
				pSolarSysState->max_ship_speed += IP_SHIP_THRUST_INCREMENT;

		// Turning jets:
		pSolarSysState->turn_wait = IP_SHIP_TURN_WAIT;
		for (i = 0; i < NUM_JET_SLOTS; i++)
			if (GLOBAL_SIS (JetSlots[i]) == TURNING_JETS)
				pSolarSysState->turn_wait -= IP_SHIP_TURN_DECREMENT;
	}

	// Make sure everything is redrawn:
	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE ||
			LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY)
	{
		LockMutex (GraphicsLock);
		DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);
		UnlockMutex (GraphicsLock);
	}
}

////////////////////////////////////////////////////////////////////////////

#if 0
// Not needed anymore, but could be useful in the future.

// Find the HELEMENT belonging to the Flagship.
static HELEMENT
findFlagshipElement (void)
{
	HELEMENT hElement, hNextElement;
	ELEMENTPTR ElementPtr;

	// Find the ship element.
	for (hElement = GetTailElement (); hElement != 0;
			hElement = hNextElement)
	{
		LockElement (hElement, &ElementPtr);

		if ((ElementPtr->state_flags & PLAYER_SHIP) != 0)
		{
			UnlockElement (hElement);
			return hElement;
		}

		hNextElement = GetPredElement (ElementPtr);
		UnlockElement (hElement);
	}
	return 0;
}
#endif

// Move the Flagship to the destination of the autopilot.
// Should only be called from HyperSpace/QuasiSpace.
// It can be called from debugHook directly after entering HS/QS though.
void
doInstantMove (void)
{
	// Move to the new location:
	GLOBAL_SIS (log_x) = UNIVERSE_TO_LOGX((GLOBAL (autopilot)).x);
	GLOBAL_SIS (log_y) = UNIVERSE_TO_LOGY((GLOBAL (autopilot)).y);

	// Check for a solar systems at the destination.
	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
	{
		// If there's a solar system at the destination, enter it.
		CurStarDescPtr = FindStar (0, &(GLOBAL (autopilot)), 0, 0);
		if (CurStarDescPtr)
		{
			// Leave HyperSpace/QuasiSpace if we're there:
			SET_GAME_STATE (USED_BROADCASTER, 0);
			GLOBAL (CurrentActivity) &= ~IN_BATTLE;

			// Enter IP:
			GLOBAL (ShipStamp.frame) = 0;
					// This causes the ship position in IP to be reset.
			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
		}
	}

	// Turn off the autopilot:
	(GLOBAL (autopilot)).x = ~0;
	(GLOBAL (autopilot)).y = ~0;
}

////////////////////////////////////////////////////////////////////////////

void
showSpheres (void)
{
	HSTARSHIP hStarShip, hNextShip;
	
	for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip != NULL; hStarShip = hNextShip)
	{
		EXTENDED_SHIP_FRAGMENTPTR StarShipPtr;

		StarShipPtr = (EXTENDED_SHIP_FRAGMENTPTR) LockStarShip (
				&GLOBAL (avail_race_q), hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);

		if ((StarShipPtr->ShipInfo.actual_strength != (COUNT) ~0) &&
				(StarShipPtr->ShipInfo.known_strength !=
				StarShipPtr->ShipInfo.actual_strength))
		{
			StarShipPtr->ShipInfo.known_strength =
					StarShipPtr->ShipInfo.actual_strength;
			StarShipPtr->ShipInfo.known_loc = StarShipPtr->ShipInfo.loc;
		}

		UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);
	}
}

////////////////////////////////////////////////////////////////////////////

void
activateAllShips (void)
{
	HSTARSHIP hStarShip, hNextShip;
	
	for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip != NULL; hStarShip = hNextShip)
	{
		EXTENDED_SHIP_FRAGMENTPTR StarShipPtr;

		StarShipPtr = (EXTENDED_SHIP_FRAGMENTPTR) LockStarShip (
				&GLOBAL (avail_race_q), hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);

		if (StarShipPtr->ShipInfo.icons != NULL)
				// Skip the Ur-Quan probe.
		{
			StarShipPtr->ShipInfo.ship_flags &= ~(GOOD_GUY | BAD_GUY);
			StarShipPtr->ShipInfo.ship_flags |= GOOD_GUY;
		}

		UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);
	}
}

////////////////////////////////////////////////////////////////////////////

void
forAllStars (void (*callback) (STAR_DESC *, void *), void *arg)
{
	int i;
	extern STAR_DESC starmap_array[];

	for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
		callback (&starmap_array[i], arg);
}

void
forAllPlanets (STAR_DESC *star, SOLARSYS_STATE *system, void (*callback) (
		STAR_DESC *, SOLARSYS_STATE *, PLANET_DESC *, void *), void *arg)
{
	COUNT i;

	assert(CurStarDescPtr = star);
	assert(pSolarSysState == system);

	for (i = 0; i < system->SunDesc[0].NumPlanets; i++)
		callback (star, system, &system->PlanetDesc[i], arg);
}

void
forAllMoons (STAR_DESC *star, SOLARSYS_STATE *system, PLANET_DESC *planet,
		void (*callback) (STAR_DESC *, SOLARSYS_STATE *, PLANET_DESC *,
		PLANET_DESC *, void *), void *arg)
{
	COUNT i;

	assert(pSolarSysState == system);
	assert(system->pBaseDesc == planet);

	for (i = 0; i < planet->NumPlanets; i++)
		callback (star, system, planet, &system->MoonDesc[i], arg);
}

////////////////////////////////////////////////////////////////////////////

void
UniverseRecurse (UniverseRecurseArg *universeRecurseArg)
{
	BOOLEAN clockRunning;
	ACTIVITY savedActivity;
	
	if (universeRecurseArg->systemFunc == NULL
			&& universeRecurseArg->planetFunc == NULL
			&& universeRecurseArg->moonFunc == NULL)
		return;
	
	clockRunning = GameClockRunning ();
	if (clockRunning)
		SuspendGameClock ();
	//TFB_DEBUG_HALT = 1;
	savedActivity = GLOBAL (CurrentActivity);
	disableInteractivity = TRUE;

	forAllStars (starRecurse, (void *) universeRecurseArg);
	
	disableInteractivity = FALSE;
	GLOBAL (CurrentActivity) = savedActivity;
	if (clockRunning)
		ResumeGameClock ();
}

static void
starRecurse (STAR_DESC *star, void *arg)
{
	UniverseRecurseArg *universeRecurseArg = (UniverseRecurseArg *) arg;

	SOLARSYS_STATE SolarSysState;
	PSOLARSYS_STATE oldPSolarSysState = pSolarSysState;
	DWORD oldSeed =
			TFB_SeedRandom (MAKE_DWORD (star->star_pt.x, star->star_pt.y));

	STAR_DESCPTR oldStarDescPtr = CurStarDescPtr;
	CurStarDescPtr = star;

	memset (&SolarSysState, 0, sizeof (SolarSysState));
	SolarSysState.SunDesc[0].pPrevDesc = 0;
	SolarSysState.SunDesc[0].rand_seed = TFB_Random ();
	SolarSysState.SunDesc[0].data_index = STAR_TYPE (star->Type);
	SolarSysState.SunDesc[0].location.x = 0;
	SolarSysState.SunDesc[0].location.y = 0;
	//SolarSysState.SunDesc[0].radius = MIN_ZOOM_RADIUS;
	SolarSysState.GenFunc = GenerateIP (star->Index);

	pSolarSysState = &SolarSysState;
	(*pSolarSysState->GenFunc) (GENERATE_PLANETS);

	if (universeRecurseArg->systemFunc != NULL)
	{
		(*universeRecurseArg->systemFunc) (
				star, &SolarSysState, universeRecurseArg->arg);
	}
	
	if (universeRecurseArg->planetFunc != NULL
			|| universeRecurseArg->moonFunc != NULL)
	{
		forAllPlanets (star, &SolarSysState, planetRecurse,
				(void *) universeRecurseArg);
	}

	pSolarSysState = oldPSolarSysState;
	CurStarDescPtr = oldStarDescPtr;
	TFB_SeedRandom (oldSeed);
}

static void
planetRecurse (STAR_DESC *star, SOLARSYS_STATE *system,
		PLANET_DESC *planet, void *arg)
{
	UniverseRecurseArg *universeRecurseArg = (UniverseRecurseArg *) arg;
	
	assert(CurStarDescPtr = star);
	assert(pSolarSysState == system);

	system->pBaseDesc = planet;
	planet->pPrevDesc = &system->SunDesc[0];

	if (universeRecurseArg->planetFunc != NULL)
	{
		system->pOrbitalDesc = planet;
		DoPlanetaryAnalysis (&system->SysInfo, planet);
				// When GenerateRandomIP is used as GenFunc,
				// GENERATE_ORBITAL will also call DoPlanetaryAnalysis,
				// but with other GenFuncs this is not guaranteed.
		(*system->GenFunc) (GENERATE_ORBITAL);
		(*universeRecurseArg->planetFunc) (
				planet, universeRecurseArg->arg);
	}

	if (universeRecurseArg->moonFunc != NULL)
	{
		DWORD oldSeed = TFB_SeedRandom (planet->rand_seed);
		
		(*system->GenFunc) (GENERATE_MOONS);

		forAllMoons (star, system, planet, moonRecurse,
				(void *) universeRecurseArg);

		TFB_SeedRandom (oldSeed);
	}
}

static void
moonRecurse (STAR_DESC *star, SOLARSYS_STATE *system, PLANET_DESC *planet,
		PLANET_DESC *moon, void *arg)
{
	UniverseRecurseArg *universeRecurseArg = (UniverseRecurseArg *) arg;
	
	assert(CurStarDescPtr = star);
	assert(pSolarSysState == system);
	assert(system->pBaseDesc == planet);
	
	moon->pPrevDesc = planet;

	if (universeRecurseArg->moonFunc != NULL)
	{
		system->pOrbitalDesc = moon;
		DoPlanetaryAnalysis (&system->SysInfo, moon);
				// When GenerateRandomIP is used as GenFunc,
				// GENERATE_ORBITAL will also call DoPlanetaryAnalysis,
				// but with other GenFuncs this is not guaranteed.
		(*system->GenFunc) (GENERATE_ORBITAL);
		(*universeRecurseArg->moonFunc) (
				moon, universeRecurseArg->arg);
	}
}

////////////////////////////////////////////////////////////////////////////

typedef struct
{
	FILE *out;
} DumpUniverseArg;

void
dumpUniverse (FILE *out)
{
	DumpUniverseArg dumpUniverseArg;
	UniverseRecurseArg universeRecurseArg;
	
	dumpUniverseArg.out = out;

	universeRecurseArg.systemFunc = dumpSystemCallback;
	universeRecurseArg.planetFunc = dumpPlanetCallback;
	universeRecurseArg.moonFunc = dumpMoonCallback;
	universeRecurseArg.arg = (void *) &dumpUniverseArg;

	UniverseRecurse (&universeRecurseArg);
}

void
dumpUniverseToFile (void)
{
	FILE *out;

#	define UNIVERSE_DUMP_FILE "PlanetInfo"
	out = fopen(UNIVERSE_DUMP_FILE, "w");
	if (out == NULL)
	{
		fprintf(stderr, "Error: Could not open file '%s' for "
				"writing: %s\n", UNIVERSE_DUMP_FILE, strerror(errno));
		return;
	}

	dumpUniverse (out);
	
	fclose(out);

	fprintf(stdout, "*** Star dump complete. The game may be in an "
			"undefined state.\n");
			// Data generation may have changed the game state,
			// in particular for special planet generation.
}

static void
dumpSystemCallback (const STAR_DESC *star, const SOLARSYS_STATE *system,
		void *arg)
{
	FILE *out = ((DumpUniverseArg *) arg)->out;
	dumpSystem (out, star, system);
}

void
dumpSystem (FILE *out, const STAR_DESC *star, const SOLARSYS_STATE *system)
{
	UNICODE name[256];
	UNICODE buf[40];

	GetClusterName ((STAR_DESCPTR) star, name);
	snprintf (buf, sizeof buf, "%s %s",
			bodyColorString (STAR_COLOR(star->Type)),
			starTypeString (STAR_TYPE(star->Type)));
	fprintf (out, "%-22s  (%3d.%1d, %3d.%1d) %-19s  %s\n",
			name,
			star->star_pt.x / 10, star->star_pt.x % 10,
			star->star_pt.y / 10, star->star_pt.y % 10,
			buf,
			starPresenceString (star->Index));

	(void) system;  /* satisfy compiler */
}

const char *
bodyColorString (BYTE col)
{
	switch (col) {
		case BLUE_BODY:
			return "blue";
		case GREEN_BODY:
			return "green";
		case ORANGE_BODY:
			return "orange";
		case RED_BODY:
			return "red";
		case WHITE_BODY:
			return "white";
		case YELLOW_BODY:
			return "yellow";
		case CYAN_BODY:
			return "cyan";
		case PURPLE_BODY:
			return "purple";
		case VIOLET_BODY:
			return "violet";
		default:
			// Should not happen
			return "???";
	}
}

const char *
starTypeString (BYTE type)
{
	switch (type) {
		case DWARF_STAR:
			return "dwarf";
		case GIANT_STAR:
			return "giant";
		case SUPER_GIANT_STAR:
			return "super giant";
		default:
			// Should not happen
			return "???";
	}
}

const char *
starPresenceString (BYTE index)
{
	switch (index) {
		case 0:
			// nothing
			return "";
		case SOL_DEFINED:
			return "Sol";
		case SHOFIXTI_DEFINED:
			return "Shofixti male";
		case MAIDENS_DEFINED:
			return "Shofixti maidens";
		case START_COLONY_DEFINED:
			return "Starting colony";
		case SPATHI_DEFINED:
			return "Spathi home";
		case ZOQFOT_DEFINED:
			return "Zoq-Fot-Pik home";
		case MELNORME0_DEFINED:
			return "Melnorme trader #0";
		case MELNORME1_DEFINED:
			return "Melnorme trader #1";
		case MELNORME2_DEFINED:
			return "Melnorme trader #2";
		case MELNORME3_DEFINED:
			return "Melnorme trader #3";
		case MELNORME4_DEFINED:
			return "Melnorme trader #4";
		case MELNORME5_DEFINED:
			return "Melnorme trader #5";
		case MELNORME6_DEFINED:
			return "Melnorme trader #6";
		case MELNORME7_DEFINED:
			return "Melnorme trader #7";
		case MELNORME8_DEFINED:
			return "Melnorme trader #8";
		case TALKING_PET_DEFINED:
			return "Talking Pet";
		case CHMMR_DEFINED:
			return "Chmmr home";
		case SYREEN_DEFINED:
			return "Syreen home";
		case BURVIXESE_DEFINED:
			return "Burvixese ruins";
		case SLYLANDRO_DEFINED:
			return "Slylandro home";
		case DRUUGE_DEFINED:
			return "Druuge home";
		case BOMB_DEFINED:
			return "Precursor bomb";
		case AQUA_HELIX_DEFINED:
			return "Aqua Helix";
		case SUN_DEVICE_DEFINED:
			return "Sun Device";
		case TAALO_PROTECTOR_DEFINED:
			return "Taalo Shield";
		case SHIP_VAULT_DEFINED:
			return "Syreen ship vault";
		case URQUAN_WRECK_DEFINED:
			return "Ur-Quan ship wreck";
		case VUX_BEAST_DEFINED:
			return "Zex' beauty";
		case SAMATRA_DEFINED:
			return "Sa-Matra";
		case ZOQ_SCOUT_DEFINED:
			return "Zoq-Fot-Pik scout";
		case MYCON_DEFINED:
			return "Mycon home";
		case EGG_CASE0_DEFINED:
			return "Mycon egg shell #0";
		case EGG_CASE1_DEFINED:
			return "Mycon egg shell #1";
		case EGG_CASE2_DEFINED:
			return "Mycon egg shell #2";
		case PKUNK_DEFINED:
			return "Pkunk home";
		case UTWIG_DEFINED:
			return "Utwig home";
		case SUPOX_DEFINED:
			return "Supox home";
		case YEHAT_DEFINED:
			return "Yehat home";
		case VUX_DEFINED:
			return "Vux home";
		case ORZ_DEFINED:
			return "Orz home";
		case THRADD_DEFINED:
			return "Thraddash home";
		case RAINBOW_DEFINED:
			return "Rainbow world";
		case ILWRATH_DEFINED:
			return "Ilwrath home";
		case ANDROSYNTH_DEFINED:
			return "Androsynth ruins";
		case MYCON_TRAP_DEFINED:
			return "Mycon trap";
		default:
			// Should not happen
			return "???";
	}
}

static void
dumpPlanetCallback (const PLANET_DESC *planet, void *arg)
{
	FILE *out = ((DumpUniverseArg *) arg)->out;
	dumpPlanet (out, planet);
}

void
dumpPlanet (FILE *out, const PLANET_DESC *planet)
{
	(*pSolarSysState->GenFunc) (GENERATE_NAME);
	fprintf (out, "- %-37s  %s\n", GLOBAL_SIS (PlanetName),
			planetTypeString (planet->data_index & ~PLANET_SHIELDED));
	dumpWorld (out, planet);
}

static void
dumpMoonCallback (const PLANET_DESC *moon, void *arg)
{
	FILE *out = ((DumpUniverseArg *) arg)->out;
	dumpMoon (out, moon);
}

void
dumpMoon (FILE *out, const PLANET_DESC *moon)
{
	const char *typeStr;
	
	if (moon->data_index == (BYTE) ~0)
	{
		typeStr = "StarBase";
	}
	else if (moon->data_index == (BYTE) (~0 - 1))
	{
		typeStr = "Sa-Matra";
	}
	else
	{
		typeStr = planetTypeString (moon->data_index & ~PLANET_SHIELDED);
	}
	fprintf (out, "  - Moon %-30c  %s\n",
			'a' + (moon - &pSolarSysState->MoonDesc[0]), typeStr);

	dumpWorld (out, moon);
}

static void
dumpWorld (FILE *out, const PLANET_DESC *world)
{
	PLANET_INFO *info;
	
	if (world->data_index == (BYTE) ~0) {
		// StarBase
		return;
	}
	
	if (world->data_index == (BYTE)(~0 - 1)) {
		// Sa-Matra
		return;
	}

	info = &pSolarSysState->SysInfo.PlanetInfo;
	fprintf(out, "          AxialTilt:  %d\n", info->AxialTilt);
	fprintf(out, "          Tectonics:  %d\n", info->Tectonics);
	fprintf(out, "          Weather:    %d\n", info->Weather);
	fprintf(out, "          Density:    %d\n", info->PlanetDensity);
	fprintf(out, "          Radius:     %d\n", info->PlanetRadius);
	fprintf(out, "          Gravity:    %d\n", info->SurfaceGravity);
	fprintf(out, "          Temp:       %d\n", info->SurfaceTemperature);
	fprintf(out, "          Day:        %d\n", info->RotationPeriod);
	fprintf(out, "          Atmosphere: %d\n", info->AtmoDensity);
	fprintf(out, "          LifeChance: %d\n", info->LifeChance);
	fprintf(out, "          DistToSun:  %d\n", info->PlanetToSunDist);

	if (world->data_index & PLANET_SHIELDED) {
		// Slave-shielded planet
		return;
	}

	fprintf (out, "          Bio: %4d    Min: %4d\n",
			calculateBioValue (pSolarSysState, world),
			calculateMineralValue (pSolarSysState, world));
}

COUNT
calculateBioValue (const SOLARSYS_STATE *system, const PLANET_DESC *world)
{
	COUNT result;
	COUNT numBio;
	COUNT i;

	assert(system->pOrbitalDesc == world);
	
	((SOLARSYS_STATE *) system)->CurNode = (COUNT)~0;
	(*system->GenFunc) (GENERATE_LIFE);
	numBio = system->CurNode;

	result = 0;
	for (i = 0; i < numBio; i++)
	{
		((SOLARSYS_STATE *) system)->CurNode = i;
		(*system->GenFunc) (GENERATE_LIFE);
		result += BIO_CREDIT_VALUE * LONIBBLE (CreatureData[
				system->SysInfo.PlanetInfo.CurType].ValueAndHitPoints);
	}
	return result;
}

void
generateBioIndex(const SOLARSYS_STATE *system,
		const PLANET_DESC *world, COUNT bio[])
{
	COUNT numBio;
	COUNT i;

	assert(system->pOrbitalDesc == world);
	
	((SOLARSYS_STATE *) system)->CurNode = (COUNT)~0;
	(*system->GenFunc) (GENERATE_LIFE);
	numBio = system->CurNode;

	for (i = 0; i < NUM_CREATURE_TYPES + NUM_SPECIAL_CREATURE_TYPES; i++)
		bio[i] = 0;
	
	for (i = 0; i < numBio; i++)
	{
		((SOLARSYS_STATE *) system)->CurNode = i;
		(*system->GenFunc) (GENERATE_LIFE);
		bio[system->SysInfo.PlanetInfo.CurType]++;
	}
}

COUNT
calculateMineralValue (const SOLARSYS_STATE *system,
		const PLANET_DESC *world)
{
	COUNT result;
	COUNT numDeposits;
	COUNT i;

	assert(system->pOrbitalDesc == world);
	
	((SOLARSYS_STATE *) system)->CurNode = (COUNT)~0;
	(*system->GenFunc) (GENERATE_MINERAL);
	numDeposits = system->CurNode;

	result = 0;
	for (i = 0; i < numDeposits; i++)
	{
		((SOLARSYS_STATE *) system)->CurNode = i;
		(*system->GenFunc) (GENERATE_MINERAL);
		result += HIBYTE (system->SysInfo.PlanetInfo.CurDensity) *
				GLOBAL (ElementWorth[ElementCategory (
				system->SysInfo.PlanetInfo.CurType)]);
	}
	return result;
}

void
generateMineralIndex(const SOLARSYS_STATE *system,
		const PLANET_DESC *world, COUNT minerals[])
{
	COUNT numDeposits;
	COUNT i;

	assert(system->pOrbitalDesc == world);
	
	((SOLARSYS_STATE *) system)->CurNode = (COUNT)~0;
	(*system->GenFunc) (GENERATE_MINERAL);
	numDeposits = system->CurNode;

	for (i = 0; i < NUM_ELEMENT_CATEGORIES; i++)
		minerals[i] = 0;
	
	for (i = 0; i < numDeposits; i++)
	{
		((SOLARSYS_STATE *) system)->CurNode = i;
		(*system->GenFunc) (GENERATE_MINERAL);
		minerals[ElementCategory(system->SysInfo.PlanetInfo.CurType)] +=
				HIBYTE (system->SysInfo.PlanetInfo.CurDensity);
	}
}

////////////////////////////////////////////////////////////////////////////

void
forAllPlanetTypes (void (*callback) (int, const PlanetFrame *, void *),
		void *arg)
{
	int i;
	extern const PlanetFrame planet_array[];

	for (i = 0; i < NUMBER_OF_PLANET_TYPES; i++)
		callback (i, &planet_array[i], arg);
}
	
typedef struct
{
	FILE *out;
} DumpPlanetTypesArg;

void
dumpPlanetTypes (FILE *out)
{
	DumpPlanetTypesArg dumpPlanetTypesArg;
	dumpPlanetTypesArg.out = out;

	forAllPlanetTypes (dumpPlanetTypeCallback, (void *) &dumpPlanetTypesArg);
}

static void
dumpPlanetTypeCallback (int index, const PlanetFrame *planetType, void *arg)
{
	DumpPlanetTypesArg *dumpPlanetTypesArg = (DumpPlanetTypesArg *) arg;

	dumpPlanetType(dumpPlanetTypesArg->out, index, planetType);
}

void
dumpPlanetType (FILE *out, int index, const PlanetFrame *planetType)
{
	int i;
	fprintf (out,
			"%s\n"
			"\tType: %s\n"
			"\tColor: %s\n"
			"\tSurface generation algoritm: %s\n"
			"\tTectonics: %s\n"
			"\tAtmosphere: %s\n"
			"\tDensity: %s\n"
			"\tElements:\n",
			planetTypeString (index),
			worldSizeString (PLANSIZE (planetType->Type)),
			bodyColorString (PLANCOLOR (planetType->Type)),
			worldGenAlgoString (PLANALGO (planetType->Type)),
			tectonicsString (planetType->BaseTectonics),
			atmosphereString (HINIBBLE (planetType->AtmoAndDensity)),
			densityString (LONIBBLE (planetType->AtmoAndDensity))
			);
	for (i = 0; i < NUM_USEFUL_ELEMENTS; i++)
	{
		const ElementEntry *entry;
		entry = &planetType->UsefulElements[i];
		if (entry->Density == 0)
			continue;
		fprintf(out, "\t\t0 to %d %s-quality (+%d) deposits of %s (%s)\n",
				DEPOSIT_QUANTITY (entry->Density),
				depositQualityString (DEPOSIT_QUALITY (entry->Density)),
				DEPOSIT_QUALITY (entry->Density) * 5,
				GAME_STRING (ELEMENTS_STRING_BASE + entry->ElementType),
				GAME_STRING (CARGO_STRING_BASE + 2 + ElementCategory (
				entry->ElementType))
			);
	}
	fprintf (out, "\n");
}

const char *
planetTypeString (int typeIndex)
{
	static UNICODE typeStr[40];

	if (typeIndex >= FIRST_GAS_GIANT)
	{
		// "Gas Giant"
		snprintf(typeStr, sizeof typeStr, "%s",
		GAME_STRING (SCAN_STRING_BASE + 4 + 51));
	}
	else
	{
		// "<type> World" (eg. "Water World")
		snprintf(typeStr, sizeof typeStr, "%s %s",
				GAME_STRING (SCAN_STRING_BASE + 4 + typeIndex),
				GAME_STRING (SCAN_STRING_BASE + 4 + 50));
	}
	return typeStr;
}

// size is what you get from PLANSIZE (planetFrame.Type)
const char *
worldSizeString (BYTE size)
{
	switch (size)
	{
		case SMALL_ROCKY_WORLD:
			return "small rocky world";
		case LARGE_ROCKY_WORLD:
			return "large rocky world";
		case GAS_GIANT:
			return "gas giant";
		default:
			// Should not happen
			return "???";
	}
}

// algo is what you get from PLANALGO (planetFrame.Type)
const char *
worldGenAlgoString (BYTE algo)
{
	switch (algo)
	{
		case TOPO_ALGO:
			return "TOPO_ALGO";
		case CRATERED_ALGO:
			return "CRATERED_ALGO";
		case GAS_GIANT_ALGO:
			return "GAS_GIANT_ALGO";
		default:
			// Should not happen
			return "???";
	}
}

// tectonics is what you get from planetFrame.BaseTechtonics
// not reentrant
const char *
tectonicsString (BYTE tectonics)
{
	static char buf[sizeof "-127"];
	switch (tectonics)
	{
		case NO_TECTONICS:
			return "none";
		case LOW_TECTONICS:
			return "low";
		case MED_TECTONICS:
			return "medium";
		case HIGH_TECTONICS:
			return "high";
		case SUPER_TECTONICS:
			return "super";
		default:
			snprintf (buf, sizeof buf, "%d", tectonics);
			return buf;
	}
}

// atmosphere is what you get from HINIBBLE (planetFrame.AtmoAndDensity)
const char *
atmosphereString (BYTE atmosphere)
{
	switch (atmosphere)
	{
		case LIGHT:
			return "thin";
		case MEDIUM:
			return "normal";
		case HEAVY:
			return "thick";
		default:
			return "super thick";
	}
}

// density is what you get from LONIBBLE (planetFrame.AtmoAndDensity)
const char *
densityString (BYTE density)
{
	switch (density)
	{
		case GAS_DENSITY:
			return "gaseous";
		case LIGHT_DENSITY:
			return "light";
		case LOW_DENSITY:
			return "low";
		case NORMAL_DENSITY:
			return "normal";
		case HIGH_DENSITY:
			return "high";
		case SUPER_DENSITY:
			return "super high";
		default:
			// Should not happen
			return "???";
	}
}

// quality is what you get from DEPOSIT_QUALITY (elementEntry.Density)
const char *
depositQualityString (BYTE quality)
{
	switch (quality)
	{
		case LIGHT:
			return "low";
		case MEDIUM:
			return "medium";
		case HEAVY:
			return "high";
		default:
			// Should not happen
			return "???";
	}
}

////////////////////////////////////////////////////////////////////////////

// Which should be GOOD_GUY or BAD_GUY
STARSHIPPTR
findPlayerShip(ELEMENT_FLAGS which) {
	HELEMENT hElement, hNextElement;

	for (hElement = GetHeadElement (); hElement; hElement = hNextElement)
	{
		ELEMENTPTR ElementPtr;

		LockElement (hElement, &ElementPtr);
		hNextElement = GetSuccElement (ElementPtr);
					
		if ((ElementPtr->state_flags & PLAYER_SHIP)	&&
				(ElementPtr->state_flags & (GOOD_GUY | BAD_GUY)) == which)
		{
			STARSHIPPTR StarShipPtr;
			GetElementStarShip (ElementPtr, &StarShipPtr);
			UnlockElement (hElement);
			return StarShipPtr;
		}
		
		UnlockElement (hElement);
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////

void
resetCrewBattle(void) {
	STARSHIPPTR StarShipPtr;
	COUNT delta;
	CONTEXT OldContext;
	
	if (!(GLOBAL (CurrentActivity) & IN_BATTLE) ||
			(LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE))
		return;
	
	StarShipPtr = findPlayerShip (GOOD_GUY);
	if (StarShipPtr == NULL || StarShipPtr->RaceDescPtr == NULL)
		return;

	delta = StarShipPtr->RaceDescPtr->ship_info.max_crew -
			StarShipPtr->RaceDescPtr->ship_info.crew_level;

	OldContext = SetContext (StatusContext);
	DeltaCrew (StarShipPtr->hShip, delta);
	SetContext (OldContext);
}

void
resetEnergyBattle(void) {
	STARSHIPPTR StarShipPtr;
	COUNT delta;
	CONTEXT OldContext;
	
	if (!(GLOBAL (CurrentActivity) & IN_BATTLE) ||
			(LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE))
		return;
	
	StarShipPtr = findPlayerShip (GOOD_GUY);
	if (StarShipPtr == NULL || StarShipPtr->RaceDescPtr == NULL)
		return;

	delta = StarShipPtr->RaceDescPtr->ship_info.max_energy -
			StarShipPtr->RaceDescPtr->ship_info.energy_level;

	OldContext = SetContext (StatusContext);
	DeltaEnergy (StarShipPtr->hShip, delta);
	SetContext (OldContext);
}


#endif  /* DEBUG */


