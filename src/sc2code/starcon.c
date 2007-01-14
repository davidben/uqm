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

#include "encount.h"
#include "element.h"
#include "fmv.h"
#include "gameev.h"
#include "types.h"
#include "globdata.h"
#include "load.h"
#include "resinst.h"
#include "restart.h"
#include "starbase.h"
#include "setup.h"
#include "master.h"
#include "controls.h"
#include "starcon.h"
#include "uqmdebug.h"
#include "libs/tasklib.h"
#include "libs/log.h"

#include "uqmversion.h"
#include "options.h"

// Open or close the periodically occuring QuasiSpace portal.
// A seperate thread is always inside this function when the player
// is in hyperspace. This thread awakens every BATTLE_FRAME_RATE seconds.
// It then changes the appearant portal size when necessary.
static int
arilou_gate_task(void *data)
{
	BYTE counter;
	DWORD TimeIn;
	Task task = (Task) data;

	TimeIn = GetTimeCounter ();

	counter = GET_GAME_STATE (ARILOU_SPACE_COUNTER);
	while (!Task_ReadState (task, TASK_EXIT))
	{
		SetSemaphore (GLOBAL (GameClock.clock_sem));

		if (GET_GAME_STATE (ARILOU_SPACE) == OPENING)
		{
			if (++counter == 10)
				counter = 9;
		}
		else
		{
			if (counter-- == 0)
				counter = 0;
		}

		LockMutex (GraphicsLock);
		SET_GAME_STATE (ARILOU_SPACE_COUNTER, counter);
		UnlockMutex (GraphicsLock);

		ClearSemaphore (GLOBAL (GameClock.clock_sem));
		SleepThreadUntil (TimeIn + BATTLE_FRAME_RATE);
		TimeIn = GetTimeCounter ();
	}
	FinishTask (task);

	return(0);
}

static void
BackgroundInitKernel (DWORD TimeOut)
{
	LoadMasterShipList ();
	TaskSwitch ();
	InitGameKernel ();

	while ((GetTimeCounter () <= TimeOut) &&
	       !(GLOBAL (CurrentActivity) & CHECK_ABORT))
	{
		UpdateInputState ();
		TaskSwitch ();
	}
}

#define DEBUG_PSYTRON 0

/* TODO: Remove these declarations once threading is gone. */
extern int snddriver, soundflags;

int
Starcon2Main (void *threadArg)
{
#if DEBUG_PSYTRON || CREATE_JOURNAL
{
int ac = argc;
char **av = argv;

while (--ac > 0)
{
	++av;
	if ((*av)[0] == '-')
	{
		switch ((*av)[1])
		{
#ifdef DEBUG_PSYTRON
			case 'd':
			{
				extern BYTE debug_psytron;

				debug_psytron = atoi (&(*av)[2]);
				break;
			}
#endif //DEBUG_PSYTRON
#if CREATE_JOURNAL
			case 'j':
				++create_journal;
				break;
#endif //CREATE_JOURNAL
		}
	}
}
}
#endif //DEBUG_PSYTRON || CREATE_JOURNAL

	/* TODO: Put initAudio back in main where it belongs once threading
	 *       is gone.
	 */
	extern sint32 initAudio (sint32 driver, sint32 flags);
	initAudio (snddriver, soundflags);

	if (LoadKernel (0,0))
	{
		log_add (log_Info, "We've loaded the Kernel");
	
		Logo ();
		
		GLOBAL (CurrentActivity) = 0;
		// show splash and init the kernel in the meantime
		SplashScreen (BackgroundInitKernel);

// OpenJournal ();
		while (StartGame ())
		{
			InitSIS ();
			InitGameClock ();

			AddInitialGameEvents();
			do
			{
				SuspendGameClock ();

#ifdef DEBUG
				if (debugHook != NULL)
				{
					void (*saveDebugHook) (void);
					saveDebugHook = debugHook;
					debugHook = NULL;
							// No further debugHook calls unless the called
							// function resets debugHook.
					(*saveDebugHook) ();
					continue;
				}
#endif
				
				if (!((GLOBAL (CurrentActivity) | NextActivity) & CHECK_LOAD))
					ZeroVelocityComponents (
							&GLOBAL (velocity)
							);
						//not going into talking pet conversation
				else if (GLOBAL (CurrentActivity) & CHECK_LOAD)
					GLOBAL (CurrentActivity) = NextActivity;

				if ((GLOBAL (CurrentActivity) & START_ENCOUNTER)
						|| GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
				{
					if (GET_GAME_STATE (CHMMR_BOMB_STATE) == 2
							&& !GET_GAME_STATE (STARBASE_AVAILABLE))
					{	/* BGD mode */
						InstallBombAtEarth ();
					}
					else if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) == (BYTE)~0
							|| GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
					{
						GLOBAL (CurrentActivity) |= START_ENCOUNTER;
						VisitStarBase ();
					}
					else
					{
						GLOBAL (CurrentActivity) |= START_ENCOUNTER;
						RaceCommunication ();
					}

					if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
					{
						GLOBAL (CurrentActivity) &= ~START_ENCOUNTER;
						if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY)
							GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
					}
				}
				else if (GLOBAL (CurrentActivity) & START_INTERPLANETARY)
				{
					GLOBAL (CurrentActivity) = MAKE_WORD (IN_INTERPLANETARY, 0);

					ExploreSolarSys ();
#ifdef TESTING
					if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
							&& GLOBAL_SIS (CrewEnlisted) != (COUNT)~0)
					{
						if (!(GLOBAL (CurrentActivity) & START_ENCOUNTER)
								&& (CurStarDescPtr = FindStar (NULL_PTR,
								&GLOBAL (autopilot), 0, 0)))
						{
							GLOBAL (autopilot.x) = ~0;
							GLOBAL (autopilot.y) = ~0;
							GLOBAL (ShipStamp.frame) = 0;
							GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
						}
					}
#endif //TESTING
				}
				else
				{
					Task ArilouTask;
					
					GLOBAL (CurrentActivity) = MAKE_WORD (IN_HYPERSPACE, 0);

					ArilouTask = AssignTask (arilou_gate_task, 128,
							"quasispace portal manager");

					TaskSwitch ();

					Battle ();
					if (ArilouTask)
						Task_SetState (ArilouTask, TASK_EXIT);
				}

				LockMutex (GraphicsLock);
				SetFlashRect (NULL_PTR, (FRAME)0);
				UnlockMutex (GraphicsLock);

				LastActivity = GLOBAL (CurrentActivity);

				if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
						&& (LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE
								//if died for some reason
						|| GLOBAL_SIS (CrewEnlisted) == (COUNT)~0))
				{
					if (GET_GAME_STATE (KOHR_AH_KILLED_ALL))
						InitCommunication (BLACKURQ_CONVERSATION);
							//surrendered to Ur-Quan
					else if (GLOBAL (CurrentActivity) & CHECK_RESTART)
						GLOBAL (CurrentActivity) &= ~CHECK_RESTART;
					break;
				}
			} while (!(GLOBAL (CurrentActivity) & CHECK_ABORT));

			StopSound ();
			UninitGameClock ();
			UninitSIS ();
		}
//		CloseJournal ();

		FreeGameData ();
	}
	else
	{
		log_add (log_Fatal, "\n  *** FATAL ERROR: Could not load basic content ***\n\nUQM requires at least the base content pack to run properly.");
		log_add (log_Fatal, "This file is typically called uqm-%d.%d.0.uqm.  UQM was expecting it", UQM_MAJOR_VERSION, UQM_MINOR_VERSION);
		log_add (log_Fatal, "in the %s/packages directory.", baseContentPath);
		log_add (log_Fatal, "Either your installation did not install the content pack at all, or it\ninstalled it in a different directory.\n\nFix your installation and rerun UQM.\n\n  *******************\n");
		exit (EXIT_FAILURE);
	}
	FreeKernel ();

	// XXX: the abort can now be changed to something cleaner;
	//   something to terminate the for(;;) loop in main()
	TFB_Abort ();

	(void) threadArg;  /* Satisfying compiler (unused parameter) */
	return 0;
}
