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

/* Originally: OpenAL specific code by Mika Kolehmainen, 2002-10-23
 * Adapted for MixSDL
 */

#ifdef SOUNDMODULE_MIXSDL

#include <assert.h>
#include "sound.h"
#include "libs/sound/trackplayer.h"

extern int do_subtitles (UNICODE *pStr, UNICODE *TimeStamp);

static int tct, tcur, no_voice;

#define MAX_CLIPS 50
static struct
{
	int text_spliced;
	UNICODE *text;
	UNICODE *timestamp;
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
	do_subtitles ((void *)~0, 0);

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
			if (track_clip[tcur].sample->decoder)
				PlayStream (track_clip[tcur].sample,
						SPEECH_SOURCE, false,
						speechVolumeScale != 0.0f);
			else
				PlayDecodedClip (track_clip[tcur].sample,
						SPEECH_SOURCE, false);
			UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
			do_subtitles (~0, 0);
			do_subtitles (0, 0);
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
		uint32 state;

		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		mixSDL_GetSourcei (soundSource[SPEECH_SOURCE].handle, MIX_SOURCE_STATE, &state);

		if (!soundSource[SPEECH_SOURCE].stream_should_be_playing && 
			state == MIX_PAUSED)
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
		if (do_subtitles (track_clip[tcur].text, track_clip[tcur].timestamp) || 
			(no_voice && ++tcur < tct && do_subtitles (0, 0)))
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
				mixSDL_Object *buffer = sample->buffer;
				sample->decoder = NULL;
				sample->buffer = NULL;
				SoundDecoder_Free (decoder);
				mixSDL_DeleteBuffers (sample->num_buffers, buffer);
				HFree (buffer);
			}
			HFree (sample);
		}
	}
	tct = tcur = 0;
	no_voice = 0;
	do_subtitles (0, 0);
}

// decodes several tracks into one and adds it to queue
// track list is NULL-terminated
void
SpliceMultiTrack (UNICODE *TrackNames[], UNICODE *TrackText)
{
#define MAX_MULTI_TRACKS  20
#define MAX_MULTI_BUFFERS 100
	TFB_SoundDecoder* track_decs[MAX_MULTI_TRACKS + 1];
	int tracks;
	int slen;

	if (!TrackText)
	{
#ifdef DEBUG
		fprintf (stderr, "SpliceMultiTrack(): no track text\n");
#endif
		return;
	}

	if (tct >= MAX_CLIPS)
	{
#ifdef DEBUG
		fprintf (stderr, "SpliceMultiTrack(): no more clip slots (%d)\n",
				MAX_CLIPS);
#endif
		return;
	}

	fprintf (stderr, "SpliceMultiTrack(): loading...\n");
	for (tracks = 0; *TrackNames && tracks < MAX_MULTI_TRACKS; TrackNames++, tracks++)
	{
		track_decs[tracks] = SoundDecoder_Load (*TrackNames, 32768);
		if (track_decs[tracks])
		{
			fprintf (stderr, "  track: %s, decoder: %s, rate %d format %x\n",
					*TrackNames,
					track_decs[tracks]->decoder_info,
					track_decs[tracks]->frequency,
					track_decs[tracks]->format);

		}
		else
		{
			fprintf (stderr, "  couldn't load %s\n", *TrackNames);
			tracks--;
		}
	}
	track_decs[tracks] = 0; // term

	if (tracks == 0)
	{
		fprintf (stderr, "  no tracks loaded\n");
		return;
	}

	slen = wstrlen (TrackText);
	track_clip[tct].text = HMalloc (slen + 1);
	wstrcpy (track_clip[tct].text, TrackText);
	track_clip[tct].text_spliced = 1;
	track_clip[tct].timestamp = NULL;

	track_clip[tct].sample = (TFB_SoundSample *) HMalloc (
			sizeof (TFB_SoundSample));
	track_clip[tct].sample->decoder = NULL;
	track_clip[tct].sample->num_buffers = tracks;
	track_clip[tct].sample->buffer = HMalloc (sizeof (mixSDL_Object)
			* track_clip[tct].sample->num_buffers);
	mixSDL_GenBuffers (track_clip[tct].sample->num_buffers, track_clip[tct].sample->buffer);

	tracks = PreDecodeClips (track_clip[tct].sample, track_decs, true);
	
	if (tracks > 0)
		++tct;
	else
	{
		fprintf (stderr, "  no tracks loaded\n");
		HFree (track_clip[tct].sample);
		track_clip[tct].sample = NULL;
	}
}

void
SpliceTrack (UNICODE *TrackName, UNICODE *TrackText, UNICODE *TimeStamp)
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

			track_clip[tct].timestamp = TimeStamp;
			fprintf (stderr, "SpliceTrack(): loading %s\n", TrackName);

			track_clip[tct].sample = (TFB_SoundSample *) HMalloc (sizeof (TFB_SoundSample));
			track_clip[tct].sample->decoder = SoundDecoder_Load (TrackName, 4096);
			
			if (track_clip[tct].sample->decoder)
			{
				fprintf (stderr, "    decoder: %s, rate %d format %x\n",
					track_clip[tct].sample->decoder->decoder_info,
					track_clip[tct].sample->decoder->frequency,
					track_clip[tct].sample->decoder->format);
				
				track_clip[tct].sample->length =
						track_clip[tct].sample->decoder->length;
				track_clip[tct].sample->num_buffers = 8;
				track_clip[tct].sample->buffer = HMalloc (
						sizeof (mixSDL_Object) *
						track_clip[tct].sample->num_buffers);
				mixSDL_GenBuffers (track_clip[tct].sample->num_buffers,
						track_clip[tct].sample->buffer);
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
	if (speechVolumeScale != 0.0f)
	{
		// speech is enabled

		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		if (soundSource[SPEECH_SOURCE].sample && soundSource[SPEECH_SOURCE].sample->decoder && 
			PlayingStream (SPEECH_SOURCE) && soundSource[SPEECH_SOURCE].sbuffer && 
			soundSource[SPEECH_SOURCE].sbuf_size > 0)
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
			assert (soundSource[SPEECH_SOURCE].sample->decoder->format == MIX_FORMAT_MONO16);

			if (delta < 0)
			{
				//fprintf (stderr, "GetSoundData(): something's messed with timing, delta %d\n", delta);
				delta = 0;
			}
			else if (delta > (int)(soundSource[SPEECH_SOURCE].sbuf_size * 2))
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
				SDWORD s;

				for (;;)
				{
					if (pos >= soundSource[SPEECH_SOURCE].sbuf_size)
						pos = pos - soundSource[SPEECH_SOURCE].sbuf_size;
					else
						break;
				}

				s = *(SWORD*) (&sbuffer[pos]);
				s = (s / 1260) + (RADAR_HEIGHT >> 1);
				if (s < 0)
					s = 0;
				else if (s >= RADAR_HEIGHT)
					s = RADAR_HEIGHT - 1;
				scopedata[i] = (UBYTE) s;

				pos += 2;
			}

			UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
			return 1;
		}
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	}
	else if (musicVolumeScale != 0.0f)
	{
		// speech is disabled but music is not so process it instead

		LockMutex (soundSource[MUSIC_SOURCE].stream_mutex);
		if (soundSource[MUSIC_SOURCE].sample && soundSource[MUSIC_SOURCE].sample->decoder && 
			PlayingStream (MUSIC_SOURCE) && soundSource[MUSIC_SOURCE].sbuffer && 
			soundSource[MUSIC_SOURCE].sbuf_size > 0)
		{
			float played_time = (GetTimeCounter () - soundSource[MUSIC_SOURCE].start_time) / 
				(float)ONE_SECOND;
			int played_data = (int) (played_time * (float)soundSource[MUSIC_SOURCE].
				sample->decoder->frequency * 4.0f);
			int delta = played_data - soundSource[MUSIC_SOURCE].total_decoded;
			unsigned int pos, i, step;
			UBYTE *scopedata = (UBYTE *) data;
			UBYTE *sbuffer = soundSource[MUSIC_SOURCE].sbuffer;

			assert (soundSource[MUSIC_SOURCE].sample->decoder->frequency >= 11025);
			assert (soundSource[MUSIC_SOURCE].sample->decoder->format == MIX_FORMAT_STEREO16);

			step = soundSource[MUSIC_SOURCE].sample->decoder->frequency / 11025 * 16;
			if (step % 2 == 1)
				step++;

			if (delta < 0)
			{
				//fprintf (stderr, "GetSoundData(): something's messed with timing, delta %d\n", delta);
				delta = 0;
			}
			else if (delta > (int)(soundSource[MUSIC_SOURCE].sbuf_size * 2))
			{
				//fprintf (stderr, "GetSoundData(): something's messed with timing, delta %d\n", delta);
				delta = 0;
			}

			//fprintf (stderr, "played_data %d total_decoded %d delta %d\n", played_data, soundSource[MUSIC_SOURCE].total_decoded, delta);

			pos = soundSource[MUSIC_SOURCE].sbuf_start + delta;
			if (pos % 2 == 1)
				pos++;

			assert (pos >= 0);

			for (i = 0; i < RADAR_WIDTH; ++i)
			{
				SDWORD s;

				for (;;)
				{
					if (pos >= soundSource[MUSIC_SOURCE].sbuf_size)
						pos = pos - soundSource[MUSIC_SOURCE].sbuf_size;
					else
						break;
				}

				s = (*(SWORD*)(&sbuffer[pos])) + (*(SWORD*)(&sbuffer[pos + 2]));
				
				s = (s / 1260) + (RADAR_HEIGHT >> 1);
				if (s < 0)
					s = 0;
				else if (s >= RADAR_HEIGHT)
					s = RADAR_HEIGHT - 1;
				scopedata[i] = (UBYTE) s;

				pos += step;
			}

			UnlockMutex (soundSource[MUSIC_SOURCE].stream_mutex);
			return 1;
		}
		UnlockMutex (soundSource[MUSIC_SOURCE].stream_mutex);
	}

	return 0;
}

// tells current position of streaming speech
int
GetSoundInfo (int *length, int *offset)
{	
	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	if (soundSource[SPEECH_SOURCE].sample)
	{
		*length = (int) (soundSource[SPEECH_SOURCE].sample->length * (float)ONE_SECOND);
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
