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

#include "melee.h"

#include "build.h"
#include "colors.h"
#include "controls.h"
#include "file.h"
#include "fmv.h"
#include "gamestr.h"
#include "globdata.h"
#include "intel.h"
#include "nameref.h"
#include "options.h"
#include "races.h"
#include "resinst.h"
#include "save.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "util.h"
#include "libs/graphics/drawable.h"
#include "libs/gfxlib.h"
#include "libs/inplib.h"
#include "libs/mathlib.h"


#include <assert.h>
#include <ctype.h>
#include <string.h>


//Added by Chris

void ConfirmSaveLoad (STAMP *MsgStamp);

//End Added by Chris
static void DrawMeleeShipStrings (PMELEE_STATE pMS, BYTE NewStarShip);


static int flash_selection_func (void *data);

enum
{
	CONTROLS_TOP,
	SAVE_TOP,
	LOAD_TOP,
	START_MELEE,
	LOAD_BOT,
	SAVE_BOT,
	CONTROLS_BOT,
	EDIT_MELEE
};

#define MELEE_X_OFFS 2
#define MELEE_Y_OFFS 21
#define MELEE_BOX_WIDTH 34
#define MELEE_BOX_HEIGHT 34
#define MELEE_BOX_SPACE 1

#define MENU_X_OFFS 29
#define NAME_AREA_HEIGHT 7
#define MELEE_WIDTH 149
#define MELEE_HEIGHT (48 + NAME_AREA_HEIGHT)

#define INFO_ORIGIN_X 4
#define INFO_WIDTH 58
#define TEAM_INFO_ORIGIN_Y 3
#define TEAM_INFO_HEIGHT (SHIP_INFO_HEIGHT + 75)
#define MODE_INFO_ORIGIN_Y (TEAM_INFO_HEIGHT + 6)
#define MODE_INFO_HEIGHT ((STATUS_HEIGHT - 3) - MODE_INFO_ORIGIN_Y)
#define RACE_INFO_ORIGIN_Y (SHIP_INFO_HEIGHT + 6)
#define RACE_INFO_HEIGHT ((STATUS_HEIGHT - 3) - RACE_INFO_ORIGIN_Y)

#define MELEE_BACKGROUND_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04)
#define MELEE_TITLE_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x0A), 0x0C)
#define MELEE_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x0A), 0x0C)
#define MELEE_TEAM_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E)

#define STATE_BACKGROUND_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)
#define STATE_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x14), 0x03)
#define ACTIVE_STATE_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x1F), 0x0B)
#define UNAVAILABLE_STATE_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09)
#define HI_STATE_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x1F), 0x0B)
#define HI_STATE_BACKGROUND_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09)

#define LIST_INFO_BACKGROUND_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x14), 0x05)
#define LIST_INFO_TITLE_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)
#define LIST_INFO_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x14, 0x14), 0x07)
#define LIST_INFO_CURENTRY_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)
#define HI_LIST_INFO_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04)
#define HI_LIST_INFO_BACKGROUND_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x1F), 0x0D)

#define TEAM_NAME_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (15, 16, 27), 0x00)
#define TEAM_NAME_EDIT_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (23, 24, 29), 0x00)
#define TEAM_NAME_EDIT_RECT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x14), 0x05)
#define TEAM_NAME_EDIT_CURS_COLOR \
		WHITE_COLOR

FRAME PickMeleeFrame;
static FRAME MeleeFrame;
static FRAME BuildPickFrame;
extern QUEUE master_q;
extern DWORD InTime;
PMELEE_STATE volatile pMeleeState;

static BOOLEAN DoMelee (PMELEE_STATE pMS);
static BOOLEAN DoEdit (PMELEE_STATE pMS);
static BOOLEAN DoPickShip (PMELEE_STATE pMS);
static void DrawTeamString (PMELEE_STATE pMS, COUNT HiLiteState);
static BOOLEAN DoLoadTeam (PMELEE_STATE pMS);
static void DrawFileStrings (PMELEE_STATE pMS, int HiLiteState);

static void
DrawMeleeIcon (COUNT which_icon)
{
	STAMP s;
			
	s.origin.x = s.origin.y = 0;
	s.frame = SetAbsFrameIndex (MeleeFrame, which_icon);
	DrawStamp (&s);
}

static void
GetShipBox (PRECT pRect, COUNT side, COUNT row, COUNT col)
{
	pRect->corner.x = MELEE_X_OFFS
			+ (col * (MELEE_BOX_WIDTH + MELEE_BOX_SPACE));
	pRect->corner.y = MELEE_Y_OFFS
			+ (side * (MELEE_Y_OFFS + MELEE_BOX_SPACE
					+ (NUM_MELEE_ROWS * (MELEE_BOX_HEIGHT + MELEE_BOX_SPACE))))
			+ (row * (MELEE_BOX_HEIGHT + MELEE_BOX_SPACE));
	pRect->extent.width = MELEE_BOX_WIDTH;
	pRect->extent.height = MELEE_BOX_HEIGHT;
}

static void
DrawShipBox (PMELEE_STATE pMS, BOOLEAN HiLite)
{
	RECT r;
	BYTE StarShip;

	GetShipBox (&r, pMS->side, pMS->row, pMS->col);

	BatchGraphics ();
	StarShip = pMS->TeamImage[pMS->side].ShipList[pMS->row][pMS->col];
	if (HiLite)
		DrawStarConBox (&r, 1,
				BUILD_COLOR (MAKE_RGB15 (0x7, 0x00, 0xC), 0x3E),
				BUILD_COLOR (MAKE_RGB15 (0xC, 0x00, 0x14), 0x3C),
				(BOOLEAN)(StarShip != (BYTE)~0),
				BUILD_COLOR (MAKE_RGB15 (0xA, 0x00, 0x11), 0x3D));
	else
		DrawStarConBox (&r, 1,
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x9), 0x56),
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0xE), 0x54),
				(BOOLEAN)(StarShip != (BYTE)~0),
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0xC), 0x55));

	if (StarShip != (BYTE)~0)
	{
		STAMP s;
		HSTARSHIP hStarShip;
		STARSHIPPTR StarShipPtr;

		hStarShip = GetStarShipFromIndex (&master_q, StarShip);
		StarShipPtr = LockStarShip (&master_q, hStarShip);
		s.origin.x = r.corner.x + (r.extent.width >> 1);
		s.origin.y = r.corner.y + (r.extent.height >> 1);
		s.frame = StarShipPtr->RaceDescPtr->ship_info.melee_icon;

		DrawStamp (&s);
		UnlockStarShip (&master_q, hStarShip);
	}
	UnbatchGraphics ();
}

static void
DrawControls (COUNT which_side, BOOLEAN HiLite)
{
	COUNT which_icon;

	if (PlayerControl[which_side] & HUMAN_CONTROL)
		which_icon = 0;
	else
	{
		switch (PlayerControl[which_side]
				& (STANDARD_RATING | GOOD_RATING | AWESOME_RATING))
		{
			case STANDARD_RATING:
				which_icon = 1;
				break;
			case GOOD_RATING:
				which_icon = 2;
				break;
			case AWESOME_RATING:
				which_icon = 3;
				break;
			default:
				// Should not happen. Satisfying compiler.
				which_icon = 0;
				break;
		}
	}

	DrawMeleeIcon (1 + (8 * (1 - which_side)) + (HiLite ? 4 : 0) + which_icon);
}

static void
DrawPickFrame (PMELEE_STATE pMS)
{
	RECT r, r0, r1, ship_r;
	STAMP s;
				
	GetShipBox (&r0, 0, 0, 0),
	GetShipBox (&r1, 1, NUM_MELEE_ROWS - 1, NUM_MELEE_COLUMNS - 1),
	BoxUnion (&r0, &r1, &ship_r);

	s.frame = SetAbsFrameIndex (BuildPickFrame, 0);
	GetFrameRect (s.frame, &r);
	r.corner.x = -(ship_r.corner.x
			+ ((ship_r.extent.width - r.extent.width) >> 1));
	if (pMS->side)
		r.corner.y = -ship_r.corner.y;
	else
		r.corner.y = -(ship_r.corner.y
				+ (ship_r.extent.height - r.extent.height));
	SetFrameHot (s.frame, MAKE_HOT_SPOT (r.corner.x, r.corner.y));
	s.origin.x = s.origin.y = 0;
	DrawStamp (&s);

	UnlockMutex (GraphicsLock);
	DrawMeleeShipStrings (pMS, (BYTE)pMS->CurIndex);
	LockMutex (GraphicsLock);
}

static void
RepairMeleeFrame (PRECT pRect)
{
	RECT r;
	CONTEXT OldContext;
	RECT OldRect;

	r.corner.x = pRect->corner.x + SAFE_X;
	r.corner.y = pRect->corner.y + SAFE_Y;
	r.extent = pRect->extent;
	if (r.corner.y & 1)
	{
		--r.corner.y;
		++r.extent.height;
	}

	OldContext = SetContext (SpaceContext);
	GetContextClipRect (&OldRect);
	SetContextClipRect (&r);
	SetFrameHot (Screen, MAKE_HOT_SPOT (r.corner.x - SAFE_X, r.corner.y - SAFE_Y));
	BatchGraphics ();

	DrawMeleeIcon (0);
	DrawMeleeIcon (26);
	{
		COUNT old_side, old_row, old_col;
	
		old_side = pMeleeState->side;
		old_row = pMeleeState->row;
		old_col = pMeleeState->col;
		for (pMeleeState->side = 0;
				pMeleeState->side < NUM_SIDES;
				++pMeleeState->side)
		{
			DrawControls (pMeleeState->side, FALSE);
			for (pMeleeState->row = 0;
					pMeleeState->row < NUM_MELEE_ROWS;
					++pMeleeState->row)
			{
				for (pMeleeState->col = 0;
						pMeleeState->col < NUM_MELEE_COLUMNS;
						++pMeleeState->col)
					DrawShipBox (pMeleeState, FALSE);
			}

			DrawTeamString (pMeleeState, 0);
		}
		pMeleeState->side = old_side;
		pMeleeState->row = old_row;
		pMeleeState->col = old_col;
	}
	
	if (pMeleeState->InputFunc == DoPickShip)
		DrawPickFrame (pMeleeState);
		
	UnbatchGraphics ();
	SetFrameHot (Screen, MAKE_HOT_SPOT (0, 0));
	SetContextClipRect (&OldRect);
	SetContext (OldContext);
}

static void
DrawTeamString (PMELEE_STATE pMS, COUNT HiLiteState)
{
	RECT r;
	TEXT lfText;

	r.corner.x = MELEE_X_OFFS - 1;
	r.corner.y = (pMS->side + 1) * (MELEE_Y_OFFS
			+ ((MELEE_BOX_HEIGHT + MELEE_BOX_SPACE) * NUM_MELEE_ROWS + 2));
	r.extent.width = NUM_MELEE_COLUMNS * (MELEE_BOX_WIDTH + MELEE_BOX_SPACE);
	r.extent.height = 13;
	if (HiLiteState == 4)
	{
		RepairMeleeFrame (&r);
		return;
	}
		
	if (HiLiteState & 1)
		r.extent.width -= 29;

	SetContextFont (MicroFont);

	lfText.pStr = pMS->TeamImage[pMS->side].TeamName;
	lfText.baseline.y = r.corner.y + r.extent.height - 3;

	lfText.baseline.x = r.corner.x + 1;
	lfText.align = ALIGN_LEFT;
	lfText.CharCount = wstrlen (lfText.pStr); // (COUNT)~0;

	BatchGraphics ();
	if (!(HiLiteState & 1))
	{
		TEXT rtText;
		UNICODE buf[10];

		wsprintf (buf, "%d", pMS->star_bucks[pMS->side]);
		rtText.pStr = buf;
		rtText.align = ALIGN_RIGHT;
		rtText.CharCount = (COUNT)~0;
		rtText.baseline.y = lfText.baseline.y;
		rtText.baseline.x = lfText.baseline.x + r.extent.width - 1;

		SetContextForeGroundColor (HiLiteState == 0
				? TEAM_NAME_TEXT_COLOR : TEAM_NAME_EDIT_TEXT_COLOR);
		font_DrawText (&lfText);
		font_DrawText (&rtText);
	}
	else
	{
		COUNT i;
		RECT text_r;
		BYTE char_deltas[MAX_TEAM_CHARS];
		PBYTE pchar_deltas;

		TextRect (&lfText, &text_r, char_deltas);

		text_r = r;
		SetContextForeGroundColor (TEAM_NAME_EDIT_RECT_COLOR);
		DrawFilledRectangle (&text_r);

		pchar_deltas = char_deltas;
		for (i = pMS->CurIndex; i > 0; --i)
			text_r.corner.x += (SIZE)*pchar_deltas++;
		if (pMS->CurIndex < lfText.CharCount) /* end of line */
			--text_r.corner.x;
		text_r.extent.width = 1;
#if 0
		/* Code for keyboardless systems */
		else
		{
			if (pMS->CurIndex == lfText.CharCount) /* end of line */
				text_r.extent.width = 1;
			else
				text_r.extent.width = (SIZE)*pchar_deltas;
		}
#endif
		++text_r.corner.x;
		++text_r.corner.y;
		text_r.extent.height -= 2;
		SetContextForeGroundColor (TEAM_NAME_EDIT_CURS_COLOR);
		DrawFilledRectangle (&text_r);

		SetContextForeGroundColor (BLACK_COLOR); // TEAM_NAME_EDIT_TEXT_COLOR);
		font_DrawText (&lfText);
	}
	UnbatchGraphics ();
}

static void
DrawPickIcon (COUNT iship, BYTE DrawErase)
{
	STAMP s;
	HSTARSHIP hStarShip;
	STARSHIPPTR StarShipPtr;
	RECT r;

	GetFrameRect (BuildPickFrame, &r);

	hStarShip = GetStarShipFromIndex (&master_q, iship);
	StarShipPtr = LockStarShip (&master_q, hStarShip);
	s.origin.x = r.corner.x + 20 + (iship % NUM_PICK_COLS) * 18;
	s.origin.y = r.corner.y +  5 + (iship / NUM_PICK_COLS) * 18;
	s.frame = StarShipPtr->RaceDescPtr->ship_info.icons;
	if (DrawErase)
	{	// draw icon
		DrawStamp (&s);
	}
	else
	{	// erase icon
		COLOR OldColor;

		OldColor = SetContextForeGroundColor (BLACK_COLOR);
		DrawFilledStamp (&s);
		SetContextForeGroundColor (OldColor);
	}
	UnlockStarShip (&master_q, hStarShip);
}

static void
Deselect (BYTE opt)
{
	switch (opt)
	{
		case START_MELEE:
			DrawMeleeIcon (25);
			break;
		case LOAD_TOP:
		case LOAD_BOT:
			if (pMeleeState->InputFunc != DoLoadTeam)
				DrawMeleeIcon (opt == LOAD_TOP ? 17 : 22);
			else
				DrawFileStrings (pMeleeState, 0);
			break;
		case SAVE_TOP:
			DrawMeleeIcon (18);
			break;
		case SAVE_BOT:
			DrawMeleeIcon (21);
			break;
		case CONTROLS_TOP:
		case CONTROLS_BOT:
		{
			COUNT which_side;
			
			which_side = opt == CONTROLS_TOP ? 1 : 0;
			DrawControls (which_side, FALSE);
			break;
		}
		case EDIT_MELEE:
			if (pMeleeState->InputFunc == DoEdit)
			{
				if (pMeleeState->row < NUM_MELEE_ROWS)
					DrawShipBox (pMeleeState, FALSE);
				else if (pMeleeState->CurIndex == (BYTE)~0)
					DrawTeamString (pMeleeState, 0);
			}
			else if (pMeleeState->InputFunc == DoPickShip)
			{
				DrawPickIcon (pMeleeState->CurIndex, 1);
			}
			break;
	}
}

static void
Select (BYTE opt)
{
	switch (opt)
	{
		case START_MELEE:
			DrawMeleeIcon (26);
			break;
		case LOAD_TOP:
		case LOAD_BOT:
			if (pMeleeState->InputFunc != DoLoadTeam)
				DrawMeleeIcon (opt == LOAD_TOP ? 19 : 24);
			else
				DrawFileStrings (pMeleeState, 1);
			break;
		case SAVE_TOP:
			DrawMeleeIcon (20);
			break;
		case SAVE_BOT:
			DrawMeleeIcon (23);
			break;
		case CONTROLS_TOP:
		case CONTROLS_BOT:
		{
			COUNT which_side;
					
			which_side = opt == CONTROLS_TOP ? 1 : 0;
			DrawControls (which_side, TRUE);
			break;
		}
		case EDIT_MELEE:
			if (pMeleeState->InputFunc == DoEdit)
			{
				if (pMeleeState->row < NUM_MELEE_ROWS)
					DrawShipBox (pMeleeState, TRUE);
				else if (pMeleeState->CurIndex == (BYTE)~0)
					DrawTeamString (pMeleeState, 2);
			}
			else if (pMeleeState->InputFunc == DoPickShip)
			{
				DrawPickIcon (pMeleeState->CurIndex, 0);
			}
			break;
	}
}

static int
flash_selection_func (void *data)
{
	DWORD TimeIn;
	Task task = (Task) data;
	
	TimeIn = GetTimeCounter ();
	while (!Task_ReadState (task, TASK_EXIT))
	{
#define FLASH_RATE (ONE_SECOND / 8)
		CONTEXT OldContext;

		LockMutex (GraphicsLock);
		OldContext = SetContext (SpaceContext);
		Deselect (pMeleeState->MeleeOption);
		SetContext (OldContext);
		UnlockMutex (GraphicsLock);
		SleepThreadUntil (TimeIn + FLASH_RATE);
		TimeIn = GetTimeCounter ();

		LockMutex (GraphicsLock);
		OldContext = SetContext (SpaceContext);
		Select (pMeleeState->MeleeOption);
		SetContext (OldContext);
		UnlockMutex (GraphicsLock);
		SleepThreadUntil (TimeIn + FLASH_RATE);
		TimeIn = GetTimeCounter ();
	}

	FinishTask (task);

	return 0;
}

static void
InitMelee (PMELEE_STATE pMS)
{
	RECT r;

	SetContext (SpaceContext);
	SetContextFGFrame (Screen);
	SetContextClipRect (NULL_PTR);
	SetContextBackGroundColor (BLACK_COLOR);
	ClearDrawable ();
	r.corner.x = SAFE_X;
	r.corner.y = SAFE_Y;
	r.extent.width = SCREEN_WIDTH - (SAFE_X * 2);
	r.extent.height = SCREEN_HEIGHT - (SAFE_Y * 2);
	SetContextClipRect (&r);

	r.corner.x = r.corner.y = 0;
	RepairMeleeFrame (&r);
	
	pMS->flash_task = AssignTask (flash_selection_func, 2048,
			"flash melee selection");
}

static void
DrawMeleeShipStrings (PMELEE_STATE pMS, BYTE NewStarShip)
{
	RECT r, OldRect;
	HSTARSHIP hStarShip;
	STARSHIPPTR StarShipPtr;
	CONTEXT OldContext;

	LockMutex (GraphicsLock);

	OldContext = SetContext (StatusContext);
	GetContextClipRect (&OldRect);
	r = OldRect;
	r.corner.x += ((SAFE_X << 1) - 32) + MENU_X_OFFS;
	r.corner.y += 76;
	r.extent.height = SHIP_INFO_HEIGHT;
	SetContextClipRect (&r);
	BatchGraphics ();

	if (NewStarShip == (BYTE)~0)
	{
		RECT r;
		TEXT t;

		ClearShipStatus (BAD_GUY_YOFFS);
		SetContextFont (StarConFont);
		r.corner.x = 3;
		r.corner.y = 4;
		r.extent.width = 57;
		r.extent.height = 60;
		SetContextForeGroundColor (BLACK_COLOR);
		DrawRectangle (&r);
		t.baseline.x = STATUS_WIDTH >> 1;
		t.baseline.y = 32;
		t.align = ALIGN_CENTER;
		if (pMS->row < NUM_MELEE_ROWS)
		{
			t.pStr = GAME_STRING (MELEE_STRING_BASE + 0);  // "Empty"
			t.CharCount = (COUNT)~0;
			font_DrawText (&t);
			t.pStr = GAME_STRING (MELEE_STRING_BASE + 1);  // "Slot"
		}
		else
		{
			t.pStr = GAME_STRING (MELEE_STRING_BASE + 2);  // "Team"
			t.CharCount = (COUNT)~0;
			font_DrawText (&t);
			t.pStr = GAME_STRING (MELEE_STRING_BASE + 3);  // "Name"
		}
		t.baseline.y += TINY_TEXT_HEIGHT;
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
	}
	else
	{
		hStarShip = GetStarShipFromIndex (&master_q, NewStarShip);
		StarShipPtr = LockStarShip (&master_q, hStarShip);

		InitShipStatus (StarShipPtr, NULL_PTR);

		UnlockStarShip (&master_q, hStarShip);
	}

	UnbatchGraphics ();
	SetContextClipRect (&OldRect);
	SetContext (OldContext);

	UnlockMutex (GraphicsLock);
}

static COUNT
GetTeamValue (TEAM_IMAGE *pTI)
{
	COUNT row, col, val;

	val = 0;
	for (row = 0; row < NUM_MELEE_ROWS; ++row)
	{
		for (col = 0; col < NUM_MELEE_COLUMNS; ++col)
		{
			BYTE StarShip;

			StarShip = pTI->ShipList[row][col];
			if (StarShip != (BYTE) ~0)
			{
				HSTARSHIP hStarShip;

				hStarShip = GetStarShipFromIndex (&master_q, StarShip);
				if (hStarShip == 0)
					pTI->ShipList[row][col] = (BYTE)~0;
				else
				{
					STARSHIPPTR StarShipPtr;

					StarShipPtr = LockStarShip (&master_q, hStarShip);
					val += StarShipPtr->RaceDescPtr->ship_info.ship_cost;
					UnlockStarShip (&master_q, hStarShip);
				}
			}
		}
	}
	
	return (val);
}

static int
ReadTeamImage (TEAM_IMAGE *pTI, uio_Stream *load_fp)
{
	return (ReadResFile (pTI, sizeof (*pTI), 1, load_fp) == 1 ? 0 : -1);
}

static int
LoadTeamImage(DIRENTRY DirEntry, TEAM_IMAGE* pTI,
		UNICODE* pFilePath)
{
	UNICODE file[NAME_MAX];	// local buf if needed
	UNICODE *pfile;
	uio_Stream *load_fp;
	int status;

	pfile = pFilePath != NULL_PTR ? pFilePath : file;

	GetDirEntryContents (DirEntry, (STRINGPTR)pfile, FALSE);
	if ((load_fp = res_OpenResFile (meleeDir, pfile, "rb")) == 0)
		status = -1;
	else
	{
		if (LengthResFile (load_fp) != sizeof (*pTI))
			status = -1;
		else
			status = ReadTeamImage (pTI, load_fp);
		res_CloseResFile (load_fp);
	}

	return status;
}

static void
DrawFileStrings (PMELEE_STATE pMS, int HiLiteState)
{
#define ENTRY_HEIGHT 32
	COORD y;
	COUNT top, bot;
	TEXT Text, rtText;
	CONTEXT OldContext;
	UNICODE buf[60];

	Text.baseline.x = 5;
	y = 34;
	Text.align = ALIGN_LEFT;

	rtText.pStr = buf;
	rtText.align = ALIGN_RIGHT;
	rtText.baseline.x = Text.baseline.x + NUM_MELEE_COLUMNS
			* (MELEE_BOX_WIDTH + MELEE_BOX_SPACE) - 1;
				
	top = pMS->TopTeamIndex;

	if (HiLiteState == 1)
	{
		COUNT new_;

		new_ = pMS->CurIndex;
		bot = pMS->BotTeamIndex;

		if (new_ < top || new_ > bot)
		{
			if (new_ < top)
				top = new_;
			else
				top += new_ - bot;
			pMS->TopTeamIndex = top;

			HiLiteState = -1;
		}
	}

	OldContext = SetContext (SpaceContext);
	SetContextFont (MicroFont);
	BatchGraphics ();
	if (HiLiteState != -1)
	{
		bot = pMS->CurIndex - top;
		Text.baseline.y = y + bot * ENTRY_HEIGHT;
		Text.pStr = pMS->FileList[bot].TeamName;
		Text.CharCount = (COUNT)~0;
		SetContextForeGroundColor (HiLiteState == 0 ?
				BUILD_COLOR (MAKE_RGB15 (15, 16, 27), 0x00)
				: BUILD_COLOR (MAKE_RGB15 (23, 24, 29), 0x00));
		font_DrawText (&Text);

		rtText.baseline.y = Text.baseline.y;
		wsprintf (buf, "%d", GetTeamValue (&pMS->FileList[bot]));
		rtText.CharCount = (COUNT)~0;
		font_DrawText (&rtText);
	}
	else
	{
		COUNT teams_left;

		DrawMeleeIcon (28);

		Text.baseline.y = y;
		teams_left = (COUNT)(
				GetDirEntryTableCount (pMS->TeamDE) + NUM_PREBUILT - top);
		if (teams_left)
		{
			bot = top - 1;
			do
			{
				COUNT row, col;
				STAMP s;

				if (++bot < NUM_PREBUILT)
					pMS->FileList[bot - top] = pMS->PreBuiltList[bot];
				else
				{
					pMS->TeamDE = SetAbsDirEntryTableIndex (
							pMS->TeamDE, bot - NUM_PREBUILT
							);
					if (-1 == LoadTeamImage (pMS->TeamDE,
							&pMS->FileList[bot - top], NULL_PTR))
					{
						pMS->FileList[bot - top] = pMS->PreBuiltList[0];
					}
				}

				{
					Text.pStr = pMS->FileList[bot - top].TeamName;
					Text.CharCount = (COUNT)~0;
					SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (15, 16, 27), 0x00));
					font_DrawText (&Text);

					rtText.baseline.y = Text.baseline.y;
					wsprintf (buf, "%d", GetTeamValue (&pMS->FileList[bot - top]));
					rtText.CharCount = (COUNT)~0;
					font_DrawText (&rtText);
				}

				s.origin.x = Text.baseline.x + 1;
				s.origin.y = Text.baseline.y + 4;
				for (row = 0; row < NUM_MELEE_ROWS; ++row)
				{
					for (col = 0; col < NUM_MELEE_COLUMNS; ++col)
					{
						BYTE StarShip;
						
						StarShip = pMS->FileList[bot - top].ShipList[row][col];
						if (StarShip != (BYTE)~0)
						{
							HSTARSHIP hStarShip;
							STARSHIPPTR StarShipPtr;

							hStarShip = GetStarShipFromIndex (&master_q, StarShip);
							StarShipPtr = LockStarShip (&master_q, hStarShip);
							s.frame = StarShipPtr->RaceDescPtr->ship_info.icons;
							DrawStamp (&s);
							s.origin.x += 17;
							UnlockStarShip (&master_q, hStarShip);
						}
					}
				}

				Text.baseline.y += ENTRY_HEIGHT;
			} while (--teams_left && bot - top < MAX_VIS_TEAMS - 1);
			pMS->BotTeamIndex = bot;
		}
	}
	UnbatchGraphics ();
	SetContext (OldContext);
}

static BOOLEAN
DoLoadTeam (PMELEE_STATE pMS)
{
	SIZE index;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN | MENU_SOUND_PAGEUP |
			MENU_SOUND_PAGEDOWN, MENU_SOUND_SELECT);

	if (!pMS->Initialized)
	{
		LockMutex (GraphicsLock);
		Select (pMS->MeleeOption);
		if ((pMS->TopTeamIndex = pMS->CurIndex) == (COUNT)~0)
			pMS->TopTeamIndex = pMS->CurIndex = 0;
		else if (pMS->TopTeamIndex <= MAX_VIS_TEAMS / 2)
			pMS->TopTeamIndex = 0;
		else
			pMS->TopTeamIndex -= MAX_VIS_TEAMS / 2;

		DrawFileStrings (pMS, -1);
		pMS->Initialized = TRUE;
		pMS->InputFunc = DoLoadTeam;
		UnlockMutex (GraphicsLock);
	}
	else if (PulsedInputState.key[KEY_MENU_SELECT] |
			PulsedInputState.key[KEY_MENU_CANCEL])
	{
		if (!PulsedInputState.key[KEY_MENU_CANCEL])
		{
			pMS->TeamImage[pMS->side] = pMS->FileList[
					pMS->CurIndex - pMS->TopTeamIndex];
			pMS->star_bucks[pMS->side] =
					GetTeamValue (&pMS->TeamImage[pMS->side]);
		}

		pMS->InputFunc = DoMelee;
		{
			RECT r;
			
			GetFrameRect (SetAbsFrameIndex (MeleeFrame, 28), &r);
			LockMutex (GraphicsLock);
			RepairMeleeFrame (&r);
			UnlockMutex (GraphicsLock);
		}
		InTime = GetTimeCounter ();
	}
	else
	{
		SIZE old_index, NewTop;

		NewTop = pMS->TopTeamIndex;
		index = old_index = pMS->CurIndex;
		if (PulsedInputState.key[KEY_MENU_UP])
		{
			if (index-- == 0)
				index = 0;

			if (index < NewTop && (NewTop -= MAX_VIS_TEAMS) < 0)
				NewTop = 0;
		}
		else if (PulsedInputState.key[KEY_MENU_DOWN])
		{
			if ((int)index < (int)GetDirEntryTableCount (pMS->TeamDE) +
					NUM_PREBUILT - 1)
				++index;

			if ((int)index > (int)pMS->BotTeamIndex)
				NewTop = index;
		}
		else if (PulsedInputState.key[KEY_MENU_PAGE_UP])
		{
			index -= MAX_VIS_TEAMS;
			if (index < 0)
			{
				index = 0;
				NewTop = 0;
			}
			else
			{
				NewTop -= MAX_VIS_TEAMS;
				if (NewTop < 0)
					NewTop = 0;
			}
		}
		else if (PulsedInputState.key[KEY_MENU_PAGE_DOWN])
		{
			index += MAX_VIS_TEAMS;
			if ((int) index <
					(int)(GetDirEntryTableCount (pMS->TeamDE) + NUM_PREBUILT))
				NewTop += MAX_VIS_TEAMS;
			else
			{
				index = GetDirEntryTableCount (pMS->TeamDE) + NUM_PREBUILT - 1;
				if (index - (MAX_VIS_TEAMS - 1) > NewTop)
				{
					NewTop = index - (MAX_VIS_TEAMS - 1);
					if (NewTop < 0)
						NewTop = 0;
				}
			}
		}

		if (index != old_index)
		{
			LockMutex (GraphicsLock);
			if ((int)NewTop == (int)pMS->TopTeamIndex)
				Deselect (pMS->MeleeOption);
			else
			{
				pMS->TopTeamIndex = NewTop;
				DrawFileStrings (pMS, -1);
			}
			pMS->CurIndex = index;
			UnlockMutex (GraphicsLock);
		}
	}

	return (TRUE);
}

// XXX: TODO: to be merged with DoTextEntry in gameopt.c
static UWORD
DoTextEntry (PMELEE_STATE pMS)
{
	UWORD ch;
	UNICODE *pStr, *pBaseStr;
	COUNT len, max_chars;
	BOOLEAN left = FALSE, right = FALSE;

	if (pMS->MeleeOption == EDIT_MELEE)
	{
		pBaseStr = pMS->TeamImage[pMS->side].TeamName;
		max_chars = MAX_TEAM_CHARS;
	}
	else
	{
		// Should not happen. Satisfying compiler.
		return 0;
	}

	ch = GetCharacter ();

	pStr = &pBaseStr[pMS->CurIndex];
	len = wstrlen (pStr);
	switch (ch)
	{
	case SK_DELETE:
		if (len)
		{
			memmove (pStr, pStr + 1, len * sizeof (*pStr));
			len = (COUNT)~0;
		}
		break;
	case '\b':
		if (pMS->CurIndex)
		{
			memmove (pStr - 1, pStr, (len + 1) * sizeof (*pStr));
			--pMS->CurIndex;
			
			len = (COUNT)~0;
		}
		break;
	case SK_LF_ARROW:
		left = TRUE;
		break;
	case SK_RT_ARROW:
		right = TRUE;
		break;
	default:
		if (isprint (ch) && (len + pStr - pBaseStr < (int)(max_chars)))
		{
			memmove (pStr + 1, pStr, (len + 1) * sizeof (*pStr));
			*pStr++ = ch;
			++pMS->CurIndex;
			
			len = (COUNT)~0;
		}
		break;
	}

	if (len == (COUNT)~0)
		DrawTeamString (pMS, 1);
	else if (left)
	{
		if (pMS->CurIndex)
		{
			--pMS->CurIndex;
			DrawTeamString (pMS, 1);
		}
	}
	else if (right)
	{
		if (len)
		{
			++pMS->CurIndex;
			DrawTeamString (pMS, 1);
		}
	}

	return (ch);
}

static int
WriteTeamImage (TEAM_IMAGE *pTI, uio_Stream *save_fp)
{
	return (WriteResFile (pTI, sizeof (*pTI), 1, save_fp));
}

static void
LoadTeamList (PMELEE_STATE pMS, UNICODE *pbuf)
{
	COUNT num_entries;
	char file[NAME_MAX];

GetNewList:
	DestroyDirEntryTable (ReleaseDirEntryTable (pMS->TeamDE));
	pMS->TeamDE = CaptureDirEntryTable (
			LoadDirEntryTable (meleeDir, "", ".mle", match_MATCH_SUFFIX,
			&num_entries));

	pMS->CurIndex = 0;
	while (num_entries--)
	{
		int status;
		TEAM_IMAGE TI;

		status = LoadTeamImage(pMS->TeamDE, &TI, file);

		if (status == -1)
		{
			DeleteResFile (meleeDir, file);
			goto GetNewList;
		}

		if (pbuf && wstricmp (file, pbuf) == 0)
		{
			pMS->CurIndex = GetDirEntryTableIndex (pMS->TeamDE) + NUM_PREBUILT;
			pbuf = 0;
		}
		pMS->TeamDE = SetRelDirEntryTableIndex (pMS->TeamDE, 1);
	}
	pMS->TeamDE = SetAbsDirEntryTableIndex (pMS->TeamDE, 0);
}

static BOOLEAN
DoSaveTeam (PMELEE_STATE pMS)
{
	STAMP MsgStamp;
	char file[NAME_MAX];
	uio_Stream *save_fp;
	CONTEXT OldContext;

	sprintf (file, "%s.mle", pMS->TeamImage[pMS->side].TeamName);

	LockMutex (GraphicsLock);
	OldContext = SetContext (ScreenContext);
	ConfirmSaveLoad (&MsgStamp);
	save_fp = res_OpenResFile (meleeDir, file, "wb");
	if (save_fp)
	{
		BOOLEAN err;

		err = (BOOLEAN)(WriteTeamImage (&pMS->TeamImage[pMS->side], save_fp) == 0);
		if (res_CloseResFile (save_fp) == 0)
			err = TRUE;
		if (err)
			save_fp = 0;
	}

	pMS->CurIndex = 0;
	if (save_fp == 0)
	{
		DrawStamp (&MsgStamp);
		DestroyDrawable (ReleaseDrawable (MsgStamp.frame));
		SetContext (OldContext);
		UnlockMutex (GraphicsLock);

		DeleteResFile (meleeDir, file);
		SaveProblem ();
	}

	LoadTeamList (pMS, file);

	if (save_fp)
	{
		DrawStamp (&MsgStamp);
		DestroyDrawable (ReleaseDrawable (MsgStamp.frame));
		SetContext (OldContext);
		UnlockMutex (GraphicsLock);
	}
	
	return (save_fp != 0);
}

static void
DeleteCurrentShip (PMELEE_STATE pMS)
{
	RECT r;
	HSTARSHIP hStarShip;
	STARSHIPPTR StarShipPtr;
	int CurIndex;

	CurIndex = pMS->TeamImage[pMS->side].ShipList[pMS->row][pMS->col];
	hStarShip = GetStarShipFromIndex (&master_q, CurIndex);
	StarShipPtr = LockStarShip (&master_q, hStarShip);
	if (StarShipPtr)
	{
		pMS->star_bucks[pMS->side] -=
			StarShipPtr->RaceDescPtr->ship_info.ship_cost;
		UnlockStarShip (&master_q, hStarShip);
	
		pMS->TeamImage[pMS->side].ShipList[pMS->row][pMS->col] = (BYTE)~0;
	}
	LockMutex (GraphicsLock);
	GetShipBox (&r, pMS->side, pMS->row, pMS->col);
	RepairMeleeFrame (&r);

	DrawTeamString (pMS, 4);
	UnlockMutex (GraphicsLock);

}

static void
AdvanceCursor (PMELEE_STATE pMS)
{
	if (++pMS->col == NUM_MELEE_COLUMNS)
	{
		if (++pMS->row < NUM_MELEE_ROWS)
			pMS->col = 0;
		else
		{
			pMS->col = NUM_MELEE_COLUMNS - 1;
			pMS->row = NUM_MELEE_ROWS - 1;
		}
	}
}

static BOOLEAN
DoEdit (PMELEE_STATE pMS)
{
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT | MENU_SOUND_DELETE);
	if (!pMS->Initialized)
	{
		pMS->CurIndex = pMS->TeamImage[pMS->side].ShipList[pMS->row][pMS->col];
		DrawMeleeShipStrings (pMS, (BYTE)pMS->CurIndex);

		pMS->Initialized = TRUE;
		pMS->InputFunc = DoEdit;
	}
	else if ((pMS->row < NUM_MELEE_ROWS || pMS->CurIndex == (BYTE)~0)
			&& (PulsedInputState.key[KEY_MENU_CANCEL]
			|| (PulsedInputState.key[KEY_MENU_RIGHT]
			&& (pMS->col == NUM_MELEE_COLUMNS - 1 || pMS->row == NUM_MELEE_ROWS))))
	{
		LockMutex (GraphicsLock);
		Deselect (EDIT_MELEE);
		pMS->CurIndex = (COUNT)~0;
		pMS->MeleeOption = START_MELEE;
		pMS->InputFunc = DoMelee;
		UnlockMutex (GraphicsLock);
		InTime = GetTimeCounter ();
	}
	else if (pMS->row < NUM_MELEE_ROWS
			&& (PulsedInputState.key[KEY_MENU_SELECT] || PulsedInputState.key[KEY_MENU_SPECIAL]))
	{
		if (PulsedInputState.key[KEY_MENU_SELECT])
			pMS->Initialized = 0;
		else
			pMS->Initialized = -1;
		TFB_ResetControls ();
		DoPickShip (pMS);
	}
	else if (pMS->row < NUM_MELEE_ROWS && PulsedInputState.key[KEY_MENU_DELETE])
	{
		DeleteCurrentShip (pMS);
		AdvanceCursor (pMS);
	}
	else
	{
		COUNT side, row, col;

		side = pMS->side;
		row = pMS->row;
		col = pMS->col;

		if (row == NUM_MELEE_ROWS)
		{
			if (pMS->CurIndex != (BYTE)~0)
			{
				UNICODE ch;
				LockMutex (GraphicsLock);
				if (pMS->Initialized == 1)
				{
					FlushInput ();
					pMS->Initialized = 2;
				}
				else if (((ch = DoTextEntry (pMS)) == '\n') || (ch == '\r'))
				{
					pMS->CurIndex = (BYTE)~0;
					DrawTeamString (pMS, 4);
					pMS->Initialized = 1;
					DisableCharacterMode ();
				}
				UnlockMutex (GraphicsLock);
				return (TRUE);
			}
			else if (PulsedInputState.key[KEY_MENU_SELECT])
			{
				pMS->CurIndex = 0;
				LockMutex (GraphicsLock);
				DrawTeamString (pMS, 1);
				UnlockMutex (GraphicsLock);
				EnableCharacterMode ();
				return (TRUE);
			}
		}

		{
			if (PulsedInputState.key[KEY_MENU_LEFT])
			{
				if (col-- == 0)
					col = 0;
			}
			else if (PulsedInputState.key[KEY_MENU_RIGHT])
			{
				if (++col == NUM_MELEE_COLUMNS)
					col = NUM_MELEE_COLUMNS - 1;
			}

			if (PulsedInputState.key[KEY_MENU_UP])
			{
				if (row-- == 0)
				{
					if (side == 0)
						row = 0;
					else
					{
						row = NUM_MELEE_ROWS;
						side = !side;
					}
				}
			}
			else if (PulsedInputState.key[KEY_MENU_DOWN])
			{
				if (row++ == NUM_MELEE_ROWS)
				{
					if (side == 1)
						row = NUM_MELEE_ROWS;
					else
					{
						row = 0;
						side = !side;
					}
				}
			}
		}

		if (col != pMS->col || row != pMS->row || side != pMS->side)
		{
			LockMutex (GraphicsLock);
			Deselect (EDIT_MELEE);
			pMS->side = side;
			pMS->row = row;
			pMS->col = col;
			if (row == NUM_MELEE_ROWS)
				pMS->CurIndex = (BYTE)~0;
			else
				pMS->CurIndex = pMS->TeamImage[side].ShipList[row][col];
			UnlockMutex (GraphicsLock);

			DrawMeleeShipStrings (pMS, (BYTE)(pMS->CurIndex));
		}
	}

	return (TRUE);
}

static BOOLEAN
DoPickShip (PMELEE_STATE pMS)
{
	STARSHIPPTR StarShipPtr;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	if (pMS->Initialized <= 0)
	{
		if (pMS->CurIndex == (BYTE)~0 && pMS->Initialized == 0)
			pMS->CurIndex = 0;
		else if (pMS->CurIndex != (BYTE)~0)
		{
			DeleteCurrentShip (pMS);
		}

		if (pMS->Initialized == 0)
		{
			LockMutex (GraphicsLock);
			Deselect (EDIT_MELEE);
			pMS->InputFunc = DoPickShip;
			DrawPickFrame (pMS);
			pMS->Initialized = TRUE;
			UnlockMutex (GraphicsLock);
		}
		else
		{
			LockMutex (GraphicsLock);
			Deselect (EDIT_MELEE);
			pMS->Initialized = TRUE;
			AdvanceCursor (pMS);
			pMS->CurIndex = pMS->TeamImage[pMS->side].ShipList[pMS->row][pMS->col];
			UnlockMutex (GraphicsLock);

			DrawMeleeShipStrings (pMS, (BYTE)(pMS->CurIndex));
		}
	}
	else if (PulsedInputState.key[KEY_MENU_SELECT] || PulsedInputState.key[KEY_MENU_CANCEL])
	{
		pMS->InputFunc = 0; /* disable ship flashing */

		if (!PulsedInputState.key[KEY_MENU_CANCEL])
		{
			HSTARSHIP hStarShip;
			
			hStarShip = GetStarShipFromIndex (&master_q, pMS->CurIndex);
			StarShipPtr = LockStarShip (&master_q, hStarShip);
			pMS->star_bucks[pMS->side] +=
					StarShipPtr->RaceDescPtr->ship_info.ship_cost;
			UnlockStarShip (&master_q, hStarShip);

			pMS->TeamImage[pMS->side].ShipList[pMS->row][pMS->col] = pMS->CurIndex;
			LockMutex (GraphicsLock);
			DrawTeamString (pMS, 4);
			DrawShipBox (pMS, FALSE);
			UnlockMutex (GraphicsLock);
			AdvanceCursor (pMS);
		}

		{
			RECT r;
			
			GetFrameRect (BuildPickFrame, &r);
			LockMutex (GraphicsLock);
			RepairMeleeFrame (&r);
			UnlockMutex (GraphicsLock);
		}

		pMS->CurIndex = pMS->TeamImage[pMS->side].ShipList[pMS->row][pMS->col];
		DrawMeleeShipStrings (pMS, (BYTE)pMS->CurIndex);

		pMS->InputFunc = DoEdit;

		return (TRUE);
	}
	else if (PulsedInputState.key[KEY_MENU_SPECIAL])
	{
		DoShipSpin (pMS->CurIndex, (MUSIC_REF)0);
	
		return (TRUE);
	}

	{
		BYTE NewStarShip;

		NewStarShip = pMS->CurIndex;

		if (PulsedInputState.key[KEY_MENU_LEFT])
		{
			if (NewStarShip-- % NUM_PICK_COLS == 0)
				NewStarShip += NUM_PICK_COLS;
		}
		else if (PulsedInputState.key[KEY_MENU_RIGHT])
		{
			if (++NewStarShip % NUM_PICK_COLS == 0)
				NewStarShip -= NUM_PICK_COLS;
		}
		
		if (PulsedInputState.key[KEY_MENU_UP])
		{
			if (NewStarShip >= NUM_PICK_COLS)
				NewStarShip -= NUM_PICK_COLS;
			else
				NewStarShip += NUM_PICK_COLS * (NUM_PICK_ROWS - 1);
		}
		else if (PulsedInputState.key[KEY_MENU_DOWN])
		{
			if (NewStarShip < NUM_PICK_COLS * (NUM_PICK_ROWS - 1))
				NewStarShip += NUM_PICK_COLS;
			else
				NewStarShip -= NUM_PICK_COLS * (NUM_PICK_ROWS - 1);
		}

		if (NewStarShip != pMS->CurIndex)
		{
			LockMutex (GraphicsLock);
			Deselect (EDIT_MELEE);
			pMS->CurIndex = NewStarShip;
			UnlockMutex (GraphicsLock);
			DrawMeleeShipStrings (pMS, NewStarShip);
		}
	}

	return (TRUE);
}

static void
LoadMeleeInfo (PMELEE_STATE pMS)
{
	STAMP	s;
	CONTEXT	OldContext;
	RECT    r;
	COUNT   i;

	// reverting to original behavior
	OldContext = SetContext (OffScreenContext);

	if (PickMeleeFrame)
		DestroyDrawable (ReleaseDrawable (PickMeleeFrame));
	PickMeleeFrame = CaptureDrawable (CreateDrawable (
			WANT_PIXMAP, MELEE_WIDTH, MELEE_HEIGHT, 2));
	s.origin.x = s.origin.y = 0;
	s.frame = CaptureDrawable (LoadGraphic (MELEE_PICK_MASK_PMAP_ANIM));
	SetContextFGFrame (PickMeleeFrame);
	DrawStamp (&s);

	s.frame = IncFrameIndex (s.frame);
	SetContextFGFrame (IncFrameIndex (PickMeleeFrame));
	DrawStamp (&s);
	DestroyDrawable (ReleaseDrawable (s.frame));

	MeleeFrame = CaptureDrawable (LoadGraphic (MELEE_SCREEN_PMAP_ANIM));

	// create team building ship selection box
	s.frame = SetAbsFrameIndex (MeleeFrame, 27);
	GetFrameRect (s.frame, &r);
	BuildPickFrame = CaptureDrawable (CreateDrawable (
			WANT_PIXMAP, r.extent.width, r.extent.height, 1));
	SetContextFGFrame (BuildPickFrame);
	SetFrameHot (s.frame, MAKE_HOT_SPOT (0, 0));
	DrawStamp (&s);
	for (i = 0; i < NUM_PICK_COLS * NUM_PICK_ROWS; ++i)
		DrawPickIcon (i, 1);

	SetContext (OldContext);

	InitSpace ();

	LoadTeamList (pMS, 0);
}

static void
FreeMeleeInfo (PMELEE_STATE pMS)
{
	if (pMS->flash_task)
	{
		ConcludeTask (pMS->flash_task);
		pMS->flash_task = 0;
	}
	DestroyDirEntryTable (ReleaseDirEntryTable (pMS->TeamDE));
	pMS->TeamDE = 0;

	UninitSpace ();

	DestroyDrawable (ReleaseDrawable (MeleeFrame));
	MeleeFrame = 0;
	DestroyDrawable (ReleaseDrawable (BuildPickFrame));
	BuildPickFrame = 0;
}

static void
BuildAndDrawShipList (PMELEE_STATE pMS)
{
	COUNT i;
	CONTEXT OldContext;

	OldContext = SetContext (OffScreenContext);

	for (i = 0; i < NUM_SIDES; ++i)
	{
		COUNT j, side;
		RECT r;
		TEXT t;
		STAMP s;
		UNICODE buf[10];

		side = !i;

		s.frame = SetAbsFrameIndex (PickMeleeFrame, side);
		SetContextFGFrame (s.frame);

		GetFrameRect (s.frame, &r);
		t.baseline.x = r.extent.width >> 1;
		t.baseline.y = r.extent.height - NAME_AREA_HEIGHT + 4;
		r.corner.x += 2;
		r.corner.y += 2;
		r.extent.width -= (2 * 2) + (ICON_WIDTH + 2) + 1;
		r.extent.height -= (2 * 2) + NAME_AREA_HEIGHT;
		SetContextForeGroundColor (PICK_BG_COLOR);
		DrawFilledRectangle (&r);
		r.corner.x += 2;
		r.extent.width += (ICON_WIDTH + 2) - (2 * 2);
		r.corner.y += r.extent.height;
		r.extent.height = NAME_AREA_HEIGHT;
		DrawFilledRectangle (&r);
		t.align = ALIGN_CENTER;
		t.pStr = pMS->TeamImage[i].TeamName;
		t.CharCount = (COUNT)~0;
		SetContextFont (TinyFont);
		SetContextForeGroundColor (BUILD_COLOR (
				MAKE_RGB15 (0xA, 0xA, 0x1F), 0x9));
		font_DrawText (&t);

		wsprintf (buf, "%d", pMS->star_bucks[i]);
		t.baseline.x = 4;
		t.baseline.y = 7;
		t.align = ALIGN_LEFT;
		t.pStr = buf;
		t.CharCount = (COUNT)~0;
		SetContextForeGroundColor (BUILD_COLOR (
				MAKE_RGB15 (0x4, 0x5, 0x1F), 0x4B));
		font_DrawText (&t);
									
		for (j = 0; j < NUM_MELEE_ROWS; ++j)
		{
			COUNT k;

			for (k = 0; k < NUM_MELEE_COLUMNS; ++k)
			{
				BYTE StarShip;

				if ((StarShip = pMS->TeamImage[i].ShipList[j][k]) != (BYTE)~0)
				{
					BYTE ship_cost;
					HSTARSHIP hStarShip, hBuiltShip;
					STARSHIPPTR StarShipPtr, BuiltShipPtr;

					hStarShip = GetStarShipFromIndex (&master_q, StarShip);
					StarShipPtr = LockStarShip (&master_q, hStarShip);
					hBuiltShip = Build (&race_q[side],
							StarShipPtr->RaceResIndex,
							1 << side,
							NameCaptain (&race_q[side], StarShipPtr));
					s.origin.x = 4 + ((ICON_WIDTH + 2) * k);
					s.origin.y = 10 + ((ICON_HEIGHT + 2) * j);
					s.frame = StarShipPtr->RaceDescPtr->ship_info.icons;
					DrawStamp (&s);
					ship_cost = StarShipPtr->RaceDescPtr->
							ship_info.ship_cost;
					UnlockStarShip (&master_q, hStarShip);

					BuiltShipPtr = LockStarShip (&race_q[side], hBuiltShip);
					BuiltShipPtr->ShipFacing = (j * NUM_MELEE_COLUMNS) + k;
					BuiltShipPtr->special_counter = ship_cost;
					BuiltShipPtr->captains_name_index =
							StarShipCaptain (BuiltShipPtr);
					BuiltShipPtr->RaceDescPtr = StarShipPtr->RaceDescPtr;
					UnlockStarShip (&race_q[side], hBuiltShip);
				}
			}
		}
	}

	SetContext (OldContext);
}

static BOOLEAN
DoMelee (PMELEE_STATE pMS)
{
	BOOLEAN force_select = FALSE;
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	if (!pMS->Initialized)
	{
		pMS->Initialized = TRUE;
		pMS->MeleeOption = START_MELEE;
		LockMutex (GraphicsLock);
		InitMelee (pMS);
		UnlockMutex (GraphicsLock);
		{
			BYTE clut_buf[] = {FadeAllToColor};
				
			XFormColorMap ((COLORMAPPTR)clut_buf, ONE_SECOND / 2);
		}
		InTime = GetTimeCounter ();
	}
	else if (PulsedInputState.key[KEY_MENU_CANCEL] || PulsedInputState.key[KEY_MENU_LEFT])
	{
		LockMutex (GraphicsLock);
		InTime = GetTimeCounter ();
		Deselect (pMS->MeleeOption);
		UnlockMutex (GraphicsLock);
		pMS->MeleeOption = EDIT_MELEE;
		pMS->Initialized = FALSE;
		if (PulsedInputState.key[KEY_MENU_CANCEL])
		{
			pMS->side = 0;
			pMS->row = 0;
			pMS->col = 0;
		}
		else
		{
			pMS->side = 0;
			pMS->row = NUM_MELEE_ROWS - 1;
			pMS->col = NUM_MELEE_COLUMNS - 1;
		}
		DoEdit (pMS);
	}
	else
	{
		MELEE_OPTIONS NewMeleeOption;

		NewMeleeOption = pMS->MeleeOption;
		if (PulsedInputState.key[KEY_MENU_UP])
		{
			InTime = GetTimeCounter ();
			if (NewMeleeOption-- == CONTROLS_TOP)
				NewMeleeOption = CONTROLS_TOP;
		}
		else if (PulsedInputState.key[KEY_MENU_DOWN])
		{
			InTime = GetTimeCounter ();
			if (NewMeleeOption++ == CONTROLS_BOT)
				NewMeleeOption = CONTROLS_BOT;
		}

		if ((PlayerControl[0] & PlayerControl[1] & PSYTRON_CONTROL)
				&& GetTimeCounter () - InTime > ONE_SECOND * 10)
		{
			force_select = TRUE;
			NewMeleeOption = START_MELEE;
		}

		if (NewMeleeOption != pMS->MeleeOption)
		{
			LockMutex (GraphicsLock);
			Deselect (pMS->MeleeOption);
			pMS->MeleeOption = NewMeleeOption;
			Select (pMS->MeleeOption);
			UnlockMutex (GraphicsLock);
		}

		if (PulsedInputState.key[KEY_MENU_SELECT] || force_select)
		{
			switch (pMS->MeleeOption)
			{
				case START_MELEE:
					if (pMS->star_bucks[0] == 0 || pMS->star_bucks[1] == 0)
					{
						PlaySoundEffect (SetAbsSoundIndex (MenuSounds, 2),
								0, NotPositional (), NULL, GAME_SOUND_PRIORITY);
						break;
					}

					if (pMS->flash_task)
					{
						ConcludeTask (pMS->flash_task);
						pMS->flash_task = 0;
					}
					
					{
						BYTE black_buf[] = {FadeAllToBlack};
						
						SleepThreadUntil (XFormColorMap (
								(COLORMAPPTR)black_buf, ONE_SECOND / 2
								) + ONE_SECOND / 60);
						FlushColorXForms ();
					}
					do
					{
						LockMutex (GraphicsLock);
						BuildAndDrawShipList (pMS);
						UnlockMutex (GraphicsLock);

						WaitForSoundEnd (TFBSOUND_WAIT_ALL);

						SeedRandomNumbers ();
						load_gravity_well ((BYTE)((COUNT)TFB_Random () % NUMBER_OF_PLANET_TYPES));
						Battle ();
						free_gravity_well ();

						if (GLOBAL (CurrentActivity) & CHECK_ABORT) return (FALSE);

						{
							BYTE black_buf[] = {FadeAllToBlack};
						
							SleepThreadUntil (XFormColorMap (
									(COLORMAPPTR)black_buf, ONE_SECOND / 2)
									+ ONE_SECOND / 60);
							FlushColorXForms ();
						}
					} while (0 /* !(GLOBAL (CurrentActivity) & CHECK_ABORT) */);
					GLOBAL (CurrentActivity) = SUPER_MELEE;

					pMS->Initialized = FALSE;
					break;
				case LOAD_TOP:
				case LOAD_BOT:
					pMS->Initialized = FALSE;
					pMS->side = pMS->MeleeOption == LOAD_TOP ? 0 : 1;
					DoLoadTeam (pMS);
					break;
				case SAVE_TOP:
				case SAVE_BOT:
					pMS->side = pMS->MeleeOption == SAVE_TOP ? 0 : 1;
					if (pMS->star_bucks[pMS->side])
						DoSaveTeam (pMS);
					else
						PlaySoundEffect (SetAbsSoundIndex (MenuSounds, 2),
								0, NotPositional (), NULL, GAME_SOUND_PRIORITY);
					break;
				case CONTROLS_TOP:
				case CONTROLS_BOT:
				{
					COUNT which_side;

					which_side = pMS->MeleeOption == CONTROLS_TOP ? 1 : 0;
					if (PlayerControl[which_side] & HUMAN_CONTROL)
						PlayerControl[which_side] = COMPUTER_CONTROL | STANDARD_RATING;
					else if (PlayerControl[which_side] & AWESOME_RATING)
						PlayerControl[which_side] = HUMAN_CONTROL | STANDARD_RATING;
					else
						PlayerControl[which_side] = ((
								PlayerControl[which_side]
								& (STANDARD_RATING | GOOD_RATING | AWESOME_RATING)
								) << 1) | COMPUTER_CONTROL;
					SetPlayerInput ();
					DrawControls (which_side, TRUE);
					break;
				}
			}
		}
	}

	return (TRUE);
}

static void
InitPreBuilt (PMELEE_STATE pMS)
{
	wstrcpy (pMS->PreBuiltList[0].TeamName, GAME_STRING (MELEE_STRING_BASE + 4));
	pMS->PreBuiltList[0].ShipList[0][0] = MELEE_ANDROSYNTH;
	pMS->PreBuiltList[0].ShipList[0][1] = MELEE_CHMMR;
	pMS->PreBuiltList[0].ShipList[0][2] = MELEE_DRUUGE;
	pMS->PreBuiltList[0].ShipList[0][3] = MELEE_URQUAN;
	pMS->PreBuiltList[0].ShipList[0][4] = MELEE_MELNORME;
	pMS->PreBuiltList[0].ShipList[0][5] = MELEE_ORZ;
	pMS->PreBuiltList[0].ShipList[0][6] = MELEE_SPATHI;
	pMS->PreBuiltList[0].ShipList[1][0] = MELEE_SYREEN;
	pMS->PreBuiltList[0].ShipList[1][1] = MELEE_UTWIG;
	pMS->PreBuiltList[0].ShipList[1][2] = (BYTE)~0;
	pMS->PreBuiltList[0].ShipList[1][3] = (BYTE)~0;
	pMS->PreBuiltList[0].ShipList[1][4] = (BYTE)~0;
	pMS->PreBuiltList[0].ShipList[1][5] = (BYTE)~0;
	pMS->PreBuiltList[0].ShipList[1][6] = (BYTE)~0;

	wstrcpy (pMS->PreBuiltList[1].TeamName, GAME_STRING (MELEE_STRING_BASE + 5));
	pMS->PreBuiltList[1].ShipList[0][0] = MELEE_ARILOU;
	pMS->PreBuiltList[1].ShipList[0][1] = MELEE_CHENJESU;
	pMS->PreBuiltList[1].ShipList[0][2] = MELEE_EARTHLING;
	pMS->PreBuiltList[1].ShipList[0][3] = MELEE_KOHR_AH;
	pMS->PreBuiltList[1].ShipList[0][4] = MELEE_MYCON;
	pMS->PreBuiltList[1].ShipList[0][5] = MELEE_YEHAT;
	pMS->PreBuiltList[1].ShipList[0][6] = MELEE_PKUNK;
	pMS->PreBuiltList[1].ShipList[1][0] = MELEE_SUPOX;
	pMS->PreBuiltList[1].ShipList[1][1] = MELEE_THRADDASH;
	pMS->PreBuiltList[1].ShipList[1][2] = MELEE_ZOQFOTPIK;
	pMS->PreBuiltList[1].ShipList[1][3] = MELEE_SHOFIXTI;
	pMS->PreBuiltList[1].ShipList[1][4] = (BYTE)~0;
	pMS->PreBuiltList[1].ShipList[1][5] = (BYTE)~0;
	pMS->PreBuiltList[1].ShipList[1][6] = (BYTE)~0;

	wstrcpy (pMS->PreBuiltList[2].TeamName, GAME_STRING (MELEE_STRING_BASE + 6));
	pMS->PreBuiltList[2].ShipList[0][0] = MELEE_ANDROSYNTH;
	pMS->PreBuiltList[2].ShipList[0][1] = MELEE_CHMMR;
	pMS->PreBuiltList[2].ShipList[0][2] = MELEE_DRUUGE;
	pMS->PreBuiltList[2].ShipList[0][3] = MELEE_MELNORME;
	pMS->PreBuiltList[2].ShipList[0][4] = MELEE_EARTHLING;
	pMS->PreBuiltList[2].ShipList[0][5] = MELEE_KOHR_AH;
	pMS->PreBuiltList[2].ShipList[0][6] = MELEE_SUPOX;
	pMS->PreBuiltList[2].ShipList[1][0] = MELEE_ORZ;
	pMS->PreBuiltList[2].ShipList[1][1] = MELEE_SPATHI;
	pMS->PreBuiltList[2].ShipList[1][2] = MELEE_ILWRATH;
	pMS->PreBuiltList[2].ShipList[1][3] = MELEE_VUX;
	pMS->PreBuiltList[2].ShipList[1][4] = (BYTE)~0;
	pMS->PreBuiltList[2].ShipList[1][5] = (BYTE)~0;
	pMS->PreBuiltList[2].ShipList[1][6] = (BYTE)~0;

	wstrcpy (pMS->PreBuiltList[3].TeamName, GAME_STRING (MELEE_STRING_BASE + 7));
	pMS->PreBuiltList[3].ShipList[0][0] = MELEE_CHENJESU;
	pMS->PreBuiltList[3].ShipList[0][1] = MELEE_CHENJESU;
	pMS->PreBuiltList[3].ShipList[0][2] = MELEE_CHMMR;
	pMS->PreBuiltList[3].ShipList[0][3] = MELEE_CHMMR;
	pMS->PreBuiltList[3].ShipList[0][4] = MELEE_KOHR_AH;
	pMS->PreBuiltList[3].ShipList[0][5] = MELEE_KOHR_AH;
	pMS->PreBuiltList[3].ShipList[0][6] = MELEE_URQUAN;
	pMS->PreBuiltList[3].ShipList[1][0] = MELEE_URQUAN;
	pMS->PreBuiltList[3].ShipList[1][1] = MELEE_UTWIG;
	pMS->PreBuiltList[3].ShipList[1][2] = MELEE_UTWIG;
	pMS->PreBuiltList[3].ShipList[1][3] = (BYTE)~0;
	pMS->PreBuiltList[3].ShipList[1][4] = (BYTE)~0;
	pMS->PreBuiltList[3].ShipList[1][5] = (BYTE)~0;
	pMS->PreBuiltList[3].ShipList[1][6] = (BYTE)~0;

	wstrcpy (pMS->PreBuiltList[4].TeamName, GAME_STRING (MELEE_STRING_BASE + 8));
	pMS->PreBuiltList[4].ShipList[0][0] = MELEE_URQUAN;
	pMS->PreBuiltList[4].ShipList[0][1] = MELEE_CHENJESU;
	pMS->PreBuiltList[4].ShipList[0][2] = MELEE_MYCON;
	pMS->PreBuiltList[4].ShipList[0][3] = MELEE_SYREEN;
	pMS->PreBuiltList[4].ShipList[0][4] = MELEE_ZOQFOTPIK;
	pMS->PreBuiltList[4].ShipList[0][5] = MELEE_SHOFIXTI;
	pMS->PreBuiltList[4].ShipList[0][6] = MELEE_EARTHLING;
	pMS->PreBuiltList[4].ShipList[1][0] = MELEE_KOHR_AH;
	pMS->PreBuiltList[4].ShipList[1][1] = MELEE_MELNORME;
	pMS->PreBuiltList[4].ShipList[1][2] = MELEE_DRUUGE;
	pMS->PreBuiltList[4].ShipList[1][3] = MELEE_PKUNK;
	pMS->PreBuiltList[4].ShipList[1][4] = MELEE_ORZ;
	pMS->PreBuiltList[4].ShipList[1][5] = (BYTE)~0;
	pMS->PreBuiltList[4].ShipList[1][6] = (BYTE)~0;

	wstrcpy (pMS->PreBuiltList[5].TeamName, "Ford's Fighters");
	pMS->PreBuiltList[5].ShipList[0][0] = MELEE_CHMMR;
	pMS->PreBuiltList[5].ShipList[0][1] = MELEE_ZOQFOTPIK;
	pMS->PreBuiltList[5].ShipList[0][2] = MELEE_MELNORME;
	pMS->PreBuiltList[5].ShipList[0][3] = MELEE_SUPOX;
	pMS->PreBuiltList[5].ShipList[0][4] = MELEE_UTWIG;
	pMS->PreBuiltList[5].ShipList[0][5] = MELEE_UMGAH;
	pMS->PreBuiltList[5].ShipList[0][6] = (BYTE)~0;
	pMS->PreBuiltList[5].ShipList[1][0] = (BYTE)~0;
	pMS->PreBuiltList[5].ShipList[1][1] = (BYTE)~0;
	pMS->PreBuiltList[5].ShipList[1][2] = (BYTE)~0;
	pMS->PreBuiltList[5].ShipList[1][3] = (BYTE)~0;
	pMS->PreBuiltList[5].ShipList[1][4] = (BYTE)~0;
	pMS->PreBuiltList[5].ShipList[1][5] = (BYTE)~0;
	pMS->PreBuiltList[5].ShipList[1][6] = (BYTE)~0;

	wstrcpy (pMS->PreBuiltList[6].TeamName, "Leyland's Lashers");
	pMS->PreBuiltList[6].ShipList[0][0] = MELEE_ANDROSYNTH;
	pMS->PreBuiltList[6].ShipList[0][1] = MELEE_EARTHLING;
	pMS->PreBuiltList[6].ShipList[0][2] = MELEE_MYCON;
	pMS->PreBuiltList[6].ShipList[0][3] = MELEE_ORZ;
	pMS->PreBuiltList[6].ShipList[0][4] = MELEE_URQUAN;
	pMS->PreBuiltList[6].ShipList[0][5] = (BYTE)~0;
	pMS->PreBuiltList[6].ShipList[0][6] = (BYTE)~0;
	pMS->PreBuiltList[6].ShipList[1][0] = (BYTE)~0;
	pMS->PreBuiltList[6].ShipList[1][1] = (BYTE)~0;
	pMS->PreBuiltList[6].ShipList[1][2] = (BYTE)~0;
	pMS->PreBuiltList[6].ShipList[1][3] = (BYTE)~0;
	pMS->PreBuiltList[6].ShipList[1][4] = (BYTE)~0;
	pMS->PreBuiltList[6].ShipList[1][5] = (BYTE)~0;
	pMS->PreBuiltList[6].ShipList[1][6] = (BYTE)~0;

	wstrcpy (pMS->PreBuiltList[7].TeamName, "The Gregorizers 200");
	pMS->PreBuiltList[7].ShipList[0][0] = MELEE_ANDROSYNTH;
	pMS->PreBuiltList[7].ShipList[0][1] = MELEE_CHMMR;
	pMS->PreBuiltList[7].ShipList[0][2] = MELEE_DRUUGE;
	pMS->PreBuiltList[7].ShipList[0][3] = MELEE_MELNORME;
	pMS->PreBuiltList[7].ShipList[0][4] = MELEE_EARTHLING;
	pMS->PreBuiltList[7].ShipList[0][5] = MELEE_KOHR_AH;
	pMS->PreBuiltList[7].ShipList[0][6] = MELEE_SUPOX;
	pMS->PreBuiltList[7].ShipList[1][0] = MELEE_ORZ;
	pMS->PreBuiltList[7].ShipList[1][1] = MELEE_PKUNK;
	pMS->PreBuiltList[7].ShipList[1][2] = MELEE_SPATHI;
	pMS->PreBuiltList[7].ShipList[1][3] = (BYTE)~0;
	pMS->PreBuiltList[7].ShipList[1][4] = (BYTE)~0;
	pMS->PreBuiltList[7].ShipList[1][5] = (BYTE)~0;
	pMS->PreBuiltList[7].ShipList[1][6] = (BYTE)~0;

	wstrcpy (pMS->PreBuiltList[8].TeamName, "300 point Armada!");
	pMS->PreBuiltList[8].ShipList[0][0] = MELEE_ANDROSYNTH;
	pMS->PreBuiltList[8].ShipList[0][1] = MELEE_CHMMR;
	pMS->PreBuiltList[8].ShipList[0][2] = MELEE_CHENJESU;
	pMS->PreBuiltList[8].ShipList[0][3] = MELEE_DRUUGE;
	pMS->PreBuiltList[8].ShipList[0][4] = MELEE_EARTHLING;
	pMS->PreBuiltList[8].ShipList[0][5] = MELEE_KOHR_AH;
	pMS->PreBuiltList[8].ShipList[0][6] = MELEE_MELNORME;
	pMS->PreBuiltList[8].ShipList[1][0] = MELEE_MYCON;
	pMS->PreBuiltList[8].ShipList[1][1] = MELEE_ORZ;
	pMS->PreBuiltList[8].ShipList[1][2] = MELEE_PKUNK;
	pMS->PreBuiltList[8].ShipList[1][3] = MELEE_SPATHI;
	pMS->PreBuiltList[8].ShipList[1][4] = MELEE_SUPOX;
	pMS->PreBuiltList[8].ShipList[1][5] = MELEE_URQUAN;
	pMS->PreBuiltList[8].ShipList[1][6] = MELEE_YEHAT;

	wstrcpy (pMS->PreBuiltList[9].TeamName, "Little Dudes with Attitudes");
	pMS->PreBuiltList[9].ShipList[0][0] = MELEE_UMGAH;
	pMS->PreBuiltList[9].ShipList[0][1] = MELEE_THRADDASH;
	pMS->PreBuiltList[9].ShipList[0][2] = MELEE_SHOFIXTI;
	pMS->PreBuiltList[9].ShipList[0][3] = MELEE_EARTHLING;
	pMS->PreBuiltList[9].ShipList[0][4] = MELEE_VUX;
	pMS->PreBuiltList[9].ShipList[0][5] = MELEE_ZOQFOTPIK;
	pMS->PreBuiltList[9].ShipList[0][6] = (BYTE)~0;
	pMS->PreBuiltList[9].ShipList[1][0] = (BYTE)~0;
	pMS->PreBuiltList[9].ShipList[1][1] = (BYTE)~0;
	pMS->PreBuiltList[9].ShipList[1][2] = (BYTE)~0;
	pMS->PreBuiltList[9].ShipList[1][3] = (BYTE)~0;
	pMS->PreBuiltList[9].ShipList[1][4] = (BYTE)~0;
	pMS->PreBuiltList[9].ShipList[1][5] = (BYTE)~0;
	pMS->PreBuiltList[9].ShipList[1][6] = (BYTE)~0;

	wstrcpy (pMS->PreBuiltList[10].TeamName, "New Alliance Ships");
	pMS->PreBuiltList[10].ShipList[0][0] = MELEE_ARILOU;
	pMS->PreBuiltList[10].ShipList[0][1] = MELEE_CHMMR;
	pMS->PreBuiltList[10].ShipList[0][2] = MELEE_EARTHLING;
	pMS->PreBuiltList[10].ShipList[0][3] = MELEE_ORZ;
	pMS->PreBuiltList[10].ShipList[0][4] = MELEE_PKUNK;
	pMS->PreBuiltList[10].ShipList[0][5] = MELEE_SHOFIXTI;
	pMS->PreBuiltList[10].ShipList[0][6] = MELEE_SUPOX;
	pMS->PreBuiltList[10].ShipList[1][0] = MELEE_SYREEN;
	pMS->PreBuiltList[10].ShipList[1][1] = MELEE_UTWIG;
	pMS->PreBuiltList[10].ShipList[1][2] = MELEE_ZOQFOTPIK;
	pMS->PreBuiltList[10].ShipList[1][3] = MELEE_YEHAT;
	pMS->PreBuiltList[10].ShipList[1][4] = MELEE_DRUUGE;
	pMS->PreBuiltList[10].ShipList[1][5] = MELEE_THRADDASH;
	pMS->PreBuiltList[10].ShipList[1][6] = MELEE_SPATHI;

	wstrcpy (pMS->PreBuiltList[11].TeamName, "Old Alliance Ships");
	pMS->PreBuiltList[11].ShipList[0][0] = MELEE_ARILOU;
	pMS->PreBuiltList[11].ShipList[0][1] = MELEE_CHENJESU;
	pMS->PreBuiltList[11].ShipList[0][2] = MELEE_EARTHLING;
	pMS->PreBuiltList[11].ShipList[0][3] = MELEE_MMRNMHRM;
	pMS->PreBuiltList[11].ShipList[0][4] = MELEE_SHOFIXTI;
	pMS->PreBuiltList[11].ShipList[0][5] = MELEE_SYREEN;
	pMS->PreBuiltList[11].ShipList[0][6] = MELEE_YEHAT;
	pMS->PreBuiltList[11].ShipList[1][0] = (BYTE)~0;
	pMS->PreBuiltList[11].ShipList[1][1] = (BYTE)~0;
	pMS->PreBuiltList[11].ShipList[1][2] = (BYTE)~0;
	pMS->PreBuiltList[11].ShipList[1][3] = (BYTE)~0;
	pMS->PreBuiltList[11].ShipList[1][4] = (BYTE)~0;
	pMS->PreBuiltList[11].ShipList[1][5] = (BYTE)~0;
	pMS->PreBuiltList[11].ShipList[1][6] = (BYTE)~0;

	wstrcpy (pMS->PreBuiltList[12].TeamName, "Old Hierarchy Ships");
	pMS->PreBuiltList[12].ShipList[0][0] = MELEE_ANDROSYNTH;
	pMS->PreBuiltList[12].ShipList[0][1] = MELEE_ILWRATH;
	pMS->PreBuiltList[12].ShipList[0][2] = MELEE_MYCON;
	pMS->PreBuiltList[12].ShipList[0][3] = MELEE_SPATHI;
	pMS->PreBuiltList[12].ShipList[0][4] = MELEE_UMGAH;
	pMS->PreBuiltList[12].ShipList[0][5] = MELEE_URQUAN;
	pMS->PreBuiltList[12].ShipList[0][6] = MELEE_VUX;
	pMS->PreBuiltList[12].ShipList[1][0] = (BYTE)~0;
	pMS->PreBuiltList[12].ShipList[1][1] = (BYTE)~0;
	pMS->PreBuiltList[12].ShipList[1][2] = (BYTE)~0;
	pMS->PreBuiltList[12].ShipList[1][3] = (BYTE)~0;
	pMS->PreBuiltList[12].ShipList[1][4] = (BYTE)~0;
	pMS->PreBuiltList[12].ShipList[1][5] = (BYTE)~0;
	pMS->PreBuiltList[12].ShipList[1][6] = (BYTE)~0;

	wstrcpy (pMS->PreBuiltList[13].TeamName, "Star Control 1");
	pMS->PreBuiltList[13].ShipList[0][0] = MELEE_ANDROSYNTH;
	pMS->PreBuiltList[13].ShipList[0][1] = MELEE_ARILOU;
	pMS->PreBuiltList[13].ShipList[0][2] = MELEE_CHENJESU;
	pMS->PreBuiltList[13].ShipList[0][3] = MELEE_EARTHLING;
	pMS->PreBuiltList[13].ShipList[0][4] = MELEE_ILWRATH;
	pMS->PreBuiltList[13].ShipList[0][5] = MELEE_MMRNMHRM;
	pMS->PreBuiltList[13].ShipList[0][6] = MELEE_MYCON;
	pMS->PreBuiltList[13].ShipList[1][0] = MELEE_SHOFIXTI;
	pMS->PreBuiltList[13].ShipList[1][1] = MELEE_SPATHI;
	pMS->PreBuiltList[13].ShipList[1][2] = MELEE_SYREEN;
	pMS->PreBuiltList[13].ShipList[1][3] = MELEE_UMGAH;
	pMS->PreBuiltList[13].ShipList[1][4] = MELEE_URQUAN;
	pMS->PreBuiltList[13].ShipList[1][5] = MELEE_VUX;
	pMS->PreBuiltList[13].ShipList[1][6] = MELEE_YEHAT;

	wstrcpy (pMS->PreBuiltList[14].TeamName, "Star Control 2");
	pMS->PreBuiltList[14].ShipList[0][0] = MELEE_CHMMR;
	pMS->PreBuiltList[14].ShipList[0][1] = MELEE_DRUUGE;
	pMS->PreBuiltList[14].ShipList[0][2] = MELEE_KOHR_AH;
	pMS->PreBuiltList[14].ShipList[0][3] = MELEE_MELNORME;
	pMS->PreBuiltList[14].ShipList[0][4] = MELEE_ORZ;
	pMS->PreBuiltList[14].ShipList[0][5] = MELEE_PKUNK;
	pMS->PreBuiltList[14].ShipList[0][6] = MELEE_SLYLANDRO;
	pMS->PreBuiltList[14].ShipList[1][0] = MELEE_SUPOX;
	pMS->PreBuiltList[14].ShipList[1][1] = MELEE_THRADDASH;
	pMS->PreBuiltList[14].ShipList[1][2] = MELEE_UTWIG;
	pMS->PreBuiltList[14].ShipList[1][3] = MELEE_ZOQFOTPIK;
	pMS->PreBuiltList[14].ShipList[1][4] = MELEE_ZOQFOTPIK;
	pMS->PreBuiltList[14].ShipList[1][5] = MELEE_ZOQFOTPIK;
	pMS->PreBuiltList[14].ShipList[1][6] = MELEE_ZOQFOTPIK;
}

void
Melee (void)
{
	InitGlobData ();
	{
		MELEE_STATE MenuState;

		pMeleeState = &MenuState;
		memset (pMeleeState, 0, sizeof (*pMeleeState));

		MenuState.InputFunc = DoMelee;
		MenuState.Initialized = FALSE;

		MenuState.CurIndex = (COUNT)~0;
		InitPreBuilt (&MenuState);

		GLOBAL (CurrentActivity) = SUPER_MELEE;

		GameSounds = CaptureSound (LoadSound (GAME_SOUNDS));
		LoadMeleeInfo (&MenuState);
		{
			uio_Stream *load_fp;

			load_fp = res_OpenResFile (configDir, "melee.cfg", "rb");
			if (load_fp)
			{
				int status;

				if (LengthResFile (load_fp) != (1 + sizeof (TEAM_IMAGE)) * 2)
					status = -1;
				else if ((status = GetResFileChar (load_fp)) != -1)
				{
					PlayerControl[0] = (BYTE)status;
					status = ReadTeamImage (&MenuState.TeamImage[0], load_fp);
					if (status != -1)
					{
						if ((status = GetResFileChar (load_fp)) != -1)
						{
							PlayerControl[1] = (BYTE)status;
							status = ReadTeamImage (&MenuState.TeamImage[1], load_fp);
						}
					}
				}
				res_CloseResFile (load_fp);
				if (status == -1)
					load_fp = 0;
			}

			if (load_fp == 0)
			{
				PlayerControl[0] = HUMAN_CONTROL | STANDARD_RATING;
				MenuState.TeamImage[0] = MenuState.PreBuiltList[0];
				PlayerControl[1] = COMPUTER_CONTROL | STANDARD_RATING;
				MenuState.TeamImage[1] = MenuState.PreBuiltList[1];
			}
		}
		SetPlayerInput ();

		MenuState.side = 0;
		MenuState.star_bucks[0] = GetTeamValue (&MenuState.TeamImage[0]);
		MenuState.star_bucks[1] = GetTeamValue (&MenuState.TeamImage[1]);
		SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
		DoInput ((PVOID)&MenuState, TRUE);

		WaitForSoundEnd (TFBSOUND_WAIT_ALL);

		{
			uio_Stream *save_fp;
			BOOLEAN err;

			err = FALSE;
			save_fp = res_OpenResFile (configDir, "melee.cfg", "wb");
			if (save_fp)
			{
				if (PutResFileChar (PlayerControl[0], save_fp) == -1)
					err = TRUE;
				if (!err && WriteTeamImage (&MenuState.TeamImage[0], save_fp) == 0)
					err = TRUE;
				if (!err && PutResFileChar (PlayerControl[1], save_fp) == -1)
					err = TRUE;
				if (!err && WriteTeamImage (&MenuState.TeamImage[1], save_fp) == 0)
					err = TRUE;
				if (res_CloseResFile (save_fp) == 0)
					err = TRUE;
			}
			else
				err = TRUE;
				
			if (err)
			{
				DeleteResFile (configDir, "melee.cfg");
			}
		}
		FreeMeleeInfo (&MenuState);
		DestroySound (ReleaseSound (GameSounds));
		GameSounds = 0;

		DestroyDrawable (ReleaseDrawable (PickMeleeFrame));
		PickMeleeFrame = 0;

		FlushInput ();
	}
}

