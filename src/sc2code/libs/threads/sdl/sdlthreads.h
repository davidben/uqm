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
#define NativeSleepThread(ms) \
		SDL_Delay ((ms))
extern void SDLWrapper_SleepThreadUntil (TimeCount wakeTime);
#define NativeSleepThreadUntil(wakeTime) \
		SDLWrapper_SleepThreadUntil ((wakeTime))
extern void SDLWrapper_TaskSwitch (void);
#define NativeTaskSwitch() \
		SDLWrapper_TaskSwitch()
#define NativeWaitThread(thread, status) \
		SDL_WaitThread ((thread), (status))
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
extern int SDLWrapper_TimeoutSetSemaphore (Semaphore sem,
		TimePeriod timeperiod);
#define NativeTimeoutSetSemaphore(sem, timeperiod) \
		SDLWrapper_TimeoutSetSemaphore ((sem), timeperiod)
#define NativeClearSemaphore(sem) \
		SDL_SemPost ((sem))

#endif  /* _SDLTHREAD_H */

