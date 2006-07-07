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

#ifndef _RACES_H
#define _RACES_H

#include "units.h"
#include "element.h"
#include "libs/compiler.h"
#include "libs/sndlib.h"


#define RACES_PER_PLAYER 7
#define MAX_SHIPS_PER_SIDE 14

/* heat of battle specific flags */
#define LEFT                   (1 << 0)
#define RIGHT                  (1 << 1)
#define THRUST                 (1 << 2)
#define WEAPON                 (1 << 3)
#define SPECIAL                (1 << 4)
#define LOW_ON_ENERGY          (1 << 5)
#define SHIP_BEYOND_MAX_SPEED  (1 << 6)
#define SHIP_AT_MAX_SPEED      (1 << 7)
#define SHIP_IN_GRAVITY_WELL   (1 << 8)
#define PLAY_VICTORY_DITTY     (1 << 9)

/* ship specific flags */
/*
#define GOOD_GUY  (1 << 0)
#define BAD_GUY   (1 << 1)
*/
#define SEEKING_WEAPON    (1 << 2)
#define SEEKING_SPECIAL   (1 << 3)
#define POINT_DEFENSE     (1 << 4)
		/* Ship has some point-defense capabilities */
#define IMMEDIATE_WEAPON  (1 << 5)
#define CREW_IMMUNE       (1 << 6)
#define FIRES_FORE        (1 << 7)
#define FIRES_RIGHT       (1 << 8)
#define FIRES_AFT         (1 << 9)
#define FIRES_LEFT        (1 << 10)
#define SHIELD_DEFENSE    (1 << 11)
#define DONT_CHASE        (1 << 12)
#define PLAYER_CAPTAIN    (1 << 13)
		/* The protagonist himself is on board. He gets a different color. */


typedef struct captain_stuff
{
	FRAME background;
	FRAME turn;
	FRAME thrust;
	FRAME weapon;
	FRAME special;
} CAPTAIN_STUFF;
typedef CAPTAIN_STUFF *PCAPTAIN_STUFF;
#define CAPTAIN_STUFFPTR PCAPTAIN_STUFF

typedef void (IntelligenceFunc) (PVOID ShipPtr, PVOID ObjectsOfConcern,
		COUNT ConcernCounter);
typedef struct
{
	COUNT ManeuverabilityIndex;
	COUNT WeaponRange;
	IntelligenceFunc *intelligence_func;
} INTEL_STUFF;

typedef struct
{
	COUNT max_thrust,
	      thrust_increment;
	BYTE energy_regeneration,
	     weapon_energy_cost,
	     special_energy_cost,
	     energy_wait,
	     turn_wait,
	     thrust_wait,
	     weapon_wait,
	     special_wait,
	     ship_mass;
} CHARACTERISTIC_STUFF;
typedef CHARACTERISTIC_STUFF *PCHARACTERISTIC_STUFF;

typedef struct
{
	UWORD ship_flags;
	BYTE var1, var2;
	COUNT crew_level, max_crew;
			/* For ships in npc_built_ship_q, the value INFINITE_FLEET for
			 * crew_level indicates an infinite number of ships. */
	BYTE energy_level, max_energy;
	POINT loc;

	/* The fields above this line are included in queues in savegames,
	 * All fields, including those below, are saved in ENCOUNTER structures
	 * in savegames. The latter was probably not supposed to happen.
	 */

	STRING race_strings;
	FRAME icons, melee_icon;
#define INFINITE_FLEET ((COUNT) ~0)
} SHIP_INFO;
typedef SHIP_INFO *PSHIP_INFO;
#define SHIP_INFOPTR PSHIP_INFO

#define ship_cost var1
#define group_counter ship_flags

enum
{
	IN_ORBIT = 0,
	EXPLORE,
	FLEE,
	ON_STATION,

	IGNORE_FLAGSHIP = 1 << 2,
	REFORM_GROUP = 1 << 3
};
#define MAX_REVOLUTIONS 5

#define GET_RACE_ID(s)          ((s)->ShipInfo.var1)
#define SET_RACE_ID(s,v)        ((s)->ShipInfo.var1 = (v))
#define GET_GROUP_LOC(s)        LONIBBLE ((s)->ShipInfo.var2)
#define SET_GROUP_LOC(s,v)      ((s)->ShipInfo.var2 = \
		MAKE_BYTE ((v), HINIBBLE ((s)->ShipInfo.var2)))
#define GET_GROUP_MISSION(s)    HINIBBLE ((s)->ShipInfo.var2)
#define SET_GROUP_MISSION(s,v)  ((s)->ShipInfo.var2 = \
		MAKE_BYTE (LONIBBLE ((s)->ShipInfo.var2), (v)))
#define GET_GROUP_DEST(s)       LONIBBLE ((s)->ShipInfo.energy_level)
#define SET_GROUP_DEST(s,v)     ((s)->ShipInfo.energy_level = \
		MAKE_BYTE ((v), HINIBBLE ((s)->ShipInfo.energy_level)))
#define GET_ORBIT_LOC(s)        HINIBBLE ((s)->ShipInfo.energy_level)
#define SET_ORBIT_LOC(s,v)      ((s)->ShipInfo.energy_level = \
		MAKE_BYTE (LONIBBLE ((s)->ShipInfo.energy_level), (v)))
#define GET_GROUP_ID(s)         ((s)->ShipInfo.max_energy)
#define SET_GROUP_ID(s,v)       ((s)->ShipInfo.max_energy = (v))

#define STATION_RADIUS 1600
#define ORBIT_RADIUS 2400

typedef struct
{
	UWORD ship_flags;
	BYTE days_left;
			/* Days left before the fleet reachers 'dest_loc'. */
	BYTE growth_fract;
	COUNT crew_level, max_crew;
			/* For ships in npc_built_ship_q, the value INFINITE_FLEET for
			 * crew_level indicates an infinite number of ships. */
	BYTE energy_level, max_energy;
	POINT loc;
			/* Location of the fleet (center) */

	STRING race_strings;
			/* Race specific strings, see doc/devel/racestrings. */
	FRAME icons, melee_icon;

	/*   -== The fields below this line are included in savegames. ==-   */

	COUNT actual_strength;
			/* Measure for the size of the sphere of influence.
			 * 0 if there is none and no ships will be generated.
	         * '(COUNT) ~0' if there is none, and the ship generation
			 * is handled separately. */
	COUNT known_strength;
			/* Measure for the size of the sphere of influence when last
			 * checked the starmap.
			 * 0 if the race's SoI is not known. */
	POINT known_loc;
			/* Location of the SoI (center) when last checked
			 * the starmap. */

	BYTE growth_err_term;
	BYTE func_index;
			/* Function index defined in clock.h (the same as in SetEvent())
			 * for the function to call when the fleet reaches 'dest_loc'.
			 * '(BYTE) ~0' means no function to call. */
	POINT dest_loc;
			/* Location to which the fleet (center) is moving. */
} EXTENDED_SHIP_INFO;
typedef EXTENDED_SHIP_INFO *PEXTENDED_SHIP_INFO;

typedef struct
{
	FRAME ship[NUM_VIEWS];
	FRAME weapon[NUM_VIEWS];
	FRAME special[NUM_VIEWS];
	CAPTAIN_STUFF captain_control;
	DWORD victory_ditty;
	SOUND ship_sounds;
} DATA_STUFF;
typedef DATA_STUFF *PDATA_STUFF;
#define DATA_STUFFPTR PDATA_STUFF


typedef struct race_desc RACE_DESC;
typedef RACE_DESC *PRACE_DESC;
#define RACE_DESCPTR PRACE_DESC

typedef void (PREPROCESS_FUNC) (PELEMENT ElementPtr);
typedef void (POSTPROCESS_FUNC) (PELEMENT ElementPtr);
typedef COUNT (INIT_WEAPON_FUNC) (PELEMENT ElementPtr, HELEMENT Weapon[]);
typedef void (UNINIT_FUNC) (RACE_DESCPTR pRaceDesc);

struct race_desc
{
	SHIP_INFO ship_info _ALIGNED_ANY;
	CHARACTERISTIC_STUFF characteristics _ALIGNED_ANY;
	DATA_STUFF ship_data _ALIGNED_ANY;
	INTEL_STUFF cyborg_control _ALIGNED_ANY;

	UNINIT_FUNC *uninit_func
			_ALIGNED_ON(sizeof (UNINIT_FUNC *));
	PREPROCESS_FUNC *preprocess_func
			_ALIGNED_ON(sizeof (PREPROCESS_FUNC *));
	POSTPROCESS_FUNC *postprocess_func 
			_ALIGNED_ON(sizeof (POSTPROCESS_FUNC *));
	INIT_WEAPON_FUNC *init_weapon_func
			_ALIGNED_ON(sizeof (INIT_WEAPON_FUNC *));

	PVOID CodeRef _ALIGNED_ON(sizeof (PVOID));
};


typedef QUEUE_HANDLE HSTARSHIP;

typedef struct
{
	HSTARSHIP pred;
	HSTARSHIP succ;

	DWORD RaceResIndex;
	
	/* This field is abused to store other data when the ship
	 * is in GLOBAL(built_ship_q), GLOBDATA(npc_built_ship_q),
	 * or race_q[], namely the side this ship is on (accessed
	 * through StarShipPlayer()), and the captains name for
	 * the ship (accessed through StarShipCaptain()).
	 * These values are set using OwnStarShip(). */
	union {
		RACE_DESCPTR RaceDescPtr;
		struct {
			COUNT Player;
			BYTE Captain;
		} s;
	};

	BYTE captains_name_index;

	BYTE weapon_counter;
	BYTE special_counter;
	BYTE energy_counter;

	BYTE ship_input_state;
	UWORD cur_status_flags _ALIGNED_ON(sizeof (UWORD));
	UWORD old_status_flags _ALIGNED_ON(sizeof (UWORD));

	FRAME silhouette _ALIGNED_ON(sizeof (FRAME));
	HELEMENT hShip _ALIGNED_ON(sizeof (HELEMENT));
	COUNT ShipFacing _ALIGNED_ON(sizeof (COUNT));
} STARSHIP;
typedef STARSHIP *PSTARSHIP;
#define STARSHIPPTR PSTARSHIP

typedef struct
{
	HSTARSHIP pred;
	HSTARSHIP succ;

	DWORD RaceResIndex;

	/* This field is abused to store other data when the ship
	 * is in GLOBAL(built_ship_q), GLOBDATA(npc_built_ship_q),
	 * or race_q[], namely the side this ship is on (accessed
	 * through StarShipPlayer()), and the captains name for
	 * the ship (accessed through StarShipCaptain()).
	 * These values are set using OwnStarShip(). */
	union {
		RACE_DESCPTR RaceDescPtr;
		struct {
			COUNT Player;
			BYTE Captain;
		} s;
	};

	SHIP_INFO ShipInfo;
} SHIP_FRAGMENT;
typedef SHIP_FRAGMENT *PSHIP_FRAGMENT;
#define SHIP_FRAGMENTPTR PSHIP_FRAGMENT

typedef struct
{
	HSTARSHIP pred;
	HSTARSHIP succ;

	DWORD RaceResIndex;
	RACE_DESCPTR RaceDescPtr;
	EXTENDED_SHIP_INFO ShipInfo;
} EXTENDED_SHIP_FRAGMENT;
typedef EXTENDED_SHIP_FRAGMENT *PEXTENDED_SHIP_FRAGMENT;
#define EXTENDED_SHIP_FRAGMENTPTR PEXTENDED_SHIP_FRAGMENT

#define AllocStarShip(pq) AllocLink (pq)
#define LockStarShip(pq,h) (STARSHIPPTR)LockLink (pq, h)
#define UnlockStarShip(pq,h) UnlockLink (pq, h)
#define FreeStarShip(pq,h) FreeLink (pq, h)

enum
{
	ARILOU_SHIP,
	CHMMR_SHIP,
	HUMAN_SHIP,
	ORZ_SHIP,
	PKUNK_SHIP,
	SHOFIXTI_SHIP,
	SPATHI_SHIP,
	SUPOX_SHIP,
	THRADDASH_SHIP,
	UTWIG_SHIP,
	VUX_SHIP,
	YEHAT_SHIP,
	MELNORME_SHIP,
	DRUUGE_SHIP,
	ILWRATH_SHIP,
	MYCON_SHIP,
	SLYLANDRO_SHIP,
	UMGAH_SHIP,
	URQUAN_SHIP,
	ZOQFOTPIK_SHIP,

	SYREEN_SHIP,
	BLACK_URQUAN_SHIP,
	YEHAT_REBEL_SHIP,
	URQUAN_PROBE_SHIP,
	SAMATRA_SHIP = URQUAN_PROBE_SHIP,

	NUM_AVAILABLE_RACES
};

#define YEHAT_REBEL_CONVERSATION (~0L)

#define RACE_COMMUNICATION \
		ARILOU_CONVERSATION,       /* ARILOU_SHIP */ \
		CHMMR_CONVERSATION,        /* CHMMR_SHIP */ \
		0L,                        /* HUMAN_SHIP */ \
		ORZ_CONVERSATION,          /* ORZ_SHIP */ \
		PKUNK_CONVERSATION,        /* PKUNK_SHIP */ \
		SHOFIXTI_CONVERSATION,     /* SHOFIXTI_SHIP */ \
		SPATHI_CONVERSATION,       /* SPATHI_SHIP */ \
		SUPOX_CONVERSATION,        /* SUPOX_SHIP */ \
		THRADD_CONVERSATION,       /* THRADDASH_SHIP */ \
		UTWIG_CONVERSATION,        /* UTWIG_SHIP */ \
		VUX_CONVERSATION,          /* VUX_SHIP */ \
		YEHAT_CONVERSATION,        /* YEHAT_SHIP */ \
		MELNORME_CONVERSATION,     /* MELNORME_SHIP */ \
		DRUUGE_CONVERSATION,       /* DRUUGE_SHIP */ \
		ILWRATH_CONVERSATION,      /* ILWRATH_SHIP */ \
		MYCON_CONVERSATION,        /* MYCON_SHIP */ \
		SLYLANDRO_CONVERSATION,    /* SLYLANDRO_SHIP */ \
		UMGAH_CONVERSATION,        /* UMGAH_SHIP */ \
		URQUAN_CONVERSATION,       /* URQUAN_SHIP */ \
		ZOQFOTPIK_CONVERSATION,    /* ZOQFOTPIK_SHIP */ \
		0L,                        /* SYREEN_SHIP */ \
		BLACKURQ_CONVERSATION,     /* BLACK_URQUAN_SHIP */ \
		YEHAT_REBEL_CONVERSATION,  /* YEHAT_REBEL_SHIP */ \
		0L,                        /* URQUAN_PROBE_SHIP */

#define RACE_SHIP_COST \
		1600,  /* ARILOU_SHIP */ \
		3000,  /* CHMMR_SHIP */ \
		1100,  /* HUMAN_SHIP */ \
		2300,  /* ORZ_SHIP */ \
		2000,  /* PKUNK_SHIP */ \
		 500,  /* SHOFIXTI_SHIP */ \
		1800,  /* SPATHI_SHIP */ \
		1600,  /* SUPOX_SHIP */ \
		1000,  /* THRADDASH_SHIP */ \
		2200,  /* UTWIG_SHIP */ \
		1200,  /* VUX_SHIP */ \
		2300,  /* YEHAT_SHIP */ \
		3600,  /* MELNORME_SHIP */ \
		1700,  /* DRUUGE_SHIP */ \
		1000,  /* ILWRATH_SHIP */ \
		2100,  /* MYCON_SHIP */ \
		4400,  /* SLYLANDRO_SHIP */ \
		 700,  /* UMGAH_SHIP */ \
		3000,  /* URQUAN_SHIP */ \
		 600,  /* ZOQFOTPIK_SHIP */ \
		1300,  /* SYREEN_SHIP */ \
		3000,  /* BLACK_URQUAN_SHIP */ \
		2300,  /* YEHAT_REBEL_SHIP */

#define LOG_TO_IP(s) ((s) << 1)
#define RACE_IP_SPEED \
		LOG_TO_IP (40),  /* ARILOU_SHIP */ \
		LOG_TO_IP (27),  /* CHMMR_SHIP */ \
		LOG_TO_IP (24),  /* HUMAN_SHIP */ \
		LOG_TO_IP (40),  /* ORZ_SHIP */ \
		LOG_TO_IP (40),  /* PKUNK_SHIP */ \
		LOG_TO_IP (35),  /* SHOFIXTI_SHIP */ \
		LOG_TO_IP (48),  /* SPATHI_SHIP */ \
		LOG_TO_IP (40),  /* SUPOX_SHIP */ \
		LOG_TO_IP (28),  /* THRADDASH_SHIP */ \
		LOG_TO_IP (30),  /* UTWIG_SHIP */ \
		LOG_TO_IP (21),  /* VUX_SHIP */ \
		LOG_TO_IP (30),  /* YEHAT_SHIP */ \
		LOG_TO_IP (40),  /* MELNORME_SHIP */ \
		LOG_TO_IP (20),  /* DRUUGE_SHIP */ \
		LOG_TO_IP (25),  /* ILWRATH_SHIP */ \
		LOG_TO_IP (27),  /* MYCON_SHIP */ \
		LOG_TO_IP (60),  /* SLYLANDRO_SHIP */ \
		LOG_TO_IP (18),  /* UMGAH_SHIP */ \
		LOG_TO_IP (30),  /* URQUAN_SHIP */ \
		LOG_TO_IP (40),  /* ZOQFOTPIK_SHIP */ \
		LOG_TO_IP (36),  /* SYREEN_SHIP */ \
		LOG_TO_IP (30),  /* BLACK_URQUAN_SHIP */ \
		LOG_TO_IP (30),  /* YEHAT_REBEL_SHIP */ \
		LOG_TO_IP (90),  /* URQUAN_PROBE_SHIP */

#define LOG_TO_HYPER(s) (WORLD_TO_VELOCITY (s) >> 1)
#define RACE_HYPER_SPEED \
		LOG_TO_HYPER (40),  /* ARILOU_SHIP */ \
		LOG_TO_HYPER (27),  /* CHMMR_SHIP */ \
		LOG_TO_HYPER (24),  /* HUMAN_SHIP */ \
		LOG_TO_HYPER (40),  /* ORZ_SHIP */ \
		LOG_TO_HYPER (40),  /* PKUNK_SHIP */ \
		LOG_TO_HYPER (35),  /* SHOFIXTI_SHIP */ \
		LOG_TO_HYPER (48),  /* SPATHI_SHIP */ \
		LOG_TO_HYPER (40),  /* SUPOX_SHIP */ \
		LOG_TO_HYPER (50),  /* THRADDASH_SHIP */ \
		LOG_TO_HYPER (30),  /* UTWIG_SHIP */ \
		LOG_TO_HYPER (21),  /* VUX_SHIP */ \
		LOG_TO_HYPER (30),  /* YEHAT_SHIP */ \
		LOG_TO_HYPER (40),  /* MELNORME_SHIP */ \
		LOG_TO_HYPER (20),  /* DRUUGE_SHIP */ \
		LOG_TO_HYPER (25),  /* ILWRATH_SHIP */ \
		LOG_TO_HYPER (27),  /* MYCON_SHIP */ \
		LOG_TO_HYPER (60),  /* SLYLANDRO_SHIP */ \
		LOG_TO_HYPER (18),  /* UMGAH_SHIP */ \
		LOG_TO_HYPER (30),  /* URQUAN_SHIP */ \
		LOG_TO_HYPER (40),  /* ZOQFOTPIK_SHIP */ \
		LOG_TO_HYPER (36),  /* SYREEN_SHIP */ \
		LOG_TO_HYPER (30),  /* BLACK_URQUAN_SHIP */ \
		LOG_TO_HYPER (30),  /* YEHAT_REBEL_SHIP */

#define RACE_HYPERSPACE_PERCENT \
		20,  /* ARILOU_SHIP */ \
		 0,  /* CHMMR_SHIP */ \
		 0,  /* HUMAN_SHIP */ \
		20,  /* ORZ_SHIP */ \
		40,  /* PKUNK_SHIP */ \
		 0,  /* SHOFIXTI_SHIP */ \
		20,  /* SPATHI_SHIP */ \
		40,  /* SUPOX_SHIP */ \
		60,  /* THRADDASH_SHIP */ \
		40,  /* UTWIG_SHIP */ \
		40,  /* VUX_SHIP */ \
		60,  /* YEHAT_SHIP */ \
		 0,  /* MELNORME_SHIP */ \
		30,  /* DRUUGE_SHIP */ \
		60,  /* ILWRATH_SHIP */ \
		40,  /* MYCON_SHIP */ \
		 2,  /* SLYLANDRO_SHIP */ \
		30,  /* UMGAH_SHIP */ \
		70,  /* URQUAN_SHIP */ \
		 0,  /* ZOQFOTPIK_SHIP */ \
		 0,  /* SYREEN_SHIP */ \
		70,  /* BLACK_URQUAN_SHIP */ \
		60,  /* YEHAT_REBEL_SHIP */ \
		 0,  /* URQUAN_PROBE_SHIP */

#define RACE_INTERPLANETARY_PERCENT \
		 0,  /* ARILOU_SHIP */ \
		 0,  /* CHMMR_SHIP */ \
		 0,  /* HUMAN_SHIP */ \
		20,  /* ORZ_SHIP */ \
		20,  /* PKUNK_SHIP */ \
		 0,  /* SHOFIXTI_SHIP */ \
		10,  /* SPATHI_SHIP */ \
		20,  /* SUPOX_SHIP */ \
		20,  /* THRADDASH_SHIP */ \
		20,  /* UTWIG_SHIP */ \
		20,  /* VUX_SHIP */ \
		40,  /* YEHAT_SHIP */ \
		 0,  /* MELNORME_SHIP */ \
		20,  /* DRUUGE_SHIP */ \
		60,  /* ILWRATH_SHIP */ \
		20,  /* MYCON_SHIP */ \
		 5,  /* SLYLANDRO_SHIP */ \
		20,  /* UMGAH_SHIP */ \
		40,  /* URQUAN_SHIP */ \
		 0,  /* ZOQFOTPIK_SHIP */ \
		 0,  /* SYREEN_SHIP */ \
		40,  /* BLACK_URQUAN_SHIP */ \
		40,  /* YEHAT_REBEL_SHIP */ \
		 0,  /* URQUAN_PROBE_SHIP */

// How many ships will an encounter consist of.
// The first number specifies the minimum, the second the maximum.
// The chance is 50% for each ship past the minimum to be present.
#define RACE_ENCOUNTER_MAKEUP \
		MAKE_BYTE (1, 5),  /* ARILOU_SHIP */ \
		0,                 /* CHMMR_SHIP */ \
		0,                 /* HUMAN_SHIP */ \
		MAKE_BYTE (1, 5),  /* ORZ_SHIP */ \
		MAKE_BYTE (1, 5),  /* PKUNK_SHIP */ \
		0,                 /* SHOFIXTI_SHIP */ \
		MAKE_BYTE (1, 5),  /* SPATHI_SHIP */ \
		MAKE_BYTE (1, 5),  /* SUPOX_SHIP */ \
		MAKE_BYTE (1, 5),  /* THRADDASH_SHIP */ \
		MAKE_BYTE (1, 5),  /* UTWIG_SHIP */ \
		MAKE_BYTE (1, 5),  /* VUX_SHIP */ \
		MAKE_BYTE (1, 5),  /* YEHAT_SHIP */ \
		MAKE_BYTE (1, 1),  /* MELNORME_SHIP */ \
		MAKE_BYTE (1, 5),  /* DRUUGE_SHIP */ \
		MAKE_BYTE (1, 5),  /* ILWRATH_SHIP */ \
		MAKE_BYTE (1, 5),  /* MYCON_SHIP */ \
		MAKE_BYTE (1, 1),  /* SLYLANDRO_SHIP */ \
		MAKE_BYTE (1, 5),  /* UMGAH_SHIP */ \
		MAKE_BYTE (1, 5),  /* URQUAN_SHIP */ \
		MAKE_BYTE (1, 5),  /* ZOQFOTPIK_SHIP */ \
		0,                 /* SYREEN_SHIP */ \
		MAKE_BYTE (1, 5),  /* BLACK_URQUAN_SHIP */ \
		MAKE_BYTE (1, 5),  /* YEHAT_REBEL_SHIP */

#define RACE_COLORS \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x10), 0x53),  /* ARILOU_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00),  /* CHMMR_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x01, 0x1f), 0x4D),  /* HUMAN_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x0E, 0x00, 0x0E), 0x36),  /* ORZ_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x06, 0x08), 0x62),  /* PKUNK_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00),  /* SHOFIXTI_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x0C, 0x05, 0x00), 0x76),  /* SPATHI_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x0C, 0x05, 0x00), 0x76),  /* SUPOX_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x06, 0x08), 0x62),  /* THRADDASH_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x06, 0x08), 0x62),  /* UTWIG_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x10), 0x53),  /* VUX_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x00, 0x11), 0x3D),  /* YEHAT_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x06, 0x08), 0x62),  /* MELNORME_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x0F, 0x00, 0x00), 0x2D),  /* DRUUGE_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x0E, 0x00, 0x0E), 0x36),  /* ILWRATH_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x0E, 0x00, 0x0E), 0x36),  /* MYCON_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x0C, 0x05, 0x00), 0x76),  /* SLYLANDRO_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x00, 0x11), 0x3D),  /* UMGAH_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x08, 0x00), 0x6E),  /* URQUAN_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x0F, 0x00, 0x00), 0x2D),  /* ZOQFOTPIK_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00),  /* SYREEN_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x06, 0x06, 0x06), 0x20),  /* BLACK_URQUAN_SHIP */ \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x07, 0x1F), 0x39),  /* YEHAT_REBEL_SHIP */

#define RADAR_SCAN_WIDTH (UNIT_SCREEN_WIDTH * NUM_RADAR_SCREENS)
#define RADAR_SCAN_HEIGHT (UNIT_SCREEN_HEIGHT * NUM_RADAR_SCREENS)

// Hyperspace coordinates of the naturally occuring portal into QuasiSpace
#define ARILOU_SPACE_X   438
#define ARILOU_SPACE_Y  6372

// QuasiSpace coordinates of the same portal
#define QUASI_SPACE_X  5000
#define QUASI_SPACE_Y  5000

// QuasiSpace coordinates of the Arilou home world
#define ARILOU_HOME_X  (QUASI_SPACE_X + ((RADAR_SCAN_WIDTH >> 1) * 3))
#define ARILOU_HOME_Y  (QUASI_SPACE_Y + ((RADAR_SCAN_HEIGHT >> 1) * 3))

// Hyperspace coordinates of the Sol system
// Should be the same as in plandata.c
#define SOL_X  1752
#define SOL_Y  1450

extern BOOLEAN InitKernel (void);

extern void DrawCaptainsWindow (STARSHIPPTR
		StarShipPtr);
extern BOOLEAN GetNextStarShip (STARSHIPPTR
		LastStarShipPtr, COUNT which_side);
extern HSTARSHIP GetEncounterStarShip (STARSHIPPTR
		LastStarShipPtr, COUNT which_player);
extern void DrawArmadaPickShip (BOOLEAN
		draw_salvage_frame, PRECT pPickRect);

extern BOOLEAN load_animation (PFRAME pixarray, DWORD
		big_res, DWORD med_res, DWORD sml_res);
extern BOOLEAN free_image (PFRAME pixarray);
extern void NotifyOthers (COUNT which_race, BYTE
		target_loc);

#endif /* _RACES_H */

