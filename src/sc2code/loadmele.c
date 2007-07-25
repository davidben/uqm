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

// This file handles loading of teams, but the UI and the actual loading.

#include "melee.h"

#include "controls.h"
#include "gameopt.h"
#include "gamestr.h"
#include "globdata.h"
#include "master.h"
#include "save.h"
#include "setup.h"
#include "sounds.h"
#include "options.h"
#include "libs/log.h"
#include "libs/misc.h"


#define LOAD_TEAM_NAME_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x0F, 0x10, 0x1B), 0x00)
#define LOAD_TEAM_NAME_TEXT_COLOR_HILITE \
		BUILD_COLOR (MAKE_RGB15 (0x17, 0x18, 0x1D), 0x00)


#define LOAD_MELEE_BOX_WIDTH 34
#define LOAD_MELEE_BOX_HEIGHT 34
#define LOAD_MELEE_BOX_SPACE 1


static void DrawFileStrings (MELEE_STATE *pMS);
static bool FillFileView (MELEE_STATE *pMS);


bool
ReadTeamImage (TEAM_IMAGE *pTI, uio_Stream *load_fp)
{
	int status;
	FleetShipIndex i;
	
	status = ReadResFile (pTI, sizeof (*pTI), 1, load_fp);
	if (status != 1)
		return false;

	// Sanity check on the entries.
	for (i = 0; i < MELEE_FLEET_SIZE; i++)
	{
		BYTE StarShip = pTI->ShipList[i];

		if (StarShip == MELEE_NONE)
			continue;

		if (StarShip >= NUM_MELEE_SHIPS)
		{
			log_add (log_Warning, "Invalid ship type in loaded team (index "
					"%d, ship type is %d, max valid is %d).",
					i, StarShip, NUM_MELEE_SHIPS - 1);
			pTI->ShipList[i] = MELEE_NONE;
		}
	}

	return true;
}

static bool
LoadTeamImage (DIRENTRY DirEntry, TEAM_IMAGE *pTI)
{
	const BYTE *fileName;
	uio_Stream *load_fp;
	bool status;

	fileName = GetDirEntryAddress (DirEntry);
	load_fp = res_OpenResFile (meleeDir, (const char *) fileName, "rb");
	if (load_fp == 0)
		return false;

	if (LengthResFile (load_fp) != sizeof (*pTI))
		status = false;
	else
		status = ReadTeamImage (pTI, load_fp);
	res_CloseResFile (load_fp);

	return status;
}

#if 0  /* Not used */
static void
UnindexFleet (MELEE_STATE *pMS, COUNT index)
{
	assert (index < pMS->load.numIndices);
	pMS->load.numIndices--;
	memmove (&pMS->load.entryIndices[index],
			&pMS->load.entryIndices[index + 1],
			(pMS->load.numIndices - index) * sizeof pMS->load.entryIndices[0]);
}
#endif

static void
UnindexFleets (MELEE_STATE *pMS, COUNT index, COUNT count)
{
	assert (index + count <= pMS->load.numIndices);

	pMS->load.numIndices -= count;
	memmove (&pMS->load.entryIndices[index],
			&pMS->load.entryIndices[index + count],
			(pMS->load.numIndices - index) * sizeof pMS->load.entryIndices[0]);
}

static bool
GetFleetByIndex (MELEE_STATE *pMS, COUNT index, TEAM_IMAGE *result)
{
	COUNT firstIndex;

	if (index < pMS->load.preBuiltCount)
	{
		*result = pMS->load.preBuiltList[index];
		return true;
	}

	index -= pMS->load.preBuiltCount;
	firstIndex = index;

	for ( ; index < pMS->load.numIndices; index++)
	{
		DIRENTRY entry = SetAbsDirEntryTableIndex (pMS->load.dirEntries,
				pMS->load.entryIndices[index]);
		if (LoadTeamImage (entry, result))
			break;  // Success

		{
			const BYTE *fileName;
			fileName = GetDirEntryAddress (entry);
			log_add (log_Warning, "Warning: File '%s' is not a valid "
					"SuperMelee team.", fileName);
		}
	}

	if (index != firstIndex)
		UnindexFleets (pMS, firstIndex, index - firstIndex);

	return index < pMS->load.numIndices;
}

// returns (COUNT) -1 if not found
static COUNT
GetFleetIndexByFileName (MELEE_STATE *pMS, const char *fileName)
{
	COUNT index;
	
	for (index = 0; index < pMS->load.numIndices; index++)
	{
		DIRENTRY entry = SetAbsDirEntryTableIndex (pMS->load.dirEntries,
				pMS->load.entryIndices[index]);
		const BYTE *entryName = GetDirEntryAddress (entry);

		if (stricmp ((const char *) entryName, fileName) == 0)
			return pMS->load.preBuiltCount + index;
	}

	return (COUNT) -1;
}

// Auxiliary function for DrawFileStrings
// If drawShips is set the ships themselves are drawn, in addition to the
// fleet name and value; if not, only the fleet name and value are drawn.
// If highlite is set the text is drawn in the color used for highlighting.
static void
DrawFileString (TEAM_IMAGE *pTI, POINT *origin, BOOLEAN drawShips,
		BOOLEAN highlite)
{
	SetContextForeGroundColor (highlite ?
			LOAD_TEAM_NAME_TEXT_COLOR_HILITE : LOAD_TEAM_NAME_TEXT_COLOR);

	// Print the name of the fleet
	{
		TEXT Text;

		Text.baseline = *origin;
		Text.align = ALIGN_LEFT;
		Text.pStr = pTI->TeamName;
		Text.CharCount = (COUNT)~0;
		font_DrawText (&Text);
	}

	// Print the value of the fleet
	{
		TEXT Text;
		UNICODE buf[60];

		sprintf (buf, "%u", GetTeamValue (pTI));
		Text.baseline = *origin;
		Text.baseline.x += NUM_MELEE_COLUMNS *
				(LOAD_MELEE_BOX_WIDTH + LOAD_MELEE_BOX_SPACE) - 1;
		Text.align = ALIGN_RIGHT;
		Text.pStr = buf;
		Text.CharCount = (COUNT)~0;
		font_DrawText (&Text);
	}

	// Draw the ships for the fleet
	if (drawShips)
	{
		STAMP s;
		FleetShipIndex index;

		s.origin.x = origin->x + 1;
		s.origin.y = origin->y + 4;
		for (index = 0; index < MELEE_FLEET_SIZE; index++)
		{
			BYTE StarShip;
				
			StarShip = pTI->ShipList[index];
			if (StarShip != MELEE_NONE)
			{
				s.frame = GetShipIconsFromIndex (StarShip);
				DrawStamp (&s);
				s.origin.x += 17;
			}
		}
	}
}

// returns true if there are any entries in the view, in which case
// pMS->load.bot gets set to the index of the bottom entry in the view.
// returns false if not, in which case, the entire view remains unchanged.
static bool
FillFileView (MELEE_STATE *pMS)
{
	COUNT viewI;

	for (viewI = 0; viewI < LOAD_TEAM_VIEW_SIZE; viewI++)
	{
		bool success = GetFleetByIndex (pMS, pMS->load.top + viewI,
				&pMS->load.view[viewI]);
		if (!success)
			break;
	}

	if (viewI == 0)
		return false;

	pMS->load.bot = pMS->load.top + viewI;
	return true;
}

#define FILE_STRING_ORIGIN_X  5
#define FILE_STRING_ORIGIN_Y  34
#define ENTRY_HEIGHT 32

void
SelectFileString (MELEE_STATE *pMS, bool hilite)
{
	CONTEXT OldContext;
	POINT origin;
	COUNT viewI;

	viewI = pMS->load.cur - pMS->load.top;

	OldContext = SetContext (SpaceContext);
	SetContextFont (MicroFont);
	BatchGraphics ();

	origin.x = FILE_STRING_ORIGIN_X;
	origin.y = FILE_STRING_ORIGIN_Y + viewI * ENTRY_HEIGHT;
	DrawFileString (&pMS->load.view[viewI], &origin, FALSE, hilite);

	UnbatchGraphics ();
	SetContext (OldContext);
}

static void
DrawFileStrings (MELEE_STATE *pMS)
{
	POINT origin;
	CONTEXT OldContext;

	origin.x = FILE_STRING_ORIGIN_X;
	origin.y = FILE_STRING_ORIGIN_Y;
		
	OldContext = SetContext (SpaceContext);
	SetContextFont (MicroFont);
	BatchGraphics ();

	DrawMeleeIcon (28);  /* The load team frame */

	if (FillFileView (pMS))
	{
		COUNT i;
		for (i = pMS->load.top; i < pMS->load.bot; i++) {
			DrawFileString (&pMS->load.view[i - pMS->load.top], &origin,
					TRUE, FALSE);
			origin.y += ENTRY_HEIGHT;
		}
	}

	UnbatchGraphics ();
	SetContext (OldContext);
}

static void
RefocusView (MELEE_STATE *pMS, COUNT index)
{
	assert (index < pMS->load.preBuiltCount + pMS->load.numIndices);
		
	pMS->load.cur = index;
	if (index <= LOAD_TEAM_VIEW_SIZE / 2)
		pMS->load.top = 0;
	else
		pMS->load.top = index - LOAD_TEAM_VIEW_SIZE / 2;
}

BOOLEAN
DoLoadTeam (MELEE_STATE *pMS)
{
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return FALSE;

	SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN | MENU_SOUND_PAGEUP |
			MENU_SOUND_PAGEDOWN, MENU_SOUND_SELECT);

	if (!pMS->Initialized)
	{
		LockMutex (GraphicsLock);
		DrawFileStrings (pMS);
		SelectFileString (pMS, true);
		pMS->Initialized = TRUE;
		pMS->InputFunc = DoLoadTeam;
		UnlockMutex (GraphicsLock);
		return TRUE;
	}

	if (PulsedInputState.menu[KEY_MENU_SELECT] ||
			PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		if (PulsedInputState.menu[KEY_MENU_SELECT])
		{
			// Copy the selected fleet to the player.
			pMS->SideState[pMS->side].TeamImage =
					pMS->load.view[pMS->load.cur - pMS->load.top];
			pMS->SideState[pMS->side].star_bucks =
					GetTeamValue (&pMS->SideState[pMS->side].TeamImage);
			entireFleetChanged (pMS, pMS->side);
			teamStringChanged (pMS, pMS->side);
		}

		pMS->InputFunc = DoMelee;
		{
			RECT r;
			
			GetFrameRect (SetAbsFrameIndex (MeleeFrame, 28), &r);
			LockMutex (GraphicsLock);
			RepairMeleeFrame (&r);
			UnlockMutex (GraphicsLock);
		}
		return TRUE;
	}
	
	{
		COUNT newTop = pMS->load.top;
		COUNT newIndex = pMS->load.cur;

		if (PulsedInputState.menu[KEY_MENU_UP])
		{
			if (newIndex > 0)
			{
				newIndex--;
				if (newIndex < newTop)
					newTop = (newTop < LOAD_TEAM_VIEW_SIZE) ?
							0 : newTop - LOAD_TEAM_VIEW_SIZE;
			}
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			COUNT numEntries = pMS->load.numIndices + pMS->load.preBuiltCount;
			if (newIndex + 1 < numEntries)
			{
				newIndex++;
				if (newIndex >= pMS->load.bot)
					newTop = pMS->load.bot;
			}
		}
		else if (PulsedInputState.menu[KEY_MENU_PAGE_UP])
		{
			newIndex = (newIndex < LOAD_TEAM_VIEW_SIZE) ?
					0 : newIndex - LOAD_TEAM_VIEW_SIZE;
			newTop = (newTop < LOAD_TEAM_VIEW_SIZE) ?
					0 : newTop - LOAD_TEAM_VIEW_SIZE;
		}
		else if (PulsedInputState.menu[KEY_MENU_PAGE_DOWN])
		{
			COUNT numEntries = pMS->load.numIndices + pMS->load.preBuiltCount;
			if (newIndex + LOAD_TEAM_VIEW_SIZE < numEntries)
			{
				newIndex += LOAD_TEAM_VIEW_SIZE;
				newTop += LOAD_TEAM_VIEW_SIZE;
			}
			else
			{
				newIndex = numEntries - 1;
				if (newTop + LOAD_TEAM_VIEW_SIZE < numEntries &&
						numEntries > LOAD_TEAM_VIEW_SIZE)
					newTop = numEntries - LOAD_TEAM_VIEW_SIZE;
			}
		}

		if (newIndex != pMS->load.cur)
		{
			LockMutex (GraphicsLock);
			if (newTop == pMS->load.top)
				SelectFileString (pMS, false);
			else
			{
				pMS->load.top = newTop;
				DrawFileStrings (pMS);
			}
			pMS->load.cur = newIndex;
			UnlockMutex (GraphicsLock);
		}
	}

	return TRUE;
}

int
WriteTeamImage (TEAM_IMAGE *pTI, uio_Stream *save_fp)
{
	return WriteResFile (pTI, sizeof (*pTI), 1, save_fp);
}

void
SelectTeamByFileName (MELEE_STATE *pMS, const char *fileName)
{
	COUNT index = GetFleetIndexByFileName (pMS, fileName);
	if (index == (COUNT) -1)
		return;

	RefocusView (pMS, index);
}

void
LoadTeamList (MELEE_STATE *pMS)
{
	COUNT i;

	DestroyDirEntryTable (ReleaseDirEntryTable (pMS->load.dirEntries));
	pMS->load.dirEntries = CaptureDirEntryTable (
			LoadDirEntryTable (meleeDir, "", ".mle", match_MATCH_SUFFIX));
	
	if (pMS->load.entryIndices != NULL)
		HFree (pMS->load.entryIndices);
	pMS->load.numIndices = GetDirEntryTableCount (pMS->load.dirEntries);
	pMS->load.entryIndices = HMalloc (pMS->load.numIndices *
			sizeof pMS->load.entryIndices[0]);
	for (i = 0; i < pMS->load.numIndices; i++)
		pMS->load.entryIndices[i] = i;
}

BOOLEAN
DoSaveTeam (MELEE_STATE *pMS)
{
	STAMP MsgStamp;
	char file[NAME_MAX];
	uio_Stream *save_fp;
	CONTEXT OldContext;

	sprintf (file, "%s.mle", pMS->SideState[pMS->side].TeamImage.TeamName);

	LockMutex (GraphicsLock);
	OldContext = SetContext (ScreenContext);
	ConfirmSaveLoad (&MsgStamp);
	save_fp = res_OpenResFile (meleeDir, file, "wb");
	if (save_fp)
	{
		BOOLEAN err;

		err = (BOOLEAN)(WriteTeamImage (
				&pMS->SideState[pMS->side].TeamImage, save_fp) == 0);
		if (res_CloseResFile (save_fp) == 0)
			err = TRUE;
		if (err)
			save_fp = 0;
	}

	pMS->load.top = 0;
	pMS->load.cur = 0;

	if (save_fp == 0)
	{
		DrawStamp (&MsgStamp);
		DestroyDrawable (ReleaseDrawable (MsgStamp.frame));
		SetContext (OldContext);
		UnlockMutex (GraphicsLock);

		DeleteResFile (meleeDir, file);
		SaveProblem ();
	}

	LoadTeamList (pMS);
	SelectTeamByFileName (pMS, file);

	if (save_fp != 0)
	{
		DrawStamp (&MsgStamp);
		DestroyDrawable (ReleaseDrawable (MsgStamp.frame));
		SetContext (OldContext);
		UnlockMutex (GraphicsLock);
	}
	
	return (save_fp != 0);
}

static void
InitPreBuilt (MELEE_STATE *pMS)
{
	TEAM_IMAGE *list;

#define PREBUILT_COUNT 15
	pMS->load.preBuiltList =
			HMalloc (PREBUILT_COUNT * sizeof pMS->load.preBuiltList[0]);
	pMS->load.preBuiltCount = PREBUILT_COUNT;
#undef PREBUILT_COUNT

	{
		FleetShipIndex shipI = 0;
		int fleetI;

		for (fleetI = 0; fleetI < pMS->load.preBuiltCount; fleetI++)
			for (shipI = 0; shipI < MELEE_FLEET_SIZE; shipI++)
				pMS->load.preBuiltList[fleetI].ShipList[shipI] = MELEE_NONE;
	}

	list = pMS->load.preBuiltList;

	{
		/* "Balanced Team 1" */
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				GAME_STRING (MELEE_STRING_BASE + 4));
		list->ShipList[i++] = MELEE_ANDROSYNTH;
		list->ShipList[i++] = MELEE_CHMMR;
		list->ShipList[i++] = MELEE_DRUUGE;
		list->ShipList[i++] = MELEE_URQUAN;
		list->ShipList[i++] = MELEE_MELNORME;
		list->ShipList[i++] = MELEE_ORZ;
		list->ShipList[i++] = MELEE_SPATHI;
		list->ShipList[i++] = MELEE_SYREEN;
		list->ShipList[i++] = MELEE_UTWIG;
		list++;
	}

	{
		/* "Balanced Team 2" */
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				GAME_STRING (MELEE_STRING_BASE + 5));
		list->ShipList[i++] = MELEE_ARILOU;
		list->ShipList[i++] = MELEE_CHENJESU;
		list->ShipList[i++] = MELEE_EARTHLING;
		list->ShipList[i++] = MELEE_KOHR_AH;
		list->ShipList[i++] = MELEE_MYCON;
		list->ShipList[i++] = MELEE_YEHAT;
		list->ShipList[i++] = MELEE_PKUNK;
		list->ShipList[i++] = MELEE_SUPOX;
		list->ShipList[i++] = MELEE_THRADDASH;
		list->ShipList[i++] = MELEE_ZOQFOTPIK;
		list->ShipList[i++] = MELEE_SHOFIXTI;
		list++;
	}

	{
		/* "200 points" */
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				GAME_STRING (MELEE_STRING_BASE + 6));
		list->ShipList[i++] = MELEE_ANDROSYNTH;
		list->ShipList[i++] = MELEE_CHMMR;
		list->ShipList[i++] = MELEE_DRUUGE;
		list->ShipList[i++] = MELEE_MELNORME;
		list->ShipList[i++] = MELEE_EARTHLING;
		list->ShipList[i++] = MELEE_KOHR_AH;
		list->ShipList[i++] = MELEE_SUPOX;
		list->ShipList[i++] = MELEE_ORZ;
		list->ShipList[i++] = MELEE_SPATHI;
		list->ShipList[i++] = MELEE_ILWRATH;
		list->ShipList[i++] = MELEE_VUX;
		list++;
	}

	{
		/* "Behemoth Zenith" */
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				GAME_STRING (MELEE_STRING_BASE + 7));
		list->ShipList[i++] = MELEE_CHENJESU;
		list->ShipList[i++] = MELEE_CHENJESU;
		list->ShipList[i++] = MELEE_CHMMR;
		list->ShipList[i++] = MELEE_CHMMR;
		list->ShipList[i++] = MELEE_KOHR_AH;
		list->ShipList[i++] = MELEE_KOHR_AH;
		list->ShipList[i++] = MELEE_URQUAN;
		list->ShipList[i++] = MELEE_URQUAN;
		list->ShipList[i++] = MELEE_UTWIG;
		list->ShipList[i++] = MELEE_UTWIG;
		list++;
	}

	{
		/* "The Peeled Eyes" */
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				GAME_STRING (MELEE_STRING_BASE + 8));
		list->ShipList[i++] = MELEE_URQUAN;
		list->ShipList[i++] = MELEE_CHENJESU;
		list->ShipList[i++] = MELEE_MYCON;
		list->ShipList[i++] = MELEE_SYREEN;
		list->ShipList[i++] = MELEE_ZOQFOTPIK;
		list->ShipList[i++] = MELEE_SHOFIXTI;
		list->ShipList[i++] = MELEE_EARTHLING;
		list->ShipList[i++] = MELEE_KOHR_AH;
		list->ShipList[i++] = MELEE_MELNORME;
		list->ShipList[i++] = MELEE_DRUUGE;
		list->ShipList[i++] = MELEE_PKUNK;
		list->ShipList[i++] = MELEE_ORZ;
		list++;
	}

	{
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				"Ford's Fighters");
		list->ShipList[i++] = MELEE_CHMMR;
		list->ShipList[i++] = MELEE_ZOQFOTPIK;
		list->ShipList[i++] = MELEE_MELNORME;
		list->ShipList[i++] = MELEE_SUPOX;
		list->ShipList[i++] = MELEE_UTWIG;
		list->ShipList[i++] = MELEE_UMGAH;
		list++;
	}

	{
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				"Leyland's Lashers");
		list->ShipList[i++] = MELEE_ANDROSYNTH;
		list->ShipList[i++] = MELEE_EARTHLING;
		list->ShipList[i++] = MELEE_MYCON;
		list->ShipList[i++] = MELEE_ORZ;
		list->ShipList[i++] = MELEE_URQUAN;
		list++;
	}

	{
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				"The Gregorizers 200");
		list->ShipList[i++] = MELEE_ANDROSYNTH;
		list->ShipList[i++] = MELEE_CHMMR;
		list->ShipList[i++] = MELEE_DRUUGE;
		list->ShipList[i++] = MELEE_MELNORME;
		list->ShipList[i++] = MELEE_EARTHLING;
		list->ShipList[i++] = MELEE_KOHR_AH;
		list->ShipList[i++] = MELEE_SUPOX;
		list->ShipList[i++] = MELEE_ORZ;
		list->ShipList[i++] = MELEE_PKUNK;
		list->ShipList[i++] = MELEE_SPATHI;
		list++;
	}

	{
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				"300 point Armada!");
		list->ShipList[i++] = MELEE_ANDROSYNTH;
		list->ShipList[i++] = MELEE_CHMMR;
		list->ShipList[i++] = MELEE_CHENJESU;
		list->ShipList[i++] = MELEE_DRUUGE;
		list->ShipList[i++] = MELEE_EARTHLING;
		list->ShipList[i++] = MELEE_KOHR_AH;
		list->ShipList[i++] = MELEE_MELNORME;
		list->ShipList[i++] = MELEE_MYCON;
		list->ShipList[i++] = MELEE_ORZ;
		list->ShipList[i++] = MELEE_PKUNK;
		list->ShipList[i++] = MELEE_SPATHI;
		list->ShipList[i++] = MELEE_SUPOX;
		list->ShipList[i++] = MELEE_URQUAN;
		list->ShipList[i++] = MELEE_YEHAT;
		list++;
	}

	{
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				"Little Dudes with Attitudes");
		list->ShipList[i++] = MELEE_UMGAH;
		list->ShipList[i++] = MELEE_THRADDASH;
		list->ShipList[i++] = MELEE_SHOFIXTI;
		list->ShipList[i++] = MELEE_EARTHLING;
		list->ShipList[i++] = MELEE_VUX;
		list->ShipList[i++] = MELEE_ZOQFOTPIK;
		list++;
	}

	{
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				"New Alliance Ships");
		list->ShipList[i++] = MELEE_ARILOU;
		list->ShipList[i++] = MELEE_CHMMR;
		list->ShipList[i++] = MELEE_EARTHLING;
		list->ShipList[i++] = MELEE_ORZ;
		list->ShipList[i++] = MELEE_PKUNK;
		list->ShipList[i++] = MELEE_SHOFIXTI;
		list->ShipList[i++] = MELEE_SUPOX;
		list->ShipList[i++] = MELEE_SYREEN;
		list->ShipList[i++] = MELEE_UTWIG;
		list->ShipList[i++] = MELEE_ZOQFOTPIK;
		list->ShipList[i++] = MELEE_YEHAT;
		list->ShipList[i++] = MELEE_DRUUGE;
		list->ShipList[i++] = MELEE_THRADDASH;
		list->ShipList[i++] = MELEE_SPATHI;
		list++;
	}

	{
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				"Old Alliance Ships");
		list->ShipList[i++] = MELEE_ARILOU;
		list->ShipList[i++] = MELEE_CHENJESU;
		list->ShipList[i++] = MELEE_EARTHLING;
		list->ShipList[i++] = MELEE_MMRNMHRM;
		list->ShipList[i++] = MELEE_SHOFIXTI;
		list->ShipList[i++] = MELEE_SYREEN;
		list->ShipList[i++] = MELEE_YEHAT;
		list++;
	}

	{
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				"Old Hierarchy Ships");
		list->ShipList[i++] = MELEE_ANDROSYNTH;
		list->ShipList[i++] = MELEE_ILWRATH;
		list->ShipList[i++] = MELEE_MYCON;
		list->ShipList[i++] = MELEE_SPATHI;
		list->ShipList[i++] = MELEE_UMGAH;
		list->ShipList[i++] = MELEE_URQUAN;
		list->ShipList[i++] = MELEE_VUX;
		list++;
	}

	{
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				"Star Control 1");
		list->ShipList[i++] = MELEE_ANDROSYNTH;
		list->ShipList[i++] = MELEE_ARILOU;
		list->ShipList[i++] = MELEE_CHENJESU;
		list->ShipList[i++] = MELEE_EARTHLING;
		list->ShipList[i++] = MELEE_ILWRATH;
		list->ShipList[i++] = MELEE_MMRNMHRM;
		list->ShipList[i++] = MELEE_MYCON;
		list->ShipList[i++] = MELEE_SHOFIXTI;
		list->ShipList[i++] = MELEE_SPATHI;
		list->ShipList[i++] = MELEE_SYREEN;
		list->ShipList[i++] = MELEE_UMGAH;
		list->ShipList[i++] = MELEE_URQUAN;
		list->ShipList[i++] = MELEE_VUX;
		list->ShipList[i++] = MELEE_YEHAT;
		list++;
	}

	{
		FleetShipIndex i = 0;
		utf8StringCopy (list->TeamName, sizeof (list->TeamName),
				"Star Control 2");
		list->ShipList[i++] = MELEE_CHMMR;
		list->ShipList[i++] = MELEE_DRUUGE;
		list->ShipList[i++] = MELEE_KOHR_AH;
		list->ShipList[i++] = MELEE_MELNORME;
		list->ShipList[i++] = MELEE_ORZ;
		list->ShipList[i++] = MELEE_PKUNK;
		list->ShipList[i++] = MELEE_SLYLANDRO;
		list->ShipList[i++] = MELEE_SUPOX;
		list->ShipList[i++] = MELEE_THRADDASH;
		list->ShipList[i++] = MELEE_UTWIG;
		list->ShipList[i++] = MELEE_ZOQFOTPIK;
		list->ShipList[i++] = MELEE_ZOQFOTPIK;
		list->ShipList[i++] = MELEE_ZOQFOTPIK;
		list->ShipList[i++] = MELEE_ZOQFOTPIK;
		list++;
	}

	assert (list == pMS->load.preBuiltList + pMS->load.preBuiltCount);
}

static void
UninitPreBuilt (MELEE_STATE *pMS) {
	HFree (pMS->load.preBuiltList);
	pMS->load.preBuiltCount = 0;
}

void
InitMeleeLoadState (MELEE_STATE *pMS)
{
	pMS->load.entryIndices = NULL;
	InitPreBuilt (pMS);
}

void
UninitMeleeLoadState (MELEE_STATE *pMS)
{
	UninitPreBuilt (pMS);
	if (pMS->load.entryIndices != NULL)
		HFree (pMS->load.entryIndices);
}


