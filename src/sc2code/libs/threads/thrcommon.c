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

void
InitThreadSystem (void)
{
	NativeInitThreadSystem ();
}

void
UnInitThreadSystem (void)
{
	NativeUnInitThreadSystem ();
}

/* The Create routines look different based on whether NAMED_SYNCHRO
   is defined or not. */

#ifdef NAMED_SYNCHRO
Thread
CreateThread_Core (ThreadFunction func, void *data, SDWORD stackSize, const char *name)
{
	return NativeCreateThread (func, data, stackSize, name);
}

Mutex
CreateMutex_Core (const char *name, DWORD syncClass)
{
	return NativeCreateMutex (name, syncClass);
}

Semaphore
CreateSemaphore_Core (DWORD initial, const char *name, DWORD syncClass)
{
	return NativeCreateSemaphore (initial, name, syncClass);
}

RecursiveMutex
CreateRecursiveMutex_Core (const char *name, DWORD syncClass)
{
	return NativeCreateRecursiveMutex (name, syncClass);
}

CondVar
CreateCondVar_Core (const char *name, DWORD syncClass)
{
	return NativeCreateCondVar (name, syncClass);
}

#else
/* These are the versions of Create* without the names. */
Thread
CreateThread_Core (ThreadFunction func, void *data, SDWORD stackSize)
{
	return NativeCreateThread (func, data, stackSize);
}

Mutex
CreateMutex_Core (void)
{
	return NativeCreateMutex ();
}

Semaphore
CreateSemaphore_Core (DWORD initial)
{
	return NativeCreateSemaphore (initial);
}

RecursiveMutex
CreateRecursiveMutex_Core (void)
{
	return NativeCreateRecursiveMutex ();
}

CondVar
CreateCondVar_Core (void)
{
	return NativeCreateCondVar ();
}
#endif

ThreadLocal *
CreateThreadLocal (void)
{
	ThreadLocal *tl = HMalloc (sizeof (ThreadLocal));
	tl->flushSem = CreateSemaphore (0, "FlushGraphics", SYNC_CLASS_VIDEO);
	return tl;
}

void
DestroyThreadLocal (ThreadLocal *tl)
{
	DestroySemaphore (tl->flushSem);
	HFree (tl);
}

ThreadLocal *
GetMyThreadLocal (void)
{
	return NativeGetMyThreadLocal ();
}

void
WaitThread (Thread thread, int *status)
{
	NativeWaitThread (thread, status);
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

void
DestroyMutex (Mutex sem)
{
	NativeDestroyMutex (sem);
}

void
LockMutex (Mutex sem)
{
	NativeLockMutex (sem);
}

void
UnlockMutex (Mutex sem)
{
	NativeUnlockMutex (sem);
}

void
DestroySemaphore (Semaphore sem)
{
	NativeDestroySemaphore (sem);
}

void
SetSemaphore (Semaphore sem)
{
	NativeSetSemaphore (sem);
}

void
ClearSemaphore (Semaphore sem)
{
	NativeClearSemaphore (sem);
}

void
DestroyCondVar (CondVar cv)
{
	NativeDestroyCondVar (cv);
}

void
WaitCondVar (CondVar cv)
{
	NativeWaitCondVar (cv);
}

void
SignalCondVar (CondVar cv)
{
	NativeSignalCondVar (cv);
}

void
BroadcastCondVar (CondVar cv)
{
	NativeBroadcastCondVar (cv);
}

void
DestroyRecursiveMutex (RecursiveMutex mutex)
{
	NativeDestroyRecursiveMutex (mutex);
}

void
LockRecursiveMutex (RecursiveMutex mutex)
{
	NativeLockRecursiveMutex (mutex);
}

void
UnlockRecursiveMutex (RecursiveMutex mutex)
{
	NativeUnlockRecursiveMutex (mutex);
}

int
GetRecursiveMutexDepth (RecursiveMutex mutex)
{
	return NativeGetRecursiveMutexDepth (mutex);
}
