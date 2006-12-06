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

#include "pickmele.h"

#include "build.h"
#include "controls.h"
#include "intel.h"
#include "melee.h"
#ifdef NETPLAY
#	include "netplay/netmelee.h"
#	include "netplay/netmisc.h"
#	include "netplay/notify.h"
#endif
#include "races.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "libs/inplib.h"
#include "libs/mathlib.h"


#define NUM_MELEE_COLS_ORIG NUM_MELEE_COLUMNS
#define PICK_X_OFFS 57
#define PICK_Y_OFFS 24
#define PICK_SIDE_OFFS 100

struct getmelee_struct {
	BOOLEAN (*InputFunc) (struct getmelee_struct *pInputState);
	COUNT MenuRepeatDelay;
	BOOLEAN Initialized;
	HSTARSHIP hBattleShip;
	COUNT row, col, ships_left, which_player;
	COUNT randomIndex;
#ifdef NETPLAY
	BOOLEAN remoteSelected;
#endif
	RECT flash_rect;
};

#ifdef NETPLAY
static void reportShipSelected (GETMELEE_STATE *gms, COUNT index);
#endif

// Returns the <index>th ship in the queue, or 0 if it is not available.
// For all the ships in the queue, the ShipFacing field contains the
// index in the queue.
static HSTARSHIP
MeleeShipByQueueIndex (const QUEUE *queue, COUNT index)
{
	HSTARSHIP hShip;
	HSTARSHIP hNextShip;
	
	for (hShip = GetHeadLink (queue); hShip != 0; hShip = hNextShip)
	{
		STARSHIPPTR StarShipPtr = LockStarShip (queue, hShip);
		if (StarShipPtr->ShipFacing == index)
		{
			hNextShip = hShip;
			if (StarShipPtr->RaceResIndex == 0)
				hShip = 0;
			UnlockStarShip (queue, hNextShip);
			break;
		}
		hNextShip = _GetSuccLink (StarShipPtr);
		UnlockStarShip (queue, ship);
	}

	return hShip;
}

// Returns the <index>th available ship in the queue.
static HSTARSHIP
MeleeShipByUsedIndex (const QUEUE *queue, COUNT index)
{
	HSTARSHIP hShip;
	HSTARSHIP hNextShip;
	
	for (hShip = GetHeadLink (queue); hShip != 0; hShip = hNextShip)
	{
		STARSHIPPTR StarShipPtr = LockStarShip (queue, hShip);
		if (StarShipPtr->RaceResIndex && index-- == 0)
		{
			UnlockStarShip (queue, hShip);
			break;
		}
		hNextShip = _GetSuccLink (StarShipPtr);
		UnlockStarShip (queue, hShip);
	}

	return hShip;
}

#if 0
static COUNT
queueIndexFromShip (HSTARSHIP hShip)
{
	COUNT result;
	STARSHIPPTR StarShipPtr = LockStarShip (queue, hShip);
	result = StarShipPtr->ShipFacing;
	UnlockStarShip (queue, hShip);
}
#endif

static void
PickMelee_ChangedSelection (GETMELEE_STATE *gms)
{
	LockMutex (GraphicsLock);
	gms->flash_rect.corner.x = PICK_X_OFFS + ((ICON_WIDTH + 2) * gms->col);
	gms->flash_rect.corner.y = PICK_Y_OFFS + ((ICON_HEIGHT + 2) * gms->row)
			+ ((1 - gms->which_player) * PICK_SIDE_OFFS);
	SetFlashRect (&gms->flash_rect, (FRAME)0);
	UnlockMutex (GraphicsLock);
}

// Select a new ship from the fleet for battle.
// Returns 'TRUE' if no choice has been made yet; this function is to be
// called again later.
// Returns 'FALSE' if a choice has been made. gms->hStarShip is set
// to the chosen (or randomly selected) ship, or to 0 if 'exit' has
// been chosen.
/* TODO: Include player timeouts */
static BOOLEAN
DoGetMelee (GETMELEE_STATE *gms)
{
	BOOLEAN left, right, up, down, select;
	COUNT which_player = gms->which_player;
	BOOLEAN done = FALSE;

	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);

	if (!gms->Initialized)
	{
		gms->Initialized = TRUE;
		gms->row = 0;
		gms->col = NUM_MELEE_COLS_ORIG;
#ifdef NETPLAY
		gms->remoteSelected = FALSE;
#endif

		// We determine in advance which ship would be chosen if the player
		// wants a random ship, to keep it simple to keep network parties
		// synchronised.
		gms->randomIndex = (COUNT)TFB_Random () % gms->ships_left;

		PickMelee_ChangedSelection (gms);
		return TRUE;
	}

	SleepThread (ONE_SECOND / 120);
#ifdef NETPLAY
	netInput ();

	if (!allConnected())
		goto aborted;
#endif
	
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		goto aborted;

	if (PlayerInput[which_player] == ComputerInput)
	{
		/* TODO: Make this a frame-by-frame thing.  This code is currently
		 *       copied from intel.c's computer_intelligence */
		SleepThread (ONE_SECOND >> 1);
		left = right = up = down = FALSE;
		select = TRUE;		
	}
#ifdef NETPLAY
	else if (PlayerInput[which_player] == NetworkInput)
	{
		flushPacketQueues ();
		if (gms->remoteSelected)
			return FALSE;
		return TRUE;
	}
#endif
	else
	{
		CONTROL_TEMPLATE template;

		if (which_player == 0)
			template = PlayerOne;
		else
			template = PlayerTwo;

		left = PulsedInputState.key[template][KEY_LEFT];
		right = PulsedInputState.key[template][KEY_RIGHT];
		up = PulsedInputState.key[template][KEY_UP];
		down = PulsedInputState.key[template][KEY_DOWN];
		select = PulsedInputState.key[template][KEY_WEAPON];
	}

	if (select)
	{
		if (gms->col == NUM_MELEE_COLS_ORIG)
		{
			if (gms->row == 0)
			{
				// Random ship
				gms->hBattleShip = MeleeShipByUsedIndex (
						&race_q[which_player], gms->randomIndex);
#ifdef NETPLAY
				reportShipSelected (gms, (COUNT)~0);
#endif
				done = TRUE;
			}
			else
			{
				// Selected exit
				if (ConfirmExit ())
					goto aborted;
			}
		}
		else
		{
			COUNT ship_index;
			
			ship_index = (gms->row * NUM_MELEE_COLS_ORIG) + gms->col;
			gms->hBattleShip = MeleeShipByQueueIndex (
					&race_q[which_player], ship_index);
			if (gms->hBattleShip != 0)
			{
				// Selection contains a ship.
#ifdef NETPLAY
				reportShipSelected (gms, ship_index);
#endif
				done = TRUE;
			}
		}
	}
	else
	{
		// Process motion commands.
		COUNT new_row, new_col;
		
		new_row = gms->row;
		new_col = gms->col;
		if (left)
		{
			if (new_col-- == 0)
				new_col = NUM_MELEE_COLS_ORIG;
		}
		else if (right)
		{
			if (new_col++ == NUM_MELEE_COLS_ORIG)
				new_col = 0;
		}
		if (up)
		{
			if (new_row-- == 0)
				new_row = NUM_MELEE_ROWS - 1;
		}
		else if (down)
		{
			if (++new_row == NUM_MELEE_ROWS)
				new_row = 0;
		}
		
		if (new_row != gms->row || new_col != gms->col)
		{
			gms->row = new_row;
			gms->col = new_col;
			
			PlayMenuSound (MENU_SOUND_MOVE);
			PickMelee_ChangedSelection (gms);
		}
	}

#ifdef NETPLAY
	flushPacketQueues ();
#endif

	return !done;

aborted:
#ifdef NETPLAY
	flushPacketQueues ();
#endif
	gms->hBattleShip = 0;
	GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
	return FALSE;
}

#ifdef NETPLAY
static void
endMeleeCallback (NetConnection *conn, void *arg)
{
	NetMelee_reenterState_inSetup (conn);
	(void) arg;
}
#endif

HSTARSHIP
GetMeleeStarShip (STARSHIPPTR LastStarShipPtr, COUNT which_player)
{
	COUNT ships_left;
	TEXT t;
	UNICODE buf[40];
	STAMP s;
	CONTEXT OldContext;
	GETMELEE_STATE gmstate;
	STARSHIPPTR StarShipPtr;
	BOOLEAN firstTime;

	if (!(GLOBAL (CurrentActivity) & IN_BATTLE))
		return (0);

#ifdef NETPLAY
	{
		NetConnection *conn = netConnections[which_player];
		if (conn != NULL) {
			BattleStateData *battleStateData;
			battleStateData =
					(BattleStateData *) NetConnection_getStateData(conn);
			battleStateData->getMeleeState = &gmstate;
		}
	}
#endif
	
	s.frame = SetAbsFrameIndex (PickMeleeFrame, which_player);

	OldContext = SetContext (OffScreenContext);
	SetContextFGFrame (s.frame);
	if (LastStarShipPtr == 0 || LastStarShipPtr->special_counter == 0)
	{
		COUNT cur_bucks;
		HSTARSHIP hBattleShip, hNextShip;

		cur_bucks = 0;
		for (hBattleShip = GetHeadLink (&race_q[which_player]);
				hBattleShip != 0; hBattleShip = hNextShip)
		{
			StarShipPtr = LockStarShip (&race_q[which_player], hBattleShip);
			if (StarShipPtr == LastStarShipPtr)
			{
				LastStarShipPtr->RaceResIndex = 0;

				gmstate.col = LastStarShipPtr->ShipFacing;
				s.origin.x = 3
					+ ((ICON_WIDTH + 2) * (gmstate.col % NUM_MELEE_COLS_ORIG));
				s.origin.y = 9
					+ ((ICON_HEIGHT + 2) * (gmstate.col / NUM_MELEE_COLS_ORIG));
				s.frame = SetAbsFrameIndex (StatusFrame, 3);
				DrawStamp (&s);
				s.frame = SetAbsFrameIndex (PickMeleeFrame, which_player);
			}
			else if (StarShipPtr->RaceResIndex)
			{
				cur_bucks += StarShipPtr->special_counter;
			}
			hNextShip = _GetSuccLink (StarShipPtr);
			UnlockStarShip (&race_q[which_player], hBattleShip);
		}

		GetFrameRect (s.frame, &gmstate.flash_rect);
		gmstate.flash_rect.extent.width -= 4;
		t.baseline.x = gmstate.flash_rect.extent.width;
		gmstate.flash_rect.corner.x =
				gmstate.flash_rect.extent.width - (6 * 3);
		gmstate.flash_rect.corner.y = 2;
		gmstate.flash_rect.extent.width = (6 * 3);
		gmstate.flash_rect.extent.height = 7 - 2;
		SetContextForeGroundColor (PICK_BG_COLOR);
		DrawFilledRectangle (&gmstate.flash_rect);

		sprintf (buf, "%d", cur_bucks);
		t.baseline.y = 7;
		t.align = ALIGN_RIGHT;
		t.pStr = buf;
		t.CharCount = (COUNT)~0;
		SetContextFont (TinyFont);
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x13, 0x00, 0x00), 0x2C));
		font_DrawText (&t);
	}

	SetContext (SpaceContext);
	
	s.origin.x = PICK_X_OFFS - 3;
	s.origin.y = PICK_Y_OFFS - 9 + ((1 - which_player) * PICK_SIDE_OFFS);

	DrawStamp (&s);

	if (LOBYTE (battle_counter) == 0 || HIBYTE (battle_counter) == 0)
	{
		// One side is out of ships. Game over.
		DWORD TimeOut;
		BOOLEAN PressState, ButtonState;

		s.origin.y = PICK_Y_OFFS - 9 + (which_player * PICK_SIDE_OFFS);
		s.frame = SetAbsFrameIndex (PickMeleeFrame,
				(COUNT) (1 - which_player));
		DrawStamp (&s);

		TimeOut = GetTimeCounter () + (ONE_SECOND * 4);
		SetContext (OldContext);
		UnlockMutex (GraphicsLock);

		PressState = PulsedInputState.menu[KEY_MENU_SELECT] ||
				PulsedInputState.menu[KEY_MENU_CANCEL];
		do
		{
			UpdateInputState ();
			ButtonState = PulsedInputState.menu[KEY_MENU_SELECT] ||
					PulsedInputState.menu[KEY_MENU_CANCEL];
			if (PressState)
			{
				PressState = ButtonState;
				ButtonState = FALSE;
			}

			TaskSwitch ();
		} while (!(GLOBAL (CurrentActivity) & CHECK_ABORT) && (!ButtonState
				&& (!(PlayerControl[0] & PlayerControl[1] & PSYTRON_CONTROL)
				|| GetTimeCounter () < TimeOut)));

#ifdef NETPLAY
		setStateConnections (NetState_endMelee);
		localReadyConnections (endMeleeCallback, NULL, true);
#endif

		LockMutex (GraphicsLock);

		return (0);
	}

	// Fade in if we're not already faded in.
#ifdef NETPLAY
	// Hack. If one side is network controlled, the top player does not have
	// to be the first player to get a chance to select his ship,
	// As what is top locally doesn't have to be the same as what it is
	// remotely.
	// This check is not so nice as it replicates the code used to decide
	// which party is first in selectAllShips.
	firstTime = (LastStarShipPtr == 0) && ((which_player == 0) ==
			(((PlayerControl[0] & NETWORK_CONTROL) &&
			!NetConnection_getDiscriminant(netConnections[0])) ||
			((PlayerControl[1] & NETWORK_CONTROL) &&
			NetConnection_getDiscriminant(netConnections[1]))));
#else
	firstTime = LastStarShipPtr == 0 && which_player == 1;
#endif
	if (firstTime)
	{
		BYTE fade_buf[] = {FadeAllToColor};
						
		SleepThreadUntil (XFormColorMap
				((COLORMAPPTR) fade_buf, ONE_SECOND / 2) + ONE_SECOND / 60);
		FlushColorXForms ();
	}

	if (which_player == 0)
		ships_left = LOBYTE (battle_counter);
	else
		ships_left = HIBYTE (battle_counter);

	gmstate.flash_rect.extent.width = (ICON_WIDTH + 2);
	gmstate.flash_rect.extent.height = (ICON_HEIGHT + 2);
#ifdef NETPLAY
	{
		bool allOk = negotiateReadyConnections (true, NetState_selectShip);
		if (!allOk)
		{
			// Some network connection has been reset.
			gmstate.hBattleShip = 0;
		}
	}
#endif
	gmstate.InputFunc = DoGetMelee;
	SetDefaultMenuRepeatDelay ();
	gmstate.Initialized = FALSE;
	gmstate.ships_left = ships_left;
	gmstate.which_player = which_player;

	UnlockMutex (GraphicsLock);
	ResetKeyRepeat ();
	DoInput ((PVOID)&gmstate, FALSE);

	LockMutex (GraphicsLock);
	SetFlashRect (NULL_PTR, (FRAME)0);
	
	if (gmstate.hBattleShip == 0)
	{
		// Aborting.
		GLOBAL (CurrentActivity) &= ~IN_BATTLE;
	}
	else
	{
		StarShipPtr =
				LockStarShip (&race_q[which_player], gmstate.hBattleShip);
		OwnStarShip (StarShipPtr,
				1 << which_player, StarShipPtr->captains_name_index);
		StarShipPtr->captains_name_index = 0;
		UnlockStarShip (&race_q[which_player], gmstate.hBattleShip);

		PlayMenuSound (MENU_SOUND_SUCCESS);

		WaitForSoundEnd (0);
	}

#ifdef NETPLAY
	{
		NetConnection *conn = netConnections[which_player];
		if (conn != NULL && NetConnection_isConnected(conn))
		{
			BattleStateData *battleStateData;
			battleStateData = (BattleStateData *)
					NetConnection_getStateData(conn);
			battleStateData->getMeleeState = NULL;
		}
	}
#endif

	return (gmstate.hBattleShip);
}

#ifdef NETPLAY
// Called when a ship selection has arrived from a remote player.
bool
updateMeleeSelection (GETMELEE_STATE *gms, COUNT player, COUNT ship)
{
	if (gms == NULL)
	{
		// This happens when we get an update message from a connection
		// for who we are not selecting a ship (yet?).
		fprintf (stderr, "Unexpected ship selection packet received.\n");
		return false;
	}

	if (ship == (COUNT) ~0)
	{
		// Random selection.
		gms->hBattleShip =
				MeleeShipByUsedIndex (&race_q[player], gms->randomIndex);
		gms->remoteSelected = TRUE;
		NetConnection_setState (netConnections[player], NetState_interBattle);
		return true;
	}
	
	gms->hBattleShip = MeleeShipByQueueIndex (&race_q[player], ship);
	if (gms->hBattleShip == 0)
	{
		fprintf (stderr, "Invalid ship selection received from remote "
				"party.\n");
		return false;
	}

	gms->remoteSelected = TRUE;
	NetConnection_setState (netConnections[player], NetState_interBattle);
	return true;
}

static void
reportShipSelected (GETMELEE_STATE *gms, COUNT index)
{
	size_t playerI;
	for (playerI = 0; playerI < NUM_PLAYERS; playerI++)
	{
		NetConnection *conn = netConnections[playerI];

		if (conn == NULL)
			continue;

		if (!NetConnection_isConnected (conn))
			continue;

		Netplay_selectShip (conn, index);
		NetConnection_setState (conn, NetState_interBattle);
	}
	(void) gms;
}
#endif

