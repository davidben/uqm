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
#include "declib.h"

void FreeSC2Data (void);

BOOLEAN LoadSC2Data (void);

static void
SaveShipQueue (DECODE_REF fh, PQUEUE pQueue)
{
	COUNT num_links;
	HSTARSHIP hStarShip;

	num_links = CountLinks (pQueue);
	cwrite ((PBYTE)&num_links, sizeof (num_links), 1, fh);

	hStarShip = GetHeadLink (pQueue);
	while (num_links--)
	{
		HSTARSHIP hNextShip;
		SHIP_FRAGMENTPTR FragPtr;
		COUNT Index, Offset;
		PBYTE Ptr;

		FragPtr = (SHIP_FRAGMENTPTR)LockStarShip (pQueue, hStarShip);
		hNextShip = _GetSuccLink (FragPtr);

		if (pQueue == &GLOBAL (avail_race_q))
		{
			Index = GetIndexFromStarShip (pQueue, hStarShip);
			Offset = 0;
		}
		else
		{
			Index = GET_RACE_ID (FragPtr);
			Offset = (PBYTE)&FragPtr->ShipInfo
					- (PBYTE)&FragPtr->RaceDescPtr;
		}

		cwrite ((PBYTE)&Index, sizeof (Index), 1, fh);

		Ptr = ((PBYTE)&FragPtr->ShipInfo) - Offset;
		cwrite ((PBYTE)Ptr,
				((PBYTE)&FragPtr->ShipInfo.race_strings) - Ptr,
				1, fh);
		if (Offset == 0)
		{
			EXTENDED_SHIP_FRAGMENTPTR ExtFragPtr;

			ExtFragPtr = (EXTENDED_SHIP_FRAGMENTPTR)FragPtr;
			Ptr = (PBYTE)&ExtFragPtr->ShipInfo.actual_strength;
			cwrite ((PBYTE)Ptr, ((PBYTE)&ExtFragPtr[1]) - Ptr, 1, fh);
		}

		UnlockStarShip (pQueue, hStarShip);
		hStarShip = hNextShip;
	}
}

static void
PrepareSummary (SUMMARY_DESC *summary_desc)
{
	extern SIZE InventoryDevices (BYTE *pDeviceMap);

	summary_desc->SS = GlobData.SIS_state;

	switch (summary_desc->Activity = LOBYTE (GLOBAL (CurrentActivity)))
	{
		case IN_HYPERSPACE:
			if (GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1)
				summary_desc->Activity = IN_QUASISPACE;
			break;
		case IN_INTERPLANETARY:
			if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) == (BYTE)~0)
				summary_desc->Activity = IN_STARBASE;
			else if (pSolarSysState && pSolarSysState->MenuState.Initialized >= 3)
				summary_desc->Activity = IN_PLANET_ORBIT;
			break;
	}

	summary_desc->MCreditLo = GET_GAME_STATE (MELNORME_CREDIT0);
	summary_desc->MCreditHi = GET_GAME_STATE (MELNORME_CREDIT1);

	{
		HSTARSHIP hStarShip, hNextShip;

		for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q)), summary_desc->NumShips = 0;
				hStarShip; hStarShip = hNextShip, ++summary_desc->NumShips)
		{
			SHIP_FRAGMENTPTR StarShipPtr;

			StarShipPtr = (SHIP_FRAGMENTPTR)LockStarShip (
					&GLOBAL (built_ship_q), hStarShip
					);
			hNextShip = _GetSuccLink (StarShipPtr);
			summary_desc->ShipList[summary_desc->NumShips] = GET_RACE_ID (StarShipPtr);
			UnlockStarShip (&GLOBAL (built_ship_q), hStarShip);
		}
	}

	summary_desc->NumDevices = InventoryDevices (summary_desc->DeviceList);

	summary_desc->Flags = GET_GAME_STATE (LANDER_SHIELDS)
			| (GET_GAME_STATE (IMPROVED_LANDER_SPEED) << (4 + 0))
			| (GET_GAME_STATE (IMPROVED_LANDER_CARGO) << (4 + 1))
			| (GET_GAME_STATE (IMPROVED_LANDER_SHOT) << (4 + 2))
			| ((GET_GAME_STATE (CHMMR_BOMB_STATE) < 2 ? 0 : 1) << (4 + 3));

	summary_desc->day_index = GLOBAL (GameClock.day_index);
	summary_desc->month_index = GLOBAL (GameClock.month_index);
	summary_desc->year_index = GLOBAL (GameClock.year_index);
}

static void
SaveProblemMessage (STAMP *MsgStamp)
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
			BUILD_COLOR (MAKE_RGB15 (0x8, 0x8, 0x8), 0x1F),
			TRUE, BUILD_COLOR (MAKE_RGB15 (0xA, 0xA, 0xA), 0x08));
	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x14, 0x14, 0x14), 0x0F));

	for (i = 0; i < MAX_MSG_LINES; ++i)
	{
		t.pStr = ppStr[i];
		if (*t.pStr == '\0')
			break;
		t.CharCount = (COUNT)~0;
		DrawText (&t);
		t.baseline.y += 11;
	}
	UnbatchGraphics ();
}

BOOLEAN
SaveProblem (void)
{
	BOOLEAN manage;
	STAMP s;
	INPUT_STATE InputState;
	CONTEXT OldContext;
	
	SetSemaphore (GraphicsSem);
	OldContext = SetContext (SpaceContext);
	
	SaveProblemMessage (&s);

	GLOBAL (CurrentActivity) |= CHECK_ABORT;

	while (AnyButtonPress (FALSE));
	do
	{
		InputState = GetInputState (NormalInput);
		if (InputState & DEVICE_BUTTON1)
			manage = TRUE;
		else if (InputState & DEVICE_BUTTON3)
			manage = FALSE;
		else
			InputState = 0;
	} while (!InputState);

	GLOBAL (CurrentActivity) &= ~CHECK_ABORT;

	BatchGraphics ();
	DrawStamp (&s);
	UnbatchGraphics ();
	SetContext (OldContext);
	DestroyDrawable (ReleaseDrawable (s.frame));
	
	ClearSemaphore (GraphicsSem);
	
	if (manage)
	{
		BYTE clut_buf[1];

		SetSemaphore (GraphicsSem);
		SetFlashRect (NULL_PTR, (FRAME)0);
		ClearSemaphore (GraphicsSem);
   
		clut_buf[0] = FadeAllToBlack;
		SleepThreadUntil (XFormColorMap ((COLORMAPPTR)clut_buf, ONE_SECOND / 2));
		FlushColorXForms ();

#if 0
		SuspendTasking (); // just in case
		ManageNVRAM ();
		ResumeTasking ();
#endif

		clut_buf[0] = FadeAllToColor;
		SleepThreadUntil (XFormColorMap ((COLORMAPPTR)clut_buf, ONE_SECOND / 2));
		FlushColorXForms ();
	}
	else
	{
#if 0
		CleanNVRAM ();
#endif
		SleepThreadUntil (GetTimeCounter () + (ONE_SECOND / 4));
	}

	return (manage);
}

BOOLEAN
SaveGame (COUNT which_game, SUMMARY_DESC *summary_desc)
{
	BOOLEAN success, made_room;
	PVOID out_fp;
	MEM_HANDLE h;
	DECODE_REF fh;

	success = TRUE;
	made_room = FALSE;
RetrySave:
	h = mem_request (10 * 1024);
	if ((out_fp = mem_lock (h)) == 0
			|| (fh = copen (out_fp, MEMORY_STREAM, STREAM_WRITE)) == 0)
	{
		if (success)
		{
			success = FALSE;
			made_room = TRUE;
			mem_unlock (h);
			mem_release (h);

			FreeSC2Data ();
//			fprintf (stderr, "Insufficient room for save buffers -- RETRYING\n");
			goto RetrySave;
		}
//		else
//			fprintf (stderr, "Insufficient room for save buffers -- GIVING UP!\n");
	}
	else
	{
		FILE *fp;
		DWORD flen;
		COUNT num_links;
		FRAME frame;
		POINT pt;
		STAR_DESC SD;
		char buf[256];
		extern void SaveFlagshipState (void);

		success = TRUE;
		if (CurStarDescPtr)
			SD = *CurStarDescPtr;
		else
		{
			SD.star_pt.x = SD.star_pt.y = 0;
			SD.Type = SD.Index = SD.Prefix = SD.Postfix = 0;
		}

		frame = GLOBAL (ShipStamp.frame);
		pt = GLOBAL (ip_location);
		SaveFlagshipState ();
		if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
				&& !(GLOBAL (CurrentActivity)
				& (START_ENCOUNTER | START_INTERPLANETARY)))
			PutGroupInfo (0L, (BYTE)~0);

		cwrite ((PBYTE)&GlobData.Game_state, sizeof (GlobData.Game_state), 1, fh);

		GLOBAL (ip_location) = pt;
		GLOBAL (ShipStamp.frame) = frame;

		SaveShipQueue (fh, &GLOBAL (avail_race_q));
		if (!(GLOBAL (CurrentActivity) & START_INTERPLANETARY))
			SaveShipQueue (fh, &GLOBAL (npc_built_ship_q));
		SaveShipQueue (fh, &GLOBAL (built_ship_q));

		num_links = CountLinks (&GLOBAL (GameClock.event_q));
		cwrite ((PBYTE)&num_links, sizeof (num_links), 1, fh);
		{
			HEVENT hEvent;

			hEvent = GetHeadLink (&GLOBAL (GameClock.event_q));
			while (num_links--)
			{
				HEVENT hNextEvent;
				EVENTPTR EventPtr;

				LockEvent (hEvent, &EventPtr);
				hNextEvent = GetSuccEvent (EventPtr);

				cwrite ((PBYTE)EventPtr, sizeof (*EventPtr), 1, fh);

				UnlockEvent (hEvent);
				hEvent = hNextEvent;
			}
		}

		num_links = CountLinks (&GLOBAL (encounter_q));
		cwrite ((PBYTE)&num_links, sizeof (num_links), 1, fh);
		{
			HENCOUNTER hEncounter;

			hEncounter = GetHeadLink (&GLOBAL (encounter_q));
			while (num_links--)
			{
				HENCOUNTER hNextEncounter;
				ENCOUNTERPTR EncounterPtr;

				LockEncounter (hEncounter, &EncounterPtr);
				hNextEncounter = GetSuccEncounter (EncounterPtr);

				cwrite ((PBYTE)EncounterPtr, sizeof (*EncounterPtr), 1, fh);

				UnlockEncounter (hEncounter);
				hEncounter = hNextEncounter;
			}
		}

		fp = OpenResFile (STARINFO_FILE, "rb");
		if (fp)
		{
			flen = LengthResFile (fp);
			cwrite ((PBYTE)&flen, sizeof (flen), 1, fh);
			while (flen)
			{
				COUNT num_bytes;

				num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
				ReadResFile (buf, num_bytes, 1, fp);
				cwrite ((PBYTE)buf, num_bytes, 1, fh);

				flen -= num_bytes;
			}
			CloseResFile (fp);
		}

		fp = OpenResFile (DEFGRPINFO_FILE, "rb");
		if (fp)
		{
			flen = LengthResFile (fp);
			cwrite ((PBYTE)&flen, sizeof (flen), 1, fh);
			while (flen)
			{
				COUNT num_bytes;

				num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
				ReadResFile (buf, num_bytes, 1, fp);
				cwrite ((PBYTE)buf, num_bytes, 1, fh);

				flen -= num_bytes;
			}
			CloseResFile (fp);
		}

		fp = OpenResFile (RANDGRPINFO_FILE, "rb");
		if (fp)
		{
			flen = LengthResFile (fp);
			cwrite ((PBYTE)&flen, sizeof (flen), 1, fh);
			while (flen)
			{
				COUNT num_bytes;

				num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
				ReadResFile (buf, num_bytes, 1, fp);
				cwrite ((PBYTE)buf, num_bytes, 1, fh);

				flen -= num_bytes;
			}
			CloseResFile (fp);
		}

		cwrite ((PBYTE)&SD, sizeof (SD), 1, fh);

		flen = cclose (fh);

		sprintf (buf, "starcon2.%02u", which_game);
//		fprintf (stderr, "'%s' is %lu bytes long\n", buf, flen + sizeof (*summary_desc));
		if (flen && (out_fp = (PVOID)OpenResFile (buf, "wb")))
		{
			PrepareSummary (summary_desc);

			success = (BOOLEAN)(WriteResFile (
					summary_desc, sizeof (*summary_desc), 1, out_fp
					) != 0);
			if (success && WriteResFile (mem_lock (h), (COUNT)flen, 1, out_fp) == 0)
				success = FALSE;

			mem_unlock (h);
			if (CloseResFile ((FILE *)out_fp) == 0)
				success = FALSE;

		}
		else
			success = FALSE;
			
		if (!success)
			DeleteResFile (buf);
	}

	mem_unlock (h);
	mem_release (h);

	if (made_room)
		LoadSC2Data ();

	return (success);
}

