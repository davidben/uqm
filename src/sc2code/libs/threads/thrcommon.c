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

/* By Serge van den Boom, 2002-09-12
 */

#include <stdio.h>
#include "libs/threadlib.h"
#include "libs/timelib.h"
#include "libs/misc.h"
#include "thrcommon.h"
#ifdef PROFILE_THREADS
#include <signal.h>
#include <unistd.h>
#endif

#define DEBUG_TRACK_SEM

#ifdef DEBUG_TRACK_SEM
// Define all semaphores to be tracked in SemList.  Make sure it is NULL terminated
// Make that  semthread is the same length  as SemList, and is initialized as all 0's
extern Semaphore GraphicsSem;
static Semaphore *SemList[] = {&GraphicsSem, NULL};
Uint32 semthread[] = {0, 0};
#endif

#ifdef THREAD_QUEUE
static volatile Thread threadQueue = NULL;
static Semaphore threadQueueSemaphore;
#endif


struct ThreadStartInfo
{
	ThreadFunction func;
	void *data;
	Semaphore sem;
	Thread thread;
};

#ifdef PROFILE_THREADS
static void
SigUSR1Handler (int signr) {
	if (getpgrp () != getpid ())
	{
		// Only act for the main process
		return;
	}
	PrintThreadsStats ();
			// It's not a good idea in general to do many things in a signal
			// handler, (and especially the locking) but I guess it will
			// have to do for now (and it's only for debugging).
	(void) signr;  /* Satisfying compiler (unused parameter) */
}
#endif

void
InitThreadSystem (void)
{
#ifdef THREAD_QUEUE
	threadQueueSemaphore = CreateSemaphore (1);
#endif  /* THREAD_QUEUE */
#ifdef PROFILE_THREADS
	signal(SIGUSR1, SigUSR1Handler);
#endif
	NativeInitThreadSystem ();
}

void
UnInitThreadSystem (void)
{
	NativeUnInitThreadSystem ();
#ifdef PROFILE_THREADS
	signal(SIGUSR1, SIG_DFL);
#endif
#ifdef THREAD_QUEUE
	DestroySemaphore (threadQueueSemaphore);
#endif  /* THREAD_QUEUE */
}

#ifdef THREAD_QUEUE
static void
QueueThread (Thread thread)
{
	SetSemaphore (threadQueueSemaphore);
	thread->next = threadQueue;
	threadQueue = thread;
	ClearSemaphore (threadQueueSemaphore);
}

static void
UnQueueThread (Thread thread)
{
	volatile Thread *ptr;

	ptr = &threadQueue;
	SetSemaphore (threadQueueSemaphore);
	while (*ptr != thread)
	{
#ifdef DEBUG_THREADS
		if (*ptr == NULL)
		{
			// Should not happen.
			fprintf (stderr, "Error: Trying to remove non-present thread "
					"from thread queue.\n");
			fflush (stderr);
			abort();
		}
#endif  /* DEBUG_THREADS */
		ptr = &(*ptr)->next;
	}
	*ptr = (*ptr)->next;
	ClearSemaphore (threadQueueSemaphore);
}
#endif  /* THREAD_QUEUE */

#ifdef DEBUG_THREADS
static const char *
ThreadName(Thread thread) {
#if defined (THREAD_QUEUE) && defined (THREAD_NAMES)
	return thread->name;
#else
	return "<<UNNAMED>>";
#endif  /* !defined (THREAD_QUEUE) || !defined (THREAD_NAMES) */
}
#endif

static int
ThreadHelper (void *startInfo) {
	ThreadFunction func;
	void *data;
	Semaphore sem;
	Thread thread;
	int result;
	
	func = ((struct ThreadStartInfo *) startInfo)->func;
	data = ((struct ThreadStartInfo *) startInfo)->data;
	sem  = ((struct ThreadStartInfo *) startInfo)->sem;

	// Wait until the Thread structure is available.
	while (SetSemaphore (sem) == -1)
		;
	DestroySemaphore (sem);
	thread = ((struct ThreadStartInfo *) startInfo)->thread;
	HFree (startInfo);

	result = (*(NativeThreadFunction) func) (data);

#ifdef DEBUG_THREADS
	fprintf (stderr, "Thread '%s' done (returned %d).\n",
			ThreadName (thread), result);
	fflush (stderr);
#endif

#ifdef THREAD_QUEUE
	UnQueueThread (thread);
#endif  /* THREAD_QUEUE */

	HFree (thread);
	return result;
}

Thread
CreateThreadAux (ThreadFunction func, void *data, SDWORD stackSize
#ifdef THREAD_NAMES
		, const char *name
#endif
		)
{
	Thread thread;
	struct ThreadStartInfo *startInfo;
	
	thread = (struct Thread *) HMalloc (sizeof *thread);
#ifdef THREAD_NAMES
	thread->name = name;
#endif
#ifdef PROFILE_THREADS
	thread->startTime = GetTimeCounter ();
#endif

	startInfo = (struct ThreadStartInfo *) HMalloc (sizeof (*startInfo));
	startInfo->func = func;
	startInfo->data = data;
	startInfo->sem = CreateSemaphore (0);
	startInfo->thread = thread;
	
	thread->native = NativeCreateThread (ThreadHelper, (void *) startInfo,
			stackSize ? stackSize + 32 : 0);
	if (!NativeThreadOk (thread->native))
	{
		HFree (startInfo);
		HFree (thread);
		return NULL;
	}
	// The responsibility to free 'startInfo' and 'thread' is now by the new
	// thread.
	
#ifdef THREAD_QUEUE
	QueueThread (thread);
#endif  /* THREAD_QUEUE */

#ifdef DEBUG_THREADS
	fprintf (stderr, "Thread '%s' created.\n", ThreadName (thread));
	fflush (stderr);
#endif

	// Signal to the new thread that the thread structure is ready
	// and it can begin to use it.
	ClearSemaphore (startInfo->sem);

	(void) stackSize;  /* Satisfying compiler (unused parameter) */
	return thread;
}

/* Important: Threads shouldn't kill themselves. That would prevent them
 *            from being removed from the queue and from displaying the
 *            'killed' debug message.
 */

/* 17 Sep: Added a TFB_BatchReset call.  If a thread is killed while
 *         batching stuff, we don't want this to freeze the game.
 *         Better safe than sorry!  --Michael
 *         TEMPORARY (TODO: handle decently)
 */
void
KillThread (Thread thread)
{
	TFB_BatchReset ();
	NativeKillThread (thread->native);
#ifdef DEBUG_THREADS
	fprintf (stderr, "Thread '%s' killed.\n", ThreadName (thread));
	fflush (stderr);
#endif
#ifdef THREAD_QUEUE
	UnQueueThread (thread);
#endif  /* THREAD_QUEUE */
}

void
WaitThread (Thread thread, int *status)
{
	NativeWaitThread (thread->native, status);
}

void
SleepThread (TimePeriod timePeriod)
{
	NativeSleepThread (timePeriod);
}

void
SleepThreadUntil (TimeCount wakeTime)
{
	NativeSleepThreadUntil (wakeTime);
}

void
TaskSwitch (void)
{
	NativeTaskSwitch ();
}

#ifdef PROFILE_THREADS
// PROFILE_THREADS implies THREAD_QUEUES
void
PrintThreadsStats (void)
{
	Thread ptr;
	int now;
	
	now = GetTimeCounter ();
	SetSemaphore (threadQueueSemaphore);
	fprintf(stderr, "--- Active threads ---\n");
	for (ptr = threadQueue; ptr != NULL; ptr = ptr->next) {
#ifndef THREAD_NAMES
		fprintf (stderr, "(Thread name not available).\n");
#else
		fprintf (stderr, "Thread named '%s'.\n", ptr->name);
#endif
		fprintf (stderr, "Started %d.%d minutes ago.\n",
				(now - ptr->startTime) / 60000,
				((now - ptr->startTime) / 1000) % 60);
		NativePrintThreadStats (ptr->native);
		if (ptr->next != NULL)
			fprintf(stderr, "\n");
	}
	ClearSemaphore (threadQueueSemaphore);
	fprintf(stderr, "----------------------\n");
	fflush (stderr);
}
#endif  /* PROFILE_THREADS */

Semaphore
CreateSemaphore (DWORD initial)
{
	return (Semaphore) NativeCreateSemaphore (initial);
}

void
DestroySemaphore (Semaphore sem)
{
	NativeDestroySemaphore ((NativeSemaphore) sem);
}

int
SetSemaphore (Semaphore sem)
{
	int i;

	i = NativeSetSemaphore ((NativeSemaphore) sem);
#ifdef DEBUG_TRACK_SEM
	if (i != 0)
		fprintf(stderr, "WARNING: SetSemaphore did not return 0, this could be bad!\n");
	for (i = 0; SemList[i] != NULL; i++)
		if (*SemList[i] == sem)
		{
			semthread[i] = SDL_ThreadID ();
			break;
		}
#endif
	return i;
}

int
TrySetSemaphore (Semaphore sem)
{
	int i;

	i = NativeTrySetSemaphore ((NativeSemaphore) sem);
#ifdef DEBUG_TRACK_SEM
	if (i == 0)
		for (i = 0; SemList[i] != NULL; i++)
			if (*SemList[i] == sem)
			{
				semthread[i] = SDL_ThreadID ();
				break;
			}
#endif
	return (i);
}

int
TimeoutSetSemaphore (Semaphore sem, TimePeriod timeout)
{
	int i;

	i = NativeTimeoutSetSemaphore ((NativeSemaphore) sem, timeout);
#ifdef DEBUG_TRACK_SEM
	if (i == 0)
		for (i = 0; SemList[i] != NULL; i++)
			if (*SemList[i] == sem)
			{
				semthread[i] = SDL_ThreadID ();
				break;
			}
#endif
	return (i);
}

void
ClearSemaphore (Semaphore sem)
{
	int i;
#ifdef DEBUG_TRACK_SEM
	Uint32 semval = SDL_SemValue (sem);
	if (semval != 0)
	{
		fprintf (stderr, "WARNING: trying to clear a free semaphore (value=%d)\n", semval);
		// Should we unset the Semaphore twice?  Perhaps not.
		return;
	}
	for (i = 0;SemList[i] != NULL; i++)
		if (*SemList[i] == sem)
		{
			if (! semthread[i] && semthread[i] != SDL_ThreadID ())
				fprintf( stderr, "WARNING: tried to free a Semaphore held by another thread!\n");
			semthread[i] = 0;
			break;
		}
#endif
	NativeClearSemaphore ((NativeSemaphore) sem);
}

Mutex
CreateMutex ()
{
	return (Mutex) NativeCreateMutex ();
}

void
DestroyMutex (Mutex sem)
{
	NativeDestroyMutex ((NativeMutex) sem);
}

int
LockMutex (Mutex sem)
{
	return NativeLockMutex ((NativeMutex) sem);
}

void
UnlockMutex (Mutex sem)
{
	NativeUnlockMutex ((NativeMutex) sem);
}

CondVar
CreateCondVar ()
{
	return NativeCreateCondVar ();
}
void DestroyCondVar (CondVar cv)
{
	NativeDestroyCondVar (cv);
}

void WaitCondVar (CondVar cv)
{
	NativeWaitCondVar (cv);
}

void SignalCondVar (CondVar cv)
{
	NativeSignalCondVar (cv);
}

void BroadcastCondVar (CondVar cv)
{
	NativeBroadcastCondVar (cv);
}
