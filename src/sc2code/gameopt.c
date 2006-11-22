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

#include "gameopt.h"

#include "build.h"
#include "colors.h"
#include "commglue.h"
#include "controls.h"
#include "encount.h"
#include "planets/lander.h"
#include "gamestr.h"
#include "inplib.h"
#include "load.h"
#include "options.h"
#include "save.h"
#include "starbase.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "state.h"
#include "util.h"
#include "libs/graphics/gfx_common.h"

#include <ctype.h>


#define MAX_SAVED_GAMES 50
#define SUMMARY_X_OFFS 14
#define SUMMARY_SIDE_OFFS 7
#define SAVES_PER_PAGE 5

static BOOLEAN DoSettings (PMENU_STATE pMS);
static BOOLEAN DoNaming (PMENU_STATE pMS);

static PMENU_STATE pLocMenuState;
static BYTE prev_save; //keeps track of the last slot that was saved or loaded


void
ConfirmSaveLoad (STAMP *MsgStamp)
{
	RECT r, clip_r;
	TEXT t;

	SetContextFont (StarConFont);
	GetContextClipRect (&clip_r);
	if (clip_r.extent.width == 0)
	{
		clip_r.corner.x = clip_r.corner.y = 0;
		clip_r.extent.width = SCREEN_WIDTH;
		clip_r.extent.height = SCREEN_HEIGHT;
	}

	t.baseline.x = clip_r.extent.width >> 1;
	t.baseline.y = (clip_r.extent.height >> 1) + 3;
	t.align = ALIGN_CENTER;
	t.CharCount = (COUNT)~0;
	if (MsgStamp)
		t.pStr = GAME_STRING (SAVEGAME_STRING_BASE + 0);
	else
		t.pStr = GAME_STRING (SAVEGAME_STRING_BASE + 1);
	TextRect (&t, &r, NULL_PTR);
	r.corner.x -= 4;
	r.corner.y -= 4;
	r.extent.width += 8;
	r.extent.height += 8;
	if (MsgStamp)
	{
		MsgStamp->origin = r.corner;
		r.corner.x += clip_r.corner.x;
		r.corner.y += clip_r.corner.y;
		MsgStamp->frame = CaptureDrawable (LoadDisplayPixmap (&r, (FRAME)0));
		r.corner.x -= clip_r.corner.x;
		r.corner.y -= clip_r.corner.y;
	}
	DrawStarConBox (&r, 2,
			BUILD_COLOR (MAKE_RGB15 (0x10, 0x10, 0x10), 0x19),
			BUILD_COLOR (MAKE_RGB15 (0x08, 0x08, 0x08), 0x1F),
			TRUE, BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08));
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x14, 0x14, 0x14), 0x0F));
	font_DrawText (&t);
}

enum
{
	SAVE_GAME = 0,
	LOAD_GAME,
	QUIT_GAME,
	SETTINGS
};

static BOOLEAN DoGameOptions (PMENU_STATE pMS);

enum
{
	SOUND_ON_SETTING,
	SOUND_OFF_SETTING,
	MUSIC_ON_SETTING,
	MUSIC_OFF_SETTING,
	CYBORG_OFF_SETTING,
	CYBORG_NORMAL_SETTING,
	CYBORG_DOUBLE_SETTING,
	CYBORG_SUPER_SETTING,
	CHANGE_CAPTAIN_SETTING,
	CHANGE_SHIP_SETTING,
	EXIT_MENU_SETTING
};

enum
{
	NO_QUIT_MENU,
	YES_QUIT_MENU
};

static void
FeedbackSetting (BYTE which_setting)
{
	UNICODE buf[128];
	const char *tmpstr;

	buf[0] = '\0';
	// pre-terminate buffer in case snprintf() overflows
	buf[sizeof (buf) - 1] = '\0';

	switch (which_setting)
	{
		case SOUND_ON_SETTING:
		case SOUND_OFF_SETTING:
			snprintf (buf, sizeof (buf) - 1, "%s %s",
					GAME_STRING (OPTION_STRING_BASE + 0),
					GLOBAL (glob_flags) & SOUND_DISABLED
					? GAME_STRING (OPTION_STRING_BASE + 3) :
					GAME_STRING (OPTION_STRING_BASE + 4));
			break;
		case MUSIC_ON_SETTING:
		case MUSIC_OFF_SETTING:
			snprintf (buf, sizeof (buf) - 1, "%s %s",
					GAME_STRING (OPTION_STRING_BASE + 1),
					GLOBAL (glob_flags) & MUSIC_DISABLED
					? GAME_STRING (OPTION_STRING_BASE + 3) :
					GAME_STRING (OPTION_STRING_BASE + 4));
			break;
		case CYBORG_OFF_SETTING:
		case CYBORG_NORMAL_SETTING:
		case CYBORG_DOUBLE_SETTING:
		case CYBORG_SUPER_SETTING:
			if (optWhichMenu == OPT_PC &&
					which_setting > CYBORG_NORMAL_SETTING)
			{
				if (which_setting == CYBORG_DOUBLE_SETTING)
					tmpstr = "+";
				else
					tmpstr = "++";
			}
			else
				tmpstr = "";
			snprintf (buf, sizeof (buf) - 1, "%s %s%s",
					GAME_STRING (OPTION_STRING_BASE + 2),
					!(GLOBAL (glob_flags) & CYBORG_ENABLED)
					? GAME_STRING (OPTION_STRING_BASE + 3) :
					GAME_STRING (OPTION_STRING_BASE + 4),
					tmpstr);
			break;
		case CHANGE_CAPTAIN_SETTING:
		case CHANGE_SHIP_SETTING:
			utf8StringCopy (buf, sizeof (buf),
					GAME_STRING (NAMING_STRING_BASE + 0));
			break;
	}

	LockMutex (GraphicsLock);
	DrawStatusMessage (buf);
	UnlockMutex (GraphicsLock);
}

static void
FeedbackQuit (BYTE which_setting)
{
	const UNICODE *buf = "";

	switch (which_setting)
	{
		case NO_QUIT_MENU:
			buf = GAME_STRING (QUITMENU_STRING_BASE + 3); // "Don't Quit"
			break;
		case YES_QUIT_MENU:
			buf = GAME_STRING (QUITMENU_STRING_BASE + 4); // "Quit Game"
			break;
	}

	LockMutex (GraphicsLock);
	DrawStatusMessage (buf);
	UnlockMutex (GraphicsLock);
}

#define DDSHS_NORMAL   0
#define DDSHS_EDIT     1
#define DDSHS_BLOCKCUR 2

static BOOLEAN
DrawDescriptionString (PMENU_STATE pMS, COUNT which_string, COUNT state)
{
	COUNT rel_index;
	RECT r;
	TEXT lf;
	COLOR BackGround, ForeGround;
	FONT Font;

	LockMutex (GraphicsLock);

	rel_index = (COUNT)(which_string - pMS->first_item.y);

	{
		r.corner.x = 2;
		r.extent.width = SHIP_NAME_WIDTH;
		r.extent.height = SHIP_NAME_HEIGHT;

		SetContext (StatusContext);
		if (pMS->CurState == CHANGE_CAPTAIN_SETTING)
		{
			Font = TinyFont;
			r.corner.y = 10;
			++r.corner.x;
			r.extent.width -= 2;
			lf.baseline.x = r.corner.x + (r.extent.width >> 1) - 1;

			BackGround = BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09);
			ForeGround = BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x1F), 0x0B);
		}
		else
		{
			Font = StarConFont;
			r.corner.y = 20;
			lf.baseline.x = r.corner.x + (r.extent.width >> 1);

			BackGround = BUILD_COLOR (MAKE_RGB15 (0x0F, 0x00, 0x00), 0x2D);
			ForeGround = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x00), 0x7D);
		}

		lf.baseline.y = r.corner.y + r.extent.height - 1;
		lf.align = ALIGN_CENTER;
	}

	SetContextFont (Font);
	lf.pStr = ((GAME_DESC *)pMS->Extra)[rel_index];
	lf.CharCount = (COUNT)~0;

	if (!(state & DDSHS_EDIT))
	{	// normal state
		SetFlashRect (SFR_MENU_3DO, (FRAME)0);

		if (pMS->InputFunc == DoNaming)
		{
			if (pMS->CurState == CHANGE_CAPTAIN_SETTING)
				DrawCaptainsName ();
			else
			{
				DrawFlagshipName (TRUE);
				if (pMenuState && pMenuState->InputFunc == DoOutfit)
					DrawFlagshipName (FALSE);
			}
		}
		else
		{	// XXX: remnants of DOS? this function is never actually
			//   called outside DoNaming nowadays, which makes this
			//   code utterly dead
			if (state == 0)
			{
				COLOR OldColor;

				OldColor = SetContextForeGroundColor (BLACK_COLOR);
				DrawFilledRectangle (&r);
				SetContextForeGroundColor (OldColor);
			}
			font_DrawText (&lf);
		}
	}
	else
	{	// editing state
		COUNT i;
		RECT text_r;
		BYTE char_deltas[MAX_DESC_CHARS];
		PBYTE pchar_deltas;

		TextRect (&lf, &text_r, char_deltas);
		if ((text_r.extent.width + 2) >= r.extent.width)
		{	// the text does not fit the input box size and so
			// will not fit when displayed later
			UnlockMutex (GraphicsLock);
			// disallow the change
			return (FALSE);
		}

		SetContextForeGroundColor (BackGround);
		DrawFilledRectangle (&r);

		pchar_deltas = char_deltas;
		for (i = pMS->first_item.x; i > 0; --i)
			text_r.corner.x += (SIZE)*pchar_deltas++;
		if ((COUNT)pMS->first_item.x < lf.CharCount) /* end of line */
			--text_r.corner.x;
		
		if (state & DDSHS_BLOCKCUR)
		{	// Use block cursor for keyboardless systems
			if ((COUNT)pMS->first_item.x == lf.CharCount)
			{	// cursor at end-line -- use insertion point
				text_r.extent.width = 1;
			}
			else if ((COUNT)pMS->first_item.x + 1 == lf.CharCount)
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
		
		text_r.corner.y = r.corner.y;
		text_r.extent.height = r.extent.height;
		SetContextForeGroundColor (BLACK_COLOR);
		DrawFilledRectangle (&text_r);

		SetContextForeGroundColor (ForeGround);
		font_DrawText (&lf);

		SetFlashRect (&r, (FRAME)0);
	}

	UnlockMutex (GraphicsLock);
	return (TRUE);
}

static BOOLEAN
OnNameChange (PTEXTENTRY_STATE pTES)
{
	PMENU_STATE pMS = (PMENU_STATE) pTES->CbParam;
	COUNT hl = DDSHS_EDIT;

	pMS->first_item.x = pTES->CursorPos;
	if (pTES->JoystickMode)
		hl |= DDSHS_BLOCKCUR;

	return DrawDescriptionString (pMS, 0, hl);
}

static BOOLEAN
DoNaming (PMENU_STATE pMS)
{
	GAME_DESC buf;
	TEXTENTRY_STATE tes;
	UNICODE *Setting;

	pMS->Initialized = TRUE;
	pMS->InputFunc = DoNaming;

	buf[0] = '\0';
	// XXX: this code abuses MENU_STATE struct members to store
	//  some values:
	//   first_item.x - current cursor position
	//   first_item.y - must be set to 0; DrawDescriptionString()
	//     treats it as base index into array of strings
	//     supplied in Extra
	pMS->first_item.x = 0;
	pMS->first_item.y = 0;
	pMS->Extra = buf;
	DrawDescriptionString (pMS, 0, DDSHS_EDIT);

	LockMutex (GraphicsLock);
	DrawStatusMessage (GAME_STRING (NAMING_STRING_BASE + 0));
	UnlockMutex (GraphicsLock);

	if (pMS->CurState == CHANGE_CAPTAIN_SETTING)
	{
		Setting = GLOBAL_SIS (CommanderName);
		tes.MaxSize = sizeof (GLOBAL_SIS (CommanderName));
	}
	else
	{
		Setting = GLOBAL_SIS (ShipName);
		tes.MaxSize = sizeof (GLOBAL_SIS (ShipName));
	}

	// text entry setup
	tes.Initialized = FALSE;
	tes.MenuRepeatDelay = 0;
	tes.BaseStr = buf;
	tes.CursorPos = 0;
	tes.CbParam = pMS;
	tes.ChangeCallback = OnNameChange;
	tes.FrameCallback = 0;

	if (DoTextEntry (&tes))
		utf8StringCopy (Setting, tes.MaxSize, buf);
	else
		utf8StringCopy (buf, sizeof (buf), Setting);
	DrawDescriptionString (pMS, 0, DDSHS_NORMAL);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	pMS->InputFunc = DoSettings;
	if (!(GLOBAL (CurrentActivity) & CHECK_ABORT))
		FeedbackSetting (pMS->CurState);

	return (TRUE);
}

static BOOLEAN
DoSettings (PMENU_STATE pMS)
{
	BYTE cur_speed;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	cur_speed = (BYTE)(GLOBAL (glob_flags) & COMBAT_SPEED_MASK) >> COMBAT_SPEED_SHIFT;
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	if (!pMS->Initialized)
	{
		DrawMenuStateStrings (PM_SOUND_ON, pMS->CurState);
		FeedbackSetting (pMS->CurState);
		pMS->Initialized = TRUE;
		pMS->InputFunc = DoSettings;
	}
	else if (PulsedInputState.menu[KEY_MENU_CANCEL]
			|| (PulsedInputState.menu[KEY_MENU_SELECT]
			&& pMS->CurState == EXIT_MENU_SETTING))
	{
		LockMutex (GraphicsLock);
		DrawStatusMessage (NULL_PTR);
		UnlockMutex (GraphicsLock);

		pMS->CurState = SETTINGS;
		pMS->InputFunc = DoGameOptions;
		pMS->Initialized = 0;
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		switch (pMS->CurState)
		{
			case SOUND_ON_SETTING:
			case SOUND_OFF_SETTING:
				ToggleSoundEffect ();
				pMS->CurState ^= 1;
				DrawMenuStateStrings (PM_SOUND_ON, pMS->CurState);
				break;
			case MUSIC_ON_SETTING:
			case MUSIC_OFF_SETTING:
				ToggleMusic ();
				pMS->CurState ^= 1;
				DrawMenuStateStrings (PM_SOUND_ON, pMS->CurState);
				break;
			case CHANGE_CAPTAIN_SETTING:
			case CHANGE_SHIP_SETTING:
				pMS->Initialized = FALSE;
				pMS->InputFunc = DoNaming;
				return (TRUE);
			default:
				if (cur_speed++ < NUM_COMBAT_SPEEDS - 1)
					GLOBAL (glob_flags) |= CYBORG_ENABLED;
				else
				{
					cur_speed = 0;
					GLOBAL (glob_flags) &= ~CYBORG_ENABLED;
				}
				GLOBAL (glob_flags) =
						(BYTE)((GLOBAL (glob_flags) & ~COMBAT_SPEED_MASK)
						| (cur_speed << COMBAT_SPEED_SHIFT));
				pMS->CurState = CYBORG_OFF_SETTING + cur_speed;
				DrawMenuStateStrings (PM_SOUND_ON, pMS->CurState);
		}

		FeedbackSetting (pMS->CurState);
	}
	else if (DoMenuChooser (pMS, PM_SOUND_ON))
		FeedbackSetting (pMS->CurState);

	return (TRUE);
}

static BOOLEAN
DoQuitMenu (PMENU_STATE pMS)
{
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	if (!pMS->Initialized)
	{
		DrawMenuStateStrings (PM_NO_QUIT, pMS->CurState);
		FeedbackQuit (pMS->CurState);
		pMS->Initialized = TRUE;
		pMS->InputFunc = DoQuitMenu;
	}
	else if (PulsedInputState.menu[KEY_MENU_CANCEL]
			|| (PulsedInputState.menu[KEY_MENU_SELECT]
			&& pMS->CurState == NO_QUIT_MENU))
	{
		LockMutex (GraphicsLock);
		DrawStatusMessage (NULL_PTR);
		UnlockMutex (GraphicsLock);

		pMS->CurState = QUIT_GAME;
		pMS->InputFunc = DoGameOptions;
		pMS->Initialized = 0;
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		switch (pMS->CurState)
		{
			case NO_QUIT_MENU:
				break;
			case YES_QUIT_MENU:
				GLOBAL (CurrentActivity) |= CHECK_ABORT;
				ExitRequested = FALSE;
				break;
		}
		FeedbackQuit (pMS->CurState);
	}
	else if (DoMenuChooser (pMS, PM_NO_QUIT))
		FeedbackQuit (pMS->CurState);

	return (TRUE);
}

static void
DrawCargo (COUNT redraw_state)
{
	BYTE i;
	RECT r;

	SetContext (SpaceContext);
	if (redraw_state)
	{
		STAMP s;

		if (redraw_state == 2)
		{
			SetContextForeGroundColor (BLACK_COLOR);
			r.corner.x = 1 + SUMMARY_X_OFFS;
			r.corner.y = 12;
			r.extent.width = ((SIS_SCREEN_WIDTH - STATUS_WIDTH) >> 1) - r.corner.x;
			r.extent.height = 62 - r.corner.y;
			DrawFilledRectangle (&r);
			GetFrameRect (SetRelFrameIndex (
					pLocMenuState->ModuleFrame, 1), &r);
			r.extent.width += SUMMARY_X_OFFS + SUMMARY_SIDE_OFFS;
			DrawFilledRectangle (&r);
		}
		else
		{
			s.origin.x = 0;
			s.origin.y = 0;
			s.frame = SetAbsFrameIndex (pLocMenuState->ModuleFrame,
					GetFrameCount (pLocMenuState->ModuleFrame) - 1);
			if (!pLocMenuState->Initialized)
			{
				DrawStamp (&s);
				s.origin.x = SUMMARY_X_OFFS + 1;
				s.frame = DecFrameIndex (s.frame);
				if (pLocMenuState->delta_item == SAVE_GAME)
					s.frame = DecFrameIndex (s.frame);
				DrawStamp (&s);
				if (((SUMMARY_DESC *)pLocMenuState->Extra)
						[pLocMenuState->CurState].year_index == 0)
					return;
			}
			else
			{
				GetContextClipRect (&r);
				r.extent.height = 136;
				SetContextClipRect (&r);
				DrawStamp (&s);
				r.extent.height = SIS_SCREEN_HEIGHT;
				SetContextClipRect (&r);
			}
		}

		s.frame = SetAbsFrameIndex (
				MiscDataFrame,
				(NUM_SCANDOT_TRANSITIONS << 1) + 3
				);
		if (redraw_state == 2
				|| (redraw_state == 1
				/*&& !(((SUMMARY_DESC *)pLocMenuState->Extra)
				[pLocMenuState->CurState].Flags & AFTER_BOMB_INSTALLED)*/))
		{
			s.origin.x = 7 + SUMMARY_X_OFFS - SUMMARY_SIDE_OFFS + 3;
			s.origin.y = 17;
			for (i = 0; i < NUM_ELEMENT_CATEGORIES; ++i)
			{
				if (i == NUM_ELEMENT_CATEGORIES >> 1)
				{
					s.origin.x += 36;
					s.origin.y = 17;
				}
				DrawStamp (&s);
				s.frame = SetRelFrameIndex (s.frame, 5);
				s.origin.y += 12;
			}
		}
		s.origin.x = 24 + SUMMARY_X_OFFS - SUMMARY_SIDE_OFFS;
		s.origin.y = 68;
		s.frame = SetAbsFrameIndex (s.frame, 68);
		DrawStamp (&s);
	}
	else
	{
		TEXT t;
		UNICODE buf[40];
		static const COLOR cargo_color[] =
		{
			BUILD_COLOR (MAKE_RGB15 (0x02, 0x0E, 0x13), 0x00),
			BUILD_COLOR (MAKE_RGB15 (0x19, 0x00, 0x00), 0x00),
			BUILD_COLOR (MAKE_RGB15 (0x10, 0x10, 0x10), 0x00),
			BUILD_COLOR (MAKE_RGB15 (0x03, 0x05, 0x1E), 0x00),
			BUILD_COLOR (MAKE_RGB15 (0x00, 0x18, 0x00), 0x00),
			BUILD_COLOR (MAKE_RGB15 (0x1B, 0x1B, 0x00), 0x00),
			BUILD_COLOR (MAKE_RGB15 (0x1E, 0x0D, 0x00), 0x00),
			BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x14), 0x05),
			BUILD_COLOR (MAKE_RGB15 (0x0F, 0x00, 0x19), 0x00),
		};

		r.extent.width = 23;
		r.extent.height = SHIP_NAME_HEIGHT;
		SetContextFont (StarConFont);
		t.baseline.x = 33 + SUMMARY_X_OFFS - SUMMARY_SIDE_OFFS + 3;
		t.baseline.y = 20;
		t.align = ALIGN_RIGHT;
		t.pStr = buf;
		for (i = 0; i < NUM_ELEMENT_CATEGORIES; ++i)
		{
			if (i == NUM_ELEMENT_CATEGORIES >> 1)
			{
				t.baseline.x += 36;
				t.baseline.y = 20;
			}
			SetContextForeGroundColor (BLACK_COLOR);
			r.corner.x = t.baseline.x - r.extent.width + 1;
			r.corner.y = t.baseline.y - r.extent.height + 1;
			DrawFilledRectangle (&r);
			SetContextForeGroundColor (cargo_color[i]);
			sprintf (buf, "%u", GLOBAL_SIS (ElementAmounts[i]));
			t.CharCount = (COUNT)~0;
			font_DrawText (&t);
			t.baseline.y += 12;
		}
		t.baseline.x = 50 + SUMMARY_X_OFFS;
		t.baseline.y = 71;
		SetContextForeGroundColor (BLACK_COLOR);
		r.corner.x = t.baseline.x - r.extent.width + 1;
		r.corner.y = t.baseline.y - r.extent.height + 1;
		DrawFilledRectangle (&r);
		SetContextForeGroundColor (cargo_color[i]);
		sprintf (buf, "%u", GLOBAL_SIS (TotalBioMass));
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
	}
}

static void
ShowSummary (SUMMARY_DESC *pSD)
{
	RECT r;
	STAMP s;

	if (pSD->year_index == 0)
	{
		// Unused save slot, draw 'Empty Game' message.
		s.origin.x = s.origin.y = 0;
		s.frame = SetAbsFrameIndex (pLocMenuState->ModuleFrame,
				GetFrameCount (pLocMenuState->ModuleFrame) - 4);
		DrawStamp (&s);
		r.corner.x = 2;
		r.corner.y = 139;
		r.extent.width = SIS_SCREEN_WIDTH - 4;
		r.extent.height = 7;
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01));
		DrawFilledRectangle (&r);
	}
	else
	{
		// Game slot used, draw information about save game.
		BYTE i;
		RECT OldRect;
		TEXT t;
		QUEUE player_q;
		CONTEXT OldContext;
		SIS_STATE SaveSS;
		UNICODE buf[256];

		SaveSS = GlobData.SIS_state;
		player_q = GLOBAL (built_ship_q);

		OldContext = SetContext (StatusContext);
		GetContextClipRect (&OldRect);

		r.corner.x = SIS_ORG_X + ((SIS_SCREEN_WIDTH - STATUS_WIDTH) >> 1) +
				SAFE_X - 16 + SUMMARY_X_OFFS;
//		r.corner.x = SIS_ORG_X + ((SIS_SCREEN_WIDTH - STATUS_WIDTH) >> 1);
		r.corner.y = SIS_ORG_Y;
		r.extent.width = STATUS_WIDTH;
		r.extent.height = STATUS_HEIGHT;
		SetContextClipRect (&r);

		GlobData.SIS_state = pSD->SS;
		InitQueue (&GLOBAL (built_ship_q),
				MAX_BUILT_SHIPS, sizeof (SHIP_FRAGMENT));
		for (i = 0; i < pSD->NumShips; ++i)
			CloneShipFragment (pSD->ShipList[i], &GLOBAL (built_ship_q), 0);
		DateToString (buf, sizeof buf,
				pSD->month_index, pSD->day_index, pSD->year_index),
		ClearSISRect (DRAW_SIS_DISPLAY);
		DrawStatusMessage (buf);
		UninitQueue (&GLOBAL (built_ship_q));

		SetContextClipRect (&OldRect);
		SetContext (SpaceContext);
		BatchGraphics ();
		DrawCargo (0);
		s.origin.y = 13;
		r.extent.width = r.extent.height = 16;
		SetContextForeGroundColor (BLACK_COLOR);
		for (i = 0; i < 4; ++i)
		{
			BYTE j;

			s.origin.x = 140 + SUMMARY_X_OFFS + SUMMARY_SIDE_OFFS;
			for (j = 0; j < 4; ++j)
			{
				if ((i << 2) + j >= pSD->NumDevices)
				{
					r.corner = s.origin;
					DrawFilledRectangle (&r);
				}
				else
				{
					s.frame = SetAbsFrameIndex (
							MiscDataFrame, 77 + pSD->DeviceList[(i << 2) + j]
							);
					DrawStamp (&s);
				}
				s.origin.x += 18;
			}
			s.origin.y += 18;
		}
		UnbatchGraphics ();

		SetContextFont (StarConFont);
		t.baseline.x = 173 + SUMMARY_X_OFFS + SUMMARY_SIDE_OFFS;
		t.align = ALIGN_CENTER;
		t.CharCount = (COUNT)~0;
		t.pStr = buf;
		if (pSD->Flags & AFTER_BOMB_INSTALLED)
		{
			s.origin.x = SUMMARY_X_OFFS - SUMMARY_SIDE_OFFS + 6;
			s.origin.y = 0;
			s.frame = SetRelFrameIndex (
					pLocMenuState->ModuleFrame, 0);
			DrawStamp (&s);
			s.origin.x = SUMMARY_X_OFFS + SUMMARY_SIDE_OFFS;
			s.frame = IncFrameIndex (s.frame);
			DrawStamp (&s);
		}
		else
		{
			SetContext (RadarContext);
			GetContextClipRect (&OldRect);
			r.corner.x = SIS_ORG_X + 10 + SUMMARY_X_OFFS - SUMMARY_SIDE_OFFS;
			r.corner.y = SIS_ORG_Y + 84;
			r.extent = OldRect.extent;
			SetContextClipRect (&r);
			UnlockMutex (GraphicsLock);
			InitLander ((unsigned char)(pSD->Flags | OVERRIDE_LANDER_FLAGS));
			LockMutex (GraphicsLock);
			SetContextClipRect (&OldRect);
			SetContext (SpaceContext);

			sprintf (buf, "%u", GLOBAL_SIS (ResUnits));
			t.baseline.y = 102;
			r.extent.width = 76;
			r.extent.height = SHIP_NAME_HEIGHT;
			r.corner.x = t.baseline.x - (r.extent.width >> 1);
			r.corner.y = t.baseline.y - SHIP_NAME_HEIGHT + 1;
			SetContextForeGroundColor (BLACK_COLOR);
			DrawFilledRectangle (&r);
			SetContextForeGroundColor (
					BUILD_COLOR (MAKE_RGB15 (0x10, 0x00, 0x10), 0x01));
			font_DrawText (&t);
			t.CharCount = (COUNT)~0;
		}
		t.baseline.y = 126;
		sprintf (buf, "%u", MAKE_WORD (pSD->MCreditLo, pSD->MCreditHi));
		r.extent.width = 30;
		r.extent.height = SHIP_NAME_HEIGHT;
		r.corner.x = t.baseline.x - (r.extent.width >> 1);
		r.corner.y = t.baseline.y - SHIP_NAME_HEIGHT + 1;
		SetContextForeGroundColor (BLACK_COLOR);
		DrawFilledRectangle (&r);
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x10, 0x00, 0x10), 0x01));
		font_DrawText (&t);
		
		r.corner.x = 2;
		r.corner.y = 139;
		r.extent.width = SIS_SCREEN_WIDTH - 4;
		r.extent.height = 7;
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01));
		DrawFilledRectangle (&r);
		t.baseline.x = /*r.corner.x + (SIS_MESSAGE_WIDTH >> 1)*/ 6;
		t.baseline.y = r.corner.y + (r.extent.height - 1);
		t.align = ALIGN_LEFT;
		t.pStr = buf;
		r.corner.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		r.corner.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
		switch (pSD->Activity)
		{
			case IN_LAST_BATTLE:
			case IN_INTERPLANETARY:
			case IN_PLANET_ORBIT:
			case IN_STARBASE:
			{
				BYTE QuasiState;
				STAR_DESCPTR SDPtr;
				
				QuasiState = GET_GAME_STATE (ARILOU_SPACE_SIDE);
				SET_GAME_STATE (ARILOU_SPACE_SIDE, 0);
				SDPtr = FindStar (NULL_PTR, &r.corner, 1, 1);
				SET_GAME_STATE (ARILOU_SPACE_SIDE, QuasiState);
				if (SDPtr)
				{
					GetClusterName (SDPtr, buf);
					r.corner = SDPtr->star_pt;
					break;
				}
			}
			default:
				buf[0] = '\0';
				break;
			case IN_HYPERSPACE:
				utf8StringCopy (buf, sizeof (buf),
						GAME_STRING (NAVIGATION_STRING_BASE + 0));
				break;
			case IN_QUASISPACE:
				utf8StringCopy (buf, sizeof (buf),
						GAME_STRING (NAVIGATION_STRING_BASE + 1));
				break;
		}

		SetContextFont (TinyFont);
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x1B, 0x00, 0x1B), 0x33));
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		t.align = ALIGN_CENTER;
		t.baseline.x = SIS_SCREEN_WIDTH - 57 - 3 + (SIS_TITLE_WIDTH >> 1);
		if (pSD->Activity == IN_STARBASE)
			utf8StringCopy (buf, sizeof (buf),
					GAME_STRING (STARBASE_STRING_BASE));
		else if (pSD->Activity == IN_PLANET_ORBIT)
			utf8StringCopy (buf, sizeof (buf), GLOBAL_SIS (PlanetName));
		else
			sprintf (buf, "%03u.%01u : %03u.%01u",
					r.corner.x / 10, r.corner.x % 10,
					r.corner.y / 10, r.corner.y % 10);
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);

		SetContext (OldContext);

		GLOBAL (built_ship_q) = player_q;
		GlobData.SIS_state = SaveSS;
	}
}

static void
LoadGameDescriptions (SUMMARY_DESC *pSD)
{
	COUNT i;

	for (i = 0; i < MAX_SAVED_GAMES; ++i, ++pSD)
	{
		if (!LoadGame (i, pSD))
			pSD->year_index = 0;
	}
}

static BOOLEAN
DoPickGame (PMENU_STATE pMS)
{
	BYTE NewState;
	SUMMARY_DESC *pSD;
	BOOLEAN first_time;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		pMS->ModuleFrame = 0;
		
		return (FALSE);
	}
	first_time = (BOOLEAN)(pMS->Initialized == 0);
	SetMenuSounds (MENU_SOUND_ARROWS | MENU_SOUND_PAGEUP | MENU_SOUND_PAGEDOWN, MENU_SOUND_SELECT);

	if (!pMS->Initialized)
	{
		pMS->delta_item = (SIZE)pMS->CurState;
		pMS->CurState = NewState = prev_save;
		pMS->InputFunc = DoPickGame;
		SleepThreadUntil ((DWORD)pMS->CurFrame);
		pMS->CurFrame = 0;
		PauseMusic ();
		StopSound ();
		FadeMusic (NORMAL_VOLUME, 0);

		{
			extern FRAME PlayFrame;

			pMS->ModuleFrame = SetAbsFrameIndex (PlayFrame, 41);
		}

		LockMutex (GraphicsLock);
		SetTransitionSource (NULL);
		BatchGraphics ();
Restart:
		SetContext (SpaceContext);
		LoadGameDescriptions ((SUMMARY_DESC *)pMS->Extra);
		DrawCargo (1);
		pMS->Initialized = TRUE;
		goto ChangeGameSelection;
	}
	else if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		LockMutex (GraphicsLock);
		SetFlashRect (SFR_MENU_3DO, (FRAME)0);
		UnlockMutex (GraphicsLock);
		pMS->ModuleFrame = 0;
		pMS->CurState = (BYTE)pMS->delta_item;
		ResumeMusic ();
		if (pSolarSysState)
		{
#define DRAW_REFRESH (1 << 5)
#define REPAIR_SCAN (1 << 6)
			extern BYTE draw_sys_flags;

			if (pSolarSysState->MenuState.Initialized < 3)
				draw_sys_flags |= DRAW_REFRESH;
			else if (pSolarSysState->MenuState.Initialized == 4)
				draw_sys_flags |= REPAIR_SCAN;
		}
		return (FALSE);
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		pSD = &((SUMMARY_DESC *)pMS->Extra)[pMS->CurState];
		prev_save = pMS->CurState;
		if (pMS->delta_item == SAVE_GAME || pSD->year_index)
		{
			LockMutex (GraphicsLock);
			if (pMS->delta_item == SAVE_GAME)
			{
				STAMP MsgStamp;

				ConfirmSaveLoad (&MsgStamp);
				if (SaveGame ((COUNT)pMS->CurState, pSD))
				{
					DestroyDrawable (ReleaseDrawable (MsgStamp.frame));
					GLOBAL (CurrentActivity) |= CHECK_LOAD;
				}
				else
				{
					SetFlashRect (NULL_PTR, (FRAME)0);
					DrawStamp (&MsgStamp);
					DestroyDrawable (ReleaseDrawable (MsgStamp.frame));
					UnlockMutex (GraphicsLock);
					
					SaveProblem ();

					pMS->Initialized = FALSE;
					NewState = pMS->CurState;
					LockMutex (GraphicsLock);
					BatchGraphics ();
					goto Restart;
				}
				ResumeMusic ();
				if (pSolarSysState)
				{
#define DRAW_REFRESH (1 << 5)
#define REPAIR_SCAN (1 << 6)
					extern BYTE draw_sys_flags;
					
					if (pSolarSysState->MenuState.Initialized < 3)
						draw_sys_flags |= DRAW_REFRESH;
					else if (pSolarSysState->MenuState.Initialized == 4)
						draw_sys_flags |= REPAIR_SCAN;
				}
			}
			else
			{
				ConfirmSaveLoad (0);
				if (LoadGame ((COUNT)pMS->CurState, NULL_PTR))
					GLOBAL (CurrentActivity) |= CHECK_LOAD;
			}
			SetFlashRect (NULL_PTR, (FRAME)0);
			UnlockMutex (GraphicsLock);

			pMS->ModuleFrame = 0;
			pMS->CurState = (BYTE)pMS->delta_item;
			return (FALSE);
		}
	}
	else
	{
		NewState = pMS->CurState;
		if (PulsedInputState.menu[KEY_MENU_LEFT] || PulsedInputState.menu[KEY_MENU_PAGE_UP])
		{
			if (NewState == 0)
				NewState = MAX_SAVED_GAMES - 1;
			else if ((NewState - SAVES_PER_PAGE) > 0)
				NewState -= SAVES_PER_PAGE;
			else 
				NewState = 0;
		}
		else if (PulsedInputState.menu[KEY_MENU_RIGHT] || PulsedInputState.menu[KEY_MENU_PAGE_DOWN])
		{
			if (NewState == MAX_SAVED_GAMES - 1)
				NewState = 0;
			else if ((NewState + SAVES_PER_PAGE) < MAX_SAVED_GAMES - 1)
				NewState += SAVES_PER_PAGE;
			else 
				NewState = MAX_SAVED_GAMES - 1;
		}
		else if (PulsedInputState.menu[KEY_MENU_UP])
		{
			if (NewState == 0)
				NewState = MAX_SAVED_GAMES - 1;
			else
				NewState--;
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			if (NewState == MAX_SAVED_GAMES - 1)
				NewState = 0;
			else
				NewState++;
		}

		if (NewState != pMS->CurState)
		{
			RECT r;
			TEXT t;
			BYTE i, SHIFT;
			UNICODE buf[256];
			UNICODE buf2[80];
			LockMutex (GraphicsLock);

			BatchGraphics ();
			if (((SUMMARY_DESC *)pMS->Extra)[NewState].year_index != 0)
			{
				if (!(((SUMMARY_DESC *)pMS->Extra)[NewState].Flags
						& AFTER_BOMB_INSTALLED))
				{
					if (((SUMMARY_DESC *)pMS->Extra)[pMS->CurState].year_index == 0)
						DrawCargo (1);
					else if (((SUMMARY_DESC *)pMS->Extra)[pMS->CurState].Flags
							& AFTER_BOMB_INSTALLED)
						DrawCargo (2);
				}
				else if (((SUMMARY_DESC *)pMS->Extra)[pMS->CurState].year_index == 0)
					DrawCargo (3);
			}

ChangeGameSelection:
			pMS->CurState = NewState;
			ShowSummary (&((SUMMARY_DESC *)pMS->Extra)[pMS->CurState]);

			SetContextFont (TinyFont);
			r.extent.width = 240;
			r.extent.height = 65;
			r.corner.x = 1;
			r.corner.y = 160;
			SetContextForeGroundColor (BLACK_COLOR);
			DrawFilledRectangle (&r);

			t.CharCount = (COUNT)~0;
			t.pStr = buf;
			t.align = ALIGN_LEFT;
#if 0
			/* This code will return in modified form later. */
			if (optSmoothScroll == OPT_3DO)  // 'Smooth' Scrolling
			{
				if (NewState <= (SAVES_PER_PAGE / 2))
					SHIFT = NewState;
				else if ((NewState > (SAVES_PER_PAGE / 2)) &&
						(NewState < (MAX_SAVED_GAMES - (SAVES_PER_PAGE / 2))))
					SHIFT = (SAVES_PER_PAGE / 2);
				else //if (NewState >= (MAX_SAVED_GAMES - (SAVES_PER_PAGE / 2)))
					SHIFT = SAVES_PER_PAGE - (MAX_SAVED_GAMES - NewState) ;
			}
			else         // 'Per-Page'  Scrolling
#endif
				SHIFT = NewState - ((NewState / SAVES_PER_PAGE) * SAVES_PER_PAGE);
			for (i = 0; i < SAVES_PER_PAGE && NewState - SHIFT + i < MAX_SAVED_GAMES; i++)
			{
				SetContextForeGroundColor ((i == SHIFT) ?
						(BUILD_COLOR (MAKE_RGB15 (0x1B, 0x00, 0x1B), 0x33)):
						(BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01)));
				r.extent.width = 15;
				if (MAX_SAVED_GAMES > 99)
					r.extent.width += 5;
				r.extent.height = 11;
				r.corner.x = 8;
				r.corner.y = 160 + (i * 13);
				DrawRectangle (&r);

				t.baseline.x = r.corner.x + 3;
				t.baseline.y = r.corner.y + 8;
				sprintf (buf, "%02i", NewState - SHIFT + i);
				if (MAX_SAVED_GAMES > 99)
					sprintf (buf, "%03i", NewState - SHIFT + i);
				font_DrawText (&t);

				r.extent.width = 204 - SAFE_X;
				r.corner.x = 30 + SAFE_X;
				DrawRectangle (&r);

				t.baseline.x = r.corner.x + 3;
				if (((SUMMARY_DESC *)pMS->Extra)[NewState - SHIFT + i].year_index == 0)
					sprintf (buf, GAME_STRING (SAVEGAME_STRING_BASE + 3)); // "Empty Slot"
				else
				{
					DateToString (buf2, sizeof buf2,
							((SUMMARY_DESC *)pMS->Extra)[NewState - SHIFT + i].month_index,
							((SUMMARY_DESC *)pMS->Extra)[NewState - SHIFT + i].day_index,
							((SUMMARY_DESC *)pMS->Extra)[NewState - SHIFT + i].year_index);
					sprintf (buf, "%s %s", GAME_STRING (SAVEGAME_STRING_BASE + 4), buf2); // "Saved Game - Date:"
				}
				font_DrawText (&t);
			}
			if (LastActivity == CHECK_LOAD)
			{
				BYTE clut_buf[] = {FadeAllToColor};

				UnbatchGraphics ();

				LastActivity = 0;
				XFormColorMap ((COLORMAPPTR)clut_buf, ONE_SECOND / 2);
			}
			else
			{
				if (first_time)
				{
					r.corner.x = SIS_ORG_X;
					r.corner.y = SIS_ORG_Y;
					r.extent.width = SIS_SCREEN_WIDTH;
					r.extent.height = SIS_SCREEN_HEIGHT;

					ScreenTransition (3, &r);
				}
				UnbatchGraphics ();
			}
			SetFlashRect (NULL_PTR, (FRAME)0);
			UnlockMutex (GraphicsLock);
		}
	}

	return (TRUE);
}

static BOOLEAN
PickGame (PMENU_STATE pMS)
{
	BOOLEAN retval;
	CONTEXT OldContext;
	SUMMARY_DESC desc_array[MAX_SAVED_GAMES];
	RECT DlgRect;
	STAMP DlgStamp;

	if (pSolarSysState)
	{
		++pSolarSysState->MenuState.Initialized;
		pSolarSysState->PauseRotate = 1;
		TaskSwitch ();
	}

	LockMutex (GraphicsLock);
	OldContext = SetContext (SpaceContext);

	DlgStamp.origin.x = 0;
	DlgStamp.origin.y = 0;
	DlgRect.corner.x = SIS_ORG_X;
	DlgRect.corner.y = SIS_ORG_Y;
	DlgRect.extent.width = SIS_SCREEN_WIDTH;
	DlgRect.extent.height = SIS_SCREEN_HEIGHT;
	DlgStamp.frame = CaptureDrawable (LoadDisplayPixmap (
			&DlgRect, (FRAME)0));

	pMS->Initialized = FALSE;
	pMS->InputFunc = DoPickGame;
	pMS->Extra = desc_array;
	UnlockMutex (GraphicsLock);

	DoInput (pMS, TRUE); 
	LockMutex (GraphicsLock);
	pMS->Initialized = -1;
	pMS->InputFunc = DoGameOptions;

	retval = TRUE;
	if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
	{
		if (pMS->CurState == SAVE_GAME)
			GLOBAL (CurrentActivity) &= ~CHECK_LOAD;

		retval = FALSE;
	}

	if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
	{
		if (CommData.ConversationPhrases
				|| !(pSolarSysState
				&& pSolarSysState->MenuState.Initialized < 3))
		{
			SetTransitionSource (&DlgRect);
			BatchGraphics ();
			DrawStamp(&DlgStamp);
			ScreenTransition (3, &DlgRect);
			UnbatchGraphics ();
		}

		if (pSolarSysState)
		{
			/* We're in interplanetary, so we let the IP
			 * functions know we're ready to draw stuff
			 * again and then update the frame twice; once
			 * for the screen transition, and once to draw
			 * the ships afterwards. */
			--pSolarSysState->MenuState.Initialized;
			pSolarSysState->PauseRotate = 0;
			IP_frame ();
			IP_frame ();


			if (CommData.ConversationPhrases == 0 && !PLRPlaying ((MUSIC_REF)~0))
			{
				if (pSolarSysState->MenuState.Initialized < 3)
				{
					PlayMusic (SpaceMusic, TRUE, 1);
				}
				else
				{
					PlayMusic (LanderMusic, TRUE, 1);
				}
			}
		}
	}

	DestroyDrawable (ReleaseDrawable (DlgStamp.frame));

	SetContext (OldContext);
	UnlockMutex (GraphicsLock);

	return (retval);
}

static BOOLEAN
DoGameOptions (PMENU_STATE pMS)
{
	BOOLEAN force_select = FALSE;
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	if (LastActivity == CHECK_LOAD)
		force_select = TRUE;

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	if (pMS->Initialized <= 0)
	{
		if (LastActivity == CHECK_LOAD)
			pMS->CurState = LOAD_GAME;
		DrawMenuStateStrings (PM_SAVE_GAME, pMS->CurState);

		pMS->Initialized = 1;
		pMS->InputFunc = DoGameOptions;
	}
	else if (PulsedInputState.menu[KEY_MENU_CANCEL]
			|| (PulsedInputState.menu[KEY_MENU_SELECT]
			&& pMS->CurState == SETTINGS + 1))
	{
		pMS->CurState = SETTINGS + 1;
		return (FALSE);
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT] || force_select)
	{
		switch (pMS->CurState)
		{
			case SAVE_GAME:
			case LOAD_GAME:
				pMS->CurFrame = (FRAME)FadeMusic (0, ONE_SECOND >> 1);
						// XXX: what is going on here? A DWORD is cast
						//      to a FRAME.
				return PickGame (pMS);
			case QUIT_GAME:
				pMS->Initialized = FALSE;
				pMS->CurState = NO_QUIT_MENU;
				pMS->InputFunc = DoQuitMenu;
				break;
			case SETTINGS:
				pMS->Initialized = FALSE;
				pMS->InputFunc = DoSettings;
				pMS->CurState = SOUND_ON_SETTING;
				break;
		}
	}
	else
		DoMenuChooser (pMS, PM_SAVE_GAME);

	return (TRUE);
}

BOOLEAN
GameOptions (void)
{
	MENU_STATE MenuState;

	pLocMenuState = &MenuState;

	memset (pLocMenuState, 0, sizeof (MenuState));

	MenuState.InputFunc = DoGameOptions;
	MenuState.CurState = SAVE_GAME;
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	DoInput ((PVOID)&MenuState, TRUE);

	pLocMenuState = 0;

	return ((GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)) ? FALSE : TRUE);
}

