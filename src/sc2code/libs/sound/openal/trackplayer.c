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

/* OpenAL specific code by Mika Kolehmainen, 2002-10-23
 */

#ifdef SOUNDMODULE_OPENAL

#include "sound.h"
#include "libs/sound/trackplayer.h"

extern int do_subtitles (UNICODE *pStr);

static int tct, tcur, no_voice;

#define MAX_CLIPS 50
static struct
{
	int text_spliced;
	UNICODE *text;
	TFB_SoundSample *sample;
} track_clip[MAX_CLIPS];


void
JumpTrack (int abort)
{
	speech_advancetrack = FALSE;

	if (track_clip[tcur].sample)
	{
		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		StopStream (SPEECH_SOURCE);
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	}

	no_voice = 1;
	do_subtitles ((void *)~0);

	if (abort)
		tcur = tct;
}

void
advance_track (int channel_finished)
{
	if (channel_finished <= 0)
	{
		if (channel_finished == 0)
			++tcur;
		if (tcur < tct)
		{
			LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
			PlayStream (track_clip[tcur].sample, SPEECH_SOURCE, FALSE);
			UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
			do_subtitles (0);
		}
		else if (channel_finished == 0)
		{
			--tcur;
			no_voice = 1;
		}
	}
}

void
ResumeTrack ()
{
	if (tcur < tct)
	{
		ALint state;

		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		alGetSourcei (soundSource[SPEECH_SOURCE].handle, AL_SOURCE_STATE, &state);

		if (!soundSource[SPEECH_SOURCE].stream_should_be_playing && 
			state == AL_PAUSED)
		{
			ResumeStream (SPEECH_SOURCE);
			UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		}
		else if (!no_voice)
		{
			if (tcur == 0)
			{
				speech_advancetrack = TRUE;
			}
			UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
			advance_track (-1);
		}
		else
		{
			UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		}
	}
}

COUNT
PlayingTrack ()
{
	if (tcur < tct)
	{
		if (do_subtitles (track_clip[tcur].text) || 
			(no_voice && ++tcur < tct && do_subtitles (0)))
		{
			return (tcur + 1);
		}
		else if (track_clip[tcur].sample)
		{
			COUNT result;

			LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
			result = (PlayingStream (SPEECH_SOURCE) ? (COUNT)(tcur + 1) : 0);
			UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);

			return result;
		}
	}

	return (0);
}

void
StopTrack ()
{
	speech_advancetrack = FALSE;
	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	StopStream (SPEECH_SOURCE);
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);

	while (tct--)
	{
		if (track_clip[tct].text_spliced)
		{
			track_clip[tct].text_spliced = 0;
			HFree (track_clip[tct].text);
			track_clip[tct].text = 0;
		}
		if (track_clip[tct].sample)
		{
			if (track_clip[tct].sample->decoder)
			{
				Sound_FreeSample (track_clip[tct].sample->decoder);
				alDeleteBuffers (NUM_SOUNDBUFFERS, track_clip[tct].sample->buffer);
			}
			HFree (track_clip[tct].sample);
			track_clip[tct].sample = 0;
		}
	}
	tct = tcur = 0;
	no_voice = 0;
	do_subtitles (0);
}

void
SpliceTrack (UNICODE *TrackName, UNICODE *TrackText)
{
	if (TrackText)
	{
		if (TrackName == 0)
		{
			if (tct)
			{
				int slen1, slen2;
				UNICODE *oTT;

				oTT = track_clip[tct - 1].text;
				slen1 = wstrlen (oTT);
				slen2 = wstrlen (TrackText);
				if (track_clip[tct - 1].text_spliced)
					track_clip[tct - 1].text = HRealloc (oTT, slen1 + slen2 + 1);
				else
				{
					track_clip[tct - 1].text = HMalloc (slen1 + slen2 + 1);
					wstrcpy (track_clip[tct - 1].text, oTT);
					track_clip[tct - 1].text_spliced = 1;
				}
				wstrcpy (&track_clip[tct - 1].text[slen1], TrackText);
			}
		}
		else if (tct < MAX_CLIPS)
		{
			track_clip[tct].text = TrackText;
			track_clip[tct].text_spliced = 0;

			fprintf (stderr, "SpliceTrack(): loading %s\n", TrackName);

			track_clip[tct].sample = (TFB_SoundSample *) HMalloc (sizeof (TFB_SoundSample));
			track_clip[tct].sample->decoder = Sound_NewSampleFromFile (TrackName, NULL, 4096);
			
			if (track_clip[tct].sample->decoder)
			{
				fprintf (stderr, "decoder %s rate %d channels %d\n",
					track_clip[tct].sample->decoder->decoder->description, 
					track_clip[tct].sample->decoder->actual.rate, 
					track_clip[tct].sample->decoder->actual.channels);
				
				alGenBuffers (NUM_SOUNDBUFFERS, track_clip[tct].sample->buffer);
			}
			else
			{
				fprintf (stderr, "SpliceTrack(): couldn't load %s: %s\n", 
					TrackName, Sound_GetError());
				HFree (track_clip[tct].sample);
				track_clip[tct].sample = NULL;
			}
			
			++tct;
		}
	}
}

void
PauseTrack ()
{
	if (tcur < tct && track_clip[tcur].sample)
	{
		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		PauseStream (SPEECH_SOURCE);
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	}
}

void
FastReverse ()
{
	JumpTrack (0);
	no_voice = 0;
	tcur = 0;
	ResumeTrack ();
}

void
FastForward ()
{
	JumpTrack (0);
	//no_voice = 0;
}

int
GetSoundData (char* data) 
		// Returns the data size. Only used in oscill.c
{
	//fprintf (stderr, "Unimplemented function activated: GetSoundData()\n");
	return 0;
}

// Status: Unimplemented
int
GetSoundInfo (int *length, int *offset)
		// Umm... How does it know which sound?
		// Answer: only used in oscill.c slider so sound must be current speech
{
	//fprintf (stderr, "Unimplemented function activated: GetSoundInfo()\n");
	*length = 1;
	*offset = 0;
	return 1;
}

#endif
