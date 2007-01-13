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

#include "controls.h"
#include "commglue.h"
#include "comm.h"
#include "colors.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "gamestr.h"
#include "libs/graphics/widgets.h"
#include "libs/inplib.h"
#include "libs/sound/trackplayer.h"
#include "libs/log.h"
#include "libs/resource/stringbank.h"

#include <ctype.h>
#include <stdlib.h>


#define CONFIRM_WIN_WIDTH 80
#define CONFIRM_WIN_HEIGHT 22

static void
DrawConfirmationWindow (BOOLEAN answer)
{
	COLOR oldfg = SetContextForeGroundColor (MENU_TEXT_COLOR);
	FONT  oldfont = SetContextFont (StarConFont);
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	RECT r;
	TEXT t;

	BatchGraphics ();
	r.corner.x = (SCREEN_WIDTH - CONFIRM_WIN_WIDTH) >> 1;
	r.corner.y = (SCREEN_HEIGHT - CONFIRM_WIN_HEIGHT) >> 1;
	r.extent.width = CONFIRM_WIN_WIDTH;
	r.extent.height = CONFIRM_WIN_HEIGHT;
	DrawShadowedBox (&r, SHADOWBOX_BACKGROUND_COLOR, 
			SHADOWBOX_DARK_COLOR, SHADOWBOX_MEDIUM_COLOR);

	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + 8;
	t.pStr = GAME_STRING (QUITMENU_STRING_BASE); // "Really Quit?"
	t.align = ALIGN_CENTER;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += 10;
	t.baseline.x = r.corner.x + (r.extent.width >> 2);
	t.pStr = GAME_STRING (QUITMENU_STRING_BASE + 1); // "Yes"
	SetContextForeGroundColor (answer ? MENU_HIGHLIGHT_COLOR : MENU_TEXT_COLOR);
	font_DrawText (&t);
	t.baseline.x += (r.extent.width >> 1);
	t.pStr = GAME_STRING (QUITMENU_STRING_BASE + 2); // "No"
	SetContextForeGroundColor (answer ? MENU_TEXT_COLOR : MENU_HIGHLIGHT_COLOR);	
	font_DrawText (&t);

	UnbatchGraphics ();

	SetContextFontEffect (oldFontEffect);
	SetContextFont (oldfont);
	SetContextForeGroundColor (oldfg);
}

/* This code assumes that you aren't in Character Mode.  This is
 * currently safe because VControl doesn't see keystrokes when you
 * are, and thus cannot conclude that an exit is necessary. */
BOOLEAN
DoConfirmExit (void)
{
	BOOLEAN result;
	static BOOLEAN in_confirm = FALSE;
	if (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE &&
			LOBYTE (GLOBAL (CurrentActivity)) != WON_LAST_BATTLE &&
			!(LastActivity & CHECK_RESTART))
		SuspendGameClock ();
	if (CommData.ConversationPhrases && PlayingTrack ())
		PauseTrack ();

	LockMutex (GraphicsLock);
	if (in_confirm)
	{
		result = FALSE;
		ExitRequested = FALSE;
	}
	else
	{
		RECT r;
		STAMP s;
		FRAME F;
		CONTEXT oldContext;
		RECT oldRect;
		BOOLEAN response = FALSE, done;

		in_confirm = TRUE;
		oldContext = SetContext (ScreenContext);
		GetContextClipRect (&oldRect);
		SetContextClipRect (NULL_PTR);

		r.extent.width = CONFIRM_WIN_WIDTH + 4;
		r.extent.height = CONFIRM_WIN_HEIGHT + 4;
		r.corner.x = (SCREEN_WIDTH - r.extent.width) >> 1;
		r.corner.y = (SCREEN_HEIGHT - r.extent.height) >> 1;
		s.origin = r.corner;
		F = CaptureDrawable (LoadDisplayPixmap (&r, (FRAME)0));

		SetSystemRect (&r);

		DrawConfirmationWindow (response);

		// Releasing the lock lets the rotate_planet_task
		// draw a frame.  PauseRotate can still allow one more frame
		// to be drawn, so it is safer to just not release the lock
		//UnlockMutex (GraphicsLock);
		FlushGraphics ();
		//LockMutex (GraphicsLock);

		GLOBAL (CurrentActivity) |= CHECK_ABORT;
		FlushInput ();
		done = FALSE;

		do {
			// Forbid recursive calls or pausing here!
			ExitRequested = FALSE;
			GamePaused = FALSE;
			UpdateInputState ();
			if (PulsedInputState.menu[KEY_MENU_SELECT])
			{
				done = TRUE;
				PlayMenuSound (MENU_SOUND_SUCCESS);
			}
			else if (PulsedInputState.menu[KEY_MENU_CANCEL])
			{
				done = TRUE;
				response = FALSE;
			}
			else if (PulsedInputState.menu[KEY_MENU_LEFT] || PulsedInputState.menu[KEY_MENU_RIGHT])
			{
				response = !response;
				DrawConfirmationWindow (response);
				PlayMenuSound (MENU_SOUND_MOVE);
			}
			TaskSwitch ();
		} while (!done);

		s.frame = F;
		DrawStamp (&s);
		DestroyDrawable (ReleaseDrawable (s.frame));
		ClearSystemRect ();
		if (response)
		{
			result = TRUE;
		}		
		else
		{
			result = FALSE;
			GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
		}
		ExitRequested = FALSE;
		GamePaused = FALSE;
		FlushInput ();
		SetContextClipRect (&oldRect);
		SetContext (oldContext);
	}
	UnlockMutex (GraphicsLock);

	if (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE &&
			LOBYTE (GLOBAL (CurrentActivity)) != WON_LAST_BATTLE &&
			!(LastActivity & CHECK_RESTART))
		ResumeGameClock ();
	if (CommData.ConversationPhrases && PlayingTrack ())
	{
		ResumeTrack ();
		if (CommData.AlienTransitionDesc.AnimFlags & TALK_DONE)
			do_subtitles ((void *)~0);
	}

	in_confirm = FALSE;
	return (result);
}

typedef struct popup_state
{
	// standard state required by DoInput
	BOOLEAN (*InputFunc) (struct popup_state *self);
	COUNT MenuRepeatDelay;
} POPUP_STATE;

static BOOLEAN
DoPopup (struct popup_state *self)
{
	(void)self;
	SleepThread (ONE_SECOND / 20);
	return !(PulsedInputState.menu[KEY_MENU_SELECT] || 
			PulsedInputState.menu[KEY_MENU_CANCEL]);
}

void
DoPopupWindow(const char *msg)
{
	stringbank *bank = StringBank_Create ();
	const char *lines[30];
	WIDGET_LABEL label;
	RECT r;
	STAMP s;
	FRAME F;
	CONTEXT oldContext;
	RECT oldRect;
	POPUP_STATE state;
	MENU_SOUND_FLAGS s0, s1;
	

	if (!bank)
	{
		log_add (log_Fatal, "FATAL: Memory exhaustion when preparing popup window");
		exit (EXIT_FAILURE);
	}

	label.tag = WIDGET_TYPE_LABEL;
	label.parent = NULL;
 	label.handleEvent = Widget_HandleEventIgnoreAll;
	label.receiveFocus = Widget_ReceiveFocusRefuseFocus;
	label.draw = Widget_DrawLabel;
	label.height = Widget_HeightLabel;
	label.width = Widget_WidthFullScreen;
	label.line_count = SplitString (msg, '\n', 30, lines, bank);
	label.lines = lines;

	LockMutex (GraphicsLock);

	oldContext = SetContext (ScreenContext);
	GetContextClipRect (&oldRect);
	SetContextClipRect (NULL_PTR);

	/* TODO: Better measure of dimensions than this */
	r.extent.width = SCREEN_WIDTH;
	r.extent.height = SCREEN_HEIGHT;
	r.corner.x = (SCREEN_WIDTH - r.extent.width) >> 1;
	r.corner.y = (SCREEN_HEIGHT - r.extent.height) >> 1;
	F = CaptureDrawable (LoadDisplayPixmap (&r, (FRAME)0));

	s.origin = r.corner;
	s.frame = F;

	DrawLabelAsWindow (&label);

	GetMenuSounds (&s0, &s1);
	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);

	state.InputFunc = DoPopup;
	DoInput (&state, TRUE);

	DrawStamp (&s);
	DestroyDrawable (ReleaseDrawable (s.frame));
	FlushInput ();
	SetContextClipRect (&oldRect);
	SetContext (oldContext);
	UnlockMutex (GraphicsLock);
	SetMenuSounds (s0, s1);
	StringBank_Free (bank);
}
