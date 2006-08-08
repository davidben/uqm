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

#include <assert.h>

#include "load.h"

#include "build.h"
#include "declib.h"
#include "encount.h"
#include "file.h"
#include "globdata.h"
#include "load.h"
#include "options.h"
#include "setup.h"
#include "state.h"

#include "libs/tasklib.h"
#include "libs/log.h"

//#define DEBUG_LOAD

ACTIVITY NextActivity;

// XXX: these should handle endian conversions later
static inline COUNT
cread_8 (DECODE_REF fh, PBYTE v)
{
	BYTE t;
	if (!v) /* read value ignored */
		v = &t;
	return cread (v, 1, 1, fh);
}

static inline COUNT
cread_16 (DECODE_REF fh, PUWORD v)
{
	UWORD t;
	if (!v) /* read value ignored */
		v = &t;
	return cread (v, 2, 1, fh);
}

static inline COUNT
cread_32 (DECODE_REF fh, PDWORD v)
{
	DWORD t;
	if (!v) /* read value ignored */
		v = &t;
	return cread (v, 4, 1, fh);
}

static inline COUNT
cread_ptr (DECODE_REF fh)
{
	DWORD t;
	return cread_32 (fh, &t); /* ptrs are useless in saves */
}

static inline COUNT
cread_a8 (DECODE_REF fh, PBYTE ar, COUNT count)
{
	assert (ar != NULL);
	return cread (ar, 1, count, fh) == count;
}

static inline COUNT
read_8 (PVOID fp, PBYTE v)
{
	BYTE t;
	if (!v) /* read value ignored */
		v = &t;
	return ReadResFile (v, 1, 1, fp);
}

static inline COUNT
read_16 (PVOID fp, PUWORD v)
{
	UWORD t;
	if (!v) /* read value ignored */
		v = &t;
	return ReadResFile (v, 2, 1, fp);
}

static inline COUNT
read_32 (PVOID fp, PDWORD v)
{
	DWORD t;
	if (!v) /* read value ignored */
		v = &t;
	return ReadResFile (v, 4, 1, fp);
}

static inline COUNT
read_ptr (PVOID fp)
{
	DWORD t;
	return read_32 (fp, &t); /* ptrs are useless in saves */
}

static inline COUNT
read_a8 (PVOID fp, PBYTE ar, COUNT count)
{
	assert (ar != NULL);
	return ReadResFile (ar, 1, count, fp) == count;
}

static inline COUNT
read_a16 (PVOID fp, PUWORD ar, COUNT count)
{
	assert (ar != NULL);

	for ( ; count > 0; --count, ++ar)
	{
		if (read_16 (fp, ar) != 1)
			return 0;
	}
	return 1;
}

static void
LoadShipQueue (DECODE_REF fh, PQUEUE pQueue, BOOLEAN MakeQ)
{
	COUNT num_links;

	cread_16 (fh, &num_links);
	if (num_links && MakeQ)
		InitQueue (pQueue, num_links, sizeof (SHIP_FRAGMENT));

	while (num_links--)
	{
		HSTARSHIP hStarShip;
		SHIP_FRAGMENTPTR FragPtr;
		COUNT Index;
		BYTE tmpb;

		cread_16 (fh, &Index);

		if (pQueue == &GLOBAL (avail_race_q))
			hStarShip = GetStarShipFromIndex (pQueue, Index);
		else
			hStarShip = CloneShipFragment (Index, pQueue, 0);

		FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (pQueue, hStarShip);

		if (pQueue != &GLOBAL (avail_race_q))
		{	// queues other than avail_race_q save SHIP_FRAGMENT elements
			// Read SHIP_FRAGMENT elements
			cread_16 (fh, &FragPtr->s.Player);
			cread_8  (fh, &FragPtr->s.Captain);
			cread_8  (fh, NULL); /* padding */
		}
		// Read SHIP_INFO elements
		cread_16 (fh, &FragPtr->ShipInfo.ship_flags);
		cread_8  (fh, &FragPtr->ShipInfo.var1);
		cread_8  (fh, &FragPtr->ShipInfo.var2);
		cread_8  (fh, &tmpb);
		FragPtr->ShipInfo.crew_level = tmpb;
		cread_8  (fh, &tmpb);
		FragPtr->ShipInfo.max_crew = tmpb;
		cread_8  (fh, &FragPtr->ShipInfo.energy_level);
		cread_8  (fh, &FragPtr->ShipInfo.max_energy);
		cread_16 (fh, &FragPtr->ShipInfo.loc.x);
		cread_16 (fh, &FragPtr->ShipInfo.loc.y);

		if (pQueue == &GLOBAL (avail_race_q))
		{
			// avail_race_q contains information not about specific ships,
			// but about a race.
			EXTENDED_SHIP_FRAGMENTPTR ExtFragPtr =
					(EXTENDED_SHIP_FRAGMENTPTR) FragPtr;

			cread_16 (fh, &ExtFragPtr->ShipInfo.actual_strength);
			cread_16 (fh, &ExtFragPtr->ShipInfo.known_strength);
			cread_16 (fh, &ExtFragPtr->ShipInfo.known_loc.x);
			cread_16 (fh, &ExtFragPtr->ShipInfo.known_loc.y);
			cread_8  (fh, &ExtFragPtr->ShipInfo.growth_err_term);
			cread_8  (fh, &ExtFragPtr->ShipInfo.func_index);
			cread_16 (fh, &ExtFragPtr->ShipInfo.dest_loc.x);
			cread_16 (fh, &ExtFragPtr->ShipInfo.dest_loc.y);
			cread_16 (fh, NULL); /* alignment padding */
		}

		UnlockStarShip (pQueue, hStarShip);
	}
}

static void
LoadEncounter (ENCOUNTERPTR EncounterPtr, DECODE_REF fh)
{
	COUNT i;

	cread_ptr (fh); /* useless ptr; HENCOUNTER pred */
	EncounterPtr->pred = 0;
	cread_ptr (fh); /* useless ptr; HENCOUNTER succ */
	EncounterPtr->succ = 0;
	cread_ptr (fh); /* useless ptr; HELEMENT hElement */
	EncounterPtr->hElement = 0;
	cread_16  (fh, &EncounterPtr->transition_state);
	cread_16  (fh, &EncounterPtr->origin.x);
	cread_16  (fh, &EncounterPtr->origin.y);
	cread_16  (fh, &EncounterPtr->radius);
	// EXTENDED_STAR_DESC fields
	cread_16  (fh, &EncounterPtr->SD.star_pt.x);
	cread_16  (fh, &EncounterPtr->SD.star_pt.y);
	cread_8   (fh, &EncounterPtr->SD.Type);
	cread_8   (fh, &EncounterPtr->SD.Index);
	cread_16  (fh, NULL); /* alignment padding */

	// Load each entry in the SHIP_INFO array:
	for (i = 0; i < MAX_HYPER_SHIPS; i++)
	{
		SHIP_INFOPTR ShipInfo = &EncounterPtr->SD.ShipList[i];
		BYTE tmpb;

		cread_16  (fh, &ShipInfo->ship_flags);
		cread_8   (fh, &ShipInfo->var1);
		cread_8   (fh, &ShipInfo->var2);
		cread_8   (fh, &tmpb);
		ShipInfo->crew_level = tmpb;
		cread_8   (fh, &tmpb);
		ShipInfo->max_crew = tmpb;
		cread_8   (fh, &ShipInfo->energy_level);
		cread_8   (fh, &ShipInfo->max_energy);
		cread_16  (fh, &ShipInfo->loc.x);
		cread_16  (fh, &ShipInfo->loc.y);
		cread_32  (fh, NULL); /* useless val; STRING race_strings */
		ShipInfo->race_strings = 0;
		cread_ptr (fh); /* useless ptr; FRAME icons */
		ShipInfo->icons = 0;
		cread_ptr (fh); /* useless ptr; FRAME melee_icon */
		ShipInfo->melee_icon = 0;
	}
	
	// Load the stuff after the SHIP_INFO array:
	cread_32  (fh, &EncounterPtr->log_x);
	cread_32  (fh, &EncounterPtr->log_y);
}

static void
LoadEvent (EVENTPTR EventPtr, DECODE_REF fh)
{
	cread_ptr (fh); /* useless ptr; HEVENT pred */
	EventPtr->pred = 0;
	cread_ptr (fh); /* useless ptr; HEVENT succ */
	EventPtr->succ = 0;
	cread_8   (fh, &EventPtr->day_index);
	cread_8   (fh, &EventPtr->month_index);
	cread_16  (fh, &EventPtr->year_index);
	cread_8   (fh, &EventPtr->func_index);
	cread_8   (fh, NULL); /* padding */
	cread_16  (fh, NULL); /* padding */
}

static void
DummyLoadQueue (PQUEUE QueuePtr, DECODE_REF fh)
{
	/* QUEUE should never actually be loaded since it contains
	 * purely internal representation and the lists
	 * involved are actually loaded separately */
	(void)QueuePtr; /* silence compiler */

	/* QUEUE format with QUEUE_TABLE defined -- UQM default */
	cread_ptr (fh); /* HLINK head */
	cread_ptr (fh); /* HLINK tail */
	cread_ptr (fh); /* PBYTE pq_tab */
	cread_ptr (fh); /* HLINK free_list */
	cread_16  (fh, NULL); /* MEM_HANDLE hq_tab */
	cread_16  (fh, NULL); /* COUNT object_size */
	cread_8   (fh, NULL); /* BYTE num_objects */
	
	cread_8   (fh, NULL); /* padding */
	cread_16  (fh, NULL); /* padding */
}

static void
LoadClockState (PCLOCK_STATE ClockPtr, DECODE_REF fh)
{
	cread_8   (fh, &ClockPtr->day_index);
	cread_8   (fh, &ClockPtr->month_index);
	cread_16  (fh, &ClockPtr->year_index);
	cread_16  (fh, &ClockPtr->tick_count);
	cread_16  (fh, &ClockPtr->day_in_ticks);
	cread_ptr (fh); /* not loading ptr; Semaphore clock_sem */
	cread_ptr (fh); /* not loading ptr; Task clock_task */
	cread_32  (fh, &ClockPtr->TimeCounter); /* theoretically useless */

	DummyLoadQueue (&ClockPtr->event_q, fh);
}

static void
LoadGameState (PGAME_STATE GSPtr, DECODE_REF fh)
{
	DWORD tmpd;

	cread_8   (fh, &GSPtr->cur_state); /* obsolete */
	cread_8   (fh, &GSPtr->glob_flags);
	cread_8   (fh, &GSPtr->CrewCost);
	cread_8   (fh, &GSPtr->FuelCost);
	cread_a8  (fh, GSPtr->ModuleCost, NUM_MODULES);
	cread_a8  (fh, GSPtr->ElementWorth, NUM_ELEMENT_CATEGORIES);
	cread_ptr (fh); /* not loading ptr; PPRIMITIVE DisplayArray */
	cread_16  (fh, &GSPtr->CurrentActivity);
	
	cread_16  (fh, NULL); /* CLOCK_STATE alignment padding */
	LoadClockState (&GSPtr->GameClock, fh);

	cread_16  (fh, &GSPtr->autopilot.x);
	cread_16  (fh, &GSPtr->autopilot.y);
	cread_16  (fh, &GSPtr->ip_location.x);
	cread_16  (fh, &GSPtr->ip_location.y);
	/* STAMP ShipStamp */
	cread_16  (fh, &GSPtr->ShipStamp.origin.x);
	cread_16  (fh, &GSPtr->ShipStamp.origin.y);
	cread_32  (fh, &tmpd); /* abused ptr to store DWORD */
	GSPtr->ShipStamp.frame = (FRAME)tmpd;

	/* VELOCITY_DESC velocity */
	cread_16  (fh, &GSPtr->velocity.TravelAngle);
	cread_16  (fh, &GSPtr->velocity.vector.width);
	cread_16  (fh, &GSPtr->velocity.vector.height);
	cread_16  (fh, &GSPtr->velocity.fract.width);
	cread_16  (fh, &GSPtr->velocity.fract.height);
	cread_16  (fh, &GSPtr->velocity.error.width);
	cread_16  (fh, &GSPtr->velocity.error.height);
	cread_16  (fh, &GSPtr->velocity.incr.width);
	cread_16  (fh, &GSPtr->velocity.incr.height);
	cread_16  (fh, NULL); /* VELOCITY_DESC padding */

	cread_32  (fh, &GSPtr->BattleGroupRef);
	
	DummyLoadQueue (&GSPtr->avail_race_q, fh);
	DummyLoadQueue (&GSPtr->npc_built_ship_q, fh);
	DummyLoadQueue (&GSPtr->encounter_q, fh);
	DummyLoadQueue (&GSPtr->built_ship_q, fh);

	cread_a8  (fh, GSPtr->GameState, sizeof (GSPtr->GameState));

	assert (sizeof (GSPtr->GameState) % 4 == 3);
	cread_8  (fh, NULL); /* GAME_STATE alignment padding */
}

static BOOLEAN
LoadSisState (PSIS_STATE SSPtr, PVOID fp)
{
	if (
			read_32  (fp, &SSPtr->log_x) != 1 ||
			read_32  (fp, &SSPtr->log_y) != 1 ||
			read_32  (fp, &SSPtr->ResUnits) != 1 ||
			read_32  (fp, &SSPtr->FuelOnBoard) != 1 ||
			read_16  (fp, &SSPtr->CrewEnlisted) != 1 ||
			read_16  (fp, &SSPtr->TotalElementMass) != 1 ||
			read_16  (fp, &SSPtr->TotalBioMass) != 1 ||
			read_a8  (fp, SSPtr->ModuleSlots, NUM_MODULE_SLOTS) != 1 ||
			read_a8  (fp, SSPtr->DriveSlots, NUM_DRIVE_SLOTS) != 1 ||
			read_a8  (fp, SSPtr->JetSlots, NUM_JET_SLOTS) != 1 ||
			read_8   (fp, &SSPtr->NumLanders) != 1 ||
			read_a16 (fp, SSPtr->ElementAmounts, NUM_ELEMENT_CATEGORIES) != 1 ||

			read_a8  (fp, SSPtr->ShipName, SIS_NAME_SIZE) != 1 ||
			read_a8  (fp, SSPtr->CommanderName, SIS_NAME_SIZE) != 1 ||
			read_a8  (fp, SSPtr->PlanetName, SIS_NAME_SIZE) != 1 ||

			read_16  (fp, NULL) != 1 /* padding */
		)
		return FALSE;
	else
		return TRUE;
}

static BOOLEAN
LoadSummary (SUMMARY_DESC *SummPtr, PVOID fp)
{
	if (!LoadSisState (&SummPtr->SS, fp))
		return FALSE;

	if (
			read_8  (fp, &SummPtr->Activity) != 1 ||
			read_8  (fp, &SummPtr->Flags) != 1 ||
			read_8  (fp, &SummPtr->day_index) != 1 ||
			read_8  (fp, &SummPtr->month_index) != 1 ||
			read_16 (fp, &SummPtr->year_index) != 1 ||
			read_8  (fp, &SummPtr->MCreditLo) != 1 ||
			read_8  (fp, &SummPtr->MCreditHi) != 1 ||
			read_8  (fp, &SummPtr->NumShips) != 1 ||
			read_8  (fp, &SummPtr->NumDevices) != 1 ||
			read_a8 (fp, SummPtr->ShipList, MAX_BUILT_SHIPS) != 1 ||
			read_a8 (fp, SummPtr->DeviceList, MAX_EXCLUSIVE_DEVICES) != 1 ||

			read_16  (fp, NULL) != 1 /* padding */
		)
		return FALSE;
	else
		return TRUE;
}

static void
LoadStarDesc (STAR_DESCPTR SDPtr, DECODE_REF fh)
{
	cread_16 (fh, &SDPtr->star_pt.x);
	cread_16 (fh, &SDPtr->star_pt.y);
	cread_8  (fh, &SDPtr->Type);
	cread_8  (fh, &SDPtr->Index);
	cread_8  (fh, &SDPtr->Prefix);
	cread_8  (fh, &SDPtr->Postfix);
}

BOOLEAN
LoadGame (COUNT which_game, SUMMARY_DESC *SummPtr)
{
	uio_Stream *in_fp;
	char file[PATH_MAX];
	char buf[256];
	SUMMARY_DESC loc_sd;
	GAME_STATE_FILE *fp;
	DECODE_REF fh;
	COUNT num_links;
	STAR_DESC SD;
	ACTIVITY Activity;

	sprintf (file, "starcon2.%02u", which_game);
	in_fp = res_OpenResFile (saveDir, file, "rb");
	if (!in_fp)
		return FALSE;

	if (!LoadSummary (&loc_sd, in_fp))
	{
		log_add (log_Error, "Warning: Savegame is corrupt");
		res_CloseResFile (in_fp);
		return FALSE;
	}

	if (!SummPtr)
	{
		SummPtr = &loc_sd;
	}
	else
	{	// only need summary for displaying to user
		memcpy (SummPtr, &loc_sd, sizeof (*SummPtr));
		res_CloseResFile (in_fp);
		return TRUE;
	}

	// Crude check for big-endian/little-endian incompatibilities.
	// year_index is suitable as it's a multi-byte value within
	// a specific recognisable range.
	if (SummPtr->year_index < START_YEAR ||
			SummPtr->year_index >= START_YEAR +
			YEARS_TO_KOHRAH_VICTORY + 1 /* Utwig intervention */ +
			1 /* time to destroy all races, plenty */ +
			25 /* for cheaters */)
	{
		log_add (log_Error, "Warning: Savegame corrupt or from "
				"an incompatible platform.");
		res_CloseResFile (in_fp);
		return FALSE;
	}

	GlobData.SIS_state = SummPtr->SS;

	if ((fh = copen (in_fp, FILE_STREAM, STREAM_READ)) == 0)
	{
		res_CloseResFile (in_fp);
		return FALSE;
	}

	ReinitQueue (&GLOBAL (GameClock.event_q));
	ReinitQueue (&GLOBAL (encounter_q));
	ReinitQueue (&GLOBAL (npc_built_ship_q));
	ReinitQueue (&GLOBAL (built_ship_q));

	memset (&GLOBAL (GameState[0]), 0, sizeof (GLOBAL (GameState)));
	Activity = GLOBAL (CurrentActivity);
	LoadGameState (&GlobData.Game_state, fh);
	NextActivity = GLOBAL (CurrentActivity);
	GLOBAL (CurrentActivity) = Activity;

	// It shouldn't be possible to ever save with TimeCounter != 0
	// But if it does happen, it needs to be reset to 0, since on load
	// the clock semaphore is gauranteed to be 0
	if (GLOBAL (GameClock.TimeCounter) != 0)
		log_add (log_Warning, "Warning: Game clock wasn't stopped during "
				"save, Savegame may be corrupt!\n");
	GLOBAL (GameClock.TimeCounter) = 0;

	LoadShipQueue (fh, &GLOBAL (avail_race_q), FALSE);
	if (!(NextActivity & START_INTERPLANETARY))
		LoadShipQueue (fh, &GLOBAL (npc_built_ship_q), FALSE);
	LoadShipQueue (fh, &GLOBAL (built_ship_q), FALSE);

	// Load the game events (compressed)
	cread_16 (fh, &num_links);
	{
#ifdef DEBUG_LOAD
		log_add (log_Debug, "EVENTS:");
#endif /* DEBUG_LOAD */
		while (num_links--)
		{
			HEVENT hEvent;
			EVENTPTR EventPtr;

			hEvent = AllocEvent ();
			LockEvent (hEvent, &EventPtr);

			LoadEvent (EventPtr, fh);

#ifdef DEBUG_LOAD
		log_add (log_Debug, "\t%u/%u/%u -- %u",
				EventPtr->month_index,
				EventPtr->day_index,
				EventPtr->year_index,
				EventPtr->func_index);
#endif /* DEBUG_LOAD */
			UnlockEvent (hEvent);
			PutEvent (hEvent);
		}
	}

	// Load the encounters (black globes in HS/QS (compressed))
	cread_16 (fh, &num_links);
	{
		while (num_links--)
		{
			BYTE i, NumShips;
			HENCOUNTER hEncounter;
			ENCOUNTERPTR EncounterPtr;

			hEncounter = AllocEncounter ();
			LockEncounter (hEncounter, &EncounterPtr);

			LoadEncounter (EncounterPtr, fh);

			NumShips = LONIBBLE (EncounterPtr->SD.Index);
			for (i = 0; i < NumShips; ++i)
			{
				HSTARSHIP hStarShip;
				SHIP_FRAGMENTPTR TemplatePtr;

				hStarShip = GetStarShipFromIndex (
						&GLOBAL (avail_race_q),
						EncounterPtr->SD.ShipList[i].var1);
				TemplatePtr = (SHIP_FRAGMENTPTR)LockStarShip (
						&GLOBAL (avail_race_q), hStarShip);
				EncounterPtr->SD.ShipList[i].race_strings =
						TemplatePtr->ShipInfo.race_strings;
				EncounterPtr->SD.ShipList[i].icons =
						TemplatePtr->ShipInfo.icons;
				EncounterPtr->SD.ShipList[i].melee_icon =
						TemplatePtr->ShipInfo.melee_icon;
				UnlockStarShip (&GLOBAL (avail_race_q), hStarShip);
			}

			UnlockEncounter (hEncounter);
			PutEncounter (hEncounter);
		}
	}

	// Copy the star info file from the compressed stream
	fp = OpenStateFile (STARINFO_FILE, "wb");
	if (fp)
	{
		DWORD flen;

		cread_32 (fh, &flen);
		while (flen)
		{
			COUNT num_bytes;

			num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
			cread (buf, num_bytes, 1, fh);
			WriteStateFile (buf, num_bytes, 1, fp);

			flen -= num_bytes;
		}
		CloseStateFile (fp);
	}

	// Copy the defined groupinfo file from the compressed stream
	fp = OpenStateFile (DEFGRPINFO_FILE, "wb");
	if (fp)
	{
		DWORD flen;

		cread_32 (fh, &flen);
		while (flen)
		{
			COUNT num_bytes;

			num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
			cread (buf, num_bytes, 1, fh);
			WriteStateFile (buf, num_bytes, 1, fp);

			flen -= num_bytes;
		}
		CloseStateFile (fp);
	}

	// Copy the random groupinfo file from the compressed stream
	fp = OpenStateFile (RANDGRPINFO_FILE, "wb");
	if (fp)
	{
		DWORD flen;

		cread_32 (fh, &flen);
		while (flen)
		{
			COUNT num_bytes;

			num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
			cread (buf, num_bytes, 1, fh);
			WriteStateFile (buf, num_bytes, 1, fp);

			flen -= num_bytes;
		}
		CloseStateFile (fp);
	}

	LoadStarDesc (&SD, fh);

	cclose (fh);
	res_CloseResFile (in_fp);

	EncounterGroup = 0;
	EncounterRace = -1;

	ReinitQueue (&race_q[0]);
	ReinitQueue (&race_q[1]);
	CurStarDescPtr = FindStar (NULL, &SD.star_pt, 0, 0);
	if (!(NextActivity & START_ENCOUNTER)
			&& LOBYTE (NextActivity) == IN_INTERPLANETARY)
		NextActivity |= START_INTERPLANETARY;

	return TRUE;
}


