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

#ifndef _PACKETSENDERS_H
#define _PACKETSENDERS_H

#include "types.h"

#include "netconnection.h"
#include "packet.h"

#include "controls.h"
		// for BATTLE_INPUT_STATE

void sendInit(NetConnection *conn);
void sendPing(NetConnection *conn, uint32 id);
void sendAck(NetConnection *conn, uint32 id);
void sendEndTurn(NetConnection *conn);
void sendReady(NetConnection *conn);
int sendEndTurnDirect(NetConnection *conn);
void sendHandshake0(NetConnection *conn);
void sendHandshake1(NetConnection *conn);
void sendHandshakeCancel(NetConnection *conn);
void sendHandshakeCancelAck(NetConnection *conn);
void sendTeamString(NetConnection *conn, NetplaySide side,
		const char *name, size_t len);
void sendFleet(NetConnection *conn, NetplaySide side, const BYTE *ships,
		size_t numShips);
void sendFleetShip(NetConnection *conn, NetplaySide player,
		size_t shipIndex, BYTE ship);
void sendSeedRandom(NetConnection *conn, uint32 seed);
void sendInputDelay(NetConnection *conn, uint32 delay);
void sendSelectShip(NetConnection *conn, COUNT ship);
void sendBattleInput(NetConnection *conn, BATTLE_INPUT_STATE input);
void sendFrameCount(NetConnection *conn, uint32 frameCount);
void sendChecksum(NetConnection *conn, uint32 frameNr, uint32 checksum);
void sendAbort(NetConnection *conn, NetplayAbortReason reason);
void sendReset(NetConnection *conn, NetplayResetReason reason);


#endif  /* _PACKETSENDERS_H */


