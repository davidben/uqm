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

#include "save.h"

#include "build.h"
#include "declib.h"
#include "controls.h"
#include "encount.h"
#include "file.h"
#include "gamestr.h"
#include "globdata.h"
#include "options.h"
#include "races.h"
#include "shipcont.h"
#include "setup.h"
#include "state.h"
#include "util.h"
#include "libs/inplib.h"
#include "libs/log.h"


// XXX: these should handle endian conversions later
static inline COUNT
cwrite_8 (DECODE_REF fh, BYTE v)
{
	return cwrite (&v, 1, 1, fh);
}

static inline COUNT
cwrite_16 (DECODE_REF fh, UWORD v)
{
	return cwrite (&v, 2, 1, fh);
}

static inline COUNT
cwrite_32 (DECODE_REF fh, DWORD v)
{
	return cwrite (&v, 4, 1, fh);
}

static inline COUNT
cwrite_ptr (DECODE_REF fh)
{
	return cwrite_32 (fh, 0); /* ptrs are useless in saves */
}

static inline COUNT
cwrite_a8 (DECODE_REF fh, PBYTE ar, COUNT count)
{
	return cwrite (ar, 1, count, fh) == count;
}

static inline COUNT
write_8 (PVOID fp, BYTE v)
{
	return WriteResFile (&v, 1, 1, fp);
}

static inline COUNT
write_16 (PVOID fp, UWORD v)
{
	return WriteResFile (&v, 2, 1, fp);
}

static inline COUNT
write_32 (PVOID fp, DWORD v)
{
	return WriteResFile (&v, 4, 1, fp);
}

static inline COUNT
write_ptr (PVOID fp)
{
	return write_32 (fp, 0); /* ptrs are useless in saves */
}

static inline COUNT
write_a8 (PVOID fp, PBYTE ar, COUNT count)
{
	return WriteResFile (ar, 1, count, fp) == count;
}

static inline COUNT
write_a16 (PVOID fp, PUWORD ar, COUNT count)
{
	for ( ; count > 0; --count, ++ar)
	{
		if (write_16 (fp, *ar) != 1)
			return 0;
	}
	return 1;
}

static void
SaveShipQueue (DECODE_REF fh, PQUEUE pQueue)
{
	COUNT num_links;
	HSTARSHIP hStarShip;

	// Write the number of entries in the queue.
	num_links = CountLinks (pQueue);
	cwrite_16 (fh, num_links);

	hStarShip = GetHeadLink (pQueue);
	while (num_links--)
	{
		HSTARSHIP hNextShip;
		SHIP_FRAGMENTPTR FragPtr;
		COUNT Index;

		FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (pQueue, hStarShip);
		hNextShip = _GetSuccLink (FragPtr);

		if (pQueue == &GLOBAL (avail_race_q))
			Index = GetIndexFromStarShip (pQueue, hStarShip);
					// The index is the position in the queue.
		else
			Index = GET_RACE_ID (FragPtr);

		// Write the number identifying this ship type.
		// See races.h; look for the enum containing NUM_AVAILABLE_RACES.
		cwrite_16 (fh, Index);

		if (pQueue != &GLOBAL (avail_race_q))
		{	// queues other than avail_race_q save SHIP_FRAGMENT elements
			// Write SHIP_FRAGMENT elements
			cwrite_16 (fh, FragPtr->s.Player);
			cwrite_8  (fh, FragPtr->s.Captain);
			cwrite_8  (fh, 0); /* padding */
		}
		// Write SHIP_INFO elements
		cwrite_16 (fh, FragPtr->ShipInfo.ship_flags);
		cwrite_8  (fh, FragPtr->ShipInfo.var1);
		cwrite_8  (fh, FragPtr->ShipInfo.var2);
		cwrite_8  (fh, FragPtr->ShipInfo.crew_level);
		cwrite_8  (fh, FragPtr->ShipInfo.max_crew);
		cwrite_8  (fh, FragPtr->ShipInfo.energy_level);
		cwrite_8  (fh, FragPtr->ShipInfo.max_energy);
		cwrite_16 (fh, FragPtr->ShipInfo.loc.x);
		cwrite_16 (fh, FragPtr->ShipInfo.loc.y);

		if (pQueue == &GLOBAL (avail_race_q))
		{
			// avail_race_q contains information not about specific ships,
			// but about a race.
			EXTENDED_SHIP_FRAGMENTPTR ExtFragPtr =
					(EXTENDED_SHIP_FRAGMENTPTR) FragPtr;

			cwrite_16 (fh, ExtFragPtr->ShipInfo.actual_strength);
			cwrite_16 (fh, ExtFragPtr->ShipInfo.known_strength);
			cwrite_16 (fh, ExtFragPtr->ShipInfo.known_loc.x);
			cwrite_16 (fh, ExtFragPtr->ShipInfo.known_loc.y);
			cwrite_8  (fh, ExtFragPtr->ShipInfo.growth_err_term);
			cwrite_8  (fh, ExtFragPtr->ShipInfo.func_index);
			cwrite_16 (fh, ExtFragPtr->ShipInfo.dest_loc.x);
			cwrite_16 (fh, ExtFragPtr->ShipInfo.dest_loc.y);
			cwrite_16 (fh, 0); /* alignment padding */
		}

		UnlockStarShip (pQueue, hStarShip);
		hStarShip = hNextShip;
	}
}

static void
SaveEncounter (ENCOUNTERPTR EncounterPtr, DECODE_REF fh)
{
	COUNT i;

	cwrite_ptr (fh); /* useless ptr; HENCOUNTER pred */
	cwrite_ptr (fh); /* useless ptr; HENCOUNTER succ */
	cwrite_ptr (fh); /* useless ptr; HELEMENT hElement */
	cwrite_16  (fh, EncounterPtr->transition_state);
	cwrite_16  (fh, EncounterPtr->origin.x);
	cwrite_16  (fh, EncounterPtr->origin.y);
	cwrite_16  (fh, EncounterPtr->radius);
	// EXTENDED_STAR_DESC fields
	cwrite_16  (fh, EncounterPtr->SD.star_pt.x);
	cwrite_16  (fh, EncounterPtr->SD.star_pt.y);
	cwrite_8   (fh, EncounterPtr->SD.Type);
	cwrite_8   (fh, EncounterPtr->SD.Index);
	cwrite_16  (fh, 0); /* alignment padding */

	// Save each entry in the SHIP_INFO array:
	for (i = 0; i < MAX_HYPER_SHIPS; i++)
	{
		SHIP_INFOPTR ShipInfo = &EncounterPtr->SD.ShipList[i];

		cwrite_16  (fh, ShipInfo->ship_flags);
		cwrite_8   (fh, ShipInfo->var1);
		cwrite_8   (fh, ShipInfo->var2);
		cwrite_8   (fh, ShipInfo->crew_level);
		cwrite_8   (fh, ShipInfo->max_crew);
		cwrite_8   (fh, ShipInfo->energy_level);
		cwrite_8   (fh, ShipInfo->max_energy);
		cwrite_16  (fh, ShipInfo->loc.x);
		cwrite_16  (fh, ShipInfo->loc.y);
		cwrite_32  (fh, 0); /* useless val; STRING race_strings */
		cwrite_ptr (fh); /* useless ptr; FRAME icons */
		cwrite_ptr (fh); /* useless ptr; FRAME melee_icon */
	}
	
	// Save the stuff after the SHIP_INFO array:
	cwrite_32  (fh, EncounterPtr->log_x);
	cwrite_32  (fh, EncounterPtr->log_y);
}

static void
SaveEvent (EVENTPTR EventPtr, DECODE_REF fh)
{
	cwrite_ptr (fh); /* useless ptr; HEVENT pred */
	cwrite_ptr (fh); /* useless ptr; HEVENT succ */
	cwrite_8   (fh, EventPtr->day_index);
	cwrite_8   (fh, EventPtr->month_index);
	cwrite_16  (fh, EventPtr->year_index);
	cwrite_8   (fh, EventPtr->func_index);
	cwrite_8   (fh, 0); /* padding */
	cwrite_16  (fh, 0); /* padding */
}

static void
DummySaveQueue (PQUEUE QueuePtr, DECODE_REF fh)
{
	/* QUEUE should never actually be saved since it contains
	 * purely internal representation and the lists
	 * involved are actually saved separately */
	(void)QueuePtr; /* silence compiler */

	/* QUEUE format with QUEUE_TABLE defined -- UQM default */
	cwrite_ptr (fh); /* HLINK head */
	cwrite_ptr (fh); /* HLINK tail */
	cwrite_ptr (fh); /* PBYTE pq_tab */
	cwrite_ptr (fh); /* HLINK free_list */
	cwrite_16  (fh, 0); /* MEM_HANDLE hq_tab */
	cwrite_16  (fh, 0); /* COUNT object_size */
	cwrite_8   (fh, 0); /* BYTE num_objects */
	
	cwrite_8   (fh, 0); /* padding */
	cwrite_16  (fh, 0); /* padding */
}

static void
SaveClockState (PCLOCK_STATE ClockPtr, DECODE_REF fh)
{
	cwrite_8   (fh, ClockPtr->day_index);
	cwrite_8   (fh, ClockPtr->month_index);
	cwrite_16  (fh, ClockPtr->year_index);
	cwrite_16  (fh, ClockPtr->tick_count);
	cwrite_16  (fh, ClockPtr->day_in_ticks);
	cwrite_ptr (fh); /* useless ptr; Semaphore clock_sem */
	cwrite_ptr (fh); /* useless ptr; Task clock_task */
	cwrite_32  (fh, ClockPtr->TimeCounter); /* theoretically useless */

	DummySaveQueue (&ClockPtr->event_q, fh);
}

static void
SaveGameState (PGAME_STATE GSPtr, DECODE_REF fh)
{
	cwrite_8   (fh, 0); /* obsolete; BYTE cur_state */
	cwrite_8   (fh, GSPtr->glob_flags);
	cwrite_8   (fh, GSPtr->CrewCost);
	cwrite_8   (fh, GSPtr->FuelCost);
	cwrite_a8  (fh, GSPtr->ModuleCost, NUM_MODULES);
	cwrite_a8  (fh, GSPtr->ElementWorth, NUM_ELEMENT_CATEGORIES);
	cwrite_ptr (fh); /* useless ptr; PPRIMITIVE DisplayArray */
	cwrite_16  (fh, GSPtr->CurrentActivity);
	
	cwrite_16  (fh, 0); /* CLOCK_STATE alignment padding */
	SaveClockState (&GSPtr->GameClock, fh);

	cwrite_16  (fh, GSPtr->autopilot.x);
	cwrite_16  (fh, GSPtr->autopilot.y);
	cwrite_16  (fh, GSPtr->ip_location.x);
	cwrite_16  (fh, GSPtr->ip_location.y);
	/* STAMP ShipStamp */
	cwrite_16  (fh, GSPtr->ShipStamp.origin.x);
	cwrite_16  (fh, GSPtr->ShipStamp.origin.y);
	cwrite_32  (fh, (DWORD)GSPtr->ShipStamp.frame); /* abused ptr to store DWORD */
	/* VELOCITY_DESC velocity */
	cwrite_16  (fh, GSPtr->velocity.TravelAngle);
	cwrite_16  (fh, GSPtr->velocity.vector.width);
	cwrite_16  (fh, GSPtr->velocity.vector.height);
	cwrite_16  (fh, GSPtr->velocity.fract.width);
	cwrite_16  (fh, GSPtr->velocity.fract.height);
	cwrite_16  (fh, GSPtr->velocity.error.width);
	cwrite_16  (fh, GSPtr->velocity.error.height);
	cwrite_16  (fh, GSPtr->velocity.incr.width);
	cwrite_16  (fh, GSPtr->velocity.incr.height);
	cwrite_16  (fh, 0); /* VELOCITY_DESC padding */

	cwrite_32  (fh, GSPtr->BattleGroupRef);
	
	DummySaveQueue (&GSPtr->avail_race_q, fh);
	DummySaveQueue (&GSPtr->npc_built_ship_q, fh);
	DummySaveQueue (&GSPtr->encounter_q, fh);
	DummySaveQueue (&GSPtr->built_ship_q, fh);

	cwrite_a8  (fh, GSPtr->GameState, sizeof (GSPtr->GameState));

	assert (sizeof (GSPtr->GameState) % 4 == 3);
	cwrite_8  (fh, 0); /* GAME_STATE alignment padding */
}

static BOOLEAN
SaveSisState (PSIS_STATE SSPtr, PVOID fp)
{
	if (
			write_32  (fp, SSPtr->log_x) != 1 ||
			write_32  (fp, SSPtr->log_y) != 1 ||
			write_32  (fp, SSPtr->ResUnits) != 1 ||
			write_32  (fp, SSPtr->FuelOnBoard) != 1 ||
			write_16  (fp, SSPtr->CrewEnlisted) != 1 ||
			write_16  (fp, SSPtr->TotalElementMass) != 1 ||
			write_16  (fp, SSPtr->TotalBioMass) != 1 ||
			write_a8  (fp, SSPtr->ModuleSlots, NUM_MODULE_SLOTS) != 1 ||
			write_a8  (fp, SSPtr->DriveSlots, NUM_DRIVE_SLOTS) != 1 ||
			write_a8  (fp, SSPtr->JetSlots, NUM_JET_SLOTS) != 1 ||
			write_8   (fp, SSPtr->NumLanders) != 1 ||
			write_a16 (fp, SSPtr->ElementAmounts, NUM_ELEMENT_CATEGORIES) != 1 ||

			write_a8  (fp, SSPtr->ShipName, SIS_NAME_SIZE) != 1 ||
			write_a8  (fp, SSPtr->CommanderName, SIS_NAME_SIZE) != 1 ||
			write_a8  (fp, SSPtr->PlanetName, SIS_NAME_SIZE) != 1 ||

			write_16  (fp, 0) != 1 /* padding */
		)
		return FALSE;
	else
		return TRUE;
}

static BOOLEAN
SaveSummary (SUMMARY_DESC *SummPtr, PVOID fp)
{
	if (!SaveSisState (&SummPtr->SS, fp))
		return FALSE;

	if (
			write_8  (fp, SummPtr->Activity) != 1 ||
			write_8  (fp, SummPtr->Flags) != 1 ||
			write_8  (fp, SummPtr->day_index) != 1 ||
			write_8  (fp, SummPtr->month_index) != 1 ||
			write_16 (fp, SummPtr->year_index) != 1 ||
			write_8  (fp, SummPtr->MCreditLo) != 1 ||
			write_8  (fp, SummPtr->MCreditHi) != 1 ||
			write_8  (fp, SummPtr->NumShips) != 1 ||
			write_8  (fp, SummPtr->NumDevices) != 1 ||
			write_a8 (fp, SummPtr->ShipList, MAX_BUILT_SHIPS) != 1 ||
			write_a8 (fp, SummPtr->DeviceList, MAX_EXCLUSIVE_DEVICES) != 1 ||

			write_16  (fp, 0) != 1 /* padding */
		)
		return FALSE;
	else
		return TRUE;
}

static void
SaveStarDesc (STAR_DESCPTR SDPtr, DECODE_REF fh)
{
	cwrite_16 (fh, SDPtr->star_pt.x);
	cwrite_16 (fh, SDPtr->star_pt.y);
	cwrite_8  (fh, SDPtr->Type);
	cwrite_8  (fh, SDPtr->Index);
	cwrite_8  (fh, SDPtr->Prefix);
	cwrite_8  (fh, SDPtr->Postfix);
}

static void
PrepareSummary (SUMMARY_DESC *SummPtr)
{
	SummPtr->SS = GlobData.SIS_state;

	switch (SummPtr->Activity = LOBYTE (GLOBAL (CurrentActivity)))
	{
		case IN_HYPERSPACE:
			if (GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1)
				SummPtr->Activity = IN_QUASISPACE;
			break;
		case IN_INTERPLANETARY:
			if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) == (BYTE)~0)
				SummPtr->Activity = IN_STARBASE;
			else if (pSolarSysState && pSolarSysState->MenuState.Initialized >= 3)
				SummPtr->Activity = IN_PLANET_ORBIT;
			break;
	}

	SummPtr->MCreditLo = GET_GAME_STATE (MELNORME_CREDIT0);
	SummPtr->MCreditHi = GET_GAME_STATE (MELNORME_CREDIT1);

	{
		HSTARSHIP hStarShip, hNextShip;

		for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q)), SummPtr->NumShips = 0;
				hStarShip; hStarShip = hNextShip, ++SummPtr->NumShips)
		{
			SHIP_FRAGMENTPTR StarShipPtr;

			StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
					&GLOBAL (built_ship_q), hStarShip
					);
			hNextShip = _GetSuccLink (StarShipPtr);
			SummPtr->ShipList[SummPtr->NumShips] = GET_RACE_ID (StarShipPtr);
			UnlockStarShip (&GLOBAL (built_ship_q), hStarShip);
		}
	}

	SummPtr->NumDevices = InventoryDevices (SummPtr->DeviceList);

	SummPtr->Flags = GET_GAME_STATE (LANDER_SHIELDS)
			| (GET_GAME_STATE (IMPROVED_LANDER_SPEED) << (4 + 0))
			| (GET_GAME_STATE (IMPROVED_LANDER_CARGO) << (4 + 1))
			| (GET_GAME_STATE (IMPROVED_LANDER_SHOT) << (4 + 2))
			| ((GET_GAME_STATE (CHMMR_BOMB_STATE) < 2 ? 0 : 1) << (4 + 3));

	SummPtr->day_index = GLOBAL (GameClock.day_index);
	SummPtr->month_index = GLOBAL (GameClock.month_index);
	SummPtr->year_index = GLOBAL (GameClock.year_index);
}

static void
SaveProblemMessage (PSTAMP MsgStamp)
{
#define MAX_MSG_LINES 1
	RECT r;
	COUNT i;
	TEXT t;
	UNICODE *ppStr[MAX_MSG_LINES];

	ppStr[0] = GAME_STRING (SAVEGAME_STRING_BASE + 2);
 
	SetContextFont (StarConFont);

	t.baseline.x = t.baseline.y = 0;
	t.align = ALIGN_CENTER;
	for (i = 0; i < MAX_MSG_LINES; ++i)
	{
		RECT tr;

		t.pStr = ppStr[i];
		if (*t.pStr == '\0')
			break;
		t.CharCount = (COUNT)~0;
		TextRect (&t, &tr, NULL_PTR);
		if (i == 0)
			r = tr;
		else
			BoxUnion (&tr, &r, &r);
		t.baseline.y += 11;
	}
	t.baseline.x = ((SIS_SCREEN_WIDTH >> 1) - (r.extent.width >> 1))
			- r.corner.x;
	t.baseline.y = ((SIS_SCREEN_HEIGHT >> 1) - (r.extent.height >> 1))
			- r.corner.y;
	r.corner.x += t.baseline.x - 4;
	r.corner.y += t.baseline.y - 4;
	r.extent.width += 8;
	r.extent.height += 8;

	{
		RECT clip_r;
		
		GetContextClipRect (&clip_r);
		MsgStamp->origin = r.corner;
		r.corner.x += clip_r.corner.x;
		r.corner.y += clip_r.corner.y;
		MsgStamp->frame = CaptureDrawable (LoadDisplayPixmap (&r, (FRAME)0));
		r.corner.x -= clip_r.corner.x;
		r.corner.y -= clip_r.corner.y;
	}
	
	BatchGraphics ();
	DrawStarConBox (&r, 2,
			BUILD_COLOR (MAKE_RGB15 (0x10, 0x10, 0x10), 0x19),
			BUILD_COLOR (MAKE_RGB15 (0x08, 0x08, 0x08), 0x1F),
			TRUE, BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08));
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x14, 0x14, 0x14), 0x0F));

	for (i = 0; i < MAX_MSG_LINES; ++i)
	{
		t.pStr = ppStr[i];
		if (*t.pStr == '\0')
			break;
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		t.baseline.y += 11;
	}
	UnbatchGraphics ();
}

void
SaveProblem (void)
{
	STAMP s;
	CONTEXT OldContext;
	
	LockMutex (GraphicsLock);
	OldContext = SetContext (SpaceContext);
	SaveProblemMessage (&s);
	FlushGraphics ();
	UnlockMutex (GraphicsLock);

	while (AnyButtonPress (FALSE));
	do
	{
		TaskSwitch ();
		UpdateInputState ();
	} while (!(PulsedInputState.menu[KEY_MENU_SELECT] ||
			PulsedInputState.menu[KEY_MENU_SPECIAL] ||
		 	(GLOBAL (CurrentActivity) & CHECK_ABORT)));

	LockMutex (GraphicsLock);
	BatchGraphics ();
	DrawStamp (&s);
	UnbatchGraphics ();
	SetContext (OldContext);
	DestroyDrawable (ReleaseDrawable (s.frame));
	UnlockMutex (GraphicsLock);
	
	return;
}

// This function first writes to a memory file, and then writes the whole
// lot to the actual save file at once.
BOOLEAN
SaveGame (COUNT which_game, SUMMARY_DESC *SummPtr)
{
	BOOLEAN success, made_room;
	PVOID out_fp;
	MEM_HANDLE h;
	DECODE_REF fh;

	success = TRUE;
	made_room = FALSE;
RetrySave:
	h = mem_request (10 * 1024);
	out_fp = mem_lock (h);
	if (out_fp == 0
			|| (fh = copen (out_fp, MEMORY_STREAM, STREAM_WRITE)) == 0)
	{
		if (success)
		{
			success = FALSE;
			made_room = TRUE;
			mem_unlock (h);
			mem_release (h);

			FreeSC2Data ();
			log_add (log_Debug, "Insufficient room for save buffers"
					" -- RETRYING");
			goto RetrySave;
		}
		else
			log_add (log_Debug, "Insufficient room for save buffers"
					" -- GIVING UP!");
	}
	else
	{
		GAME_STATE_FILE *fp;
		DWORD flen;
		COUNT num_links;
		FRAME frame;
		POINT pt;
		STAR_DESC SD;
		char buf[256], file[PATH_MAX];

		success = TRUE;
		if (CurStarDescPtr)
			SD = *CurStarDescPtr;
		else
			memset (&SD, 0, sizeof (SD));

		// XXX: Backup: ShipStamp.frame is abused to store DWORD info
		//    SaveFlagshipState() overwrites it with a DWORD value
		frame = GLOBAL (ShipStamp.frame);
		pt = GLOBAL (ip_location);
		SaveFlagshipState ();
		if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
				&& !(GLOBAL (CurrentActivity)
				& (START_ENCOUNTER | START_INTERPLANETARY)))
			PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);

		SaveGameState (&GlobData.Game_state, fh);

		GLOBAL (ip_location) = pt;
		// XXX: Restore: ShipStamp.frame is abused to store DWORD info
		GLOBAL (ShipStamp.frame) = frame;

		SaveShipQueue (fh, &GLOBAL (avail_race_q));
		if (!(GLOBAL (CurrentActivity) & START_INTERPLANETARY))
			SaveShipQueue (fh, &GLOBAL (npc_built_ship_q));
		SaveShipQueue (fh, &GLOBAL (built_ship_q));

		// Save the number of game events (compressed).
		num_links = CountLinks (&GLOBAL (GameClock.event_q));
		cwrite_16 (fh, num_links);
		// Save the game events themselves (compressed):
		{
			HEVENT hEvent;

			hEvent = GetHeadLink (&GLOBAL (GameClock.event_q));
			while (num_links--)
			{
				HEVENT hNextEvent;
				EVENTPTR EventPtr;

				LockEvent (hEvent, &EventPtr);
				hNextEvent = GetSuccEvent (EventPtr);

				SaveEvent (EventPtr, fh);

				UnlockEvent (hEvent);
				hEvent = hNextEvent;
			}
		}

		// Save the number of encounters (black globes in HS/QS (compressed))
		num_links = CountLinks (&GLOBAL (encounter_q));
		// Save the encounters themselves (compressed):
		cwrite_16 (fh, num_links);
		{
			HENCOUNTER hEncounter;

			hEncounter = GetHeadLink (&GLOBAL (encounter_q));
			while (num_links--)
			{
				HENCOUNTER hNextEncounter;
				ENCOUNTERPTR EncounterPtr;

				LockEncounter (hEncounter, &EncounterPtr);
				hNextEncounter = GetSuccEncounter (EncounterPtr);

				SaveEncounter (EncounterPtr, fh);

				UnlockEncounter (hEncounter);
				hEncounter = hNextEncounter;
			}
		}

		// Copy the star info file to the memory file (compressed).
		fp = OpenStateFile (STARINFO_FILE, "rb");
		if (fp)
		{
			flen = LengthStateFile (fp);
			// Write the uncompressed size.
			cwrite_32 (fh, flen);
			while (flen)
			{
				COUNT num_bytes;

				num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
				ReadStateFile (buf, num_bytes, 1, fp);
				cwrite (buf, num_bytes, 1, fh);

				flen -= num_bytes;
			}
			CloseStateFile (fp);
		}

		// Copy the defined groupinfo file into the memory file (compressed)
		fp = OpenStateFile (DEFGRPINFO_FILE, "rb");
		if (fp)
		{
			flen = LengthStateFile (fp);
			// Write the uncompressed size.
			cwrite_32 (fh, flen);
			while (flen)
			{
				COUNT num_bytes;

				num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
				ReadStateFile (buf, num_bytes, 1, fp);
				cwrite (buf, num_bytes, 1, fh);

				flen -= num_bytes;
			}
			CloseStateFile (fp);
		}

		// Copy the random groupinfo file into the memory file (compressed)
		fp = OpenStateFile (RANDGRPINFO_FILE, "rb");
		if (fp)
		{
			flen = LengthStateFile (fp);
			// Write the uncompressed size.
			cwrite_32 (fh, flen);
			while (flen)
			{
				COUNT num_bytes;

				num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
				ReadStateFile (buf, num_bytes, 1, fp);
				cwrite (buf, num_bytes, 1, fh);

				flen -= num_bytes;
			}
			CloseStateFile (fp);
		}

		// Write the current star desc into the memory file (compressed).
		SaveStarDesc (&SD, fh);

		flen = cclose (fh);

		// Write the memory file to the actual savegame file.
		sprintf (file, "starcon2.%02u", which_game);
		log_add (log_Debug, "'%s' is %u bytes long", file,
				flen + sizeof (*SummPtr));
		if (flen && (out_fp = res_OpenResFile (saveDir, file, "wb")))
		{
			PrepareSummary (SummPtr);

			success = SaveSummary (SummPtr, out_fp);
			// Then write the rest of the data.
			if (success && WriteResFile (mem_lock (h), (COUNT)flen, 1,
						out_fp) == 0)
				success = FALSE;

			mem_unlock (h);
			if (res_CloseResFile ((uio_Stream *)out_fp) == 0)
				success = FALSE;

		}
		else
			success = FALSE;
			
		if (!success)
			DeleteResFile (saveDir, file);
	}

	mem_unlock (h);
	mem_release (h);

	if (made_room)
		LoadSC2Data ();

	return (success);
}

