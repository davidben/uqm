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

#ifndef _NOTIFY_H
#define _NOTIFY_H

#include "netplay.h"
		// for NETPLAY_CHECKSUM
#include "netconnection.h"
#include "controls.h"
		// for BATTLE_INPUT_STATE
#ifdef NETPLAY_CHECKSUM
#	include "checksum.h"
#endif

void Netplay_selectShip(NetConnection *conn, uint16 index);
void Netplay_battleInput(NetConnection *conn, BATTLE_INPUT_STATE input);
void Netplay_teamStringChanged(NetConnection *conn, int player,
		const char *name, size_t len);
void Netplay_entireFleetChanged(NetConnection *conn, int player,
		const BYTE *fleet, size_t fleetSize);
void Netplay_fleetShipChanged(NetConnection *conn, int player,
		size_t index, BYTE ship);
void Netplay_seedRandom(NetConnection *conn, uint32 seed);
void Netplay_sendInputDelay(NetConnection *conn, uint32 delay);
void Netplay_sendFrameCount(NetConnection *conn,
		BattleFrameCounter frameCount);
#ifdef NETPLAY_CHECKSUM
void Netplay_sendChecksum(NetConnection *conn,
		BattleFrameCounter frameCount, Checksum checksum);
#endif


#endif  /* _NOTIFY_H */

