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

#include "sound.h"
#include "../compiler.h"
#include "../tasklib.h"

static Task FadeTask;
static SIZE TTotal;
static SIZE volume_end;

int musicVolume = NORMAL_VOLUME;
float musicVolumeScale;
float sfxVolumeScale;
float speechVolumeScale;
TFB_SoundSource soundSource[NUM_SOUNDSOURCES];


void
StopSound (void)
{
	int i;

	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		StopSource (i);
	}
}

void
CleanSource (int iSource)
{
#define MAX_STACK_BUFFERS 64	
	audio_IntVal processed;

	soundSource[iSource].positional_object = NULL;
	audio_GetSourcei (soundSource[iSource].handle,
			audio_BUFFERS_PROCESSED, &processed);
	if (processed != 0)
	{
		audio_Object stack_bufs[MAX_STACK_BUFFERS];
		audio_Object *bufs;

		if (processed > MAX_STACK_BUFFERS)
			bufs = (audio_Object *) HMalloc (
					sizeof (audio_Object) * processed);
		else
			bufs = stack_bufs;

		audio_SourceUnqueueBuffers (soundSource[iSource].handle,
				processed, bufs);
		
		if (processed > MAX_STACK_BUFFERS)
			HFree (bufs);
	}
	// set the source state to 'initial'
	audio_SourceRewind (soundSource[iSource].handle);
}

void
StopSource (int iSource)
{
	audio_SourceStop (soundSource[iSource].handle);
	CleanSource (iSource);
}

BOOLEAN
SoundPlaying (void)
{
	int i;

	for (i = 0; i < NUM_SOUNDSOURCES; ++i)
	{
		TFB_SoundSample *sample;
		sample = soundSource[i].sample;
		if (sample && sample->decoder)
		{
			BOOLEAN result;
			LockMutex (soundSource[i].stream_mutex);
			result = PlayingStream (i);
			UnlockMutex (soundSource[i].stream_mutex);
			if (result)
				return TRUE;
		}
		else
		{
			audio_IntVal state;
			audio_GetSourcei (soundSource[i].handle, audio_SOURCE_STATE, &state);
			if (state == audio_PLAYING)
				return TRUE;
		}
	}

	return FALSE;
}

// for now just spin in a sleep() loop
// perhaps later change to condvar implementation
void
WaitForSoundEnd (COUNT Channel)
{
	while (Channel == TFBSOUND_WAIT_ALL ?
			SoundPlaying () : ChannelPlaying (Channel))
	{
		SleepThread (ONE_SECOND / 20);
	}
}


// Status: Ignored
BOOLEAN
InitSound (int argc, char* argv[])
{
	/* Quell compiler warnings */
	(void)argc;
	(void)argv;
	return TRUE;
}

// Status: Ignored
void
UninitSound (void)
{
}

void
SetSFXVolume (float volume)
{
	int i;
	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		audio_Sourcef (soundSource[i].handle, audio_GAIN, volume);
	}	
}

void
SetSpeechVolume (float volume)
{
	audio_Sourcef (soundSource[SPEECH_SOURCE].handle, audio_GAIN, volume);
}

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
		SleepThreadUntil (CurTime + ONE_SECOND / 120);
		CurTime = GetTimeCounter ();
		if ((TDelta = (SIZE) (CurTime - StartTime)) > TTotal)
			TDelta = TTotal;

		SetMusicVolume ((COUNT) (volume_beg + (SIZE)
				((long) (volume_end - volume_beg) * TDelta / TTotal)));
	} while (TDelta < TTotal);

	FadeTask = 0;
	FinishTask (task);
	return 1;
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

	TTotal = TimeInterval;
	if (TTotal <= 0)
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

	return TimeOut;
}


