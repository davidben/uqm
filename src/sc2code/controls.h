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

#include "starcon.h"
#include "compiler.h"

// This struct controls the various controls available to the player.

typedef struct _controller_input_state {
	int p1_thrust, p1_left, p1_right, p1_weapon, p1_special, p1_escape;
	int p2_thrust, p2_left, p2_right, p2_weapon, p2_special;
	int lander_thrust, lander_left, lander_right, lander_weapon, lander_escape;
	int pause, exit, abort;
} CONTROLLER_INPUT_STATE;

typedef struct _menu_input_state {
	int up, down, left, right, select, cancel, special;
	int page_up, page_down, zoom_in, zoom_out;
} MENU_INPUT_STATE;

typedef UBYTE BATTLE_INPUT_STATE;
#define BATTLE_LEFT       ((BATTLE_INPUT_STATE)(1 << 0))
#define BATTLE_RIGHT      ((BATTLE_INPUT_STATE)(1 << 1))
#define BATTLE_THRUST     ((BATTLE_INPUT_STATE)(1 << 2))
#define BATTLE_WEAPON     ((BATTLE_INPUT_STATE)(1 << 3))
#define BATTLE_SPECIAL    ((BATTLE_INPUT_STATE)(1 << 4))
#define BATTLE_ESCAPE     ((BATTLE_INPUT_STATE)(1 << 5))

typedef BATTLE_INPUT_STATE (*battle_summary_func) (void);
extern battle_summary_func ComputerInput, HumanInput[NUM_PLAYERS];
extern battle_summary_func PlayerInput[NUM_PLAYERS];

extern CONTROLLER_INPUT_STATE CurrentInputState;
extern MENU_INPUT_STATE CurrentMenuState;
extern volatile CONTROLLER_INPUT_STATE ImmediateInputState;
extern volatile MENU_INPUT_STATE ImmediateMenuState;

void UpdateInputState (void);
void TFB_ResetControls (void);
void SetMenuRepeatDelay (DWORD min, DWORD max, DWORD step, BOOLEAN gestalt);
void SetDefaultMenuRepeatDelay (void);
void ResetKeyRepeat (void);
BOOLEAN PauseGame (void);

BATTLE_INPUT_STATE p1_combat_summary (void);
BATTLE_INPUT_STATE p2_combat_summary (void);

#endif
