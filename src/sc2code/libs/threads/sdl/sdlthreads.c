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

/* By Serge van den Boom
 */

#include <stdlib.h>
#include "misc.h"
#include "sdlthreads.h"
#ifdef PROFILE_THREADS
#include <signal.h>
#include <unistd.h>
#endif

#if defined(PROFILE_THREADS) && !defined(WIN32)
#include <sys/time.h>
#include <sys/resource.h>
#endif

typedef struct _thread {
	void *native;
#ifdef NAMED_SYNCHRO
	const char *name;
#endif
#ifdef PROFILE_THREADS
	int startTime;
#endif  /*  PROFILE_THREADS */
	struct _thread *next;
} *TrueThread;

static volatile TrueThread threadQueue = NULL;
static SDL_mutex *threadQueueMutex;

struct ThreadStartInfo
{
	ThreadFunction func;
	void *data;
	SDL_sem *sem;
	TrueThread thread;
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

static void
LocalStats (SDL_Thread *thread) {
#if defined (WIN32) || !defined(SDL_PTHREADS)
	fprintf (stderr, "Thread ID %u\n", SDL_GetThreadID (thread));
#else  /* !defined (WIN32) && defined(SDL_PTHREADS) */
	// This only works if SDL implements threads as processes
	pid_t pid;
	struct rusage ru;
	long seconds;

	pid = (pid_t) SDL_GetThreadID (thread);
	fprintf (stderr, "Pid %d\n", (int) pid);
	getrusage(RUSAGE_SELF, &ru);
	seconds = ru.ru_utime.tv_sec + ru.ru_utime.tv_sec;
	fprintf (stderr, "Used %ld.%ld minutes of processor time.\n",
			seconds / 60, seconds % 60);
#endif  /* defined (WIN32) && defined(SDL_PTHREADS) */
}

void
PrintThreadsStats_SDL (void)
{
	TrueThread ptr;
	int now;
	
	now = GetTimeCounter ();
	SDL_mutexP (threadQueueMutex);
	fprintf(stderr, "--- Active threads ---\n");
	for (ptr = threadQueue; ptr != NULL; ptr = ptr->next) {
		fprintf (stderr, "Thread named '%s'.\n", ptr->name);
		fprintf (stderr, "Started %d.%d minutes ago.\n",
				(now - ptr->startTime) / 60000,
				((now - ptr->startTime) / 1000) % 60);
		LocalStats (ptr->native);
		if (ptr->next != NULL)
			fprintf(stderr, "\n");
	}
	SDL_mutexV (threadQueueMutex);
	fprintf(stderr, "----------------------\n");
	fflush (stderr);
}
#endif  /* PROFILE_THREADS */

void
InitThreadSystem_SDL (void)
{
	threadQueueMutex = SDL_CreateMutex ();
#ifdef PROFILE_THREADS
	signal(SIGUSR1, SigUSR1Handler);
#endif
	init_cond_bank ();
}

void
UnInitThreadSystem_SDL (void)
{
	uninit_cond_bank ();
#ifdef PROFILE_THREADS
	signal(SIGUSR1, SIG_DFL);
#endif
	SDL_DestroyMutex (threadQueueMutex);
}

static void
QueueThread (TrueThread thread)
{
	SDL_mutexP (threadQueueMutex);
	thread->next = threadQueue;
	threadQueue = thread;
	SDL_mutexV (threadQueueMutex);
}

static void
UnQueueThread (TrueThread thread)
{
	volatile TrueThread *ptr;

	ptr = &threadQueue;
	SDL_mutexP (threadQueueMutex);
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
	SDL_mutexV (threadQueueMutex);
}

static TrueThread
FindThreadInfo (Uint32 threadID)
{
	TrueThread ptr;

	ptr = threadQueue;
	SDL_mutexP (threadQueueMutex);
	while (ptr)
	{
		if (SDL_GetThreadID (ptr->native) == threadID)
		{
			SDL_mutexV (threadQueueMutex);
			return ptr;
		}
		ptr = ptr->next;
	}
	SDL_mutexV (threadQueueMutex);
	return NULL;
}

#ifdef NAMED_SYNCHRO
static const char *
MyThreadName (void)
{
	TrueThread t = FindThreadInfo (SDL_ThreadID ());
	return t ? t->name : "Unknown (probably renderer)";
}
#endif

static int
ThreadHelper (void *startInfo) {
	ThreadFunction func;
	void *data;
	SDL_sem *sem;
	TrueThread thread;
	int result;
	
	func = ((struct ThreadStartInfo *) startInfo)->func;
	data = ((struct ThreadStartInfo *) startInfo)->data;
	sem  = ((struct ThreadStartInfo *) startInfo)->sem;

	// Wait until the Thread structure is available.
	SDL_SemWait (sem);
	SDL_DestroySemaphore (sem);
	thread = ((struct ThreadStartInfo *) startInfo)->thread;
	HFree (startInfo);

	result = (*func) (data);

#ifdef DEBUG_THREADS
	fprintf (stderr, "Thread '%s' done (returned %d).\n",
			thread->name, result);
	fflush (stderr);
#endif

	UnQueueThread (thread);

	HFree (thread);
	return result;
}

Thread
CreateThread_SDL (ThreadFunction func, void *data, SDWORD stackSize
#ifdef NAMED_SYNCHRO
		  , const char *name
#endif
	)
{
	TrueThread thread;
	struct ThreadStartInfo *startInfo;
	
	thread = (struct _thread *) HMalloc (sizeof *thread);
#ifdef NAMED_SYNCHRO
	thread->name = name;
#endif
#ifdef PROFILE_THREADS
	thread->startTime = GetTimeCounter ();
#endif

	startInfo = (struct ThreadStartInfo *) HMalloc (sizeof (*startInfo));
	startInfo->func = func;
	startInfo->data = data;
	startInfo->sem = SDL_CreateSemaphore (0);
	startInfo->thread = thread;
	
	thread->native = SDL_CreateThread (ThreadHelper, (void *) startInfo);
	if (!(thread->native))
	{
		HFree (startInfo);
		HFree (thread);
		return NULL;
	}
	// The responsibility to free 'startInfo' and 'thread' is now by the new
	// thread.
	
	QueueThread (thread);

#ifdef DEBUG_THREADS
	fprintf (stderr, "Thread '%s' created.\n", ThreadName (thread));
	fflush (stderr);
#endif

	// Signal to the new thread that the thread structure is ready
	// and it can begin to use it.
	SDL_SemPost (startInfo->sem);

	(void) stackSize;  /* Satisfying compiler (unused parameter) */
	return thread;
}

void
SleepThread_SDL (TimeCount sleepTime)
{
	SDL_Delay (sleepTime * 1000 / ONE_SECOND);
}

void
SleepThreadUntil_SDL (TimeCount wakeTime) {
	TimeCount now;

	now = GetTimeCounter ();
	if (wakeTime <= now)
		TaskSwitch_SDL ();
	else
		SDL_Delay ((wakeTime - now) * 1000 / ONE_SECOND);
}

void
TaskSwitch_SDL (void) {
	SDL_Delay (1);
}

void
WaitThread_SDL (Thread thread, int *status) {
	SDL_WaitThread (((TrueThread)thread)->native, status);
}

/* These are the SDL implementations of the UQM synchronization objects. */
/* TODO: Remove the names of the following data types when compiling
 * under release mode. */

/* Mutexes. */
/* TODO.  The w_memlib uses Mutexes right now, so we can't use HMalloc
 * or HFree. */

/* Semaphores. */

typedef struct _sem {
	SDL_sem *sem;
#ifdef NAMED_SYNCHRO
	const char *name;
#endif
} Sem;

Semaphore
CreateSemaphore_SDL (DWORD initial
#ifdef NAMED_SYNCHRO
		  , const char *name
#endif
	)
{
	Sem *sem = (Sem *) HMalloc (sizeof (struct _sem));
#ifdef NAMED_SYNCHRO
	sem->name = name;
#endif
	sem->sem = SDL_CreateSemaphore (initial);
	return sem;
}

void
DestroySemaphore_SDL (Semaphore s)
{
	Sem *sem = (Sem *)s;
	SDL_DestroySemaphore (sem->sem);
	HFree (sem);
}

void
SetSemaphore_SDL (Semaphore s)
{
	Sem *sem = (Sem *)s;
#ifdef TRACK_CONTENTION
	BOOLEAN contention = !(SDL_SemValue (sem->sem));
	if (contention)
	{
		fprintf (stderr, "Thread '%s' goes to sleep, waiting on semaphore '%s'\n", MyThreadName (), sem->name);
	}
#endif
	while (SDL_SemWait (sem->sem) == -1)
	{
		TaskSwitch_SDL ();
	}
#ifdef TRACK_CONTENTION
	if (contention)
	{
		fprintf (stderr, "Thread '%s' awakens, released from semaphore '%s'\n", MyThreadName (), sem->name);
	}
#endif
}

void
ClearSemaphore_SDL (Semaphore s)
{
	Sem *sem = (Sem *)s;
	while (SDL_SemPost (sem->sem) == -1)
	{
		TaskSwitch_SDL ();
	}
}

/* Recursive mutexes. Adapted from mixSDL code, which was adapted from
   the original DCQ code. */

typedef struct _recm {
	SDL_mutex *mutex;
	Uint32 thread_id;
	Uint32 locks;
#ifdef NAMED_SYNCHRO
	const char *name;
#endif
} RecM;

RecursiveMutex
#ifdef NAMED_SYNCHRO
CreateRecursiveMutex_SDL (const char *name)
#else
CreateRecursiveMutex_SDL (void)
#endif
{
	RecM *mtx = (RecM *) HMalloc (sizeof (struct _recm));

	mtx->thread_id = 0;
	mtx->mutex = SDL_CreateMutex ();
#ifdef NAMED_SYNCHRO
	mtx->name = name;
#endif
	mtx->locks = 0;
	return (RecursiveMutex) mtx;
}

void
DestroyRecursiveMutex_SDL (RecursiveMutex val)
{
	RecM *mtx = (RecM *)val;
	SDL_DestroyMutex (mtx->mutex);
	HFree (mtx);
}

void
LockRecursiveMutex_SDL (RecursiveMutex val)
{
	RecM *mtx = (RecM *)val;
	Uint32 thread_id = SDL_ThreadID();
	if (mtx->thread_id != thread_id)
	{
#ifdef TRACK_CONTENTION
		if (mtx->thread_id)
		{
			fprintf (stderr, "Thread '%s' blocking on '%s'\n", MyThreadName (), mtx->name);
		}
#endif
		while (SDL_mutexP (mtx->mutex))
			TaskSwitch_SDL ();
		mtx->thread_id = thread_id;
	}
	mtx->locks++;
}

void
UnlockRecursiveMutex_SDL (RecursiveMutex val)
{
	RecM *mtx = (RecM *)val;
	Uint32 thread_id = SDL_ThreadID();
	if (mtx->thread_id != thread_id)
	{
#ifdef NAMED_SYNCHRO
		fprintf (stderr, "'%s' attempted to unlock %s when it didn't hold it\n", MyThreadName (), mtx->name);
#endif
	}
	else
	{
		mtx->locks--;
		if (!mtx->locks)
		{
			mtx->thread_id = 0;
			SDL_mutexV (mtx->mutex);
		}
	}
}

int
GetRecursiveMutexDepth_SDL (RecursiveMutex val)
{
	RecM *mtx = (RecM *)val;
	return mtx->locks;
}

typedef struct _cond {
	SDL_cond *cond;
	SDL_mutex *mutex;
#ifdef NAMED_SYNCHRO
	const char *name;
#endif
} cvar;

CondVar
#ifdef NAMED_SYNCHRO
CreateCondVar_SDL (const char *name)
#else
CreateCondVar_SDL (void)
#endif
{
	cvar *cv = (cvar *) HMalloc (sizeof (cvar));
	cv->cond = SDL_CreateCond ();
	cv->mutex = SDL_CreateMutex ();
#ifdef NAMED_SYNCHRO
	cv->name = name;
#endif
	return cv;
}

void
DestroyCondVar_SDL (CondVar c)
{
	cvar *cv = (cvar *) c;
	SDL_DestroyCond (cv->cond);
	SDL_DestroyMutex (cv->mutex);
	HFree (cv);
}

void
WaitCondVar_SDL (CondVar c)
{
	cvar *cv = (cvar *) c;
	SDL_mutexP (cv->mutex);
#ifdef TRACK_CONTENTION
	fprintf (stderr, "Thread '%s' waiting for signal from '%s'\n", MyThreadName (), cv->name);
#endif
	while (SDL_CondWait (cv->cond, cv->mutex) != 0)
	{
		TaskSwitch_SDL ();
	}
#ifdef TRACK_CONTENTION
	fprintf (stderr, "Thread '%s' received signal from '%s', awakening.\n", MyThreadName (), cv->name);
#endif
	SDL_mutexV (cv->mutex);
}

void
WaitProtectedCondVar_SDL (CondVar c, Mutex m)
{
	cvar *cv = (cvar *) c;
#ifdef TRACK_CONTENTION
	fprintf (stderr, "Thread '%s' waiting for signal from '%s'\n", MyThreadName (), cv->name);
#endif
	if (SDL_CondWait (cv->cond, m) != 0) {
		TaskSwitch_SDL ();
	}	
#ifdef TRACK_CONTENTION
	fprintf (stderr, "Thread '%s' received signal from '%s', awakening.\n", MyThreadName (), cv->name);
#endif
}

void
SignalCondVar_SDL (CondVar c)
{
	cvar *cv = (cvar *) c;
	SDL_CondSignal (cv->cond);
}

void
BroadcastCondVar_SDL (CondVar c)
{
	cvar *cv = (cvar *) c;
	SDL_CondBroadcast (cv->cond);
}
