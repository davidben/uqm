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

#include "sdlthreads.h"

#if defined(PROFILE_THREADS) && !defined(WIN32)
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef PROFILE_THREADS
void
SDLWrapper_PrintThreadStats (SDL_Thread *thread) {
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
#endif

void
SDLWrapper_SleepThreadUntil (TimeCount wakeTime) {
	TimeCount now;

	now = GetTimeCounter ();
	if (wakeTime <= now)
		SDLWrapper_TaskSwitch ();
	else
		SDL_Delay ((wakeTime - now) * 1000 / ONE_SECOND);
}

int
SDLWrapper_TimeoutSetSemaphore (Semaphore sem, TimePeriod timeout) {
	return SDL_SemWaitTimeout (sem, timeout * 1000 / ONE_SECOND);
}

void
SDLWrapper_TaskSwitch (void) {
	SDL_Delay (1);
}

void
SDLWrapper_WaitCondVar (CondVar cv) {
	int result;
	Mutex temp = CreateMutex ();
	LockMutex (temp);
	result = SDL_CondWait (cv, temp);
	UnlockMutex (temp);
	DestroyMutex (temp);
	if (result != 0) {
		fprintf (stderr, "Error result from SDL_CondWait: %d\n", result);
	}
}
