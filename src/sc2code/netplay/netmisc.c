/*
 *  Copyright 2006  Serge van den Boom <svdb@stack.nl>
 *
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "netplay.h"
#include "netmisc.h"

#include "netmelee.h"
#include "packetsenders.h"
#include "proto/ready.h"

#include "melee.h"
		// For feedback functions.

#include <stdlib.h>


static BattleStateData *BattleStateData_alloc(void);
static void BattleStateData_free(BattleStateData *battleStateData);
static inline BattleStateData *BattleStateData_new(
		struct melee_state *meleeState,
		struct battlestate_struct *battleState,
		struct getmelee_struct *getMeleeState);
static void BattleStateData_delete(BattleStateData *battleStateData);


static BattleStateData *
BattleStateData_alloc(void) {
	return (BattleStateData *) malloc(sizeof (BattleStateData));
}

static void
BattleStateData_free(BattleStateData *battleStateData) {
	free(battleStateData);
}

static inline BattleStateData *
BattleStateData_new(struct melee_state *meleeState,
		struct battlestate_struct *battleState,
		struct getmelee_struct *getMeleeState) {
	BattleStateData *battleStateData = BattleStateData_alloc();
	battleStateData->releaseFunction =
			(NetConnectionStateData_ReleaseFunction) BattleStateData_delete;
	battleStateData->meleeState = meleeState;
	battleStateData->battleState = battleState;
	battleStateData->getMeleeState = getMeleeState;
	return battleStateData;
}

static void
BattleStateData_delete(BattleStateData *battleStateData) {
	BattleStateData_free(battleStateData);
}


////////////////////////////////////////////////////////////////////////////


static void NetMelee_enterState_inSetup(NetConnection *conn, void *arg);

// Called when a connection has been established.
void
NetMelee_connectCallback(NetConnection *conn) {
	BattleStateData *battleStateData;
	struct melee_state *meleeState;

	meleeState = (struct melee_state *) NetConnection_getExtra(conn);
	battleStateData = BattleStateData_new(meleeState, NULL, NULL);
	NetConnection_setStateData(conn, (void *) battleStateData);
	NetConnection_setExtra(conn, NULL);

	sendInit(conn);
	Netplay_localReady (conn, NetMelee_enterState_inSetup, NULL, false);
}

// Called when a connection is closed.
void
NetMelee_closeCallback(NetConnection *conn) {
	closeFeedback(conn);
}

// Called when a network error occurs during connect.
void
NetMelee_errorCallback(NetConnection *conn,
		const NetConnectionError *error) {
	errorFeedback(conn);
	(void) error;
}

// Callback function for when both sides have finished initialisation after
// initial connect.
static void
NetMelee_enterState_inSetup(NetConnection *conn, void *arg) {
	BattleStateData *battleStateData;
	struct melee_state *meleeState;
	int player;

	NetConnection_setState(conn, NetState_inSetup);
		
	battleStateData = (BattleStateData *) NetConnection_getStateData(conn);
	meleeState = battleStateData->meleeState;
	
	player = NetConnection_getPlayerNr(conn);

	connectedFeedback(conn);

	entireFleetChanged(meleeState, player);
	teamStringChanged(meleeState, player);

	flushPacketQueues();

	(void) arg;
}

// Callback function for when both sides have confirmed that the battle
// has ended.
void
NetMelee_reenterState_inSetup(NetConnection *conn) {
	BattleStateData *battleStateData;
	struct melee_state *meleeState;

	NetConnection_setState(conn, NetState_inSetup);
	
	battleStateData = (BattleStateData *) NetConnection_getStateData(conn);
	meleeState = battleStateData->meleeState;

	// The player who entered the menu first should send his changes over
	// to the other side when the other player enters setup too.
	// As myTurn is set when a change is made, and will never be unset
	// until the other side makes a change, it can be used to determine
	// which side was first. In the case there were no changes made, myTurn
	// may be the second player, but in this case it doesn't matter
	// who sends what to whom.
	if (NetConnection_isMyTurn(conn))
	{
		size_t side;

		for (side = 0; side < NUM_SIDES; side++)
		{
			entireFleetChanged(meleeState, side);
			teamStringChanged(meleeState, side);
		}
		flushPacketQueues();
	}
}




