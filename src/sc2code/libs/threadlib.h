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

/*
This is now a compile-time define
#define DEBUG_TRACK_SEM
*/

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
Thread CreateThreadAux (ThreadFunction func, void *data,
		SDWORD stackSize, const char *name);
#	define CreateThread(func, data, stackSize, name) \
		CreateThreadAux ((func), (data), (stackSize), (name))
#else  /* !defined(THREAD_NAMES) */
Thread CreateThreadAux (ThreadFunction func, void *data,
		SDWORD stackSize);
#	define CreateThread(func, data, stackSize, name) \
		CreateThreadAux ((func), (data), (stackSize))
#endif  /* !defined(THREAD_NAMES) */
void SleepThread (TimePeriod timePeriod);
void SleepThreadUntil (TimeCount wakeTime);
void TaskSwitch (void);
void WaitThread (Thread thread, int *status);

typedef void *Semaphore;
#ifdef DEBUG_TRACK_SEM
Semaphore CreateSemaphoreAux (DWORD initial, const char *sem_name);
#	define CreateSemaphore(initial,sem_name) \
		CreateSemaphoreAux ((initial), (sem_name))
void ResetSemaphoreOwnerAux (Semaphore sem);
#	define ResetSemaphoreOwner(sem_name) \
		ResetSemaphoreOwnerAux (sem_name)
#else
Semaphore CreateSemaphoreAux (DWORD initial);
#	define CreateSemaphore(initial,sem_name) \
		CreateSemaphoreAux ((initial))
#	define ResetSemaphoreOwner(sem_name)
#endif
DWORD SemaphoreValue (Semaphore sem);
void DestroySemaphore (Semaphore sem);
int SetSemaphore (Semaphore sem);
int TrySetSemaphore (Semaphore sem);
int TimeoutSetSemaphore (Semaphore sem, TimePeriod timeout);
void ClearSemaphore (Semaphore sem);
#ifdef PROFILE_THREADS
void PrintThreadsStats (void);
#endif  /* PROFILE_THREADS */

typedef void *Mutex;
Mutex CreateMutex (void);
void DestroyMutex (Mutex sem);
int LockMutex (Mutex sem);
void UnlockMutex (Mutex sem);

typedef void *RecursiveMutex;
RecursiveMutex CreateRecursiveMutex (const char *name);
void DestroyRecursiveMutex (RecursiveMutex m);
void LockRecursiveMutex (RecursiveMutex m);
void UnlockRecursiveMutex (RecursiveMutex m);
int  GetRecursiveMutexDepth (RecursiveMutex m);

typedef void *CrossThreadMutex;
CrossThreadMutex CreateCrossThreadMutex (const char *name);
void DestroyCrossThreadMutex (CrossThreadMutex ctm);
int LockCrossThreadMutex (CrossThreadMutex ctm);
void UnlockCrossThreadMutex (CrossThreadMutex ctm);

typedef void *CondVar;
CondVar CreateCondVar (void);
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

