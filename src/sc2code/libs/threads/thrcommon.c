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

#ifdef DEBUG_TRACK_SEM
#include <string.h>
// The semaphore tracker looks for possible semaphore issues.
// It will report semaphores cleared by threads other than what set them
// and when the semaphore value is larger than 1
#define NUM_SEMAPHORES 50
// Set the timeout to 60 seconds
#define SEM_TIMEOUT 6000
#undef DEBUG_SEM_DEADLOCK
typedef struct {
	Semaphore Sem;
	Uint32 Thread;
	char Name[20];
#if defined (THREAD_QUEUE) && defined (THREAD_NAMES)
	char ThreadName[20];
#endif
}  MonitorSem;
Semaphore SemMutex;
static MonitorSem SemMon[NUM_SEMAPHORES];
static UWORD numSems = 0;
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
	threadQueueSemaphore = CreateSemaphore (1, "ThreadQueue");
#endif  /* THREAD_QUEUE */
#ifdef PROFILE_THREADS
	signal(SIGUSR1, SigUSR1Handler);
#endif
	NativeInitThreadSystem ();
	init_cond_bank ();
}

void
UnInitThreadSystem (void)
{
	uninit_cond_bank ();
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

#if defined (DEBUG_TRACK_SEM) && defined (THREAD_QUEUE) \
	&& defined (THREAD_NAMES)
static char *ThreadNameNative (Uint32 native)
{
	volatile Thread ptr;

	if (threadQueueSemaphore)
		NativeSetSemaphore (threadQueueSemaphore);
	ptr = threadQueue;
	while (ptr && NativeGetThreadID (ptr->native) != native)
	{
		ptr = ptr->next;
	}
	if(threadQueueSemaphore)
		NativeClearSemaphore (threadQueueSemaphore);
	if (ptr)
		return ((char *)ptr->name);
	else
		return (NULL);
}
#endif  /* DEBUG_TRACK_SEM */

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
	startInfo->sem = CreateSemaphore (0, "StartThread");
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
CreateSemaphoreAux (DWORD initial
#ifdef DEBUG_TRACK_SEM
				 , const char *sem_name
#endif
				 )
{
	Semaphore sem = (Semaphore)NativeCreateSemaphore (initial);
#ifdef DEBUG_TRACK_SEM
	int pos;
	if (SemMutex == 0)
		SemMutex = NativeCreateSemaphore (0);
	else
		NativeSetSemaphore (SemMutex);
	for (pos = 0; pos < numSems; pos++)
		if (SemMon[pos].Sem == 0)
			break;
	if (pos == numSems) 
	{
		numSems++;
		if (numSems == NUM_SEMAPHORES)
		{
			fprintf(stderr, "Error: We ran out of semaphores.  aborting!\n");
			NativeClearSemaphore (SemMutex);
			return (0);
		}
	}
	SemMon[pos].Sem = sem;
	SemMon[pos].Thread = 0;
	strncpy(SemMon[pos].Name, sem_name, 20);
	SemMon[pos].Name[19] = 0;
#if defined (THREAD_QUEUE) && defined (THREAD_NAMES)
//	fprintf (stderr, "Created Semaphore  # %d: %s in thread '%s'\n", 
//		numSems, SemMon[pos].Name, ThreadNameNative (NativeThreadID ()));
#else
//	fprintf (stderr, "Created Semaphore  # %d: %s\n", 
//		numSems, SemMon[pos].Name);
#endif
	NativeClearSemaphore (SemMutex);
#endif
	return (sem);
}

void
DestroySemaphore (Semaphore sem)
{
#ifdef DEBUG_TRACK_SEM
	int i;
	NativeSetSemaphore (SemMutex);
	for (i = 0; i <numSems; i++)
		if (SemMon[i].Sem == sem)
		{
			SemMon[i].Sem = 0;
			break;
		}
	NativeClearSemaphore (SemMutex);
#endif
	NativeDestroySemaphore ((NativeSemaphore) sem);
}

#ifdef DEBUG_TRACK_SEM
static void debug_set_sem (Semaphore sem
#if defined (THREAD_QUEUE) && defined (THREAD_NAMES)
	, char *name
#endif
	)
{
	int i;
	for (i = 0; i < numSems; i++)
	{
		if (SemMon[i].Sem == sem)
		{
			SemMon[i].Thread = NativeThreadID ();
#if defined (THREAD_QUEUE) && defined (THREAD_NAMES)
			if (name != NULL)
			{
				strncpy(SemMon[i].ThreadName,name,20);
				SemMon[i].ThreadName[19] = 0;
			}
#endif
			break;
		}
	}
}
#endif

int
SetSemaphore (Semaphore sem)
{
	int i;

#if defined (DEBUG_TRACK_SEM) && defined (THREAD_QUEUE) \
	&& defined (THREAD_NAMES)
	char *name = ThreadNameNative (NativeThreadID ());
#endif

#if defined (DEBUG_TRACK_SEM) && defined (DEBUG_SEM_DEADLOCK)
	do
	{
		i = NativeTimeoutSetSemaphore ((NativeSemaphore) sem, SEM_TIMEOUT);
		if (i == NATIVE_MUTEX_TIMEOUT)
		{
			int j = -1;
			for (j = 0; j < numSems; j++)
			{
				if (SemMon[j].Sem == sem)
					break;
			}
			if (j != -1) 
#if defined (THREAD_QUEUE) && defined (THREAD_NAMES)
				fprintf (stderr, "Failed to acquire '%s' semaphore in thread '%s'.\n\tIt is held by thread '%s'.  Retrying\n",
					SemMon[j].Name,
					name,
					SemMon[j].ThreadName);
#else
				fprintf (stderr, "Failed to acquire '%s' semaphore.  Retrying\n",
					SemMon[j].Name);
#endif /* THREAD_NAMES */
		}
	} while (i == NATIVE_MUTEX_TIMEOUT);
#else
	i = NativeSetSemaphore ((NativeSemaphore) sem);
#endif /* DEBUG_SEM_DEADLOCK */
#ifdef DEBUG_TRACK_SEM
	if (i != 0)
		fprintf(stderr, "WARNING: SetSemaphore did not return 0, this could be bad!\n");
#if defined (THREAD_QUEUE) && defined (THREAD_NAMES)
	debug_set_sem (sem, name);
#else
	debug_set_sem (sem);
#endif /* THREAD_NAMES */
#endif /* DEBUG_TRACK_SEM */
	return i;
}

int
TrySetSemaphore (Semaphore sem)
{
	int i;

#if defined (DEBUG_TRACK_SEM) && defined (THREAD_QUEUE) \
	&& defined (THREAD_NAMES)
	char *name = ThreadNameNative (NativeThreadID ());
#endif

	i = NativeTrySetSemaphore ((NativeSemaphore) sem);
#ifdef DEBUG_TRACK_SEM
	if (i == 0)
#if defined (THREAD_QUEUE) && defined (THREAD_NAMES)
		debug_set_sem (sem, name);
#else
		debug_set_sem (sem);
#endif /* THREAD_NAMES */
#endif /* DEBUG_TRACK_SEM */
	return (i);
}

int
TimeoutSetSemaphore (Semaphore sem, TimePeriod timeout)
{
	int i;

#if defined (DEBUG_TRACK_SEM) && defined (THREAD_QUEUE) \
	&& defined (THREAD_NAMES)
	char *name = ThreadNameNative (NativeThreadID ());
#endif

	i = NativeTimeoutSetSemaphore ((NativeSemaphore) sem, timeout);
#ifdef DEBUG_TRACK_SEM
	if (i == 0)
#if defined (THREAD_QUEUE) && defined (THREAD_NAMES)
		debug_set_sem (sem, name);
#else
		debug_set_sem (sem);
#endif /* THREAD_NAMES */
#endif /* DEBUG_TRACK_SEM */
	return (i);
}

DWORD
SemaphoreValue (Semaphore sem)
{
	return NativeSemValue (sem);
}

#ifdef DEBUG_TRACK_SEM
// Use this function to prevent messages when it  is known that
// a semaphore will be cleared by a different thread than set it
void 
ResetSemaphoreOwnerAux (Semaphore sem)
{
	int i;
	for (i = 0; i < numSems; i++)
		if (SemMon[i].Sem == sem)
		{
			SemMon[i].Thread = 0;
			break;
		}
}
#endif
void
ClearSemaphore (Semaphore sem)
{
#ifdef DEBUG_TRACK_SEM
	int i;
	Uint32 semval = NativeSemValue (sem);
	char *sem_name = NULL;

	for (i = 0; i < numSems; i++)
		if (SemMon[i].Sem == sem)
		{
			sem_name = SemMon[i].Name;
			if (SemMon[i].Thread && SemMon[i].Thread != NativeThreadID ())
#if defined (THREAD_QUEUE) && defined (THREAD_NAMES)
				if (ThreadNameNative (SemMon[i].Thread) == NULL)
					fprintf( stderr, "Freeing %s Semaphore in '%s' set by defunct thread '%s'!\n",
						sem_name, 
						ThreadNameNative (NativeThreadID ()),
						SemMon[i].ThreadName);
				else
					fprintf( stderr, "Freeing %s Semaphore in '%s' set by thread '%s'!\n",
						sem_name, 
						ThreadNameNative (NativeThreadID ()),
						ThreadNameNative (SemMon[i].Thread));
			SemMon[i].ThreadName[0] = 0;
#else
				fprintf (stderr, "Freeing %s Semaphore that was set by a different thread\n",
					sem_name);
#endif
			SemMon[i].Thread = 0;
			break;
		}
	if (semval != 0)
		fprintf (stderr, "Incrementing semaphore '%s' (newvalue=%d)\n", sem_name, semval + 1);
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

DWORD CurrentThreadID ()
{
	return (DWORD)NativeThreadID ();
}
