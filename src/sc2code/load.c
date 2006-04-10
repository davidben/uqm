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


//#define DEBUG_LOAD

ACTIVITY NextActivity;

static BOOLEAN LoadSummary (uio_Stream *in_fp);

static void
LoadShipQueue (DECODE_REF fh, PQUEUE pQueue, BOOLEAN MakeQ)
{
	COUNT num_links;

	cread ((PBYTE)&num_links, sizeof (num_links), 1, fh);
	if (num_links)
	{
		if (MakeQ)
			InitQueue (pQueue, num_links, sizeof (SHIP_FRAGMENT));

		do
		{
			HSTARSHIP hStarShip;
			SHIP_FRAGMENTPTR FragPtr;
			COUNT Index, Offset;
			PBYTE Ptr;

			cread ((PBYTE)&Index, sizeof (Index), 1, fh);

			if (pQueue == &GLOBAL (avail_race_q))
			{
				hStarShip = GetStarShipFromIndex (pQueue, Index);
				FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (pQueue, hStarShip);
				Offset = 0;
			}
			else
			{
				hStarShip = CloneShipFragment (Index, pQueue, 0);
				FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (pQueue, hStarShip);
				Offset = (PBYTE)&FragPtr->ShipInfo
						- (PBYTE)&FragPtr->RaceDescPtr;
			}

			Ptr = ((PBYTE)&FragPtr->ShipInfo) - Offset;
			cread ((PBYTE)Ptr,
					((PBYTE)&FragPtr->ShipInfo.race_strings) - Ptr, 1, fh);

			// Hack to retain savegame backwards compatibility, after
			// increasing crew_level and max_crew from BYTE to COUNT.
			// For the queues that are saved here, a BYTE is large enough.
			FragPtr->ShipInfo.max_crew = FragPtr->ShipInfo.dummy_max_crew;
			FragPtr->ShipInfo.crew_level = FragPtr->ShipInfo.dummy_crew_level;

			if (Offset == 0)
			{
				EXTENDED_SHIP_FRAGMENTPTR ExtFragPtr;

				ExtFragPtr = (EXTENDED_SHIP_FRAGMENTPTR)FragPtr;
				Ptr = (PBYTE)&ExtFragPtr->ShipInfo.actual_strength;
				cread ((PBYTE)Ptr, ((PBYTE)&ExtFragPtr[1]) - Ptr, 1, fh);
			}
			UnlockStarShip (pQueue, hStarShip);
		} while (--num_links);
	}
}

static void
LoadEncounter (ENCOUNTERPTR EncounterPtr, DECODE_REF fh)
{
	COUNT i;
#define LOAD_BLOCK(file, start, end) \
		cread((PBYTE) (start), (PBYTE) (end) - (PBYTE) start, \
		1, (file))
	// An ENCOUNTER structure (indirectly) contains an array of
	// SHIP_INFO  structure, which is bigger now because of the
	// larger type for crew.
	// This hack makes sure the savegames format is unchanged.
	// The original code consisted of just one line:
	// cread ((PBYTE)EncounterPtr, sizeof (*EncounterPtr), 1, fh);

	// Load the stuff before the SHIP_INFO array:
	LOAD_BLOCK(fh, EncounterPtr, EncounterPtr->SD.ShipList);

	// Load each entry in the SHIP_INFO array:
	for (i = 0; i < MAX_HYPER_SHIPS; i++)
	{
		SHIP_INFO *ShipInfo = &EncounterPtr->SD.ShipList[i];
		LOAD_BLOCK(fh, ShipInfo, &ShipInfo->crew_level);
		ShipInfo->crew_level = ShipInfo->dummy_crew_level;
		ShipInfo->max_crew = ShipInfo->dummy_max_crew;
	}
	
	// Load the stuff after the SHIP_INFO array:
	LOAD_BLOCK(fh, &EncounterPtr->log_x, &EncounterPtr[1]);
}

static BOOLEAN
LoadSummary (uio_Stream *in_fp)
{
	// XXX: This function seems to be unused.
	SUMMARY_DESC S;

	return (ReadResFile (&S, sizeof (S), 1, in_fp) == sizeof (S));
}

BOOLEAN
LoadGame (COUNT which_game, SUMMARY_DESC *summary_desc)
{
	uio_Stream *in_fp;
	char buf[256], file[PATH_MAX];

	sprintf (file, "starcon2.%02u", which_game);
	in_fp = res_OpenResFile (saveDir, file, "rb");
	if (in_fp)
	{
		GAME_STATE_FILE *fp;
		DECODE_REF fh;
		COUNT num_links;
		Semaphore clock_sem;
		Task clock_task;
		QUEUE event_q, encounter_q, avail_q, npc_q, player_q;
		STAR_DESC SD;
		ACTIVITY Activity;

		ReadResFile (buf, sizeof (*summary_desc), 1, in_fp);

		if (summary_desc == 0)
			summary_desc = (SUMMARY_DESC *)buf;
		else
		{
			memcpy (summary_desc, buf, sizeof (*summary_desc));
			res_CloseResFile (in_fp);
			return TRUE;
		}

		// Crude check for big-endian/little-endian incompatibilities.
		// year_index is suitable as it's a multi-byte value within
		// a specific recognisable range.
		if (summary_desc->year_index < START_YEAR ||
				summary_desc->year_index >= START_YEAR +
				YEARS_TO_KOHRAH_VICTORY + 1 /* Utwig intervention */ +
				1 /* time to destroy all races, plenty */)
		{
			fprintf (stderr, "Warning: Savegame corrupt or from an "
					"an incompatible platform.\n");
			res_CloseResFile (in_fp);
			return FALSE;
		}

		GlobData.SIS_state = summary_desc->SS;

		if ((fh = copen (in_fp, FILE_STREAM, STREAM_READ)) == 0)
		{
			res_CloseResFile (in_fp);
			return FALSE;
		}

		ReinitQueue (&GLOBAL (GameClock.event_q));
		ReinitQueue (&GLOBAL (encounter_q));
		ReinitQueue (&GLOBAL (npc_built_ship_q));
		ReinitQueue (&GLOBAL (built_ship_q));

		clock_sem = GLOBAL (GameClock.clock_sem);
		clock_task = GLOBAL (GameClock.clock_task);
		event_q = GLOBAL (GameClock.event_q);
		encounter_q = GLOBAL (encounter_q);
		avail_q = GLOBAL (avail_race_q);
		npc_q = GLOBAL (npc_built_ship_q);
		player_q = GLOBAL (built_ship_q);

		memset ((PBYTE)&GLOBAL (GameState[0]),
				0, sizeof (GLOBAL (GameState)));
		Activity = GLOBAL (CurrentActivity);
		cread ((PBYTE)&GlobData.Game_state, sizeof (GlobData.Game_state), 1,
				fh);
		NextActivity = GLOBAL (CurrentActivity);
		GLOBAL (CurrentActivity) = Activity;

		GLOBAL (GameClock.clock_sem) = clock_sem;
		GLOBAL (GameClock.clock_task) = clock_task;
		GLOBAL (GameClock.event_q) = event_q;
		GLOBAL (encounter_q) = encounter_q;
		GLOBAL (avail_race_q) = avail_q;
		GLOBAL (npc_built_ship_q) = npc_q;
		GLOBAL (built_ship_q) = player_q;
		// It shouldn't be possible to ever save with TimeCounter != 0
		// But if it does happen, it needs to be reset to 0, since on load
		// the clock semaphore is gauranteed to be 0
		if (GLOBAL (GameClock.TimeCounter) != 0)
			fprintf (stderr, "Warning: Game clock wasn't stopped during "
					"save, Savegame may be corrupt!\n");
		GLOBAL (GameClock.TimeCounter) = 0;

		LoadShipQueue (fh, &GLOBAL (avail_race_q), FALSE);
		if (!(NextActivity & START_INTERPLANETARY))
			LoadShipQueue (fh, &GLOBAL (npc_built_ship_q), FALSE);
		LoadShipQueue (fh, &GLOBAL (built_ship_q), FALSE);

		cread ((PBYTE)&num_links, sizeof (num_links), 1, fh);
		{
#ifdef DEBUG_LOAD
			fprintf (stderr, "EVENTS:\n");
#endif /* DEBUG_LOAD */
			while (num_links--)
			{
				HEVENT hEvent;
				EVENTPTR EventPtr;

				hEvent = AllocEvent ();
				LockEvent (hEvent, &EventPtr);

				cread ((PBYTE)EventPtr, sizeof (*EventPtr), 1, fh);

#ifdef DEBUG_LOAD
			fprintf (stderr, "\t%u/%u/%u -- %u\n",
					EventPtr->month_index,
					EventPtr->day_index,
					EventPtr->year_index,
					EventPtr->func_index);
#endif /* DEBUG_LOAD */
				UnlockEvent (hEvent);
				PutEvent (hEvent);
			}
		}

		cread ((PBYTE)&num_links, sizeof (num_links), 1, fh);
		{
			while (num_links--)
			{
				BYTE i, NumShips;
				HENCOUNTER hEncounter;
				ENCOUNTERPTR EncounterPtr;

				hEncounter = AllocEncounter ();
				LockEncounter (hEncounter, &EncounterPtr);

				LoadEncounter(EncounterPtr, fh);
				EncounterPtr->hElement = 0;
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

		fp = OpenStateFile (STARINFO_FILE, "wb");
		if (fp)
		{
			DWORD flen;

			cread ((PBYTE)&flen, sizeof (flen), 1, fh);
			while (flen)
			{
				COUNT num_bytes;

				num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
				cread ((PBYTE)buf, num_bytes, 1, fh);
				WriteStateFile (buf, num_bytes, 1, fp);

				flen -= num_bytes;
			}
			CloseStateFile (fp);
		}

		fp = OpenStateFile (DEFGRPINFO_FILE, "wb");
		if (fp)
		{
			DWORD flen;

			cread ((PBYTE)&flen, sizeof (flen), 1, fh);
			while (flen)
			{
				COUNT num_bytes;

				num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
				cread ((PBYTE)buf, num_bytes, 1, fh);
				WriteStateFile (buf, num_bytes, 1, fp);

				flen -= num_bytes;
			}
			CloseStateFile (fp);
		}

		fp = OpenStateFile (RANDGRPINFO_FILE, "wb");
		if (fp)
		{
			DWORD flen;

			cread ((PBYTE)&flen, sizeof (flen), 1, fh);
			while (flen)
			{
				COUNT num_bytes;

				num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
				cread ((PBYTE)buf, num_bytes, 1, fh);
				WriteStateFile (buf, num_bytes, 1, fp);

				flen -= num_bytes;
			}
			CloseStateFile (fp);
		}

		cread ((PBYTE)&SD, sizeof (SD), 1, fh);

		cclose (fh);
		res_CloseResFile (in_fp);

		battle_counter = 0;
		ReinitQueue (&race_q[0]);
		ReinitQueue (&race_q[1]);
		CurStarDescPtr = FindStar (NULL_PTR, &SD.star_pt, 0, 0);
		if (!(NextActivity & START_ENCOUNTER)
				&& LOBYTE (NextActivity) == IN_INTERPLANETARY)
			NextActivity |= START_INTERPLANETARY;

		GLOBAL (DisplayArray) = DisplayArray;

		return TRUE;
	}

	return FALSE;
}


