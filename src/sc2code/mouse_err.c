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

#include "controls.h"
#include "options.h"
#include "setup.h"
#include "sounds.h"
#include "libs/gfxlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/widgets.h"
#include "libs/graphics/tfb_draw.h"

typedef struct mouse_error_state {
	BOOLEAN (*InputFunc) (struct mouse_error_state *pInputState);
	COUNT MenuRepeatDelay;
	BOOLEAN initialized;
	int anim_frame_count;
	DWORD NextTime;
} MOUSE_ERROR_STATE;

typedef MOUSE_ERROR_STATE *PMOUSE_ERROR_STATE;

#define NUM_STEPS 20
#define X_STEP (SCREEN_WIDTH / NUM_STEPS)
#define Y_STEP (SCREEN_HEIGHT / NUM_STEPS)
#define MENU_FRAME_RATE (ONE_SECOND / 60)

BOOLEAN
DoMouseError (PMOUSE_ERROR_STATE pInputState)
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
		FRAME oldFontEffect = SetContextFontEffect (NULL);
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
		t.baseline.x = r.extent.width >> 1;
		t.baseline.y = r.extent.height >> 1;
		t.align = ALIGN_CENTER;
		t.valign = VALIGN_MIDDLE;
		t.pStr = "Mouse input is not supported at this time.";
		t.CharCount = ~0;
		font_DrawText (&t);

		t.pStr = "Use the keyboard or joystick to navigate the menus.";
		t.baseline.y += 8;
		font_DrawText (&t);

		t.pStr = "Please press Select or Cancel to return.";
		t.baseline.y += 16;
		font_DrawText (&t);

		SetContextFontEffect (oldFontEffect);
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
MouseError (void)
{
	MOUSE_ERROR_STATE s;

	s.InputFunc = DoMouseError;
	s.initialized = FALSE;
	SetMenuSounds (0, 0);
	DoInput ((PVOID)&s, TRUE);
}
