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

#include <assert.h>
#include "sound.h"
#include "libs/sound/trackplayer.h"

extern int do_subtitles (UNICODE *pStr, UNICODE *TimeStamp, int method);

static int tct;               //total number of subtitle tracks
static int tcur;              //currently playing subtitle track
static int cur_page = 0;      //current page of subtitle track 
static int no_page_break = 0;
static int no_voice;
static int track_pos_changed = 0; // set whenever  ff, frev is enabled


#define MAX_CLIPS 50
#define is_sample_playing(samp) (((samp)->read_chain_ptr && (samp)->play_chain_ptr) ||\
			(!(samp)->read_chain_ptr && (samp)->decoder))
static TFB_SoundSample *sound_sample = NULL;
static TFB_SoundChain *first_chain = NULL; //first decoder in linked list
static TFB_SoundChain *last_chain = NULL;  //last decoder in linked list
static UNICODE *TrackTextArray[MAX_CLIPS]; //storage for subtitlle text
static Mutex track_mutex; //protects tcur and tct
void recompute_track_pos (TFB_SoundSample *sample, 
						  TFB_SoundChain *first_chain, sint32 offset);

//JumpTrack currently aborts the current track. However, it doesn't clear the
//data-structures as StopTrack does.  this allows for rewind even after the
//track has finished playing
//Question:  Should 'abort' call StopTrack?
void
JumpTrack (int abort)
{
	if (sound_sample)
	{
		uint32 cur_time;
		sint32 total_length = (sint32)((last_chain->start_time + 
				last_chain->decoder->length) * (float)ONE_SECOND);
		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		PauseStream (SPEECH_SOURCE);
		cur_time = GetTimeCounter();
		soundSource[SPEECH_SOURCE].start_time = 
				(sint32)cur_time - total_length;;
		track_pos_changed = 1;
		sound_sample->play_chain_ptr = last_chain;
		recompute_track_pos(sound_sample, first_chain, total_length);
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		PlayingTrack();
		return;
	}

	no_voice = 1;
	do_subtitles ((void *)~0, (void *)~0, 1);

	if (abort)
	{
		LockMutex (track_mutex);
		tcur = tct;
		UnlockMutex(track_mutex);
	}
}

//advance to the next track and start playing
// we no longer support advancing tracks this way.  Instead this should just start playing
// a stream.
void
PlayTrack (void)
{
	if (sound_sample && (sound_sample->read_chain_ptr || sound_sample->decoder))
	{
		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		PlayStream (sound_sample,
				SPEECH_SOURCE, false,
				speechVolumeScale != 0.0f, !track_pos_changed);
		track_pos_changed = 0;
 		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	}
	else
	{
		no_voice = 1;
	}
}

// ResumeTrack should resume a paused track, or start a stopped track, and do nothing
// for a playing track
void
ResumeTrack ()
{
	if (sound_sample && (sound_sample->read_chain_ptr || sound_sample->decoder))
	{
		// Only try to start the track if there is something to play
		TFBSound_IntVal state;
		BOOLEAN playing;

		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		TFBSound_GetSourcei (soundSource[SPEECH_SOURCE].handle, TFBSOUND_SOURCE_STATE, &state);
		playing = PlayingStream (SPEECH_SOURCE);
		if (!track_pos_changed && !playing && state == TFBSOUND_PAUSED)
		{
			ResumeStream (SPEECH_SOURCE);
			UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		}
		else if (!no_voice && ! playing)
		{
			UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
			PlayTrack ();
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
	// this is not a great way to detect whether the track is playing,
	// but as it should work during fast-forward/rewind, 'PlayingStream' can't be used
//	if (tct == 0)
//		return ((COUNT)~0);
	if (sound_sample && is_sample_playing (sound_sample))
	{
		int last_track, last_page;
		LockMutex (track_mutex);
		last_track = tcur;
		last_page = cur_page;
		UnlockMutex (track_mutex);
		if (do_subtitles (TrackTextArray[last_track], (void *)last_page, 1) || 
			(no_voice && ++tcur < tct && do_subtitles (0, (void *)~0, 1)))
		{
			return (tcur + 1);
		}
		else if (sound_sample)
		{
			COUNT result;

			LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
			result = (PlayingStream (SPEECH_SOURCE) ? (COUNT)(last_track + 1) : 0);
			UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
			return result;
		}
	}

	return (0);
}

void
StopTrack ()
{
	int i;
	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	StopStream (SPEECH_SOURCE);
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);

	for (i = 0; i < tct; i++)
	{
		HFree (TrackTextArray[i]);
		TrackTextArray[i] = 0;
	}
	if (first_chain)
	{
		destroy_soundchain (first_chain);
		first_chain = NULL;
	}
	if (sound_sample)
	{
		DestroyMutex (track_mutex);
		TFBSound_DeleteBuffers (sound_sample->num_buffers, sound_sample->buffer);
		HFree (sound_sample->buffer);
		HFree (sound_sample->buffer_tag);
		HFree (sound_sample);
		sound_sample = NULL;
	}
	tct = tcur = 0;
	cur_page = 0;
	no_voice = 0;
	do_subtitles ((void *)~0, (void *)~0, 1);
}

void 
DoTrackTag (TFB_SoundTag *tag)
{
	if (tag->type == MIX_BUFFER_TAG_TEXT)
	{
		LockMutex (track_mutex);
		if (tag->callback)
			tag->callback ();
		tcur = ((int)tag->value) >> 8;
		cur_page = ((int)tag->value) & 0xFF;
		UnlockMutex (track_mutex);
	}
}

int 
GetTimeStamps(UNICODE *TimeStamps, uint32 *time_stamps)
{
	int pos;
	int num = 0;
	while (*TimeStamps && (pos = wstrcspn (TimeStamps, ",\n")))
	{
		UNICODE valStr[10];
		wstrncpy (valStr, TimeStamps, pos);
		valStr[pos] = '\0';
		*time_stamps = wstrtoul(valStr,NULL,10);
		if (*time_stamps)
		{
			num++;
			time_stamps++;
		}
		TimeStamps += pos;
		if (*TimeStamps)
			TimeStamps++;
	}
	*time_stamps = 0;
	return (num);
}

// decodes several tracks into one and adds it to queue
// track list is NULL-terminated
void
SpliceMultiTrack (UNICODE *TrackNames[], UNICODE *TrackText)
{
#define MAX_MULTI_TRACKS  20
#define MAX_MULTI_BUFFERS 100
	TFB_SoundDecoder* track_decs[MAX_MULTI_TRACKS + 1];
	TFB_SoundChain *begin_chain;
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
		fprintf (stderr, "SpliceMultiTrack(): no more clip slots (%d)\n",
				MAX_CLIPS);
		return;
	}

	if (! sound_sample)
	{
		fprintf (stderr, "SpliceMultiTrack(): Cannot be called before SpliceTrack()\n");
		return;
	}

	fprintf (stderr, "SpliceMultiTrack(): loading...\n");
	for (tracks = 0; *TrackNames && tracks < MAX_MULTI_TRACKS; TrackNames++, tracks++)
	{
		track_decs[tracks] = SoundDecoder_Load (*TrackNames, 32768, 0, 0);
		if (track_decs[tracks])
		{
			fprintf (stderr, "  track: %s, decoder: %s, rate %d format %x\n",
					*TrackNames,
					track_decs[tracks]->decoder_info,
					track_decs[tracks]->frequency,
					track_decs[tracks]->format);
			SoundDecoder_DecodeAll (track_decs[tracks]);

			last_chain->next = create_soundchain (track_decs[tracks], sound_sample->length);
			last_chain = last_chain->next;
			if (tracks == 0)
				begin_chain = last_chain;
			sound_sample->length += track_decs[tracks]->length;
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
	if (tct)
	{
		int slen1;
		slen1 = wstrlen (TrackTextArray[tct - 1]);
		TrackTextArray[tct - 1] = HRealloc (TrackTextArray[tct - 1], slen1 + slen + 1);
		wstrcpy (&TrackTextArray[tct - 1][slen1], TrackText);
	}
	else
	{
		TrackTextArray[tct] = HMalloc (slen + 1);
		wstrcpy (TrackTextArray[tct++], TrackText);
		begin_chain->tag.type = MIX_BUFFER_TAG_TEXT;
		begin_chain->tag.value = (void *)((tct - 1) << 8);
	}
	no_page_break = 1;
	
	if (tracks > 0)
		return;
	else
	{
		fprintf (stderr, "  no tracks loaded\n");
	}
}

void
SpliceTrack (UNICODE *TrackName, UNICODE *TrackText, UNICODE *TimeStamp, void (*cb) ())
{
	unsigned long startTime;
	if (TrackText)
	{
		if (TrackName == 0)
		{
			if (tct)
			{
				int slen1, slen2;
				UNICODE *oTT;

				oTT = TrackTextArray[tct - 1];
				slen1 = wstrlen (oTT);
				slen2 = wstrlen (TrackText);
				TrackTextArray[tct - 1] = HRealloc (oTT, slen1 + slen2 + 1);
				wstrcpy (&TrackTextArray[tct - 1][slen1], TrackText);
			}
		}
		else
		{
			int num_pages, page_counter;
			uint32 time_stamps[50];

			if (no_page_break && tct)
			{
				int slen1, slen2;
				UNICODE *oTT;

				oTT = TrackTextArray[tct - 1];
				slen1 = wstrlen (oTT);
				slen2 = wstrlen (TrackText);
				TrackTextArray[tct - 1] = HRealloc (oTT, slen1 + slen2 + 1);
				wstrcpy (&TrackTextArray[tct - 1][slen1], TrackText);
			}
			else
			{
				TrackTextArray[tct] = HMalloc (sizeof (UNICODE) * (wstrlen (TrackText) + 1));
				wstrcpy (TrackTextArray[tct++], TrackText);
			}
			if (TimeStamp)
				num_pages = GetTimeStamps (TimeStamp, time_stamps);
			else
				num_pages = 0;

			fprintf (stderr, "SpliceTrack(): loading %s\n", TrackName);

			startTime = 0;
			for (page_counter = 0; page_counter <= num_pages; page_counter++)
			{
				if (! sound_sample)
				{
					TFB_SoundDecoder *decoder;
					track_mutex = CreateMutex();
					sound_sample = (TFB_SoundSample *) HMalloc (sizeof (TFB_SoundSample));
					sound_sample->num_buffers = 8;
					sound_sample->buffer_tag = HMalloc (sizeof (TFB_SoundTag *) * sound_sample->num_buffers);
					sound_sample->buffer = HMalloc (sizeof (uint32) * sound_sample->num_buffers);
					TFBSound_GenBuffers (sound_sample->num_buffers, sound_sample->buffer);
					decoder = SoundDecoder_Load (TrackName, 4096, startTime, time_stamps[page_counter]);
					sound_sample->read_chain_ptr = create_soundchain (decoder, 0.0);
					sound_sample->decoder = decoder;
					first_chain = last_chain = sound_sample->read_chain_ptr;
					sound_sample->length = 0;
					sound_sample->play_chain_ptr = 0;
				}
				else
				{
					TFB_SoundDecoder *decoder;
					decoder =  SoundDecoder_Load (TrackName, 4096, startTime, time_stamps[page_counter]);
					last_chain->next = create_soundchain (decoder, sound_sample->length);
					last_chain = last_chain->next;
				}
				startTime += time_stamps[page_counter];
			
				// fprintf (stderr, "page (%d of %d): %d\n",page_counter, num_pages, startTime);
				if (last_chain->decoder)
				{
					//fprintf (stderr, "    decoder: %s, rate %d format %x\n",
					//	last_chain->decoder->decoder_info,
					//	last_chain->decoder->frequency,
					//	last_chain->decoder->format);

					sound_sample->length += last_chain->decoder->length;
					if (! no_page_break)
					{
						last_chain->tag.type = MIX_BUFFER_TAG_TEXT;
						last_chain->tag.value = (void *)(((tct - 1) << 8) | page_counter);
						last_chain->tag.callback = cb;
					}
					no_page_break = 0;
				}
				else
				{
					fprintf (stderr, "SpliceTrack(): couldn't load %s\n", TrackName);
					TFBSound_DeleteBuffers (sound_sample->num_buffers, sound_sample->buffer);
					destroy_soundchain (first_chain);
					first_chain = NULL;
					HFree (sound_sample->buffer);
					HFree (sound_sample);
					sound_sample = NULL;
				}
			}
		}
	}
}

void
PauseTrack ()
{
	if (sound_sample && sound_sample->decoder)
	{
		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		PauseStream (SPEECH_SOURCE);
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	}
}

void
recompute_track_pos (TFB_SoundSample *sample, TFB_SoundChain *first_chain, sint32 offset)
{
	TFB_SoundChain *cur_chain = first_chain;
	if (! sample)
		return;
	while (cur_chain->next &&
			(sint32)(cur_chain->next->start_time * (float)ONE_SECOND) < offset)
		cur_chain = cur_chain->next;
	if (cur_chain->tag.type)
		DoTrackTag (&cur_chain->tag);
	if ((sint32)((cur_chain->start_time + cur_chain->decoder->length) * (float)ONE_SECOND) < offset)
	{
		sample->read_chain_ptr = NULL;
		sample->decoder = NULL;
	}
	else
	{
		sample->read_chain_ptr = cur_chain;
		SoundDecoder_Seek(sample->read_chain_ptr->decoder,
				(uint32)(1000 * (offset / (float)ONE_SECOND - cur_chain->start_time)));
	}
}

void
FastReverse_Smooth ()
{
	if (sound_sample)
	{
		sint32 offset;
		uint32 cur_time;
		sint32 total_length = (sint32)((last_chain->start_time + 
				last_chain->decoder->length) * (float)ONE_SECOND);
		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		PauseStream (SPEECH_SOURCE);
		cur_time = GetTimeCounter();
		track_pos_changed = 1;
		if ((sint32)cur_time - soundSource[SPEECH_SOURCE].start_time > total_length)
			soundSource[SPEECH_SOURCE].start_time = 
				(sint32)cur_time - total_length;

		soundSource[SPEECH_SOURCE].start_time += ACCEL_SCROLL_SPEED;
		if (soundSource[SPEECH_SOURCE].start_time > (sint32)cur_time)
		{
			soundSource[SPEECH_SOURCE].start_time = cur_time;
			offset = 0;
		}
		else
			offset = cur_time - soundSource[SPEECH_SOURCE].start_time;
		recompute_track_pos(sound_sample, first_chain, offset);
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		PlayingTrack();

	}
}

void
FastReverse_Page ()
{
	if (sound_sample)
	{
		TFB_SoundChain *prev_chain;
		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		prev_chain = get_previous_chain(first_chain, sound_sample->play_chain_ptr);
		if (prev_chain)
		{
			sound_sample->read_chain_ptr = prev_chain;
			PlayStream (sound_sample,
					SPEECH_SOURCE, false,
					speechVolumeScale != 0.0f, true);
		}
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	}
}

void
FastForward_Smooth ()
{
	if (sound_sample)
	{
		sint32 offset;
		uint32 cur_time;
		sint32 total_length = (sint32)((last_chain->start_time + 
				last_chain->decoder->length) * (float)ONE_SECOND);
		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		PauseStream (SPEECH_SOURCE);
		cur_time = GetTimeCounter();
		soundSource[SPEECH_SOURCE].start_time -= ACCEL_SCROLL_SPEED;
		if ((sint32)cur_time - soundSource[SPEECH_SOURCE].start_time > total_length)
			soundSource[SPEECH_SOURCE].start_time = 
				(sint32)cur_time - total_length;
		offset = cur_time - soundSource[SPEECH_SOURCE].start_time;
		track_pos_changed = 1;
		recompute_track_pos(sound_sample, first_chain, offset);
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		PlayingTrack();

	}
}

void
FastForward_Page ()
{
	if (sound_sample)
	{
		TFB_SoundChain *cur_ptr = sound_sample->play_chain_ptr;
		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		while (cur_ptr->next && ! cur_ptr->tag.type)
			cur_ptr = cur_ptr->next;
		if (cur_ptr->next)
		{
			sound_sample->read_chain_ptr = cur_ptr->next;
			PlayStream (sound_sample,
					SPEECH_SOURCE, false,
					speechVolumeScale != 0.0f, true);
		}
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	}
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
			float played_time = (GetTimeCounter () - soundSource[SPEECH_SOURCE].sbuf_lasttime) / 
				(float)ONE_SECOND;
			long delta = (int) (played_time * (float)soundSource[SPEECH_SOURCE].
				sample->decoder->frequency * 2.0f);
			unsigned long pos;
			int i;
			UBYTE *scopedata = (UBYTE *) data;
			UBYTE *sbuffer = soundSource[SPEECH_SOURCE].sbuffer;

			assert (soundSource[SPEECH_SOURCE].sample->decoder->frequency == 11025);
			assert (soundSource[SPEECH_SOURCE].sample->decoder->format == TFBSOUND_FORMAT_MONO16);

			if (delta < 0)
			{
				fprintf (stderr, "GetSoundData(): something's messed with timing, delta %d\n", delta);
				delta = 0;
			}
			else if (delta > (int)(soundSource[SPEECH_SOURCE].sbuf_size * 2))
			{
				//fprintf (stderr, "GetSoundData(): something's messed with timing, delta %d\n", delta);
				delta = 0;
			}

			//fprintf (stderr, "played_data %d total_decoded %d delta %d\n", played_data, soundSource[SPEECH_SOURCE].total_decoded, delta);

			pos = soundSource[SPEECH_SOURCE].sbuf_offset + delta;
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
			float played_time = (GetTimeCounter () - soundSource[MUSIC_SOURCE].sbuf_lasttime) / 
				(float)ONE_SECOND;
			int delta = (int) (played_time * (float)soundSource[MUSIC_SOURCE].
				sample->decoder->frequency * 4.0f);
			unsigned long pos;
			int i, step;
			UBYTE *scopedata = (UBYTE *) data;
			UBYTE *sbuffer = soundSource[MUSIC_SOURCE].sbuffer;

			assert (soundSource[MUSIC_SOURCE].sample->decoder->frequency >= 11025);
			assert (soundSource[MUSIC_SOURCE].sample->decoder->format == TFBSOUND_FORMAT_STEREO16);

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

			pos = soundSource[MUSIC_SOURCE].sbuf_offset + delta;
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
GetSoundInfo (int max_len)
{	
	uint32 length, offset;
	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	if (soundSource[SPEECH_SOURCE].sample)
	{
		length = (uint32) (soundSource[SPEECH_SOURCE].sample->length * (float)ONE_SECOND);
		offset = (uint32) (GetTimeCounter () - soundSource[SPEECH_SOURCE].start_time);
	}
	else
	{
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		return (0);
	}
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	if (offset > length)
		return max_len;
	return (int)(max_len * offset / length);
}
