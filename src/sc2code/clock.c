//Copyright Paul Reiche, Fred Ford. 1992-2002

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

#include <stdlib.h>
#include "gameev.h"
#include "globdata.h"
#include "setup.h"
#include "libs/compiler.h"
#include "libs/gfxlib.h"
#include "libs/tasklib.h"
#include "libs/threadlib.h"
#include "libs/log.h"
#include "libs/misc.h"

// the running of the game-clock is based on game framerates
// *not* on the system (or translated) timer
// and is hard-coded to the original 24 fps
#define CLOCK_BASE_FRAMERATE 24

static int clock_task_func(void* data);

static Mutex clock_mutex;

static BOOLEAN
IsLeapYear (COUNT year)
{
	//     every 4th year      but not 100s          yet still 400s
	return (year & 3) == 0 && ((year % 100) != 0 || (year % 400) == 0);
}

/* month is 1-based: 1=Jan, 2=Feb, etc. */
static BYTE
DaysInMonth (COUNT month, COUNT year)
{
	static const BYTE days_in_month[12] =
	{
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	};

	if (month == 2 && IsLeapYear (year))
		return 29; /* February, leap year */

	return days_in_month[month - 1];
}

static int
clock_task_func(void* data)
{
	BOOLEAN LastPilot;
	DWORD LastTime;
	DWORD cycle_index, delay_count;
	static const COLOR cycle_tab[] =
	{
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x14, 0x18), 0x5B),
		BUILD_COLOR (MAKE_RGB15 (0x06, 0x10, 0x16), 0x5C),
		BUILD_COLOR (MAKE_RGB15 (0x03, 0x0E, 0x14), 0x5D),
		BUILD_COLOR (MAKE_RGB15 (0x02, 0x0C, 0x11), 0x5E),
		BUILD_COLOR (MAKE_RGB15 (0x01, 0x0B, 0x0F), 0x5F),
		BUILD_COLOR (MAKE_RGB15 (0x01, 0x09, 0x0D), 0x60),
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x07, 0x0B), 0x61),
	};
#define NUM_CYCLES (sizeof (cycle_tab) / sizeof (cycle_tab[0]))
#define NUM_DELAYS (ONE_SECOND * 3 / 40) // 9 @ 120 ticks/second
	Task task = (Task) data;

	LastPilot = FALSE;
	LastTime = 0;
	cycle_index = delay_count = 0;
	while (GLOBAL (GameClock).day_in_ticks == 0 && !Task_ReadState (task, TASK_EXIT))
		TaskSwitch ();

	while (!Task_ReadState (task, TASK_EXIT))
	{
		BOOLEAN OnAutoPilot;
		DWORD TimeIn;

				/* use semaphore so that time passage
				 * can be halted. (e.g. during battle
				 * or communications)
				 */
		SetSemaphore (GLOBAL (GameClock.clock_sem));
		TimeIn = GetTimeCounter ();

		if (GLOBAL (GameClock).tick_count <= 0
				&& (GLOBAL (GameClock).tick_count = GLOBAL (GameClock).day_in_ticks) > 0)
		{			
			/* next day -- move the calendar */
			if (++GLOBAL (GameClock).day_index > DaysInMonth (
						GLOBAL (GameClock).month_index,
						GLOBAL (GameClock).year_index))
			{
				GLOBAL (GameClock).day_index = 1;
				if (++GLOBAL (GameClock).month_index > 12)
				{
					GLOBAL (GameClock).month_index = 1;
					++GLOBAL (GameClock).year_index;
				}
			}

			LockMutex (GraphicsLock);
			DrawStatusMessage (NULL_PTR);
			{
				HEVENT hEvent;

				while ((hEvent = GetHeadEvent ()))
				{
					EVENTPTR EventPtr;

					LockEvent (hEvent, &EventPtr);

					if (GLOBAL (GameClock).day_index != EventPtr->day_index
							|| GLOBAL (GameClock).month_index != EventPtr->month_index
							|| GLOBAL (GameClock).year_index != EventPtr->year_index)
					{
						UnlockEvent (hEvent);
						break;
					}
					RemoveEvent (hEvent);
					EventHandler (EventPtr->func_index);

					UnlockEvent (hEvent);
					FreeEvent (hEvent);
				}
			}
			UnlockMutex (GraphicsLock);
		}

		OnAutoPilot = (BOOLEAN)(
				(GLOBAL (autopilot.x) != ~0
				&& GLOBAL (autopilot.y) != ~0)
				|| GLOBAL_SIS (FuelOnBoard) == 0
				);
		if (OnAutoPilot || OnAutoPilot != LastPilot)
		{
			DWORD num_ticks;

			LockMutex (GraphicsLock);
			num_ticks = GetTimeCounter () - LastTime;
			if (!OnAutoPilot)
			{
				DrawSISMessage (NULL_PTR);
				cycle_index = delay_count = 0;
			}
			else if (delay_count > num_ticks)
			{
				delay_count -= num_ticks;
			}
			else
			{
				if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT))
						&& GLOBAL_SIS (CrewEnlisted) != (COUNT)~0)
				{
					// 2002/11/30 this additional 'if' fixes autopilot indicator blinking on combat/starmap
					// TODO: is there a better (more exact) way of determining if player is in starmap menu or not?
					if (LOBYTE (GLOBAL (CurrentActivity)) != IN_ENCOUNTER &&
						(!pMenuState || (pMenuState && pMenuState->InputFunc == DoFlagshipCommands)))
					{
						CONTEXT OldContext;

						OldContext = SetContext (OffScreenContext);
						SetContextForeGroundColor (cycle_tab[cycle_index]);
						DrawSISMessage ((UNICODE *)~0L);
						SetContext (OldContext);
					}
				}

				cycle_index = (cycle_index + 1) % NUM_CYCLES;
				delay_count = NUM_DELAYS;
			}
			UnlockMutex (GraphicsLock);

			LastPilot = OnAutoPilot;
			LastTime += num_ticks;
		}
		
		ClearSemaphore (GLOBAL (GameClock.clock_sem));
		SleepThreadUntil (TimeIn + ONE_SECOND / 120);
	}
	FinishTask (task);
	return(0);
}

BOOLEAN
InitGameClock (void)
{
	if (!InitQueue (&GLOBAL (GameClock.event_q), NUM_EVENTS, sizeof (EVENT)))
		return (FALSE);
	clock_mutex = CreateMutex ("Clock Mutex", SYNC_CLASS_TOPLEVEL);
	GLOBAL (GameClock.month_index) = 2;
	GLOBAL (GameClock.day_index) = 17;
	GLOBAL (GameClock.year_index) = START_YEAR; /* Feb 17, START_YEAR */
	GLOBAL (GameClock).tick_count = GLOBAL (GameClock).day_in_ticks = 0;
	SuspendGameClock ();
	if ((GLOBAL (GameClock.clock_task) =
			AssignTask (clock_task_func, 2048,
			"game clock")) == 0)
		return (FALSE);

	return (TRUE);
}

BOOLEAN
UninitGameClock (void)
{
	if (GLOBAL (GameClock.clock_task))
	{
		ResumeGameClock ();

		ConcludeTask (GLOBAL (GameClock.clock_task));
		
		GLOBAL (GameClock.clock_task) = 0;
	}
	DestroyMutex (clock_mutex);
	clock_mutex = NULL;

	UninitQueue (&GLOBAL (GameClock.event_q));

	return (TRUE);
}

void
SuspendGameClock (void)
{
	if (!clock_mutex)
	{
		log_add (log_Fatal, "BUG: "
				"Attempted to suspend non-existent game clock");
#ifdef DEBUG
		explode ();
#endif
		return;
	}
	LockMutex (clock_mutex);
	if (GameClockRunning ())
	{
		SetSemaphore (GLOBAL (GameClock.clock_sem));
		GLOBAL (GameClock.TimeCounter) = 0;
	}
	UnlockMutex (clock_mutex);
}

void
ResumeGameClock (void)
{
	if (!clock_mutex)
	{
		log_add (log_Fatal, "BUG: "
				"Attempted to resume non-existent game clock\n");
#ifdef DEBUG
		explode ();
#endif
		return;
	}
	LockMutex (clock_mutex);
	if (!GameClockRunning ())
	{
		GLOBAL (GameClock.TimeCounter) = GetTimeCounter ();
		ClearSemaphore (GLOBAL (GameClock.clock_sem));
	}
	UnlockMutex (clock_mutex);
}

BOOLEAN
GameClockRunning (void)
{
	return ((BOOLEAN)(GLOBAL (GameClock.TimeCounter) != 0));
}

void
SetGameClockRate (COUNT seconds_per_day)
{
	SIZE new_day_in_ticks, new_tick_count;

	SetSemaphore (GLOBAL (GameClock.clock_sem));
	new_day_in_ticks = (SIZE)(seconds_per_day * CLOCK_BASE_FRAMERATE);
	if (GLOBAL (GameClock.day_in_ticks) == 0)
		new_tick_count = new_day_in_ticks;
	else if (GLOBAL (GameClock.tick_count) <= 0)
		new_tick_count = 0;
	else if ((new_tick_count = (SIZE)((DWORD)GLOBAL (GameClock.tick_count)
			* new_day_in_ticks / GLOBAL (GameClock.day_in_ticks))) == 0)
		new_tick_count = 1;
	GLOBAL (GameClock.day_in_ticks) = new_day_in_ticks;
	GLOBAL (GameClock.tick_count) = new_tick_count;
	ClearSemaphore (GLOBAL (GameClock.clock_sem));
}

BOOLEAN
ValidateEvent (EVENT_TYPE type, PCOUNT pmonth_index, PCOUNT pday_index,
		PCOUNT pyear_index)
{
	COUNT month_index, day_index, year_index;

	month_index = *pmonth_index;
	day_index = *pday_index;
	year_index = *pyear_index;
	if (type == RELATIVE_EVENT)
	{
		month_index += GLOBAL (GameClock.month_index) - 1;
		year_index += GLOBAL (GameClock.year_index) + (month_index / 12);
		month_index = (month_index % 12) + 1;

		day_index += GLOBAL (GameClock.day_index);
		while (day_index > DaysInMonth (month_index, year_index))
		{
			day_index -= DaysInMonth (month_index, year_index);
			if (++month_index > 12)
			{
				month_index = 1;
				++year_index;
			}
		}

		*pmonth_index = month_index;
		*pday_index = day_index;
		*pyear_index = year_index;
	}

	// translation: return (BOOLEAN) !(date < GLOBAL (Gameclock.date));
	return (BOOLEAN) (!(year_index < GLOBAL (GameClock.year_index)
			|| (year_index == GLOBAL (GameClock.year_index)
			&& (month_index < GLOBAL (GameClock.month_index)
			|| (month_index == GLOBAL (GameClock.month_index)
			&& day_index < GLOBAL (GameClock.day_index))))));
}

HEVENT
AddEvent (EVENT_TYPE type, COUNT month_index, COUNT day_index, COUNT
		year_index, BYTE func_index)
{
	HEVENT hNewEvent;

	if (type == RELATIVE_EVENT
			&& month_index == 0
			&& day_index == 0
			&& year_index == 0)
		EventHandler (func_index);
	else if (ValidateEvent (type, &month_index, &day_index, &year_index)
			&& (hNewEvent = AllocEvent ()))
	{
		EVENTPTR EventPtr;

		LockEvent (hNewEvent, &EventPtr);
		EventPtr->day_index = (BYTE)day_index;
		EventPtr->month_index = (BYTE)month_index;
		EventPtr->year_index = year_index;
		EventPtr->func_index = func_index;
		UnlockEvent (hNewEvent);

		{
			HEVENT hEvent, hSuccEvent;
			for (hEvent = GetHeadEvent (); hEvent != 0; hEvent = hSuccEvent)
			{
				LockEvent (hEvent, &EventPtr);
				if (year_index < EventPtr->year_index
						|| (year_index == EventPtr->year_index
						&& (month_index < EventPtr->month_index
						|| (month_index == EventPtr->month_index
						&& day_index < EventPtr->day_index))))
				{
					UnlockEvent (hEvent);
					break;
				}

				hSuccEvent = GetSuccEvent (EventPtr);
				UnlockEvent (hEvent);
			}
			
			InsertEvent (hNewEvent, hEvent);
		}

		return (hNewEvent);
	}

	return (0);
}

SIZE
ClockTick (void)
{
	return (--GLOBAL (GameClock.tick_count));
}

