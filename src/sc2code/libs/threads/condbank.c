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

/* This defines the Condition Variable Bank.  Individual threads may
   sleep until they are signaled by their Thread ID.  If the Condition 
   Variable Bank is exhausted (unlikely!) it will wait on the DCQ's 
   condition variable.  This may produce stuttering. */

#include <stdio.h>
#include "starcon.h"
#include "libs/threadlib.h"

/* The Condition Variable Bank. */

#define CONDVAR_BANK_SIZE 10

static Mutex bank_mutex;

static struct {
	CondVar   var;
	DWORD     id;
	int       used;
	Mutex     control;
} bank[CONDVAR_BANK_SIZE];

void
init_cond_bank ()
{
	int i;
	bank_mutex = CreateMutex ();
	for (i = 0; i < CONDVAR_BANK_SIZE; i++)
	{
		bank[i].var = CreateCondVar ("FlushGraphics Bank");
		bank[i].id = bank[i].used = 0;
		bank[i].control = CreateMutex ();
	}
}

void
uninit_cond_bank ()
{
	int i;
	for (i = 0; i < CONDVAR_BANK_SIZE; i++)
	{
		DestroyCondVar (bank[i].var);
		DestroyMutex (bank[i].control);
	}
	DestroyMutex (bank_mutex);
}

int
FindSignalChannel ()
{
	int i;

	LockMutex (bank_mutex);
	for (i = 0; i < CONDVAR_BANK_SIZE; i++)
	{
		if (!bank[i].used)
		{
			LockMutex (bank[i].control);
			return i;
		}
	}
	return -1;
}

void
WaitForSignal (int i)
{
	DWORD me = CurrentThreadID ();

	if (i == -1)
	{
		/* The bank is full! */
		fprintf(stderr, "Condvar bank is full, %lu is waiting on DCQ.\n", me);
		UnlockMutex (bank_mutex);
		WaitCondVar (RenderingCond);
	}
	else
	{
		bank[i].used = 1;
		bank[i].id = me;
		UnlockMutex (bank_mutex);
		// fprintf (stderr, "Thread %lu waiting on cond var %d (control: %p)\n", me, i, bank[i].control);
		WaitProtectedCondVar (bank[i].var, bank[i].control);
		// fprintf (stderr, "Thread %lu signaled via cond var %d\n", me, i);
		UnlockMutex (bank[i].control);
		LockMutex (bank_mutex);
		bank[i].used = bank[i].id = 0;
		UnlockMutex (bank_mutex);
	}
}

void
SignalThread (DWORD id)
{
	int i;
	LockMutex (bank_mutex);
	for (i = 0; i < CONDVAR_BANK_SIZE; i++)
	{
		if (bank[i].used && bank[i].id == id)
		{
			UnlockMutex (bank_mutex);
			// fprintf (stderr, "Blocking on var %d's control: %p\n", i, bank[i].control);
			LockMutex (bank[i].control);
			// fprintf (stderr, "Signaling var %d, thread %lu, control %p\n", i, id, bank[i].control);
			SignalCondVar (bank[i].var);
			UnlockMutex (bank[i].control);
			return;
		}
	}
	fprintf (stderr, "Warning: Couldn't find thread to signal!\n");
	UnlockMutex (bank_mutex);
}
