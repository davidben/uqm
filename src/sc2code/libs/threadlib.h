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

#define NAMED_SYNCHRO           /* Should synchronizable objects have names? */
// #define TRACK_CONTENTION       /* Should we report when a thread sleeps on synchronize? */

#ifdef TRACK_CONTENTION
#	ifndef NAMED_SYNCHRO
#		define NAMED_SYNCHRO
#	endif
#endif /* TRACK_CONTENTION */

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

#if defined (DEBUG_TRACK_SEM)
#	if !defined (THREAD_QUEUE)
#		define THREAD_QUEUE
#	endif
#	if !defined (THREAD_NAMES)
#		define THREAD_NAMES
#	endif
#endif

void InitThreadSystem (void);
void UnInitThreadSystem (void);
void init_cond_bank (void);
void uninit_cond_bank (void);

typedef int (*ThreadFunction) (void *);

typedef void *Thread;
typedef void *Mutex;
typedef void *Semaphore;
typedef void *RecursiveMutex;
typedef void *CondVar;

#ifdef NAMED_SYNCHRO
/* Prototypes with the "name" field */

Thread CreateThread_Core (ThreadFunction func, void *data, SDWORD stackSize, const char *name);
Semaphore CreateSemaphore_Core (DWORD initial, const char *name);
Mutex CreateMutex_Core (void);
RecursiveMutex CreateRecursiveMutex_Core (const char *name);
CondVar CreateCondVar_Core (const char *name);

/* Preprocessor directives to forward to the appropriate routines */

#define CreateThread(func, data, stackSize, name) \
	CreateThread_Core ((func), (data), (stackSize), (name))
#define CreateSemaphore(initial, name) \
	CreateSemaphore_Core ((initial), (name))
#define CreateMutex() \
	CreateMutex_Core ()
#define CreateRecursiveMutex(name) \
	CreateRecursiveMutex_Core((name))
#define CreateCondVar(name) \
	CreateCondVar_Core ((name))

#else

/* Prototypes without the "name" field. */
Thread CreateThread_Core (ThreadFunction func, void *data, SDWORD stackSize);
Semaphore CreateSemaphore_Core (DWORD initial);
Mutex CreateMutex_Core (void);
RecursiveMutex CreateRecursiveMutex_Core (void);
CondVar CreateCondVar_Core (void);


/* Preprocessor directives to forward to the appropriate routines.
   The "name" field is stripped away in preprocessing. */

#define CreateThread(func, data, stackSize, name) \
	CreateThread_Core ((func), (data), (stackSize))
#define CreateSemaphore(initial, name) \
	CreateSemaphore_Core ((initial))
#define CreateMutex() \
	CreateMutex_Core ()
#define CreateRecursiveMutex(name) \
	CreateRecursiveMutex_Core()
#define CreateCondVar(name) \
	CreateCondVar_Core ()

#endif

void SleepThread (TimePeriod timePeriod);
void SleepThreadUntil (TimeCount wakeTime);
void TaskSwitch (void);
void WaitThread (Thread thread, int *status);

#ifdef PROFILE_THREADS
void PrintThreadsStats (void);
#endif  /* PROFILE_THREADS */


void DestroySemaphore (Semaphore sem);
void SetSemaphore (Semaphore sem);
void ClearSemaphore (Semaphore sem);

void DestroyMutex (Mutex sem);
void LockMutex (Mutex sem);
void UnlockMutex (Mutex sem);

void DestroyRecursiveMutex (RecursiveMutex m);
void LockRecursiveMutex (RecursiveMutex m);
void UnlockRecursiveMutex (RecursiveMutex m);
int  GetRecursiveMutexDepth (RecursiveMutex m);

void DestroyCondVar (CondVar);
void WaitCondVar (CondVar);
void WaitProtectedCondVar (CondVar, Mutex);
void SignalCondVar (CondVar);
void BroadcastCondVar (CondVar);

DWORD CurrentThreadID (void);

int  FindSignalChannel ();
void WaitForSignal (int);
void SignalThread (DWORD);

#endif  /* _THREADLIB_H */

