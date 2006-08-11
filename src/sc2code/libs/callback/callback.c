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

#include <sys/types.h>
//#include <stdbool.h>
#include "types.h"
#include <stdlib.h>

typedef struct CallbackLink CallbackLink;

#define CALLBACK_INTERNAL
#include "callback.h"

struct CallbackLink {
	CallbackLink *next;
	CallbackFunction callback;
	CallbackArg arg;
};

static CallbackLink *callbacks;
static CallbackLink *oldCallbacks;
		// callback list currently being processed
static CallbackLink **callbacksEnd;

static inline void
CallbackList_lock(void) {
	// TODO
	// Necessary for reentrant operation
}

static inline void
CallbackList_unlock(void) {
	// TODO
	// Necessary for reentrant operation
}

#if 0
static inline bool
CallbackList_isLocked(void) {
	// TODO
}
#endif

void
Callback_init(void) {
	callbacks = NULL;
	callbacksEnd = &callbacks;
}

// Callbacks are guaranteed to be called in the order that they are queued.
CallbackID
Callback_add(CallbackFunction callback, CallbackArg arg) {
	CallbackLink *link = malloc(sizeof (CallbackLink));
	link->callback = callback;
	link->arg = arg;
	link->next = NULL;
		
	CallbackList_lock();
	*callbacksEnd = link;
	callbacksEnd = &link->next;
	CallbackList_unlock();
	return (CallbackID) link;
}


// Pre: CallbackList is locked.
static void
CallbackLink_unlink(CallbackLink **linkPtr) {
	*linkPtr = (*linkPtr)->next;
}

static void
CallbackLink_delete(CallbackLink *link) {
	free(link);
}

// Pre: CallbackList is locked.
static CallbackLink **
CallbackLink_find(CallbackLink *link) {
	CallbackLink **ptr;

	//assert(CallbackList_isLocked());
	for (ptr = &callbacks; *ptr != NULL; ptr = &(*ptr)->next) {
		if (*ptr == link)
			return ptr;
	}
	return NULL;
}

bool
Callback_removeCallback(CallbackID id) {
	CallbackLink *link = (CallbackLink *) id;
	CallbackLink **linkPtr;

	CallbackList_lock();

	linkPtr = CallbackLink_find(link);
	if (linkPtr == NULL) {
		CallbackList_unlock();
		return false;
	}

	CallbackLink_unlink(linkPtr);
	CallbackLink_delete(link);
	
	CallbackList_unlock();
	return true;
}

static inline void
CallbackLink_doCallback(CallbackLink *link) {
	(link->callback)(link->arg);
}

// Call all queued callbacks currently in the queue. Callbacks queued
// from inside the called functions will not be processed until the next
// call of Callback_process().
// It is not allowed to remove callbacks from inside the called functions.
// NB: Callback_process() must never be called from more than one thread
//     at the same time. It's the only sensible way to ensure that the
//     callbacks are called in the order in which they were queued.
//     It is however allowed to call Callback_process() from inside the
//     callback function called by Callback_process() itself.
void
Callback_process(void) {
	CallbackLink *link;

	// Create a new queue so that the old queue can be processed
	// at once, without additional locking.
	CallbackList_lock();
	oldCallbacks = callbacks;
	callbacks = NULL;
	callbacksEnd = &callbacks;
	CallbackList_unlock();

	for (;;) {
		link = oldCallbacks;
		if (link == NULL)
			break;
		oldCallbacks = link->next;

		CallbackLink_doCallback(link);
		CallbackLink_delete(link);
	}
}


