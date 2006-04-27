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

#include "uqmlog.h"
#include <string.h>
#include <stdio.h>
#include "libs/threadlib.h"

#ifndef MAX_LOG_ENTRY_SIZE
#	define MAX_LOG_ENTRY_SIZE 256
#endif

#ifndef MAX_LOG_ENTRIES
#	define MAX_LOG_ENTRIES 128
#endif

typedef char log_Entry[MAX_LOG_ENTRY_SIZE];

// static buffers in case we run out of memory
static log_Entry queue[MAX_LOG_ENTRIES];
static log_Entry msgNoThread;
static char msgBuf[16384];

static int maxLevel = log_Debug;
static int maxStreamLevel = log_Debug;
static int maxDisp = 10;
static int qtotal = 0;
static int qhead = 0;
static int qtail = 0;
static volatile bool noThreadReady = 0;
static bool forceBox = 0;

static FILE *streamOut;

static volatile int qlock = 0;
static Mutex qmutex;

static void displayLog (bool isError);
static void displayBox (const char *title, bool isError, const char *msg);

static void
lockQueue (void)
{
	if (!qlock)
		return;

	LockMutex (qmutex);
}

static void
unlockQueue (void)
{
	if (!qlock)
		return;

	UnlockMutex (qmutex);
}

static void
removeExcess (int room)
{
	room = maxDisp - room;
	if (room < 0)
		room = 0;

	for ( ; qtotal > room; --qtotal, ++qtail)
		;
	qtail %= MAX_LOG_ENTRIES;
}

static int
acquireSlot (void)
{
	int slot;

	lockQueue ();
	
	removeExcess (1);
	slot = qhead;
	qhead = (qhead + 1) % MAX_LOG_ENTRIES;
	++qtotal;
	
	unlockQueue ();

	return slot;
}

// queues the non-threaded message when present
static void
queueNonThreaded (void)
{
	int slot;

	// This is not perfect. A race condition still exists
	// between buffering the no-thread message and setting
	// the noThreadReady flag. Neither does this prevent
	// the fully or partially overwritten message (by
	// another competing thread). But it is 'good enough'
	if (!noThreadReady)
		return;
	noThreadReady = false;

	slot = acquireSlot ();
	memcpy (queue[slot], msgNoThread, sizeof (msgNoThread));
}

void
log_init (int max_lines)
{
	int i;

	maxDisp = max_lines;
	streamOut = stderr;

	// pre-term queue strings
	for (i = 0; i < MAX_LOG_ENTRIES; ++i)
		queue[i][MAX_LOG_ENTRY_SIZE - 1] = '\0';
	
	msgBuf[sizeof (msgBuf) - 1] = '\0';
	msgNoThread[sizeof (msgNoThread) - 1] = '\0';
}

void
log_initThreads (void)
{
	qmutex = CreateMutex ("Logging Lock", SYNC_CLASS_RESOURCE);
	qlock = 1;
}

int
log_exit (int code)
{
	if (code != EXIT_SUCCESS)
		displayLog (true);
	else if (forceBox)
		displayLog (false);

	if (qlock)
	{
		qlock = 0;
		DestroyMutex (qmutex);
	}

	return code;
}

void
logged_abort (void)
{
	log_exit (3);
	abort ();
}

void
log_setLevel (int level)
{
	maxLevel = level;
	//maxStreamLevel = level;
}

FILE *
log_setOutput (FILE *out)
{
	FILE *old = streamOut;
	streamOut = out;
	
	return old;
}

void
log_addV (log_Level level, const char *fmt, va_list list)
{
	if ((int)level <= maxStreamLevel)
	{
		vfprintf (streamOut, fmt, list);
		fputc ('\n', streamOut);
	}

	if ((int)level <= maxLevel)
	{
		int slot;

		queueNonThreaded ();
		
		slot = acquireSlot ();
		vsnprintf (queue[slot], sizeof (queue[0]) - 1, fmt, list);
	}
}

void
log_add (log_Level level, const char *fmt, ...)
{
	va_list list;

	va_start (list, fmt);
	log_addV (level, fmt, list);
	va_end (list);
}

// non-threaded version of 'add'
// uses single-instance static storage with entry into the
// queue delayed until the next threaded 'add' or 'exit'
void
log_add_nothreadV (log_Level level, const char *fmt, va_list list)
{
	if ((int)level <= maxStreamLevel)
	{
		vfprintf (streamOut, fmt, list);
		fputc ('\n', streamOut);
	}

	if ((int)level <= maxLevel)
	{
		vsnprintf (msgNoThread, sizeof (msgNoThread) - 1, fmt, list);
		noThreadReady = true;
	}
}

void
log_add_nothread (log_Level level, const char *fmt, ...)
{
	va_list list;

	va_start (list, fmt);
	log_add_nothreadV (level, fmt, list);
	va_end (list);
}

void
log_forceBox (bool force)
{
	forceBox = force;
}

// sets the maximum log lines captured for the final
// display to the user on failure exit
void
log_captureLines (int num)
{
	if (num > MAX_LOG_ENTRIES)
		num = MAX_LOG_ENTRIES;
	if (num < 1)
		num = 1;
	maxDisp = num;

	// remove any extra lines already on queue
	lockQueue ();
	removeExcess (0);
	unlockQueue ();
}

static void
displayLog (bool isError)
{
	char *p = msgBuf;
	int left = sizeof (msgBuf) - 1;
	int len;

	queueNonThreaded ();

	if (isError)
	{
		strcpy (p, "The Ur-Quan Masters encountered a fatal error.\n"
				"Part of the log follows:\n\n");
		len = strlen (p);
		p += len;
		left -= len;
	}

	// glue the log entries together
	lockQueue ();
	while (qtail != qhead && left > 0)
	{
		len = strlen (queue[qtail]) + 1;
		if (len > left)
			len = left;
		memcpy (p, queue[qtail], len);
		p[len - 1] = '\n';
		p += len;
		left -= len;
		qtail = (qtail + 1) % MAX_LOG_ENTRIES;
	}
	unlockQueue ();

	displayBox ("The Ur-Quan Masters", isError, msgBuf);
}


#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)

extern int __stdcall MessageBoxA (void *hWnd, const char* lpText,
		const char* lpCaption, uint32 uType);

#define MB_ICONEXCLAMATION          0x00000030L
#define MB_ICONASTERISK             0x00000040L

static void
displayBox (const char *title, bool isError, const char *msg)
{
	MessageBoxA (0, msg, title,
			isError ? MB_ICONEXCLAMATION : MB_ICONASTERISK);
}

#elif defined(__APPLE__)

static void
displayBox (const char *title, bool isError, const char *msg)
{
	// do something here
}

#else

static void
displayBox (const char *title, bool isError, const char *msg)
{
	// do nothing here
}

#endif /* OS disjunction */
