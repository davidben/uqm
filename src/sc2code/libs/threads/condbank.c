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
static int wait = 0, signal = 0;

static struct {
	Semaphore var;
	DWORD   id;
	int     used;
} bank[CONDVAR_BANK_SIZE];

void
init_cond_bank ()
{
	int i;
	bank_mutex = CreateMutex ();
	for (i = 0; i < CONDVAR_BANK_SIZE; i++)
	{
		char str[20];
		sprintf (str, "bank sem %d", i);
		bank[i].var = CreateSemaphore (0, str); 
		bank[i].id = bank[i].used = 0;
	}
}

void
uninit_cond_bank ()
{
	int i;
	for (i = 0; i < CONDVAR_BANK_SIZE; i++)
	{
		DestroyCondVar (bank[i].var);
	}
	DestroyMutex (bank_mutex);
}

void
LockSignalMutex ()
{
	LockMutex (bank_mutex);
}
void
WaitForSignal ()
{
	int i;
	int index = -1;
	DWORD me = CurrentThreadID ();
	for (i = 0; i < CONDVAR_BANK_SIZE; i++)
	{
		if (!bank[i].used)
		{
			index = i;
			break;
		}
	}
	if (index == -1)
	{
		/* The bank is full! */
		fprintf(stderr, "Condvar bank is full, %ul is waiting on DCQ.", me);
		UnlockMutex (bank_mutex);
		WaitCondVar (RenderingCond);
	}
	else
	{
		bank[i].used = 1;
		bank[i].id = me;
		// Initialize the Semaphore to a value of '0' in case it isn't already
		while (SemaphoreValue (bank[i].var))
			SetSemaphore (bank[i].var);
		UnlockMutex (bank_mutex);
		// Block on Semaphore until it is cleared by the Signal
		SetSemaphore (bank[i].var);
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
			bank[i].id = bank[i].used = 0;
			ResetSemaphoreOwner (bank[i].var);
			ClearSemaphore (bank[i].var);
			break;
		}
	}
	if (i == CONDVAR_BANK_SIZE)
		fprintf (stderr, "Warning: Couldn't find thread to signal!\n");
	UnlockMutex (bank_mutex);
}

