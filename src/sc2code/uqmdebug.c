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

#ifdef DEBUG
		/* This file is not even looked at in the unix debug build,
		 * but MSVC will compile and link it.
		 */

#include <stdio.h>

#include "starcon.h"
#include "planets/plandata.h"
#include "uqmdebug.h"

static void dumpEventCallback (EVENTPTR eventPtr, void *arg);
static void dumpStarCallback(STAR_DESC *star, void *arg);
static void dumpPlanetCallback (PLANET_DESC *planet, void *arg);
static void dumpPlanetTypeCallback (int index, const PlanetFrame *planet,
		void *arg);

BOOLEAN instantMove = FALSE;

void
debugKeyPressed (void)
{
	// State modifying:
	equipShip ();
	instantMove = !instantMove;
	showSpheres ();
//	forwardToNextEvent (TRUE);		

	// Informational:
//	dumpEvents (stderr);
//	dumpPlanetTypes(stderr);
//	dumpStars (stderr, DUMP_PLANETS);
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

void
forAllStars (void (*callback) (STAR_DESC *, void *), void *arg)
{
	int i;
	extern STAR_DESC starmap_array[];

	for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
		callback (&starmap_array[i], arg);
}
	
typedef struct
{
	FILE *out;
	UWORD flags;
} DumpStarsArg;

void
dumpStars (FILE *out, UWORD flags)
{
	DumpStarsArg dumpStarsArg;
	dumpStarsArg.out = out;
	dumpStarsArg.flags = flags;

	forAllStars (dumpStarCallback, (void *) &dumpStarsArg);
}

static void
dumpStarCallback (STAR_DESC *star, void *arg)
{
	DumpStarsArg *dumpStarsArg = (DumpStarsArg *) arg;

	dumpStar(dumpStarsArg->out, star, dumpStarsArg->flags);
}

void
dumpStar (FILE *out, const STAR_DESC *star, UWORD flags)
{
	UNICODE name[40];
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
	if (flags & DUMP_PLANETS)
		dumpPlanets (out, star, flags);
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

// Currently, this function only works when called from the top of
// ExploreSolarSys(). The state afterwards is undefined.
void
forAllPlanets (STAR_DESC *star,
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

typedef struct
{
	FILE *out;
	UWORD flags;
} DumpPlanetsArg;

void
dumpPlanets (FILE *out, const STAR_DESC *star, UWORD flags)
{
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
dumpPlanet (FILE *out, const PLANET_DESC *planet, UWORD flags)
{
	PLANET_DESC *oldPlanetDesc;
	
	oldPlanetDesc= pSolarSysState->pBaseDesc;

	// For now, only the name is printed. Other data may be added later.
	pSolarSysState->pBaseDesc = (PLANET_DESC *) planet;
	(*pSolarSysState->GenFunc) (GENERATE_NAME);
	fprintf (out, "- %-37s  %s\n", GLOBAL_SIS (PlanetName),
			planetTypeString (planet->data_index));

	pSolarSysState->pBaseDesc = oldPlanetDesc;

	(void) flags;
}

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

#endif  /* DEBUG */


