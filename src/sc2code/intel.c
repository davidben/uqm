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

#include "starcon.h"

SIZE cur_player;

INPUT_STATE
computer_intelligence (INPUT_REF InputRef, INPUT_STATE InputState)
{
	if (InputState || LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
		return (InputState);

	if (CyborgDescPtr)
	{
		if (PlayerControl[cur_player] & CYBORG_CONTROL)
			InputState = tactical_intelligence ();
		else
			InputState = GetInputState (CombinedInput[cur_player]);
	}
	else if (!(PlayerControl[cur_player] & PSYTRON_CONTROL))
		InputState = 0;
	else
	{
		switch (LOBYTE (GLOBAL (CurrentActivity)))
		{
			case SUPER_MELEE:
			{
				SleepThread (ONE_SECOND >> 1);
				InputState = DEVICE_BUTTON1; /* pick a random ship */
				break;
			}
		}
	}
	(void) InputRef;  /* Satisfying compiler (unused parameter) */
	return (InputState);
}

