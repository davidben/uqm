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
#include "libs/sound/sound.h"
#include "libs/tasklib.h"

int SoundDriver = TFB_SOUNDDRIVER_MIXSDL;

int musicVolume = (MAX_VOLUME >> 1);
float musicVolumeScale = 1.0f;
float sfxVolumeScale = 1.0f;
float speechVolumeScale = 1.0f;

static Task FadeTask;
static SIZE TTotal;
static SIZE volume_end;

static int
fade_task (void *data)
{
	SIZE TDelta, volume_beg;
	DWORD StartTime, CurTime;
	Task task = (Task) data;

	volume_beg = musicVolume;
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
	FinishTask (task);
	return (1);
}

DWORD
FadeMusic (BYTE end_vol, SIZE TimeInterval)
{
	DWORD TimeOut;

	if (FadeTask)
	{
		volume_end = musicVolume;
		TTotal = 1;
		do
			TaskSwitch ();
		while (FadeTask);
		TaskSwitch ();
	}

	if ((TTotal = TimeInterval) <= 0)
		TTotal = 1; /* prevent divide by zero and negative fade */
	volume_end = end_vol;
		
	if (TTotal > 1 && (FadeTask = AssignTask (fade_task, 0,
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

int 
TFB_InitSound (int driver, int flags)
{
#ifdef  HAVE_OPENAL
	return (TFB_choose_InitSound (driver, flags));
#else
	if (driver == TFB_SOUNDDRIVER_OPENAL)
	{
		fprintf (stderr, "OpenAL driver not compiled in, so using MixSDL\n");
		driver = TFB_SOUNDDRIVER_MIXSDL;
	}
	return (TFB_mixSDL_InitSound (driver, flags));
#endif
}

void
TFB_UninitSound (void)
{
#ifdef  HAVE_OPENAL
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		TFB_alUninitSound ();
	else
		TFB_mixSDL_UninitSound ();
#else
	TFB_mixSDL_UninitSound ();
#endif
}

