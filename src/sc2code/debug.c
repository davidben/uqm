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

#include <stdio.h>

#include "starcon.h"
#include "debug.h"

static void dumpEventCallback (EVENTPTR eventPtr, void *arg);
static void dumpStarCallback(STAR_DESC *star, void *arg);
static void dumpPlanetCallback (PLANET_DESC *planet, void *arg);

BOOLEAN instantMove = FALSE;

void
debugKeyPressed (void)
{
	// State modifying:
	equipShip ();
	instantMove = !instantMove;
//	forwardToNextEvent (TRUE);		

	// Informational:
//	dumpEvents (stderr);
//	dumpStars (stdout, DUMP_PLANETS);
			// Currently dumpStars() does not work when called from here.
			// It needs to be called from the top of ExploreSolarSys().

	// Interactive:
//	uio_debugInteractive(stdin, stdout, stderr);
}

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
dumpEventCallback (EVENTPTR eventPtr, void *arg)
{
	FILE *out = (FILE *) arg;
	dumpEvent (out, eventPtr);
}

void
dumpEvent (FILE *out, EVENTPTR eventPtr)
{
	fprintf (out, "%04u/%02u/%02u: %s\n",
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

void
forAllStars(void (*callback) (STAR_DESC *, void *), void *arg) {
	int i;
	extern STAR_DESC starmap_array[];

	for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
		callback (&starmap_array[i], arg);
}
	
typedef struct {
	FILE *out;
	UWORD flags;
} DumpStarsArg;

void
dumpStars(FILE *out, UWORD flags) {
	DumpStarsArg dumpStarsArg;
	dumpStarsArg.out = out;
	dumpStarsArg.flags = flags;

	forAllStars (dumpStarCallback, (void *) &dumpStarsArg);
}

static void
dumpStarCallback(STAR_DESC *star, void *arg) {
	DumpStarsArg *dumpStarsArg = (DumpStarsArg *) arg;

	dumpStar(dumpStarsArg->out, star, dumpStarsArg->flags);
}

void
dumpStar(FILE *out, const STAR_DESC *star, UWORD flags) {
	UNICODE name[40];
	UNICODE buf[40];

	GetClusterName ((STAR_DESCPTR) star, name);
	snprintf (buf, sizeof buf, "%s %s",
			starColorString (STAR_COLOR(star->Type)),
			starTypeString (STAR_TYPE(star->Type)));
	fprintf (out, "%-22s  (%3d.%1d, %3d.%1d) %-19s  %s\n",
			name,
			star->star_pt.x / 10, star->star_pt.x % 10,
			star->star_pt.y / 10, star->star_pt.y % 10,
			buf,
			starPresenceString (star->Index));
	if (flags & DUMP_PLANETS)
		dumpPlanets (out, star, flags);
}

const char *
starColorString (BYTE col) {
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
		default:
			// Should not happen
			return "???";
	}
}

const char *
starTypeString (BYTE type) {
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
starPresenceString (BYTE index) {
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

// Currently, this function only works when called from the top of
// ExploreSolarSys(). The state afterwards is undefined.
void
forAllPlanets(STAR_DESC *star,
		void (*callback) (PLANET_DESC *, void *), void *arg)
{
	// These two functions were originally static to solarsys.c
	extern void InitSolarSys(void);
	extern void UninitSolarSys(void);
	
	SOLARSYS_STATE solarSysState;
	STAR_DESC *oldStarDescPtr;
	COUNT i;

	oldStarDescPtr = CurStarDescPtr;
	CurStarDescPtr = star;

	GLOBAL_SIS (log_x) = UNIVERSE_TO_LOGX (CurStarDescPtr->star_pt.x);
	GLOBAL_SIS (log_y) = UNIVERSE_TO_LOGY (CurStarDescPtr->star_pt.y);

	memset (&solarSysState, 0, sizeof solarSysState);
	solarSysState.GenFunc = GenerateIP (CurStarDescPtr->Index);
	
	pSolarSysState = &solarSysState;

	GLOBAL (CurrentActivity) = MAKE_WORD (IN_INTERPLANETARY, 0);
	SuspendGameClock ();
	InitSolarSys ();

	for (i = 0; i < pSolarSysState->SunDesc[0].NumPlanets; i++)
		callback (&pSolarSysState->PlanetDesc[i], arg);
	
	GLOBAL (autopilot.x) = ~0;
	GLOBAL (autopilot.y) = ~0;
	GLOBAL (ShipStamp.frame) = 0;
	GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
	UninitSolarSys ();
	pSolarSysState = 0;

	CurStarDescPtr = oldStarDescPtr;
}

typedef struct {
	FILE *out;
	UWORD flags;
} DumpPlanetsArg;

void
dumpPlanets (FILE *out, const STAR_DESC *star, UWORD flags) {
	DumpPlanetsArg dumpPlanetsArg;
	dumpPlanetsArg.out = out;
	dumpPlanetsArg.flags = flags;

	forAllPlanets ((STAR_DESC *) star, dumpPlanetCallback,
			(void *) &dumpPlanetsArg);
}

static void
dumpPlanetCallback (PLANET_DESC *planet, void *arg)
{
	DumpPlanetsArg *dumpPlanetsArg = (DumpPlanetsArg *) arg;
	dumpPlanet (dumpPlanetsArg->out, planet, dumpPlanetsArg->flags);
}

void
dumpPlanet(FILE *out, const PLANET_DESC *planet, UWORD flags)
{
	PLANET_DESC *oldPlanetDesc;
	
	oldPlanetDesc= pSolarSysState->pBaseDesc;

	// For now, only the name is printed. Other data may be added later.
	pSolarSysState->pBaseDesc = (PLANET_DESC *) planet;
	(*pSolarSysState->GenFunc) (GENERATE_NAME);
	fprintf (out, "- %s\n", GLOBAL_SIS (PlanetName));

	pSolarSysState->pBaseDesc = oldPlanetDesc;

	(void) flags;
}


