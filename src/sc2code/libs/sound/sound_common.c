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

#include "libs/graphics/gfx_common.h"
#include "libs/sound/sound_common.h"

extern int _music_volume;

static Thread FadeTask;
static SIZE TTotal;
static SIZE volume_end;

static int
fade_task (void *data)
{
	SIZE TDelta, volume_beg;
	DWORD StartTime, CurTime;

	volume_beg = _music_volume;
	StartTime = CurTime = GetTimeCounter ();
	do
	{
		SleepThreadUntil (CurTime + 1);
		CurTime = GetTimeCounter ();
		if ((TDelta = (SIZE) (CurTime - StartTime)) > TTotal)
			TDelta = TTotal;

		SetMusicVolume ((COUNT) (volume_beg + (SIZE)
				((long) (volume_end - volume_beg) * TDelta / TTotal)));
	} while (TDelta < TTotal);

	FadeTask = 0;
	return (1);
}

DWORD
FadeMusic (BYTE end_vol, SIZE TimeInterval)
{
	DWORD TimeOut;

	if (FadeTask)
	{
		volume_end = _music_volume;
		TTotal = 1;
		do
			TaskSwitch ();
		while (FadeTask);
		TaskSwitch ();
	}

	if ((TTotal = TimeInterval) <= 0)
		TTotal = 1; /* prevent divide by zero and negative fade */
	volume_end = end_vol;
		
	if (TTotal > 1 && (FadeTask = CreateThread (fade_task, NULL, 0,
			"fade music")))
	{
		TimeOut = GetTimeCounter () + TTotal + 1;
	}
	else
	{
		SetMusicVolume (end_vol);
		TimeOut = GetTimeCounter ();
	}

	return (TimeOut);
}
