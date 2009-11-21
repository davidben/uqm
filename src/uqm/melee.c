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
#include "status.h"
#include "colors.h"
#include "comm.h"
		// for getLineWithinWidth
#include "cons_res.h"
#include "controls.h"
#include "libs/file.h"
#include "fmv.h"
#include "gamestr.h"
#include "globdata.h"
#include "intel.h"
#include "master.h"
#include "nameref.h"
#ifdef NETPLAY
#	include "netplay/netconnection.h"
#	include "netplay/netmelee.h"
#	include "netplay/notify.h"
#	include "libs/graphics/widgets.h"
#	include "cnctdlg.h"
#endif
#include "options.h"
#include "races.h"
#include "resinst.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "util.h"
#include "planets/planets.h"
		// for NUMBER_OF_PLANET_TYPES
#include "libs/graphics/drawable.h"
#include "libs/gfxlib.h"
#include "libs/mathlib.h"
#include "libs/log.h"


#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>


static void DrawMeleeShipStrings (MELEE_STATE *pMS, BYTE NewStarShip);
static void StartMelee (MELEE_STATE *pMS);
static ssize_t numPlayersReady (void);

enum
{
#ifdef NETPLAY
	NET_TOP,
#endif
	CONTROLS_TOP,
	SAVE_TOP,
	LOAD_TOP,
	START_MELEE,
	LOAD_BOT,
	SAVE_BOT,
	CONTROLS_BOT,
#ifdef NETPLAY
	NET_BOT,
#endif
	QUIT_BOT,
	EDIT_MELEE
};

#ifdef NETPLAY
#define TOP_ENTRY NET_TOP
#else
#define TOP_ENTRY CONTROLS_TOP
#endif

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

#define MELEE_STATUS_X_OFFS 1
#define MELEE_STATUS_Y_OFFS 201
#define MELEE_STATUS_WIDTH  (NUM_MELEE_COLUMNS * \
		(MELEE_BOX_WIDTH + MELEE_BOX_SPACE))
#define MELEE_STATUS_HEIGHT 38

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

// XXX: The following entries are unused:
#define LIST_INFO_BACKGROUND_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x14), 0x05)
#define LIST_INFO_TITLE_COLOR \
		WHITE_COLOR
#define LIST_INFO_TEXT_COLOR \
		LT_GRAY_COLOR
#define LIST_INFO_CURENTRY_TEXT_COLOR \
		WHITE_COLOR
#define HI_LIST_INFO_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04)
#define HI_LIST_INFO_BACKGROUND_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x1F), 0x0D)

#define TEAM_NAME_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x0F, 0x10, 0x1B), 0x00)
#define TEAM_NAME_EDIT_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x17, 0x18, 0x1D), 0x00)
#define TEAM_NAME_EDIT_RECT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x14), 0x05)
#define TEAM_NAME_EDIT_CURS_COLOR \
		WHITE_COLOR

#define PICKSHIP_TEAM_NAME_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09)
#define PICKSHIP_TEAM_START_VALUE_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x04, 0x05, 0x1F), 0x4B)

#define SHIPBOX_TOPLEFT_COLOR_NORMAL \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x09), 0x56)
#define SHIPBOX_BOTTOMRIGHT_COLOR_NORMAL \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x0E), 0x54)
#define SHIPBOX_INTERIOR_COLOR_NORMAL \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x0C), 0x55)

#define SHIPBOX_TOPLEFT_COLOR_HILITE \
		BUILD_COLOR (MAKE_RGB15 (0x07, 0x00, 0x0C), 0x3E)
#define SHIPBOX_BOTTOMRIGHT_COLOR_HILITE \
		BUILD_COLOR (MAKE_RGB15 (0x0C, 0x00, 0x14), 0x3C)
#define SHIPBOX_INTERIOR_COLOR_HILITE \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x00, 0x11), 0x3D)

#define MELEE_STATUS_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x00), 0x02)


FRAME PickMeleeFrame;
FRAME MeleeFrame;
		// Loaded from melee/melebkgd.ani
static FRAME BuildPickFrame;
		// Constructed.
MELEE_STATE *pMeleeState;

BOOLEAN DoMelee (MELEE_STATE *pMS);
static BOOLEAN DoEdit (MELEE_STATE *pMS);
static BOOLEAN DoPickShip (MELEE_STATE *pMS);
static BOOLEAN DoConfirmSettings (MELEE_STATE *pMS);

#define DTSHS_NORMAL   0
#define DTSHS_EDIT     1
#define DTSHS_SELECTED 2
#define DTSHS_REPAIR   4
#define DTSHS_BLOCKCUR 8

static BOOLEAN DrawTeamString (MELEE_STATE *pMS, COUNT side,
		COUNT HiLiteState);


// These icons come from melee/melebkgd.ani
void
DrawMeleeIcon (COUNT which_icon)
{
	STAMP s;
			
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SetAbsFrameIndex (MeleeFrame, which_icon);
	DrawStamp (&s);
}

static void
GetShipBox (RECT *pRect, COUNT side, COUNT row, COUNT col)
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

static FleetShipIndex
GetShipIndex (BYTE row, BYTE col)
{
	return row * NUM_MELEE_COLUMNS + col;
}

static BYTE
GetShipRow (FleetShipIndex index)
{
	return index / NUM_MELEE_COLUMNS;
}

static BYTE
GetShipColumn (int index)
{
	return index % NUM_MELEE_COLUMNS;
}

static void
DrawShipBox (COUNT side, COUNT row, COUNT col, BYTE ship, BOOLEAN HiLite)
{
	RECT r;

	GetShipBox (&r, side, row, col);

	BatchGraphics ();
	if (HiLite)
		DrawStarConBox (&r, 1,
				SHIPBOX_TOPLEFT_COLOR_HILITE,
				SHIPBOX_BOTTOMRIGHT_COLOR_HILITE,
				(BOOLEAN)(ship != MELEE_NONE),
				SHIPBOX_INTERIOR_COLOR_HILITE);
	else
		DrawStarConBox (&r, 1,
				SHIPBOX_TOPLEFT_COLOR_NORMAL,
				SHIPBOX_BOTTOMRIGHT_COLOR_NORMAL,
				(BOOLEAN)(ship != MELEE_NONE),
				SHIPBOX_INTERIOR_COLOR_NORMAL);

	if (ship != MELEE_NONE)
	{
		STAMP s;
		s.origin.x = r.corner.x + (r.extent.width >> 1);
		s.origin.y = r.corner.y + (r.extent.height >> 1);
		s.frame = GetShipMeleeIconsFromIndex (ship);

		DrawStamp (&s);
	}
	UnbatchGraphics ();
}

static void
DrawShipBoxCurrent (MELEE_STATE *pMS, BOOLEAN HiLite)
{
	FleetShipIndex index = GetShipIndex (pMS->row, pMS->col);
	BYTE ship = pMS->SideState[pMS->side].TeamImage.ShipList[index];
	DrawShipBox (pMS->side, pMS->row, pMS->col, ship, HiLite);
}

// Draw an image for one of the control method selection buttons.
static void
DrawControls (COUNT which_side, BOOLEAN HiLite)
{
	COUNT which_icon;

	if (PlayerControl[which_side] & NETWORK_CONTROL)
	{
		DrawMeleeIcon (31 + (HiLite ? 1 : 0) + 2 * (1 - which_side));
				/* "Network Control" */
		return;
	}
	
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
DrawPickFrame (MELEE_STATE *pMS)
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
	s.origin.x = 0;
	s.origin.y = 0;
	DrawStamp (&s);

	UnlockMutex (GraphicsLock);
	DrawMeleeShipStrings (pMS, (BYTE)pMS->CurIndex);
	LockMutex (GraphicsLock);
}

void
RepairMeleeFrame (RECT *pRect)
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
	SetFrameHot (Screen,
			MAKE_HOT_SPOT (r.corner.x - SAFE_X, r.corner.y - SAFE_Y));
	BatchGraphics ();

	DrawMeleeIcon (0);   /* Entire melee screen */
#ifdef NETPLAY
	DrawMeleeIcon (35);  /* "Net..." (top, not highlighted) */
	DrawMeleeIcon (37);  /* "Net..." (bottom, not highlighted) */
#endif
	DrawMeleeIcon (26);  /* "Battle!" (highlighted) */
	{
		COUNT side;
		COUNT row;
		COUNT col;
	
		for (side = 0; side < NUM_SIDES; side++)
		{
			DrawControls (side, FALSE);
			for (row = 0; row < NUM_MELEE_ROWS; row++)
			{
				for (col = 0; col < NUM_MELEE_COLUMNS; col++)
				{
					FleetShipIndex index = GetShipIndex (row, col);
					BYTE ship = pMeleeState->SideState[side].TeamImage.
							ShipList[index];
					DrawShipBox (side, row, col, ship, FALSE);
				}
			}

			DrawTeamString (pMeleeState, side, DTSHS_NORMAL);
		}
	}
	
	if (pMeleeState->InputFunc == DoPickShip)
		DrawPickFrame (pMeleeState);
		
	UnbatchGraphics ();
	SetFrameHot (Screen, MAKE_HOT_SPOT (0, 0));
	SetContextClipRect (&OldRect);
	SetContext (OldContext);
}

static void
RedrawMeleeFrame (void)
{
	RECT r;

	r.corner.x = 0;
	r.corner.y = 0;
	r.extent.width = SCREEN_WIDTH;
	r.extent.height = SCREEN_HEIGHT;

	RepairMeleeFrame (&r);
}

static BOOLEAN
DrawTeamString (MELEE_STATE *pMS, COUNT side, COUNT HiLiteState)
{
	RECT r;
	TEXT lfText;

	r.corner.x = MELEE_X_OFFS - 1;
	r.corner.y = (side + 1) * (MELEE_Y_OFFS
			+ ((MELEE_BOX_HEIGHT + MELEE_BOX_SPACE) * NUM_MELEE_ROWS + 2));
	r.extent.width = NUM_MELEE_COLUMNS * (MELEE_BOX_WIDTH + MELEE_BOX_SPACE);
	r.extent.height = 13;
	if (HiLiteState == DTSHS_REPAIR)
	{
		RepairMeleeFrame (&r);
		return (TRUE);
	}
		
	SetContextFont (MicroFont);

	lfText.pStr = pMS->SideState[side].TeamImage.TeamName;
	lfText.baseline.y = r.corner.y + r.extent.height - 3;

	lfText.baseline.x = r.corner.x + 1;
	lfText.align = ALIGN_LEFT;
	lfText.CharCount = strlen (lfText.pStr);

	BatchGraphics ();
	if (!(HiLiteState & DTSHS_EDIT))
	{	// normal or selected state
		TEXT rtText;
		UNICODE buf[30];

		sprintf (buf, "%u", pMS->SideState[side].star_bucks);
		rtText.pStr = buf;
		rtText.align = ALIGN_RIGHT;
		rtText.CharCount = (COUNT)~0;
		rtText.baseline.y = lfText.baseline.y;
		rtText.baseline.x = lfText.baseline.x + r.extent.width - 1;

		SetContextForeGroundColor (!(HiLiteState & DTSHS_SELECTED)
				? TEAM_NAME_TEXT_COLOR : TEAM_NAME_EDIT_TEXT_COLOR);
		font_DrawText (&lfText);
		font_DrawText (&rtText);
	}
	else
	{	// editing state
		COUNT i;
		RECT text_r;
		BYTE char_deltas[MAX_TEAM_CHARS];
		BYTE *pchar_deltas;

		// not drawing team bucks
		r.extent.width -= 29;

		TextRect (&lfText, &text_r, char_deltas);
		if ((text_r.extent.width + 2) >= r.extent.width)
		{	// the text does not fit the input box size and so
			// will not fit when displayed later
			UnbatchGraphics ();
			// disallow the change
			return (FALSE);
		}

		text_r = r;
		SetContextForeGroundColor (TEAM_NAME_EDIT_RECT_COLOR);
		DrawFilledRectangle (&text_r);

		// calculate the cursor position and draw it
		pchar_deltas = char_deltas;
		for (i = pMS->CurIndex; i > 0; --i)
			text_r.corner.x += (SIZE)*pchar_deltas++;
		if (pMS->CurIndex < lfText.CharCount) /* cursor mid-line */
			--text_r.corner.x;
		if (HiLiteState & DTSHS_BLOCKCUR)
		{	// Use block cursor for keyboardless systems
			if (pMS->CurIndex == lfText.CharCount)
			{	// cursor at end-line -- use insertion point
				text_r.extent.width = 1;
			}
			else if (pMS->CurIndex + 1 == lfText.CharCount)
			{	// extra pixel for last char margin
				text_r.extent.width = (SIZE)*pchar_deltas + 2;
			}
			else
			{	// normal mid-line char
				text_r.extent.width = (SIZE)*pchar_deltas + 1;
			}
		}
		else
		{	// Insertion point cursor
			text_r.extent.width = 1;
		}
		// position cursor within input field rect
		++text_r.corner.x;
		++text_r.corner.y;
		text_r.extent.height -= 2;
		SetContextForeGroundColor (TEAM_NAME_EDIT_CURS_COLOR);
		DrawFilledRectangle (&text_r);

		SetContextForeGroundColor (BLACK_COLOR); // TEAM_NAME_EDIT_TEXT_COLOR);
		font_DrawText (&lfText);
	}
	UnbatchGraphics ();

	return (TRUE);
}

// Draw a ship icon in the ship selection popup.
static void
DrawPickIcon (COUNT iship, BYTE DrawErase)
{
	STAMP s;
	RECT r;

	GetFrameRect (BuildPickFrame, &r);

	s.origin.x = r.corner.x + 20 + (iship % NUM_PICK_COLS) * 18;
	s.origin.y = r.corner.y +  5 + (iship / NUM_PICK_COLS) * 18;
	s.frame = GetShipIconsFromIndex (iship);
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
}

#ifdef NETPLAY
// This function is generic. It should probably be moved to elsewhere.
// The caller should hold the GraphicsLock.
static void
multiLineDrawText (TEXT *textIn, RECT *clipRect) {
	RECT oldRect;

	SIZE leading;
	TEXT text;
	SIZE lineWidth;

	GetContextClipRect (&oldRect);

	SetContextClipRect (clipRect);
	GetContextFontLeading (&leading);

	text = *textIn;
	text.baseline.x = 1;
	text.baseline.y = 0;

	if (clipRect->extent.width <= text.baseline.x)
		goto out;

	lineWidth = clipRect->extent.width - text.baseline.x;

	while (*text.pStr != '\0') {
		const unsigned char *nextLine;

		text.baseline.y += leading;
		text.CharCount = (COUNT) ~0;
		getLineWithinWidth (&text, &nextLine, lineWidth, text.CharCount);
				// This will also fill in text->CharCount.
			
		font_DrawText (&text);

		text.pStr = nextLine;
	}

out:
	SetContextClipRect (&oldRect);
}

// Use an empty string to clear the status area.
static void
DrawMeleeStatusMessage (const char *message)
{
	CONTEXT oldContext;
	RECT r;

	LockMutex (GraphicsLock);
	oldContext = SetContext (SpaceContext);

	r.corner.x = MELEE_STATUS_X_OFFS;
	r.corner.y = MELEE_STATUS_Y_OFFS;
	r.extent.width = MELEE_STATUS_WIDTH;
	r.extent.height = MELEE_STATUS_HEIGHT;

	RepairMeleeFrame (&r);
	
	if (message[0] != '\0')
	{
		TEXT lfText;
		lfText.pStr = message;
		lfText.align = ALIGN_LEFT;
		lfText.CharCount = (COUNT)~0;

		SetContextFont (MicroFont);
		SetContextForeGroundColor (MELEE_STATUS_COLOR);
		
		BatchGraphics ();
		multiLineDrawText (&lfText, &r);
		UnbatchGraphics ();
	}

	SetContext (oldContext);
	UnlockMutex (GraphicsLock);
}

static void
UpdateMeleeStatusMessage (ssize_t player)
{
	NetConnection *conn;

	assert (player == -1 || (player >= 0 && player < NUM_PLAYERS));

	if (player == -1)
	{
		DrawMeleeStatusMessage ("");
		return;
	}

	conn = netConnections[player];
	if (conn == NULL)
	{
		DrawMeleeStatusMessage (GAME_STRING (NETMELEE_STRING_BASE + 0));
				// "Unconnected. Press LEFT to connect."
		return;
	}
	
	switch (NetConnection_getState (conn)) {
		case NetState_unconnected:
			DrawMeleeStatusMessage (GAME_STRING (NETMELEE_STRING_BASE + 0));
					// "Unconnected. Press LEFT to connect."
			break;
		case NetState_connecting:
			if (NetConnection_getPeerOptions (conn)->isServer)
				DrawMeleeStatusMessage (
						GAME_STRING (NETMELEE_STRING_BASE + 1));
						// "Awaiting incoming connection...\n"
						// "Press RIGHT to cancel."
			else
				DrawMeleeStatusMessage (
						GAME_STRING (NETMELEE_STRING_BASE + 2));
						// "Attempting outgoing connection...\n"
						// "Press RIGHT to cancel."
			break;
		case NetState_init:
		case NetState_inSetup:
			DrawMeleeStatusMessage (GAME_STRING (NETMELEE_STRING_BASE + 3));
					// "Connected. Press RIGHT to disconnect."
			break;
		default:
			DrawMeleeStatusMessage ("");
			break;
	}
}
#endif  /* NETPLAY */

// XXX: this function is called when the current selection is blinking off.
// The caller should hold the GraphicsLock.
static void
Deselect (BYTE opt)
{
	switch (opt)
	{
		case START_MELEE:
			DrawMeleeIcon (25);  /* "Battle!" (not highlighted) */
			break;
		case LOAD_TOP:
			DrawMeleeIcon (17); /* "Load" (top, not highlighted) */
			break;
		case LOAD_BOT:
			DrawMeleeIcon (22); /* "Load" (bottom, not highlighted) */
			break;
		case SAVE_TOP:
			DrawMeleeIcon (18);  /* "Save" (top, not highlighted) */
			break;
		case SAVE_BOT:
			DrawMeleeIcon (21);  /* "Save" (bottom, not highlighted) */
			break;
#ifdef NETPLAY
		case NET_TOP:
			DrawMeleeIcon (35);  /* "Net..." (top, not highlighted) */
			break;
		case NET_BOT:
			DrawMeleeIcon (37);  /* "Net..." (bottom, not highlighted) */
			break;
#endif
		case QUIT_BOT:
			DrawMeleeIcon (29);  /* "Quit" (not highlighted) */
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
					DrawShipBoxCurrent (pMeleeState, FALSE);
				else if (pMeleeState->CurIndex == (BYTE)~0)
					DrawTeamString (pMeleeState, pMeleeState->side,
							DTSHS_NORMAL);
			}
			else if (pMeleeState->InputFunc == DoPickShip)
			{
				DrawPickIcon (pMeleeState->CurIndex, 1);
			}
			break;
	}
}

// XXX: this function is called when the current selection is blinking off.
// The caller should hold the GraphicsLock.
static void
Select (BYTE opt)
{
	switch (opt)
	{
		case START_MELEE:
			DrawMeleeIcon (26);  /* "Battle!" (highlighted) */
			break;
		case LOAD_TOP:
			DrawMeleeIcon (19); /* "Load" (top, highlighted) */
			break;
		case LOAD_BOT:
			DrawMeleeIcon (24); /* "Load" (bottom, highlighted) */
			break;
		case SAVE_TOP:
			DrawMeleeIcon (20);  /* "Save" (top; highlighted) */
			break;
		case SAVE_BOT:
			DrawMeleeIcon (23);  /* "Save" (bottom; highlighted) */
			break;
#ifdef NETPLAY
		case NET_TOP:
			DrawMeleeIcon (36);  /* "Net..." (top; highlighted) */
			break;
		case NET_BOT:
			DrawMeleeIcon (38);  /* "Net..." (bottom; highlighted) */
			break;
#endif
		case QUIT_BOT:
			DrawMeleeIcon (30);  /* "Quit" (highlighted) */
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
					DrawShipBoxCurrent (pMeleeState, TRUE);
				else if (pMeleeState->CurIndex == (BYTE)~0)
					DrawTeamString (pMeleeState, pMeleeState->side,
							DTSHS_SELECTED);
			}
			else if (pMeleeState->InputFunc == DoPickShip)
				DrawPickIcon (pMeleeState->CurIndex, 0);
			break;
	}
}

static void
flashSelection (MELEE_STATE *pMS)
{
#define FLASH_RATE (ONE_SECOND / 9)
	static TimeCount NextTime = 0;
	static bool select = false;
	TimeCount Now = GetTimeCounter ();

	if (Now >= NextTime)
	{
		CONTEXT OldContext;

		NextTime = Now + FLASH_RATE;
		select ^= true;

		LockMutex (GraphicsLock);
		OldContext = SetContext (SpaceContext);
		if (select)
			Select (pMS->MeleeOption);
		else
			Deselect (pMS->MeleeOption);
		SetContext (OldContext);
		UnlockMutex (GraphicsLock);
	}
}

static void
InitMelee (MELEE_STATE *pMS)
{
	RECT r;

	SetContext (SpaceContext);
	SetContextFGFrame (Screen);
	SetContextClipRect (NULL);
	SetContextBackGroundColor (BLACK_COLOR);
	ClearDrawable ();
	r.corner.x = SAFE_X;
	r.corner.y = SAFE_Y;
	r.extent.width = SCREEN_WIDTH - (SAFE_X * 2);
	r.extent.height = SCREEN_HEIGHT - (SAFE_Y * 2);
	SetContextClipRect (&r);

	r.corner.x = r.corner.y = 0;
	RedrawMeleeFrame ();
}

static void
DrawMeleeShipStrings (MELEE_STATE *pMS, BYTE NewStarShip)
{
	RECT r, OldRect;
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

	if (NewStarShip == MELEE_NONE)
	{
		RECT r;
		TEXT t;

		ClearShipStatus (0);
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
		HMASTERSHIP hMasterShip;
		MASTER_SHIP_INFO *MasterPtr;

		hMasterShip = GetStarShipFromIndex (&master_q, NewStarShip);
		MasterPtr = LockMasterShip (&master_q, hMasterShip);

		InitShipStatus (&MasterPtr->ShipInfo, NULL, NULL);

		UnlockMasterShip (&master_q, hMasterShip);
	}

	UnbatchGraphics ();
	SetContextClipRect (&OldRect);
	SetContext (OldContext);

	UnlockMutex (GraphicsLock);
}

static void
UpdateCurrentShip (MELEE_STATE *pMS)
{
	FleetShipIndex fleetShipIndex = GetShipIndex (pMS->row, pMS->col);
	if (pMS->row == NUM_MELEE_ROWS)
		pMS->CurIndex = MELEE_NONE;
	else
		pMS->CurIndex =
				pMS->SideState[pMS->side].TeamImage.ShipList[fleetShipIndex];
	DrawMeleeShipStrings (pMS, (BYTE)(pMS->CurIndex));
}

// returns (COUNT) ~0 for an invalid ship.
static COUNT
GetShipValue (BYTE StarShip)
{
	COUNT val;

	if (StarShip == MELEE_NONE)
		return 0;

	val = GetShipCostFromIndex (StarShip);
	if (val == 0)
		val = (COUNT)~0;

	return val;
}

COUNT
GetTeamValue (TEAM_IMAGE *pTI)
{
	FleetShipIndex index;
	COUNT val;

	val = 0;
	for (index = 0; index < MELEE_FLEET_SIZE; index++)
	{
		BYTE StarShip = pTI->ShipList[index];
		COUNT shipVal = GetShipValue (StarShip);
		if (shipVal == (COUNT)~0)
			pTI->ShipList[index] = MELEE_NONE;
		val += shipVal;
	}
	
	return val;
}

static void
DeleteCurrentShip (MELEE_STATE *pMS)
{
	RECT r;
	FleetShipIndex fleetShipIndex;
	int CurIndex;

	fleetShipIndex = GetShipIndex (pMS->row, pMS->col);
	CurIndex = pMS->SideState[pMS->side].TeamImage.ShipList[fleetShipIndex];
	pMS->SideState[pMS->side].star_bucks -= GetShipCostFromIndex (CurIndex);
	pMS->SideState[pMS->side].TeamImage.ShipList[fleetShipIndex] = MELEE_NONE;

	LockMutex (GraphicsLock);
	GetShipBox (&r, pMS->side, pMS->row, pMS->col);
	RepairMeleeFrame (&r);

	DrawTeamString (pMS, pMS->side, DTSHS_REPAIR);
	UnlockMutex (GraphicsLock);
}

static void
AdvanceCursor (MELEE_STATE *pMS)
{
	++pMS->col;
	if (pMS->col == NUM_MELEE_COLUMNS)
	{
		++pMS->row;
		if (pMS->row < NUM_MELEE_ROWS)
			pMS->col = 0;
		else
		{
			pMS->col = NUM_MELEE_COLUMNS - 1;
			pMS->row = NUM_MELEE_ROWS - 1;
		}
	}
}

static BOOLEAN
OnTeamNameChange (TEXTENTRY_STATE *pTES)
{
	MELEE_STATE *pMS = (MELEE_STATE*) pTES->CbParam;
	BOOLEAN ret;
	COUNT hl = DTSHS_EDIT;

	pMS->CurIndex = pTES->CursorPos;
	if (pTES->JoystickMode)
		hl |= DTSHS_BLOCKCUR;

	LockMutex (GraphicsLock);
	ret = DrawTeamString (pMS, pMS->side, hl);
	UnlockMutex (GraphicsLock);

	return ret;
}

static BOOLEAN
DoEdit (MELEE_STATE *pMS)
{
	DWORD TimeIn = GetTimeCounter ();

	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);
	
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT | MENU_SOUND_DELETE);
	if (!pMS->Initialized)
	{
		FleetShipIndex fleetShipIndex = GetShipIndex (pMS->row, pMS->col);
		pMS->CurIndex =
				pMS->SideState[pMS->side].TeamImage.ShipList[fleetShipIndex];
		DrawMeleeShipStrings (pMS, (BYTE)pMS->CurIndex);

		pMS->Initialized = TRUE;
		pMS->InputFunc = DoEdit;
		return TRUE;
	}

#ifdef NETPLAY
	netInput ();
#endif
	if ((pMS->row < NUM_MELEE_ROWS || pMS->CurIndex == (BYTE)~0)
			&& (PulsedInputState.menu[KEY_MENU_CANCEL]
			|| (PulsedInputState.menu[KEY_MENU_RIGHT]
			&& (pMS->col == NUM_MELEE_COLUMNS - 1
			|| pMS->row == NUM_MELEE_ROWS))))
	{
		// Done editing the teams.
		LockMutex (GraphicsLock);
		Deselect (EDIT_MELEE);
		pMS->CurIndex = (COUNT)~0;
		pMS->MeleeOption = START_MELEE;
		pMS->InputFunc = DoMelee;
		UnlockMutex (GraphicsLock);
		pMS->LastInputTime = GetTimeCounter ();
	}
	else if (pMS->row < NUM_MELEE_ROWS
			&& PulsedInputState.menu[KEY_MENU_SELECT])
	{
		// Show a popup to add a new ship to the current team.
		pMS->Initialized = 0;
		FlushInput ();
		DoPickShip (pMS);
	}
	else if (pMS->row < NUM_MELEE_ROWS
			&& PulsedInputState.menu[KEY_MENU_SPECIAL])
	{
		// TODO: this is a stub; Should we display a ship spin?
		LockMutex (GraphicsLock);
		Deselect (EDIT_MELEE);
		if (pMS->CurIndex != (BYTE)~0)
		{
			// Do something with pMS->CurIndex here
		}
		UnlockMutex (GraphicsLock);
	}
	else if (pMS->row < NUM_MELEE_ROWS &&
			PulsedInputState.menu[KEY_MENU_DELETE])
	{
		// Remove the currently selected ship from the current team.
		FleetShipIndex fleetShipIndex;
		DeleteCurrentShip (pMS);
		fleetShipIndex = GetShipIndex (pMS->row, pMS->col);
		fleetShipChanged (pMS, pMS->side, fleetShipIndex);
		AdvanceCursor (pMS);
		UpdateCurrentShip (pMS);
	}
	else
	{
		COUNT side, row, col;

		side = pMS->side;
		row = pMS->row;
		col = pMS->col;

		if (row == NUM_MELEE_ROWS)
		{
			// Edit the name of the current team.
			if (PulsedInputState.menu[KEY_MENU_SELECT])
			{
				TEXTENTRY_STATE tes;

				// going to enter text
				// XXX: CurIndex is abused here; DrawTeamString expects
				//   CurIndex to contain the current cursor position
				pMS->CurIndex = 0;
				LockMutex (GraphicsLock);
				DrawTeamString (pMS, pMS->side, DTSHS_EDIT);
				UnlockMutex (GraphicsLock);

				tes.Initialized = FALSE;
				tes.MenuRepeatDelay = 0;
				tes.BaseStr = pMS->SideState[pMS->side].TeamImage.TeamName;
				tes.CursorPos = 0;
				tes.MaxSize = MAX_TEAM_CHARS + 1;
				tes.CbParam = pMS;
				tes.ChangeCallback = OnTeamNameChange;
				tes.FrameCallback = 0;
				DoTextEntry (&tes);
				
				// done entering
				pMS->CurIndex = (BYTE)~0;
				LockMutex (GraphicsLock);
				DrawTeamString (pMS, pMS->side, DTSHS_REPAIR);
				UnlockMutex (GraphicsLock);
		
				teamStringChanged (pMS, pMS->side);
				
				return (TRUE);
			}
		}

		{
			if (PulsedInputState.menu[KEY_MENU_LEFT])
			{
				if (col > 0)
					--col;
			}
			else if (PulsedInputState.menu[KEY_MENU_RIGHT])
			{
				if (col < NUM_MELEE_COLUMNS - 1)
					++col;
			}

			if (PulsedInputState.menu[KEY_MENU_UP])
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
			else if (PulsedInputState.menu[KEY_MENU_DOWN])
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
			UnlockMutex (GraphicsLock);

			UpdateCurrentShip (pMS);
		}
	}

#ifdef NETPLAY
	flushPacketQueues ();
#endif

	flashSelection (pMS);

	SleepThreadUntil (TimeIn + ONE_SECOND / 30);

	return (TRUE);
}

// Handle the popup from which a ship to add to the fleet can be chosen.
static BOOLEAN
DoPickShip (MELEE_STATE *pMS)
{
	DWORD TimeIn = GetTimeCounter ();

	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	if (!pMS->Initialized)
	{
		if (pMS->CurIndex == (BYTE)~0)
			pMS->CurIndex = 0;

		LockMutex (GraphicsLock);
		Deselect (EDIT_MELEE);
		pMS->InputFunc = DoPickShip;
		DrawPickFrame (pMS);
		pMS->Initialized = TRUE;
		UnlockMutex (GraphicsLock);
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT] ||
			PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		FleetShipIndex index = GetShipIndex (pMS->row, pMS->col);
		pMS->InputFunc = 0; /* disable ship flashing */

		if (!PulsedInputState.menu[KEY_MENU_CANCEL])
		{
			// Add the currently selected ship to the fleet.
			pMS->SideState[pMS->side].star_bucks +=
					GetShipCostFromIndex (pMS->CurIndex);
			pMS->SideState[pMS->side].TeamImage.ShipList[index] =
					pMS->CurIndex;
			LockMutex (GraphicsLock);
			DrawTeamString (pMS, pMS->side, DTSHS_REPAIR);
			DrawShipBoxCurrent (pMS, FALSE);
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

		fleetShipChanged (pMS, pMS->side, index);
		UpdateCurrentShip (pMS);

		pMS->InputFunc = DoEdit;

		return (TRUE);
	}
	else if (PulsedInputState.menu[KEY_MENU_SPECIAL]
			&& (pMeleeState->CurIndex != (BYTE)~0))
	{
		DoShipSpin (pMS->CurIndex, (MUSIC_REF)0);
	
		return (TRUE);
	}

	{
		BYTE NewStarShip;

		NewStarShip = pMS->CurIndex;

		if (PulsedInputState.menu[KEY_MENU_LEFT])
		{
			if (NewStarShip % NUM_PICK_COLS == 0)
				NewStarShip += NUM_PICK_COLS;
			--NewStarShip;
		}
		else if (PulsedInputState.menu[KEY_MENU_RIGHT])
		{
			++NewStarShip;
			if (NewStarShip % NUM_PICK_COLS == 0)
				NewStarShip -= NUM_PICK_COLS;
		}
		
		if (PulsedInputState.menu[KEY_MENU_UP])
		{
			if (NewStarShip >= NUM_PICK_COLS)
				NewStarShip -= NUM_PICK_COLS;
			else
				NewStarShip += NUM_PICK_COLS * (NUM_PICK_ROWS - 1);
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
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

	flashSelection (pMS);

	SleepThreadUntil (TimeIn + ONE_SECOND / 30);

	return (TRUE);
}

// Returns -1 if a connection has been aborted.
static ssize_t
numPlayersReady (void)
{
	size_t player;
	size_t numDone;

	numDone = 0;
	for (player = 0; player < NUM_PLAYERS; player++)
	{
		if (!(PlayerControl[player] & NETWORK_CONTROL))
		{
			numDone++;
			continue;
		}

#ifdef NETPLAY
		{
			NetConnection *conn;

			conn = netConnections[player];

			if (conn == NULL || !NetConnection_isConnected (conn))
				return -1;

			if (NetConnection_getState (conn) > NetState_inSetup)
				numDone++;
		}
#endif
	}

	return numDone;
}

// The player has pressed "Start Game", and all Network players are
// connected. We're now just waiting for the confirmation of the other
// party.
// When the other party changes something in the settings, the confirmation
// is cancelled.
static BOOLEAN
DoConfirmSettings (MELEE_STATE *pMS)
{
#ifdef NETPLAY
	ssize_t numDone;
#endif

	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;

	if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		pMS->InputFunc = DoMelee;
#ifdef NETPLAY
		cancelConfirmations ();
		DrawMeleeStatusMessage (GAME_STRING (NETMELEE_STRING_BASE + 4));
				// "Confirmation cancelled. Press FIRE to reconfirm."
#endif
		return TRUE;
	}

	if (PulsedInputState.menu[KEY_MENU_LEFT] ||
			PulsedInputState.menu[KEY_MENU_UP] ||
			PulsedInputState.menu[KEY_MENU_DOWN])
	{
		pMS->InputFunc = DoMelee;
#ifdef NETPLAY
		cancelConfirmations ();
		DrawMeleeStatusMessage ("");
#endif
		return DoMelee (pMS);
				// Let the pressed keys take effect immediately.
	}

#ifndef NETPLAY
	pMS->InputFunc = DoMelee;
	SeedRandomNumbers ();
	StartMelee (pMS);
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return FALSE;
	return TRUE;
#else
	closeDisconnectedConnections ();
	netInput ();
	SleepThread (ONE_SECOND / 120);

	numDone = numPlayersReady ();
	if (numDone == -1)
	{
		// Connection aborted
		cancelConfirmations ();
		flushPacketQueues ();
		pMS->InputFunc = DoMelee;
		return TRUE;
	}
	else if (numDone != NUM_SIDES)
	{
		// Still waiting for some confirmation.
		return TRUE;
	}
		
	// All sides have confirmed.

	// Send our own prefered frame delay.
	sendInputDelayConnections (netplayOptions.inputDelay);

	// Synchronise the RNGs:
	{
		COUNT player;

		for (player = 0; player < NUM_PLAYERS; player++)
		{
			NetConnection *conn;

			if (!(PlayerControl[player] & NETWORK_CONTROL))
				continue;

			conn = netConnections[player];
			assert (conn != NULL);

			if (!NetConnection_isConnected (conn))
				continue;

			if (NetConnection_getDiscriminant (conn))
				Netplay_seedRandom (conn, SeedRandomNumbers ());
		}
		flushPacketQueues ();
	}

	{
		// One side will send the seed followed by 'Done' and wait
		// for the other side to report 'Done'.
		// The other side will report 'Done' and will wait for the other
		// side to report 'Done', but before the reception of 'Done'
		// it will have received the seed.
		bool allOk = negotiateReadyConnections (true, NetState_interBattle);
		if (!allOk)
			return FALSE;
	}

	// The maximum value for all connections is used.
	{
		bool ok = setupInputDelay (netplayOptions.inputDelay);
		if (!ok)
			return FALSE;
	}
	
	pMS->InputFunc = DoMelee;
	
	StartMelee (pMS);
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return FALSE;

	return TRUE;
#endif  /* defined (NETPLAY) */
}

static void
LoadMeleeInfo (MELEE_STATE *pMS)
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
	s.origin.x = 0;
	s.origin.y = 0;
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
			// 5x5 grid of ships to pick from
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

	LoadTeamList (pMS);
}

static void
FreeMeleeInfo (MELEE_STATE *pMS)
{
	DestroyDirEntryTable (ReleaseDirEntryTable (pMS->load.dirEntries));
	pMS->load.dirEntries = 0;

	if (pMS->hMusic)
	{
		DestroyMusic (pMS->hMusic);
		pMS->hMusic = 0;
	}

	UninitSpace ();

	DestroyDrawable (ReleaseDrawable (MeleeFrame));
	MeleeFrame = 0;
	DestroyDrawable (ReleaseDrawable (BuildPickFrame));
	BuildPickFrame = 0;

#ifdef NETPLAY
	closeAllConnections ();
#endif
}

static void
BuildAndDrawShipList (MELEE_STATE *pMS)
{
	COUNT i;
	CONTEXT OldContext;

	OldContext = SetContext (OffScreenContext);

	for (i = 0; i < NUM_SIDES; ++i)
	{
		COUNT side;
		RECT r;
		TEXT t;
		STAMP s;
		UNICODE buf[30];
		FleetShipIndex index;

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

		// Team name at the bottom of the frame:
		t.align = ALIGN_CENTER;
		t.pStr = pMS->SideState[i].TeamImage.TeamName;
		t.CharCount = (COUNT)~0;
		SetContextFont (TinyFont);
		SetContextForeGroundColor (PICKSHIP_TEAM_NAME_TEXT_COLOR);
		font_DrawText (&t);

		// Total team value of the starting team:
		sprintf (buf, "%u", pMS->SideState[i].star_bucks);
		t.baseline.x = 4;
		t.baseline.y = 7;
		t.align = ALIGN_LEFT;
		t.pStr = buf;
		t.CharCount = (COUNT)~0;
		SetContextForeGroundColor (PICKSHIP_TEAM_START_VALUE_COLOR);
		font_DrawText (&t);

		assert (CountLinks (&race_q[side]) == 0);

		for (index = 0; index < MELEE_FLEET_SIZE; index++)
		{
			BYTE StarShip;

			StarShip = pMS->SideState[i].TeamImage.ShipList[index];
			if (StarShip == MELEE_NONE)
				continue;

			{
				BYTE row, col;
				BYTE ship_cost;
				HMASTERSHIP hMasterShip;
				HSTARSHIP hBuiltShip;
				MASTER_SHIP_INFO *MasterPtr;
				STARSHIP *BuiltShipPtr;
				BYTE captains_name_index;

				hMasterShip = GetStarShipFromIndex (&master_q, StarShip);
				MasterPtr = LockMasterShip (&master_q, hMasterShip);

				captains_name_index = NameCaptain (&race_q[side],
						MasterPtr->SpeciesID);
				hBuiltShip = Build (&race_q[side], MasterPtr->SpeciesID);

				// Draw the icon.
				row = GetShipRow (index);
				col = GetShipColumn (index);
				s.origin.x = 4 + ((ICON_WIDTH + 2) * col);
				s.origin.y = 10 + ((ICON_HEIGHT + 2) * row);
				s.frame = MasterPtr->ShipInfo.icons;
				DrawStamp (&s);

				ship_cost = MasterPtr->ShipInfo.ship_cost;
				UnlockMasterShip (&master_q, hMasterShip);

				BuiltShipPtr = LockStarShip (&race_q[side], hBuiltShip);
				BuiltShipPtr->index = index;
				BuiltShipPtr->ship_cost = ship_cost;
				BuiltShipPtr->playerNr = side;
				BuiltShipPtr->captains_name_index = captains_name_index;
				// The next ones are not used in Melee
				BuiltShipPtr->crew_level = 0;
				BuiltShipPtr->max_crew = 0;
				BuiltShipPtr->race_strings = 0;
				BuiltShipPtr->icons = 0;
				BuiltShipPtr->RaceDescPtr = 0;
				UnlockStarShip (&race_q[side], hBuiltShip);
			}
		}
	}

	SetContext (OldContext);
}

static void
StartMelee (MELEE_STATE *pMS)
{
	{
		BYTE black_buf[] = {FadeAllToBlack};
		
		FadeMusic (0, ONE_SECOND / 2);
		SleepThreadUntil (XFormColorMap (
				(COLORMAPPTR)black_buf, ONE_SECOND / 2) + ONE_SECOND / 60);
		FlushColorXForms ();
		StopMusic ();
	}
	FadeMusic (NORMAL_VOLUME, 0);
	if (pMS->hMusic)
	{
		DestroyMusic (pMS->hMusic);
		pMS->hMusic = 0;
	}

	do
	{
		if (!SetPlayerInputAll ())
			break;
		LockMutex (GraphicsLock);
		BuildAndDrawShipList (pMS);
		UnlockMutex (GraphicsLock);

		WaitForSoundEnd (TFBSOUND_WAIT_ALL);

		load_gravity_well ((BYTE)((COUNT)TFB_Random () %
					NUMBER_OF_PLANET_TYPES));
		Battle (NULL);
		free_gravity_well ();
		ClearPlayerInputAll ();

		if (GLOBAL (CurrentActivity) & CHECK_ABORT)
			return;

		{
			BYTE black_buf[] = { FadeAllToBlack };
		
			SleepThreadUntil (XFormColorMap (
					(COLORMAPPTR)black_buf, ONE_SECOND / 2)
					+ ONE_SECOND / 60);
			FlushColorXForms ();
		}
	} while (0 /* !(GLOBAL (CurrentActivity) & CHECK_ABORT) */);
	GLOBAL (CurrentActivity) = SUPER_MELEE;

	pMS->Initialized = FALSE;
}

static void
StartMeleeButtonPressed (MELEE_STATE *pMS)
{
	if (pMS->SideState[0].star_bucks == 0 ||
			pMS->SideState[1].star_bucks == 0)
	{
		PlayMenuSound (MENU_SOUND_FAILURE);
		return;
	}
	
#ifdef NETPLAY
	if ((PlayerControl[0] & NETWORK_CONTROL) &&
			(PlayerControl[1] & NETWORK_CONTROL))
	{
		DrawMeleeStatusMessage (GAME_STRING (NETMELEE_STRING_BASE + 32));
				// "Only one side at a time can be network controlled."
		return;
	}

	if (((PlayerControl[0] & NETWORK_CONTROL) &&
			(PlayerControl[1] & COMPUTER_CONTROL)) ||
			((PlayerControl[0] & COMPUTER_CONTROL) &&
			(PlayerControl[1] & NETWORK_CONTROL)))
	{
		DrawMeleeStatusMessage (GAME_STRING (NETMELEE_STRING_BASE + 33));
				// "Netplay with a computer-controlled side is currently
				// not possible."
		return;
	}

	// Check whether all network parties are ready;
	{
		COUNT player;
		bool netReady = true;

		// We collect all error conditions, instead of only reporting
		// the first one.
		for (player = 0; player < NUM_PLAYERS; player++)
		{
			NetConnection *conn;

			if (!(PlayerControl[player] & NETWORK_CONTROL))
				continue;

			conn = netConnections[player];
			if (conn == NULL || !NetConnection_isConnected (conn))
			{
				// Connection for player not established.
				netReady = false;
				if (player == 0)
					DrawMeleeStatusMessage (
							GAME_STRING (NETMELEE_STRING_BASE + 5));
							// "Connection for bottom player not "
							// "established."
				else
					DrawMeleeStatusMessage (
							GAME_STRING (NETMELEE_STRING_BASE + 6));
							// "Connection for top player not "
							// "established."
			}
			else if (NetConnection_getState (conn) != NetState_inSetup)
			{
				// This side may be in the setup, but the network connection
				// is not in a state that setup information can be sent.
				netReady = false;
				if (player == 0)
					DrawMeleeStatusMessage (
							GAME_STRING (NETMELEE_STRING_BASE + 14));
							// "Connection for bottom player not ready."
				else
					DrawMeleeStatusMessage (
							GAME_STRING (NETMELEE_STRING_BASE + 15));
							// "Connection for top player not ready."

			}
		}
		if (!netReady)
		{
			PlayMenuSound (MENU_SOUND_FAILURE);
			return;
		}
		
		if (numPlayersReady () != NUM_PLAYERS)
			DrawMeleeStatusMessage (GAME_STRING (NETMELEE_STRING_BASE + 7));
					// "Waiting for remote confirmation."
		confirmConnections ();
	}
#endif
	
	pMS->InputFunc = DoConfirmSettings;
}

#ifdef NETPLAY

static BOOLEAN
DoConnectingDialog (MELEE_STATE *pMS)
{
	DWORD TimeIn = GetTimeCounter ();
	COUNT which_side = (pMS->MeleeOption == NET_TOP) ? 1 : 0;
	NetConnection *conn;

	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
	if (!pMS->Initialized)
	{
		RECT r;
		FONT oldfont;
		COLOR oldcolor;
		TEXT t;

		// Build a network connection.
		if (netConnections[which_side] != NULL)
			closePlayerNetworkConnection (which_side);

		pMS->Initialized = TRUE;
		conn = openPlayerNetworkConnection (which_side, pMS);
		pMS->InputFunc = DoConnectingDialog;

		/* Draw the dialog box here */
		LockMutex (GraphicsLock);
		oldfont = SetContextFont (StarConFont);
		oldcolor = SetContextForeGroundColor (BLACK_COLOR);
		BatchGraphics ();
		r.extent.width = 200;
		r.extent.height = 30;
		r.corner.x = (SCREEN_WIDTH - r.extent.width) >> 1;
		r.corner.y = (SCREEN_HEIGHT - r.extent.height) >> 1;
		DrawShadowedBox (&r, SHADOWBOX_BACKGROUND_COLOR, 
				SHADOWBOX_DARK_COLOR, SHADOWBOX_MEDIUM_COLOR);

		if (NetConnection_getPeerOptions (conn)->isServer)
		{
			t.pStr = GAME_STRING (NETMELEE_STRING_BASE + 1);
					/* "Awaiting incoming connection */
		}
		else
		{
			t.pStr = GAME_STRING (NETMELEE_STRING_BASE + 2);
					/* "Awaiting outgoing connection */
		}
		t.baseline.y = r.corner.y + 10;
		t.baseline.x = SCREEN_WIDTH >> 1;
		t.align = ALIGN_CENTER;
		t.CharCount = ~0;
		font_DrawText (&t);

		t.pStr = GAME_STRING (NETMELEE_STRING_BASE + 18);
				/* "Press SPACE to cancel" */
		t.baseline.y += 16;
		font_DrawText (&t);

		// Restore original graphics
		SetContextFont (oldfont);
		SetContextForeGroundColor (oldcolor);
		UnbatchGraphics ();
		UnlockMutex (GraphicsLock);
	}

	netInput ();

	if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		// Terminate a network connection.
		if (netConnections[which_side] != NULL) {
			closePlayerNetworkConnection (which_side);
			UpdateMeleeStatusMessage (which_side);
		}
		RedrawMeleeFrame ();
		pMS->InputFunc = DoMelee;
		pMS->LastInputTime = GetTimeCounter ();

		flushPacketQueues ();

		return TRUE;
	}

	conn = netConnections[which_side];
	if (conn != NULL)
	{
		NetState status = NetConnection_getState (conn);
		if ((status == NetState_init) ||
		    (status == NetState_inSetup))
		{
			/* Connection complete! */
			PlayerControl[which_side] = NETWORK_CONTROL | STANDARD_RATING;
			DrawControls (which_side, TRUE);

			RedrawMeleeFrame ();

			UpdateMeleeStatusMessage (which_side);
			pMS->InputFunc = DoMelee;
			pMS->LastInputTime = GetTimeCounter ();
			Deselect (pMS->MeleeOption);
			pMS->MeleeOption = START_MELEE;
		}
	}

	flushPacketQueues ();

	SleepThreadUntil (TimeIn + ONE_SECOND / 30);

	return TRUE;
}

/* Check for disconnections, and revert to human control if so */
static void
check_for_disconnections (MELEE_STATE *pMS)
{
	COUNT player;
	bool changed = FALSE;

	for (player = 0; player < NUM_PLAYERS; player++)
	{
		NetConnection *conn;

		if (!(PlayerControl[player] & NETWORK_CONTROL))
			continue;

		conn = netConnections[player];
		if (conn == NULL || !NetConnection_isConnected (conn))
		{
			PlayerControl[player] = HUMAN_CONTROL | STANDARD_RATING;
			DrawControls (player, FALSE);
			log_add (log_User, "Player %d has disconnected; shifting "
					"controls\n", player);
			changed = TRUE;
		}
	}

	(void) pMS;
}

#endif

static void
nextControlType (COUNT which_side)
{
	switch (PlayerControl[which_side])
	{
		case HUMAN_CONTROL | STANDARD_RATING:
			PlayerControl[which_side] =  COMPUTER_CONTROL | STANDARD_RATING;
			break;
		case COMPUTER_CONTROL | STANDARD_RATING:
			PlayerControl[which_side] =  COMPUTER_CONTROL | GOOD_RATING;
			break;
		case COMPUTER_CONTROL | GOOD_RATING:
			PlayerControl[which_side] =  COMPUTER_CONTROL | AWESOME_RATING;
			break;
		case COMPUTER_CONTROL | AWESOME_RATING:
			PlayerControl[which_side] =  HUMAN_CONTROL | STANDARD_RATING;
			break;

#ifdef NETPLAY
		case NETWORK_CONTROL | STANDARD_RATING:
			if (netConnections[which_side] != NULL)
				closePlayerNetworkConnection (which_side);
			UpdateMeleeStatusMessage (-1);
			PlayerControl[which_side] =  HUMAN_CONTROL | STANDARD_RATING;
			break;
#endif  /* NETPLAY */
		default:
			log_add (log_Error, "Error: Bad control type (%d) in "
					"nextControlType().\n", PlayerControl[which_side]);
			PlayerControl[which_side] =  HUMAN_CONTROL | STANDARD_RATING;
			break;
	}

	DrawControls (which_side, TRUE);
}

static MELEE_OPTIONS
MeleeOptionDown (MELEE_OPTIONS current) {
	if (current == QUIT_BOT)
		return QUIT_BOT;
	return current + 1;
}

static MELEE_OPTIONS
MeleeOptionUp (MELEE_OPTIONS current)
{
	if (current == TOP_ENTRY)
		return TOP_ENTRY;
	return current - 1;
}

static void
MeleeOptionSelect (MELEE_STATE *pMS)
{
	switch (pMS->MeleeOption)
	{
		case START_MELEE:
			StartMeleeButtonPressed (pMS);
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
			if (pMS->SideState[pMS->side].star_bucks)
				DoSaveTeam (pMS);
			else
				PlayMenuSound (MENU_SOUND_FAILURE);
			break;
		case QUIT_BOT:
			GLOBAL (CurrentActivity) |= CHECK_ABORT;
			break;
#ifdef NETPLAY
		case NET_TOP:
		case NET_BOT:
		{
			COUNT which_side;
			BOOLEAN confirmed;

			which_side = pMS->MeleeOption == NET_TOP ? 1 : 0;
			confirmed = MeleeConnectDialog (which_side);
			RedrawMeleeFrame ();
			pMS->LastInputTime = GetTimeCounter ();
			if (confirmed)
			{
				pMS->Initialized = FALSE;
				pMS->InputFunc = DoConnectingDialog;
			}
			break;
		}
#endif  /* NETPLAY */
		case CONTROLS_TOP:
		case CONTROLS_BOT:
		{
			COUNT which_side = (pMS->MeleeOption == CONTROLS_TOP) ? 1 : 0;
			nextControlType (which_side);
			break;
		}
	}
}

BOOLEAN
DoMelee (MELEE_STATE *pMS)
{
	DWORD TimeIn = GetTimeCounter ();
	BOOLEAN force_select = FALSE;

	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	if (!pMS->Initialized)
	{
		if (pMS->hMusic)
		{
			StopMusic ();
			DestroyMusic (pMS->hMusic);
			pMS->hMusic = 0;
		}
		pMS->hMusic = LoadMusic (MELEE_MUSIC);
		pMS->Initialized = TRUE;
		
		pMS->MeleeOption = START_MELEE;
		PlayMusic (pMS->hMusic, TRUE, 1);
		LockMutex (GraphicsLock);
		InitMelee (pMS);
		UnlockMutex (GraphicsLock);
		{
			BYTE clut_buf[] = {FadeAllToColor};
				
			XFormColorMap ((COLORMAPPTR)clut_buf, ONE_SECOND / 2);
		}
		pMS->LastInputTime = GetTimeCounter ();
		return TRUE;
	}

#ifdef NETPLAY
	netInput ();
#endif
	
	if (PulsedInputState.menu[KEY_MENU_CANCEL] ||
			PulsedInputState.menu[KEY_MENU_LEFT])
	{
		// Start editing the teams.
		LockMutex (GraphicsLock);
		pMS->LastInputTime = GetTimeCounter ();
		Deselect (pMS->MeleeOption);
		UnlockMutex (GraphicsLock);
		pMS->MeleeOption = EDIT_MELEE;
		pMS->Initialized = FALSE;
		if (PulsedInputState.menu[KEY_MENU_CANCEL])
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
		if (PulsedInputState.menu[KEY_MENU_UP])
		{
			pMS->LastInputTime = GetTimeCounter ();
			NewMeleeOption = MeleeOptionUp (pMS->MeleeOption);
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			pMS->LastInputTime = GetTimeCounter ();
			NewMeleeOption = MeleeOptionDown (pMS->MeleeOption);
		}

		if ((PlayerControl[0] & PlayerControl[1] & PSYTRON_CONTROL)
				&& GetTimeCounter () - pMS->LastInputTime > ONE_SECOND * 10)
		{
			force_select = TRUE;
			NewMeleeOption = START_MELEE;
		}

		if (NewMeleeOption != pMS->MeleeOption)
		{
#ifdef NETPLAY
			if (pMS->MeleeOption == CONTROLS_TOP ||
					pMS->MeleeOption == CONTROLS_BOT)
				UpdateMeleeStatusMessage (-1);
#endif
			LockMutex (GraphicsLock);
			Deselect (pMS->MeleeOption);
			pMS->MeleeOption = NewMeleeOption;
			Select (pMS->MeleeOption);
			UnlockMutex (GraphicsLock);
#ifdef NETPLAY
			if (NewMeleeOption == CONTROLS_TOP ||
					NewMeleeOption == CONTROLS_BOT)
			{
				COUNT side = (NewMeleeOption == CONTROLS_TOP) ? 1 : 0;
				if (PlayerControl[side] & NETWORK_CONTROL)
					UpdateMeleeStatusMessage (side);
				else
					UpdateMeleeStatusMessage (-1);
			}
#endif
		}

		if (PulsedInputState.menu[KEY_MENU_SELECT] || force_select)
		{
			MeleeOptionSelect (pMS);
			if (GLOBAL (CurrentActivity) & CHECK_ABORT)
				return (FALSE);
		}
	}

#ifdef NETPLAY
	flushPacketQueues ();

	check_for_disconnections (pMS);
#endif

	flashSelection (pMS);

	SleepThreadUntil (TimeIn + ONE_SECOND / 30);

	return (TRUE);
}

int
LoadMeleeConfig (MELEE_STATE *pMS)
{
	uio_Stream *load_fp;
	int status;
	COUNT side;

	load_fp = res_OpenResFile (configDir, "melee.cfg", "rb");
	if (!load_fp)
		goto err;

	if (LengthResFile (load_fp) != (1 + sizeof (TEAM_IMAGE)) * NUM_SIDES)
		goto err;

	for (side = 0; side < NUM_SIDES; side++)
	{
		status = GetResFileChar (load_fp);
		if (status == -1)
			goto err;
		PlayerControl[side] = (BYTE)status;

		if (!ReadTeamImage (&pMS->SideState[side].TeamImage, load_fp))
			goto err;
	
		/* Do not allow netplay mode at the start. */
		if (PlayerControl[side] & NETWORK_CONTROL)
			PlayerControl[side] = HUMAN_CONTROL | STANDARD_RATING;
	}

	res_CloseResFile (load_fp);
	return 0;

err:
	if (load_fp)
		res_CloseResFile (load_fp);
	return -1;
}

int
WriteMeleeConfig (MELEE_STATE *pMS)
{
	uio_Stream *save_fp;
	COUNT side;

	save_fp = res_OpenResFile (configDir, "melee.cfg", "wb");
	if (!save_fp)
		goto err;

	for (side = 0; side < NUM_SIDES; side++)
	{
		if (PutResFileChar (PlayerControl[side], save_fp) == -1)
			goto err;

		if (WriteTeamImage (&pMS->SideState[side].TeamImage, save_fp) == 0)
			goto err;
	}

	if (!res_CloseResFile (save_fp))
		goto err;

	return 0;

err:
	if (save_fp)
	{
		res_CloseResFile (save_fp);
		DeleteResFile (configDir, "melee.cfg");
	}
	return -1;
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

		MenuState.randomContext = RandomContext_New ();
		RandomContext_SeedRandom (MenuState.randomContext,
				GetTimeCounter ());
				// Using the current time still leave the random state a bit
				// predictable, but it is good enough.

#ifdef NETPLAY
		{
			COUNT player;
			for (player = 0; player < NUM_PLAYERS; player++)
				netConnections[player] = NULL;
		}
#endif
		
		MenuState.CurIndex = (COUNT)~0;
		InitMeleeLoadState (&MenuState);

		GLOBAL (CurrentActivity) = SUPER_MELEE;

		GameSounds = CaptureSound (LoadSound (GAME_SOUNDS));
		LoadMeleeInfo (&MenuState);
		if (LoadMeleeConfig (&MenuState) == -1)
		{
			PlayerControl[0] = HUMAN_CONTROL | STANDARD_RATING;
			MenuState.SideState[0].TeamImage = MenuState.load.preBuiltList[0];
			PlayerControl[1] = COMPUTER_CONTROL | STANDARD_RATING;
			MenuState.SideState[1].TeamImage = MenuState.load.preBuiltList[1];
		}
		teamStringChanged (&MenuState, 0);
		teamStringChanged (&MenuState, 1);
		entireFleetChanged (&MenuState, 0);
		entireFleetChanged (&MenuState, 1);

		MenuState.side = 0;
		MenuState.SideState[0].star_bucks =
				GetTeamValue (&MenuState.SideState[0].TeamImage);
		MenuState.SideState[1].star_bucks =
				GetTeamValue (&MenuState.SideState[1].TeamImage);
		SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
		DoInput (&MenuState, TRUE);

		StopMusic ();
		WaitForSoundEnd (TFBSOUND_WAIT_ALL);

		WriteMeleeConfig (&MenuState);
		FreeMeleeInfo (&MenuState);
		DestroySound (ReleaseSound (GameSounds));
		GameSounds = 0;

		DestroyDrawable (ReleaseDrawable (PickMeleeFrame));
		PickMeleeFrame = 0;

		UninitMeleeLoadState (&MenuState);

		RandomContext_Delete (MenuState.randomContext);

		FlushInput ();
	}
}

// Notify the network connections of a team name change.
void
teamStringChanged (MELEE_STATE *pMS, int player)
{
#ifdef NETPLAY
	const char *name;
	size_t len;
	size_t playerI;

	name = pMS->SideState[player].TeamImage.TeamName;
	len = strlen (name);
	for (playerI = 0; playerI < NUM_PLAYERS; playerI++)
	{
		NetConnection *conn = netConnections[playerI];

		if (conn == NULL)
			continue;

		if (!NetConnection_isConnected (conn))
			continue;

		if (NetConnection_getState (conn) != NetState_inSetup)
			continue;

		Netplay_teamStringChanged (conn, player, name, len);
	}
#else
	(void) pMS;
	(void) player;
#endif
}

// Notify the network connections of the configuration of a fleet.
void
entireFleetChanged (MELEE_STATE *pMS, int player)
{
#ifdef NETPLAY
	size_t playerI;
	for (playerI = 0; playerI < NUM_PLAYERS; playerI++) {
		NetConnection *conn = netConnections[playerI];

		if (conn == NULL)
			continue;

		if (!NetConnection_isConnected (conn))
			continue;

		if (NetConnection_getState (conn) != NetState_inSetup)
			continue;

		Netplay_entireFleetChanged (conn, player,
				pMS->SideState[player].TeamImage.ShipList, MELEE_FLEET_SIZE);
	}
#else
	(void) player;
#endif
	(void) pMS;
}

// Notify the network of a change in the configuration of a fleet.
void
fleetShipChanged (MELEE_STATE *pMS, int player, size_t index)
{
#ifdef NETPLAY
	size_t playerI;
	for (playerI = 0; playerI < NUM_PLAYERS; playerI++)
	{
		NetConnection *conn = netConnections[playerI];

		if (conn == NULL)
			continue;

		if (!NetConnection_isConnected (conn))
			continue;

		if (NetConnection_getState (conn) != NetState_inSetup)
			continue;

		Netplay_fleetShipChanged (conn, player, index,
				pMS->SideState[player].TeamImage.ShipList[index]);
	}
#else
	(void) player;
	(void) index;
#endif
	(void) pMS;
}

#ifdef NETPLAY
// NB: 'len' does not include the terminating 0.
//     'len' counts in bytes, not in characters.
void
updateTeamName (MELEE_STATE *pMS, COUNT side, const char *name,
		size_t len)
{
	// NB: MAX_TEAM_CHARS is the maximum number of characters,
	//     without the terminating '\0'.
	if (len > MAX_TEAM_CHARS)
		len = MAX_TEAM_CHARS;

	// TeamName has space for at least MAX_TEAM_CHARS + 1 bytes.
	strncpy (pMS->SideState[side].TeamImage.TeamName, name, len);
	pMS->SideState[side].TeamImage.TeamName[len] = '\0';
	
	LockMutex (GraphicsLock);
#if 0  /* DTSHS_REPAIR does not combine with other options */
	if (pMS->MeleeOption == EDIT_MELEE && pMS->side == side
			&& pMS->row == NUM_MELEE_ROWS)
		DrawTeamString (pMS, side, DTSHS_REPAIR | DTSHS_SELECTED);
	else
#endif
		DrawTeamString (pMS, side, DTSHS_REPAIR);
	UnlockMutex (GraphicsLock);
}

// Update a ship in a fleet as specified by a remote party.
bool
updateFleetShip (MELEE_STATE *pMS, COUNT side, COUNT index, BYTE ship)
{
	BYTE row = GetShipRow (index);
	BYTE col = GetShipColumn (index);
	COUNT val;
	
	FleetShipIndex selectedShipIndex;
	BOOLEAN isSelected;

	if (ship >= NUM_MELEE_SHIPS && ship != MELEE_NONE) {
		log_add (log_Warning, "Invalid ship type number %d (max = %d).\n",
				ship, NUM_MELEE_SHIPS - 1);
		return false;
	}
	
	if (index >= MELEE_FLEET_SIZE)
	{
		log_add (log_Warning, "Invalid ship position number %d (max = %d).\n",
				index, MELEE_FLEET_SIZE - 1);
		return false;
	}
	
	val = GetShipValue (ship);
	if (val == (COUNT) ~0)
		return false;

	pMS->SideState[side].star_bucks -=
			GetShipValue (pMS->SideState[side].TeamImage.ShipList[index]);
	pMS->SideState[side].star_bucks += val;

	pMS->SideState[side].TeamImage.ShipList[index] = ship;

	selectedShipIndex = GetShipIndex (pMS->row, pMS->col);
	isSelected = (pMS->MeleeOption == EDIT_MELEE) &&
			(pMS->side == side) && (index == selectedShipIndex);
			// Ship to be updated is the currently selected one.

	LockMutex (GraphicsLock);
	if (ship == MELEE_NONE)
	{
		RECT r;
		GetShipBox (&r, side, row, col);
		RepairMeleeFrame (&r);
	} else
		DrawShipBox (side, row, col, ship, isSelected);

	if (isSelected)
	{
		pMS->CurIndex = ship;
		DrawMeleeShipStrings (pMS, ship);
	}
	
	// Reprint the team value:
	//DrawTeamString (pMeleeState, side, DTSHS_NORMAL);
	DrawTeamString (pMS, side, DTSHS_REPAIR);
	UnlockMutex (GraphicsLock);

	return true;
}

void
updateRandomSeed (MELEE_STATE *pMS, COUNT side, DWORD seed)
{
	TFB_SeedRandom (seed);
	(void) pMS;
	(void) side;
}

// The remote player has done something which invalidates our confirmation.
void
confirmationCancelled (MELEE_STATE *pMS, COUNT side)
{
	LockMutex (GraphicsLock);
	if (side == 0)
		DrawMeleeStatusMessage (GAME_STRING (NETMELEE_STRING_BASE + 16));
				// "Bottom player changed something -- need to reconfirm."
	else
		DrawMeleeStatusMessage (GAME_STRING (NETMELEE_STRING_BASE + 17));
				// "Top player changed something -- need to reconfirm."
	UnlockMutex (GraphicsLock);

	if (pMS->InputFunc == DoConfirmSettings)
		pMS->InputFunc = DoMelee;
}

static void
connectionFeedback (NetConnection *conn, const char *str, bool forcePopup) {
	struct battlestate_struct *bs = NetMelee_getBattleState (conn);

	if (bs == NULL && !forcePopup)
	{
		// bs == NULL means the game has not started yet.
		LockMutex (GraphicsLock);
		DrawMeleeStatusMessage (str);
		UnlockMutex (GraphicsLock);
	}
	else
	{
		DoPopupWindow (str);
	}
}

void
connectedFeedback (NetConnection *conn) {
	if (NetConnection_getPlayerNr (conn) == 0)
		connectionFeedback (conn, GAME_STRING (NETMELEE_STRING_BASE + 8),
				false);
				// "Bottom player is connected."
	else
		connectionFeedback (conn, GAME_STRING (NETMELEE_STRING_BASE + 9),
				false);
				// "Top player is connected."

	PlayMenuSound (MENU_SOUND_INVOKED);
}

const char *
abortReasonString (NetplayAbortReason reason)
{
	switch (reason)
	{
		case AbortReason_unspecified:
			return GAME_STRING (NETMELEE_STRING_BASE + 25);
					// "Disconnect for an unspecified reason.'
		case AbortReason_versionMismatch:
			return GAME_STRING (NETMELEE_STRING_BASE + 26);
					// "Connection aborted due to version mismatch."
		case AbortReason_invalidHash:
			return GAME_STRING (NETMELEE_STRING_BASE + 27);
					// "Connection aborted because the remote side sent a "
					// "fake signature."
		case AbortReason_protocolError:
			return GAME_STRING (NETMELEE_STRING_BASE + 28);
					// "Connection aborted due to an internal protocol "
					// "error."
	}
	
	return NULL;
			// Should not happen.
}

void
abortFeedback (NetConnection *conn, NetplayAbortReason reason)
{
	const char *msg;

	msg = abortReasonString (reason);
	if (msg != NULL)
		connectionFeedback (conn, msg, true);
}

const char *
resetReasonString (NetplayResetReason reason)
{
	switch (reason)
	{
		case ResetReason_unspecified:
			return GAME_STRING (NETMELEE_STRING_BASE + 29);
					// "Game aborted for an unspecified reason."
		case ResetReason_syncLoss:
			return GAME_STRING (NETMELEE_STRING_BASE + 30);
					// "Game aborted due to loss of synchronisation."
		case ResetReason_manualReset:
			return GAME_STRING (NETMELEE_STRING_BASE + 31);
					// "Game aborted by the remote player."
	}

	return NULL;
			// Should not happen.
}

void
resetFeedback (NetConnection *conn, NetplayResetReason reason,
		bool byRemote)
{
	const char *msg;

	GLOBAL (CurrentActivity) |= CHECK_ABORT;
	flushPacketQueues ();
			// If the local side queued a reset packet as a result of a
			// remote reset, that packet will not have been sent yet.
			// We flush the queue now, so that the remote side won't be
			// waiting for the reset packet while this side is waiting
			// for an acknowledgement of the feedback message.
	
	if (reason == ResetReason_manualReset && !byRemote) {
		// No message needed, the player initiated the reset.
		return;
	}

	msg = resetReasonString (reason);
	if (msg != NULL)
		connectionFeedback (conn, msg, false);
}

void
errorFeedback (NetConnection *conn)
{
	if (NetConnection_getPlayerNr (conn) == 0)
		connectionFeedback (conn, GAME_STRING (NETMELEE_STRING_BASE + 10),
				false);
				// "Bottom player: connection failed."
	else
		connectionFeedback (conn, GAME_STRING (NETMELEE_STRING_BASE + 11),
				false);
				// "Top player: connection failed."
}

void
closeFeedback (NetConnection *conn)
{
	if (NetConnection_getPlayerNr (conn) == 0)
		connectionFeedback (conn, GAME_STRING (NETMELEE_STRING_BASE + 12),
				false);
				// "Bottom player: connection closed."
	else
		connectionFeedback (conn, GAME_STRING (NETMELEE_STRING_BASE + 13),
				false);
				// "Top player: connection closed."
}

#endif  /* NETPLAY */

