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
#include <ctype.h>
#include "sound.h"
#include "libs/sound/trackplayer.h"
#include "comm.h"

static int tct;               //total number of subtitle tracks
static int tcur;              //currently playing subtitle track
static UNICODE *cur_page = 0; //current page of subtitle track 
static int no_page_break = 0;
static int track_pos_changed = 0; // set whenever  ff, frev is enabled
static TFB_SoundTag *last_tag = NULL;

#define MAX_CLIPS 50
#define is_sample_playing(samp) (((samp)->read_chain_ptr && (samp)->play_chain_ptr) ||\
			(!(samp)->read_chain_ptr && (samp)->decoder))
static TFB_SoundSample *sound_sample = NULL;
TFB_SoundChain *first_chain = NULL; //first decoder in linked list
static TFB_SoundChain *last_chain = NULL;  //last decoder in linked list
static TFB_SoundChain *last_ts_chain = NULL; //last element in the chain with a subtitle

static Mutex track_mutex; //protects tcur and tct
void recompute_track_pos (TFB_SoundSample *sample, 
						  TFB_SoundChain *first_chain, sint32 offset);

//JumpTrack currently aborts the current track. However, it doesn't clear the
//data-structures as StopTrack does.  this allows for rewind even after the
//track has finished playing
//Question:  Should 'abort' call StopTrack?
void
JumpTrack (void)
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
				(sint32)cur_time - total_length;
		track_pos_changed = 1;
		sound_sample->play_chain_ptr = last_chain;
		recompute_track_pos(sound_sample, first_chain, total_length);
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		PlayingTrack();
		return;
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
			/*adjust start time so the slider doesn't go crazy*/
			soundSource[SPEECH_SOURCE].start_time += GetTimeCounter () - soundSource[SPEECH_SOURCE].pause_time;
			ResumeStream (SPEECH_SOURCE);
			UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		}
		else if (! playing)
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
		int last_track;
		UNICODE *last_page;
		LockMutex (track_mutex);
		last_track = tcur;
		last_page = cur_page;
		UnlockMutex (track_mutex);
		if (do_subtitles (last_page))
		{
			return (tcur + 1);
		}
		else
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
	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	StopStream (SPEECH_SOURCE);
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);

	if (first_chain)
	{
		destroy_soundchain (first_chain);
		first_chain = NULL;
		last_chain = NULL;
		last_ts_chain = NULL;
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
	do_subtitles ((void *)~0);
}

void 
DoTrackTag (TFB_SoundTag *tag)
{
	if (tag->type == MIX_BUFFER_TAG_TEXT)
	{
		LockMutex (track_mutex);
		if (tag->callback)
			tag->callback ();
		tcur = tag->value;
		cur_page = (UNICODE *)tag->data;
		UnlockMutex (track_mutex);
	}
}

int
GetTimeStamps(UNICODE *TimeStamps, sint32 *time_stamps)
{
	int pos;
	int num = 0;
	while (*TimeStamps && (pos = wstrcspn (TimeStamps, ",\n")))
	{
		UNICODE valStr[10];
		uint32 val;
		wstrncpy (valStr, TimeStamps, pos);
		valStr[pos] = '\0';
		val = wstrtoul(valStr,NULL,10);
		if (val)
		{
			*time_stamps = (sint32)val;
			num++;
			time_stamps++;
		}
		TimeStamps += pos;
		if (*TimeStamps)
			TimeStamps++;
	}
	return (num);
}

#define TEXT_SPEED 70
UNICODE **
SplitSubPages (UNICODE *text, sint32 *timestamp, int *num_pages)
{
	UNICODE **split_text = NULL;
	int pos = 0, ellips = 0;
	COUNT page = 0;
	while (text[pos])
	{
		if (text[pos] == '\n' || text[pos] == '\r')
		{
			if (! split_text)
				split_text = (UNICODE **) HMalloc (sizeof (UNICODE *) * (page + 1));
			else
				split_text = (UNICODE **) HRealloc (split_text, 
						sizeof (UNICODE *) * (page + 1));
			if (!ispunct (text[pos - 1]) && !isspace (text[pos - 1]))
			{
				split_text[page] = HMalloc (sizeof (UNICODE) * (pos + ellips + 4));
				if (ellips)
					wstrcpy (split_text[page], "..");
				wstrncpy (split_text[page] + ellips, text, pos);
				wstrcpy (split_text[page] + ellips + pos, "...");
				timestamp[page] = - pos * TEXT_SPEED;
				ellips = 2;
				text = text + pos + 1;
				pos = 0;
				page ++;
			}
			else
			{
				split_text[page] = HMalloc (sizeof (UNICODE) * (pos + ellips + 1));
				if (ellips)
					wstrcpy (split_text[page], "..");
				wstrncpy (split_text[page] + ellips, text, pos);
				*(split_text[page] + ellips + pos) = 0;
				timestamp[page] = - pos * TEXT_SPEED;
				ellips = 0;
				text = text + pos + 1;
				pos = 0;
				page ++;
			}
		}
		else
			pos++;
	}
	if (pos)
	{
		if (! split_text)
			split_text = (UNICODE **) HMalloc (sizeof (UNICODE *) * (page + 1));
		else
			split_text = (UNICODE **) HRealloc (split_text, 
					sizeof (UNICODE *) * (page + 1));
		split_text[page] = HMalloc (sizeof (UNICODE) * (pos + ellips + 1));
		if (ellips)
			wstrcpy (split_text[page], "..");
		wstrncpy (split_text[page] + ellips, text, pos);
		*(split_text[page] + ellips + pos) = 0;
		timestamp[page] = - pos * TEXT_SPEED;
		page ++;
	}
	*num_pages = page;
	return (split_text);
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
		track_decs[tracks] = SoundDecoder_Load (*TrackNames, 32768, 0, - 3 * TEXT_SPEED);
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
		slen1 = wstrlen ((UNICODE *)last_tag->data);
		last_tag->data = HRealloc ((UNICODE *)last_tag->data, slen1 + slen + 1);
		wstrcpy (&((UNICODE *)last_tag->data)[slen1], TrackText);
	}
	else
	{
		begin_chain->tag.data = HMalloc (slen + 1);
		wstrcpy ((UNICODE *)begin_chain->tag.data, TrackText);
		last_tag = &begin_chain->tag;
		tct++;
		begin_chain->tag.type = MIX_BUFFER_TAG_TEXT;
		begin_chain->tag.value = tct - 1;
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
SpliceTrack (UNICODE *TrackName, UNICODE *TrackText, UNICODE *TimeStamp, void (*cb) (void))
{
	unsigned long startTime;
	UNICODE **split_text = NULL;
	if (TrackText)
	{
		if (TrackName == 0)
		{
			if (tct)
			{
				int slen1, slen2;
				UNICODE *oTT;
				int num_pages = 0, page_counter;
				sint32 time_stamps[50];

				if (! last_ts_chain || !last_ts_chain->tag.data)
				{
					fprintf (stderr, 
						"SpliceTrack(): Tried to append a subtitle to a NULL string\n");
					return;
				}
				split_text = SplitSubPages (TrackText, time_stamps, &num_pages);
				if (! split_text)
				{
					fprintf (stderr, "SpliceTrack(): Failed to parse sutitles\n");
					return;
				}
				oTT = (UNICODE *)last_ts_chain->tag.data;
				slen1 = wstrlen (oTT);
				slen2 = wstrlen (split_text[0]);
				last_ts_chain->tag.data = HRealloc (oTT, slen1 + slen2 + 1);
				wstrcpy (&((UNICODE *)last_ts_chain->tag.data)[slen1], split_text[0]);
				HFree (split_text[0]);
				for (page_counter = 1; page_counter < num_pages; page_counter++)
				{
					if (! last_ts_chain->next)
					{
						fprintf (stderr, "SpliceTrack(): More text pages than timestamps!\n");
						break;
					}
					last_ts_chain = last_ts_chain->next;
					last_ts_chain->tag.data = split_text[page_counter];
				}
			}
		}
		else
		{
			int num_pages = 0, num_timestamps = 0, page_counter;
			sint32 time_stamps[50];
			int i;

			for (i = 0; i < 50; i++)
				time_stamps[i] = -1000;

			split_text = SplitSubPages (TrackText, time_stamps, &num_pages);
			if (! split_text)
			{
				fprintf (stderr, "SpliceTrack(): Failed to parse sutitles\n");
				return;
			}
			if (no_page_break && tct)
			{
				int slen1, slen2;
				UNICODE *oTT;

				oTT = (UNICODE *)last_tag->data;
				slen1 = wstrlen (oTT);
				slen2 = wstrlen (split_text[0]);
				last_tag->data = HRealloc (oTT, slen1 + slen2 + 1);
				wstrcpy (&((UNICODE *)last_tag->data)[slen1], split_text[0]);
				HFree (split_text[0]);
			}
			else
				tct++;

			fprintf (stderr, "SpliceTrack(): loading %s\n", TrackName);

			if (TimeStamp)
			{
				num_timestamps = GetTimeStamps (TimeStamp, time_stamps) + 1;
				if (num_timestamps < num_pages)
					fprintf (stderr, "SpliceTrack(): number of timestamps doesn't match number of pages!\n");
			}
			else
				num_timestamps = num_pages;
			startTime = 0;
			for (page_counter = 0; page_counter < num_timestamps; page_counter++)
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
				startTime += abs (time_stamps[page_counter]);
			
				// fprintf (stderr, "page (%d of %d): %d ts: %d\n",page_counter, num_pages, startTime, time_stamps[page_counter]);
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
						// last_chain->tag.value = (void *)(((tct - 1) << 8) | page_counter);
						last_chain->tag.value = tct - 1;
						if (page_counter < num_pages)
						{
							last_chain->tag.data = (void *)split_text[page_counter];
							last_ts_chain = last_chain;
						}
						last_chain->tag.callback = cb;
						last_tag = &last_chain->tag;
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
		soundSource[SPEECH_SOURCE].pause_time = GetTimeCounter ();
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
	{
		if (cur_chain->tag.type)
			DoTrackTag (&cur_chain->tag);
		cur_chain = cur_chain->next;
	}
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

int
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
			UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
			return TRUE;
		}
		else //means there are no more pages left
		{
			UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
			return FALSE;
		}
	}
	return TRUE;
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

			for (i = 0; i < RADAR_WIDTH - 2; ++i)
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
				s = (s / 1360) + (RADAR_HEIGHT >> 1);
				if (s < 1)
					s = 1;
				else if (s > RADAR_HEIGHT - 2)
					s = RADAR_HEIGHT - 2;
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

			for (i = 0; i < RADAR_WIDTH - 2; ++i)
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
				
				s = (s / 1800) + (RADAR_HEIGHT >> 1);
				if (s < 1)
					s = 1;
				else if (s > RADAR_HEIGHT - 2)
					s = RADAR_HEIGHT - 2;
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
