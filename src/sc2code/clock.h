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

#ifndef _CLOCK_H
#define _CLOCK_H

#include "libs/tasklib.h"
#include "displist.h"


#define START_YEAR 2155

#define UPDATE_DAY (1 << 0)
#define UPDATE_MONTH (1 << 1)
#define UPDATE_YEAR (1 << 2)

typedef struct
{
	BYTE day_index, month_index;
	COUNT year_index;
	SIZE tick_count, day_in_ticks;
	Semaphore clock_sem;
	Task clock_task;
	DWORD TimeCounter;

	QUEUE event_q;
} CLOCK_STATE;
typedef CLOCK_STATE *PCLOCK_STATE;

typedef QUEUE_HANDLE HEVENT;

typedef struct event
{
	HEVENT pred, succ;

	BYTE day_index, month_index;
	COUNT year_index;
	BYTE func_index;
} EVENT;
typedef EVENT *PEVENT;

typedef enum
{
	ABSOLUTE_EVENT = 0,
	RELATIVE_EVENT
} EVENT_TYPE;

#define EVENTPTR PEVENT

#define AllocEvent() AllocLink (&GLOBAL (GameClock.event_q))
#define PutEvent(h) PutQueue (&GLOBAL (GameClock.event_q), (h))
#define InsertEvent(h,i) InsertQueue (&GLOBAL (GameClock.event_q), (h), (i))
#define GetHeadEvent() GetHeadLink (&GLOBAL (GameClock.event_q))
#define GetTailEvent() GetTailLink (&GLOBAL (GameClock.event_q))
#define LockEvent(h,eptr) *(eptr) = (EVENTPTR)LockLink ( \
		&GLOBAL (GameClock.event_q), (h))
#define UnlockEvent(h) UnlockLink (&GLOBAL (GameClock.event_q), (h))
#define RemoveEvent(h) RemoveQueue (&GLOBAL (GameClock.event_q), (h))
#define FreeEvent(h) FreeLink (&GLOBAL (GameClock.event_q), (h))
#define GetPredEvent(l) _GetPredLink (l)
#define GetSuccEvent(l) _GetSuccLink (l)
#define ForAllEvents(callback, arg) ForAllLinks(&GLOBAL (GameClock.event_q), \
		(void (*)(LINKPTR, void *)) (callback), (arg))

				/* rates are in seconds per game day */
#define HYPERSPACE_CLOCK_RATE 5
#define INTERPLANETARY_CLOCK_RATE 30

extern BOOLEAN InitGameClock (void);
extern BOOLEAN UninitGameClock (void);
extern void SuspendGameClock (void);
extern void ResumeGameClock (void);
extern BOOLEAN GameClockRunning (void);
extern void SetGameClockRate (COUNT seconds_per_day);
extern BOOLEAN ValidateEvent (EVENT_TYPE type, PCOUNT pmonth_index,
		PCOUNT pday_index, PCOUNT pyear_index);
extern HEVENT AddEvent (EVENT_TYPE type, COUNT month_index, COUNT
		day_index, COUNT year_index, BYTE func_index);
extern void EventHandler (BYTE selector);
extern SIZE ClockTick (void);

#endif /* _CLOCK_H */

