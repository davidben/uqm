// Copyright Michael Martin, 2004.

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
#include "setupmenu.h"
#include "controls.h"
#include "libs/graphics/widgets.h"

typedef struct setup_menu_state {
	BOOLEAN (*InputFunc) (struct setup_menu_state *pInputState);
	COUNT MenuRepeatDelay;
	BOOLEAN initialized;
	int anim_frame_count;
	DWORD NextTime;
} SETUP_MENU_STATE;

typedef SETUP_MENU_STATE *PSETUP_MENU_STATE;

#define NUM_STEPS 20
#define X_STEP (SCREEN_WIDTH / NUM_STEPS)
#define Y_STEP (SCREEN_HEIGHT / NUM_STEPS)
#define MENU_FRAME_RATE (ONE_SECOND / 60)

BOOLEAN
DoSetupMenu (PSETUP_MENU_STATE pInputState)
{
	if (!pInputState->initialized) 
	{
		SetDefaultMenuRepeatDelay ();
		pInputState->anim_frame_count = NUM_STEPS;
		pInputState->initialized = TRUE;
		pInputState->NextTime = GetTimeCounter ();
	}
	
	BatchGraphics ();
	if (pInputState->anim_frame_count)
	{
		RECT r;
		COLOR bg, dark, medium;
		int frame = --pInputState->anim_frame_count;
		
		r.corner.x = ((frame * X_STEP) >> 1) + 2;
		r.corner.y = ((frame * Y_STEP) >> 1) + 2;
		r.extent.width = X_STEP * (NUM_STEPS - frame) - 4;
		r.extent.height = Y_STEP * (NUM_STEPS - frame) - 4;

		dark = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x08), 0x01);
		medium = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x10), 0x01);
		bg = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x1A), 0x09);

		DrawShadowedBox(&r, bg, dark, medium);
	}
	else
	{
		RECT r;
		COLOR bg, dark, medium, text, oldtext;
		FONT  oldfont = SetContextFont (StarConFont);
		FONTEFFECT oldFontEffect = SetContextFontEffect (0, 0, 0);
		TEXT t;
		
		r.corner.x = 2;
		r.corner.y = 2;
		r.extent.width = SCREEN_WIDTH - 4;
		r.extent.height = SCREEN_HEIGHT - 4;

		dark = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x08), 0x01);
		medium = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x10), 0x01);
		bg = BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x1A), 0x09);
		text = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x00), 0x0E);

		DrawShadowedBox(&r, bg, dark, medium);

		oldtext = SetContextForeGroundColor (text);
		t.baseline.x = r.corner.x + (r.extent.width >> 1);
		t.baseline.y = r.corner.y + 8;
		t.pStr = "Sorry!";
		t.align = ALIGN_CENTER;
		t.valign = VALIGN_BOTTOM;
		t.CharCount = ~0;
		font_DrawText (&t);

		t.baseline.y += 100;
		t.pStr = "The Setup menu has not yet been implemented.";
		font_DrawText (&t);

		t.baseline.y += 8;
		t.pStr = "Please press Select or Cancel to return.";
		font_DrawText (&t);		
		
		SetContextFontEffect (oldFontEffect.type, oldFontEffect.from, oldFontEffect.to);
		SetContextFont (oldfont);
		SetContextForeGroundColor (oldtext);
	}

	UnbatchGraphics ();
	SleepThreadUntil (pInputState->NextTime + MENU_FRAME_RATE);
	pInputState->NextTime = GetTimeCounter ();
	return !(PulsedInputState.key[KEY_MENU_CANCEL] ||
		 PulsedInputState.key[KEY_MENU_SELECT]);
}

void
SetupMenu (void)
{
	SETUP_MENU_STATE s;

	s.InputFunc = DoSetupMenu;
	s.initialized = FALSE;
	SetMenuSounds (0, MENU_SOUND_SELECT | MENU_SOUND_CANCEL);
	DoInput ((PVOID)&s, TRUE);
}
