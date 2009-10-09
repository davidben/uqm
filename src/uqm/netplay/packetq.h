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

#ifndef _PACKETQ_H
#define _PACKETQ_H

typedef struct PacketQueue PacketQueue;

#include "packet.h"
#include "types.h"

#include <sys/types.h>

typedef struct PacketQueueLink PacketQueueLink;
struct PacketQueueLink {
	PacketQueueLink *next;
	Packet *packet;
};

struct PacketQueue {
	size_t size;
	PacketQueueLink *first;
	PacketQueueLink **end;
	
	PacketQueueLink *firstUrgent;
	PacketQueueLink **endUrgent;

	// first points to the first entry in the queue
	// end points to the location where the next non-urgent message should
	//     be inserted.
	// 'firstUrgent' and 'endUrgent' are analogous to 'first' and 'end'.

	// Urgent packets should only be used for packets that may not
	// be delayed while we wait for some remote confirmation.
	// As such, these messages themselves should not require a state change
	// which needs to be remotely confirmed.
	// For example, the endTurn message can be sent when we want to change
	// which party is allowed to transmit specific packets. That message
	// should not be delayed by messages which require that *we* are that
	// party, as without the endTurn message, it will never become our turn.
	// Also, ping and ack packets should give some indication of the round
	// trip time, which they can only do if they aren't delayed by other
	// packets.
};

void PacketQueue_init(PacketQueue *queue);
void PacketQueue_uninit(PacketQueue *queue);
void queuePacket(NetConnection *conn, Packet *packet, bool urgent);
int flushPacketQueue(NetConnection *conn);


#endif

