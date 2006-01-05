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

#ifndef _CONTROLS_H_
#define _CONTROLS_H_

#include "libs/compiler.h"
#include "libs/strlib.h"


// Enumerated type for controls
enum {
	KEY_P1_THRUST,
	KEY_P1_LEFT,
	KEY_P1_RIGHT,
	KEY_P1_DOWN,
	KEY_P1_WEAPON,
	KEY_P1_SPECIAL,
	KEY_P1_ESCAPE,
	KEY_P2_THRUST,
	KEY_P2_LEFT,
	KEY_P2_RIGHT,
	KEY_P2_DOWN,
	KEY_P2_WEAPON,
	KEY_P2_SPECIAL,
	KEY_LANDER_THRUST,
	KEY_LANDER_LEFT,
	KEY_LANDER_RIGHT,
	KEY_LANDER_WEAPON,
	KEY_LANDER_ESCAPE,
	KEY_PAUSE,
	KEY_EXIT,
	KEY_ABORT,
	KEY_DEBUG,
	KEY_MENU_UP,
	KEY_MENU_DOWN,
	KEY_MENU_LEFT,
	KEY_MENU_RIGHT,
	KEY_MENU_SELECT,
	KEY_MENU_CANCEL,
	KEY_MENU_SPECIAL,
	KEY_MENU_PAGE_UP,
	KEY_MENU_PAGE_DOWN,
	KEY_MENU_HOME,
	KEY_MENU_END,
	KEY_MENU_ZOOM_IN,
	KEY_MENU_ZOOM_OUT,
	KEY_MENU_DELETE,
	KEY_MENU_BACKSPACE,
	KEY_MENU_EDIT_CANCEL,
	KEY_CHARACTER, /* abstract char key */
	NUM_KEYS
};

typedef struct _controller_input_state {
	int key[NUM_KEYS];
} CONTROLLER_INPUT_STATE;

typedef UBYTE BATTLE_INPUT_STATE;
#define BATTLE_LEFT       ((BATTLE_INPUT_STATE)(1 << 0))
#define BATTLE_RIGHT      ((BATTLE_INPUT_STATE)(1 << 1))
#define BATTLE_THRUST     ((BATTLE_INPUT_STATE)(1 << 2))
#define BATTLE_WEAPON     ((BATTLE_INPUT_STATE)(1 << 3))
#define BATTLE_SPECIAL    ((BATTLE_INPUT_STATE)(1 << 4))
#define BATTLE_ESCAPE     ((BATTLE_INPUT_STATE)(1 << 5))
#define BATTLE_DOWN       ((BATTLE_INPUT_STATE)(1 << 6))

typedef BATTLE_INPUT_STATE (*battle_summary_func) (void);
extern battle_summary_func ComputerInput, HumanInput[];
extern battle_summary_func PlayerInput[];

extern CONTROLLER_INPUT_STATE CurrentInputState, PulsedInputState;
extern volatile CONTROLLER_INPUT_STATE ImmediateInputState;

void UpdateInputState (void);
void FlushInputState (void);
void TFB_ResetControls (void);
void SetMenuRepeatDelay (DWORD min, DWORD max, DWORD step, BOOLEAN gestalt);
void SetDefaultMenuRepeatDelay (void);
void ResetKeyRepeat (void);
BOOLEAN PauseGame (void);
BOOLEAN DoConfirmExit (void);
void TFB_Abort (void);
BOOLEAN WaitAnyButtonOrQuit (BOOLEAN CheckSpecial);
extern void WaitForNoInput (SIZE Duration);
extern BOOLEAN ConfirmExit (void);
extern void DoInput (PVOID pInputState, BOOLEAN resetInput);

BATTLE_INPUT_STATE p1_combat_summary (void);
BATTLE_INPUT_STATE p2_combat_summary (void);

extern volatile BOOLEAN GamePaused, ExitRequested;

typedef struct textentry_state
{
	// standard state required by DoInput
	BOOLEAN (*InputFunc) (struct textentry_state *pTES);
	COUNT MenuRepeatDelay;

	// these are semi-private read-only
	BOOLEAN Initialized;
	BOOLEAN Success;   // edit confirmed or canceled
	UNICODE *CacheStr; // cached copy to revert immediate changes
	STRING JoyAlphaString; // joystick alphabet definition
	BOOLEAN JoystickMode;  // TRUE when doing joystick input
	BOOLEAN UpperRegister; // TRUE when entering Caps
	UNICODE *JoyAlphaStr;  // joystick alphabet
	// these are public and must be set before calling DoTextEntry
	UNICODE *BaseStr;  // set to string to edit
	UNICODE *InsPt;    // set to curremt pos of insertion point
	int MaxSize;       // set to max size of edited string

	BOOLEAN (*ChangeCallback) (struct textentry_state *pTES);
			// returns TRUE if last change is OK
	void *CbParam;     // callback parameter, use as you like
	
} TEXTENTRY_STATE;
typedef TEXTENTRY_STATE *PTEXTENTRY_STATE;

BOOLEAN DoTextEntry (PTEXTENTRY_STATE pTES);

#endif


