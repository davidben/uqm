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

#ifndef _THREADLIB_H
#define _THREADLIB_H

#define THREADLIB SDL

#ifdef DEBUG
#	ifndef DEBUG_THREADS
#		define DEBUG_THREADS
#	endif
#endif  /* DEBUG */

#ifdef DEBUG_THREADS
#	ifndef THREAD_QUEUE
#		define THREAD_QUEUE
#	endif
#	ifndef THREAD_NAMES
#		define THREAD_NAMES
#	endif
#	ifndef PROFILE_THREADS
#		define PROFILE_THREADS
#	endif
#endif  /* DEBUG_THREADS */

#include <sys/types.h>
#include "libs/timelib.h"

#if defined (PROFILE_THREADS) || defined (DEBUG_THREADS)
#define THREAD_NAMES
#endif

#if defined (PROFILE_THREADS)
#	if !defined (THREAD_QUEUE)
#		define THREAD_QUEUE
#	endif
#endif

extern void InitThreadSystem (void);
extern void UnInitThreadSystem (void);


typedef int (*ThreadFunction) (void *);

typedef struct Thread {
	void *native;
#ifdef THREAD_NAMES
	const char *name;
#endif
#ifdef PROFILE_THREADS
	int startTime;
#endif  /*  PROFILE_THREADS */
#ifdef THREAD_QUEUE
	struct Thread *next;
#endif
} *Thread;

#ifdef THREAD_NAMES
extern Thread CreateThreadAux (ThreadFunction func, void *data,
		SDWORD stackSize, const char *name);
#	define CreateThread(func, data, stackSize, name) \
		CreateThreadAux ((func), (data), (stackSize), (name))
#else  /* !defined(THREAD_NAMES) */
extern Thread CreateThreadAux (ThreadFunction func, void *data,
		SDWORD stackSize);
#	define CreateThread(func, data, stackSize, name) \
		CreateThreadAux ((func), (data), (stackSize))
#endif  /* !defined(THREAD_NAMES) */
extern void SleepThread (TimePeriod timePeriod);
extern void SleepThreadUntil (TimeCount wakeTime);
extern void TaskSwitch (void);
extern void WaitThread (Thread thread, int *status);

typedef void *Semaphore;
extern Semaphore CreateSemaphore (DWORD initial);
extern void DestroySemaphore (Semaphore sem);
extern int SetSemaphore (Semaphore sem);
extern int TrySetSemaphore (Semaphore sem);
extern int TimeoutSetSemaphore (Semaphore sem, TimePeriod timeout);
extern void ClearSemaphore (Semaphore sem);
#ifdef PROFILE_THREADS
extern void PrintThreadsStats (void);
#endif  /* PROFILE_THREADS */

typedef void *Mutex;
extern Mutex CreateMutex (void);
extern void DestroyMutex (Mutex sem);
extern int LockMutex (Mutex sem);
extern void UnlockMutex (Mutex sem);

typedef void *CondVar;
extern CondVar CreateCondVar (void);
extern void DestroyCondVar (CondVar);
extern void WaitCondVar (CondVar);
extern void SignalCondVar (CondVar);
extern void BroadcastCondVar (CondVar);

#endif  /* _THREADLIB_H */

