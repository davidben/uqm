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

#ifndef _SDLTHREAD_H
#define _SDLTHREAD_H

#include "SDL.h"
#include "SDL_thread.h"
#include "libs/threadlib.h"
#include "libs/timelib.h"

typedef SDL_Thread *NativeThread;
typedef int (*NativeThreadFunction) (void *);
#define NativeInitThreadSystem()		
#define NativeUnInitThreadSystem()		
#define NativeCreateThread(func, data, stackSize) \
		SDL_CreateThread ((func), (data))
#define NativeKillThread(thread) \
		SDL_KillThread ((thread))
extern void SDLWrapper_SleepThread (TimeCount sleepTime);
#define NativeSleepThread(sleepTime) \
		SDLWrapper_SleepThread ((sleepTime))
extern void SDLWrapper_SleepThreadUntil (TimeCount wakeTime);
#define NativeSleepThreadUntil(wakeTime) \
		SDLWrapper_SleepThreadUntil ((wakeTime))
extern void SDLWrapper_TaskSwitch (void);
#define NativeTaskSwitch() \
		SDLWrapper_TaskSwitch()
#define NativeWaitThread(thread, status) \
		SDL_WaitThread ((thread), (status))
#define NativeGetThreadID(thread) SDL_GetThreadID ((thread))
#define NativeThreadID() SDL_ThreadID ()
#ifdef PROFILE_THREADS
extern void SDLWrapper_PrintThreadStats (SDL_Thread *thread);
#define NativePrintThreadStats(thread) \
		SDLWrapper_PrintThreadStats ((thread))
#endif
#define NativeThreadOk(thread) \
		((thread) != NULL)

typedef SDL_sem *NativeSemaphore;
#define NativeCreateSemaphore(initial) \
		SDL_CreateSemaphore ((initial))
#define NativeDestroySemaphore(sem) \
		SDL_DestroySemaphore ((sem))
#define NativeSetSemaphore(sem) \
		SDL_SemWait ((sem))
#define NativeTrySetSemaphore(sem) \
		SDL_SemTryWait ((sem))
#define NativeSemValue(sem) \
		SDL_SemValue ((sem))
#define NATIVE_MUTEX_TIMEOUT SDL_MUTEX_TIMEDOUT
extern int SDLWrapper_TimeoutSetSemaphore (Semaphore sem,
		TimePeriod timeperiod);
#define NativeTimeoutSetSemaphore(sem, timeperiod) \
		SDLWrapper_TimeoutSetSemaphore ((sem), timeperiod)
#define NativeClearSemaphore(sem) \
		SDL_SemPost ((sem))

typedef SDL_mutex *NativeMutex;
#define NativeCreateMutex() \
		SDL_CreateMutex ()
#define NativeDestroyMutex(mutex) \
		SDL_DestroyMutex ((mutex))
#define NativeLockMutex(mutex) \
		SDL_mutexP ((mutex))
#define NativeUnlockMutex(mutex) \
		SDL_mutexV ((mutex))

typedef SDL_cond *NativeCondVar;
#define NativeCreateCondVar() \
		SDL_CreateCond ()
#define NativeDestroyCondVar(condvar) \
		SDL_DestroyCond ((condvar))
extern void SDLWrapper_WaitCondVar (CondVar candvar);
#define NativeWaitCondVar(condvar) \
		SDLWrapper_WaitCondVar ((condvar))
#define NativeSignalCondVar(condvar) \
		SDL_CondSignal ((condvar))
#define NativeBroadcastCondVar(condvar) \
		SDL_CondBroadcast ((condvar))

#define NativeCurrentThreadID() \
		SDL_ThreadID ()

#endif  /* _SDLTHREAD_H */

