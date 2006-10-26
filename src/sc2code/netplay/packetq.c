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

#define NETCONNECTION_INTERNAL
#include "netplay.h"
#include "netconnection.h"
#include "packetq.h"
#include "netsend.h"
#include "packetsenders.h"
#ifdef NETPLAY_DEBUG
#	include "libs/log.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static inline PacketQueueLink *
PacketQueueLink_alloc(void) {
	// XXX: perhaps keep a pool of links?
	return malloc(sizeof (PacketQueueLink));
}

static inline void
PacketQueueLink_delete(PacketQueueLink *link) {
	free(link);
}

// 'maxSize' should at least be 1
void
PacketQueue_init(PacketQueue *queue) {
	queue->size = 0;
	queue->first = NULL;
	queue->end = &queue->first;
	
	queue->firstUrgent = NULL;
	queue->endUrgent = &queue->first;
}

static void
PacketQueue_deleteLinks(PacketQueueLink *link) {
	while (link != NULL) {
		PacketQueueLink *next = link->next;
		Packet_delete(link->packet);
		PacketQueueLink_delete(link);
		link = next;
	}
}

void
PacketQueue_uninit(PacketQueue *queue) {
	PacketQueue_deleteLinks(queue->firstUrgent);
	PacketQueue_deleteLinks(queue->first);
}

void
queuePacket(NetConnection *conn, Packet *packet, bool urgent) {
	PacketQueue *queue;
	PacketQueueLink *link;

	assert(NetConnection_isConnected(conn));
	assert(!urgent || !packetTypeData[packetType(packet)].inTurn);
			// Urgent packets should never stall the connection.
	
	queue = &conn->queue;

	link = PacketQueueLink_alloc();
	link->packet = packet;
	link->next = NULL;
	if (urgent) {
		*queue->endUrgent = link;
		queue->endUrgent = &link->next;
	} else {
		*queue->end = link;
		queue->end = &link->next;
	}

	queue->size++;
	// XXX: perhaps check that this queue isn't getting too large?

#ifdef NETPLAY_DEBUG
	if (packetType(packet) != PACKET_BATTLEINPUT &&
			packetType(packet) != PACKET_CHECKSUM) {
		// Reporting BattleInput or Checksum would get so spammy that it
		// would slow down the battle.
		log_add(log_Debug, "NETPLAY: [%d] ==> Queueing packet of type %s.\n",
				NetConnection_getPlayerNr(conn),
				packetTypeData[packetType(packet)].name);
	}
#endif
}

// If an error occurs during sending, we leave the unsent packets in
// the queue, and let the caller decide what to do with them.
// This function may return -1 with errno EAGAIN if we're waiting for
// the other party to act first.
static int
flushPacketQueueLinks(NetConnection *conn, PacketQueueLink **first) {
	PacketQueueLink *link;
	PacketQueueLink *next;
	PacketQueue *queue = &conn->queue;
	
	for (link = *first; link != NULL; link = next) {
		if (packetTypeData[packetType(link->packet)].inTurn &&
				(!conn->stateFlags.myTurn || conn->stateFlags.endingTurn)) {
			// This packet requires it to be 'our turn', and it isn't,
			// or we've already told the other party we wanted to end our
			// turn.
			// This should never happen in the urgent queue.
			assert(first != &queue->firstUrgent);

			if (!conn->stateFlags.myTurn && !conn->stateFlags.endingTurn) {
				conn->stateFlags.endingTurn = true;
				if (sendEndTurnDirect(conn) == -1) {
					// errno is set
					*first = link;
					return -1;
				}
			}

			*first = link;
			errno = EAGAIN;
					// We need to wait for the reply to the turn change.
			return -1;
		}
		
		if (sendPacket(conn, link->packet) == -1) {
			// Errno is set.
			*first = link;
			return -1;
		}
		
		next = link->next;
		Packet_delete(link->packet);
		PacketQueueLink_delete(link);
		queue->size--;
	}

	*first = link;
	return 0;
}

int
flushPacketQueue(NetConnection *conn) {
	int flushResult;
	PacketQueue *queue = &conn->queue;
	
	assert(NetConnection_isConnected(conn));

	flushResult = flushPacketQueueLinks(conn, &queue->firstUrgent);
	if (queue->firstUrgent == NULL)
		queue->endUrgent = &queue->firstUrgent;
	if (flushResult == -1) {
		// errno is set
		return -1;
	}

	flushResult = flushPacketQueueLinks(conn, &queue->first);
	if (queue->first == NULL)
		queue->end = &queue->first;
	if (flushResult == -1) {
		// errno is set
		return -1;
	}
	
	// If a turn change had been requested by the other side while it was
	// our turn, we first sent everything we still had to send in our turn.
	// Now that is done, it is the time to actually give up the turn.
	if (conn->stateFlags.pendingTurnChange) {
		assert(conn->stateFlags.myTurn);
		conn->stateFlags.myTurn = false;
		conn->stateFlags.pendingTurnChange = false;

		// Send the confirmation to the other side:
		if (sendEndTurnDirect(conn) == -1) {
			// errno is set
			return -1;
		}
	}
	
	return 0;
}



