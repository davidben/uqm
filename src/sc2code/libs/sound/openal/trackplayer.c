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

#include <assert.h>
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
			PlayStream (track_clip[tcur].sample, SPEECH_SOURCE, FALSE, TRUE);
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
			TFB_SoundSample *sample = track_clip[tct].sample;
			track_clip[tct].sample = NULL;
			if (sample->decoder)
			{
				TFB_SoundDecoder *decoder = sample->decoder;
				ALuint *buffer = sample->buffer;
				sample->decoder = NULL;
				sample->buffer = NULL;
				SoundDecoder_Free (decoder);
				alDeleteBuffers (sample->num_buffers, buffer);
				HFree (buffer);
			}
			HFree (sample);
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
			track_clip[tct].sample->decoder = SoundDecoder_Load (TrackName, 4096);
			
			if (track_clip[tct].sample->decoder)
			{
				fprintf (stderr, "    decoder: %s, rate %d format %x\n",
					track_clip[tct].sample->decoder->decoder_info,
					track_clip[tct].sample->decoder->frequency,
					track_clip[tct].sample->decoder->format);
				
				track_clip[tct].sample->num_buffers = 8;
				track_clip[tct].sample->buffer = HMalloc (sizeof (ALuint) * track_clip[tct].sample->num_buffers);
				alGenBuffers (track_clip[tct].sample->num_buffers, track_clip[tct].sample->buffer);
			}
			else
			{
				fprintf (stderr, "SpliceTrack(): couldn't load %s\n", TrackName);
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

// processes sound data to oscilloscope
int
GetSoundData (void *data) 
{
	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);

	if (soundSource[SPEECH_SOURCE].sample && soundSource[SPEECH_SOURCE].sample->decoder &&
		PlayingStream (SPEECH_SOURCE) && soundSource[SPEECH_SOURCE].sbuffer)
	{
		float played_time = (GetTimeCounter () - soundSource[SPEECH_SOURCE].start_time) / 
			(float)ONE_SECOND;
		int played_data = (int) (played_time * (float)soundSource[SPEECH_SOURCE].
			sample->decoder->frequency * 2.0f);
		int delta = played_data - soundSource[SPEECH_SOURCE].total_decoded;
		unsigned int pos, i;
		UBYTE *scopedata = (UBYTE *) data;
		UBYTE *sbuffer = soundSource[SPEECH_SOURCE].sbuffer;

		assert (soundSource[SPEECH_SOURCE].sample->decoder->frequency == 11025);
		assert (soundSource[SPEECH_SOURCE].sample->decoder->format == AL_FORMAT_MONO16);

		if (delta < 0)
		{
			//fprintf (stderr, "GetSoundData(): something's messed with timing, delta %d\n", delta);
			delta = 0;
		}
		else if (delta > soundSource[SPEECH_SOURCE].sbuf_size * 2)
		{
			//fprintf (stderr, "GetSoundData(): something's messed with timing, delta %d\n", delta);
		    delta = 0;
		}
		
		//fprintf (stderr, "played_data %d total_decoded %d delta %d\n", played_data, soundSource[SPEECH_SOURCE].total_decoded, delta);

		pos = soundSource[SPEECH_SOURCE].sbuf_start + delta;
		if (pos % 2 == 1)
			pos++;

		assert (pos >= 0);

		for (i = 0; i < RADAR_WIDTH; ++i)
		{
			SWORD s;
			
			for (;;)
			{
				if (pos >= soundSource[SPEECH_SOURCE].sbuf_size)
					pos = pos - soundSource[SPEECH_SOURCE].sbuf_size;
				else
					break;
			}

			s = *(SWORD*) (&sbuffer[pos]);
			s = (s / 1236) + (RADAR_HEIGHT >> 1);
			if (s < 0)
				s = 0;
			else if (s > RADAR_HEIGHT)
				s = RADAR_HEIGHT;
			scopedata[i] = (UBYTE) s;

			pos += 2;
		}

		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		return 1;
	}

	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	return 0;
}

// tells current position of streaming speech
int
GetSoundInfo (int *length, int *offset)
{	
	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	if (soundSource[SPEECH_SOURCE].sample && soundSource[SPEECH_SOURCE].sample->decoder)
	{
		 *length = (int) (soundSource[SPEECH_SOURCE].sample->decoder->length * (float)ONE_SECOND);
		 *offset = (int) (GetTimeCounter () - soundSource[SPEECH_SOURCE].start_time);
	}
	else
	{
		*length = 1;
		*offset = 0;
	}
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);

	return 1;
}

#endif
