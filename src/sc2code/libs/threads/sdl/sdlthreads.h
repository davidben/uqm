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

#define NativeGetThreadID(thread) SDL_GetThreadID ((thread))
#define NativeThreadID() SDL_ThreadID ()

typedef SDL_mutex *NativeMutex;
#define NativeCreateMutex() \
		SDL_CreateMutex ()
#define NativeDestroyMutex(mutex) \
		SDL_DestroyMutex ((mutex))
#define NativeLockMutex(mutex) \
		SDL_mutexP ((mutex))
#define NativeUnlockMutex(mutex) \
		SDL_mutexV ((mutex))

#define NativeCurrentThreadID() \
		SDL_ThreadID ()

void InitThreadSystem_SDL (void);
void UnInitThreadSystem_SDL (void);

#ifdef NAMED_SYNCHRO
/* Prototypes with the "name" field */
Thread CreateThread_SDL (ThreadFunction func, void *data, SDWORD stackSize, const char *name);
Semaphore CreateSemaphore_SDL (DWORD initial, const char *name);
RecursiveMutex CreateRecursiveMutex_SDL (const char *name);
CondVar CreateCondVar_SDL (const char *name);
#else
/* Prototypes without the "name" field. */
Thread CreateThread_SDL (ThreadFunction func, void *data, SDWORD stackSize);
Semaphore CreateSemaphore_SDL (DWORD initial);
RecursiveMutex CreateRecursiveMutex_SDL (void);
CondVar CreateCondVar_SDL (void);
#endif

void SleepThread_SDL (TimeCount sleepTime);
void SleepThreadUntil_SDL (TimeCount wakeTime);
void TaskSwitch_SDL (void);
void WaitThread_SDL (Thread thread, int *status);

void DestroySemaphore_SDL (Semaphore sem);
void SetSemaphore_SDL (Semaphore sem);
void ClearSemaphore_SDL (Semaphore sem);

void DestroyCondVar_SDL (CondVar c);
void WaitCondVar_SDL (CondVar c);
void WaitProtectedCondVar_SDL (CondVar c, Mutex m);
void SignalCondVar_SDL (CondVar c);
void BroadcastCondVar_SDL (CondVar c);

void DestroyRecursiveMutex_SDL (RecursiveMutex m);
void LockRecursiveMutex_SDL (RecursiveMutex m);
void UnlockRecursiveMutex_SDL (RecursiveMutex m);
int  GetRecursiveMutexDepth_SDL (RecursiveMutex m);

#define NativeInitThreadSystem InitThreadSystem_SDL
#define NativeUnInitThreadSystem UnInitThreadSystem_SDL

#define NativeCreateThread CreateThread_SDL
#define NativeSleepThread SleepThread_SDL
#define NativeSleepThreadUntil SleepThreadUntil_SDL
#define NativeTaskSwitch TaskSwitch_SDL
#define NativeWaitThread WaitThread_SDL

#define NativeCreateSemaphore CreateSemaphore_SDL
#define NativeDestroySemaphore DestroySemaphore_SDL
#define NativeSetSemaphore SetSemaphore_SDL
#define NativeClearSemaphore ClearSemaphore_SDL

#define NativeCreateCondVar CreateCondVar_SDL
#define NativeDestroyCondVar DestroyCondVar_SDL
#define NativeWaitCondVar WaitCondVar_SDL
#define NativeWaitProtectedCondVar WaitProtectedCondVar_SDL
#define NativeSignalCondVar SignalCondVar_SDL
#define NativeBroadcastCondVar BroadcastCondVar_SDL

#define NativeCreateRecursiveMutex CreateRecursiveMutex_SDL
#define NativeDestroyRecursiveMutex DestroyRecursiveMutex_SDL
#define NativeLockRecursiveMutex LockRecursiveMutex_SDL
#define NativeUnlockRecursiveMutex UnlockRecursiveMutex_SDL
#define NativeGetRecursiveMutexDepth GetRecursiveMutexDepth_SDL

#endif  /* _SDLTHREAD_H */

