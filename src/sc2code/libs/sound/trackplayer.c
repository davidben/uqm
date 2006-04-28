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
#include "libs/sound/trackplayer.h"
#include "libs/sound/trackint.h"
#include "libs/log.h"
#include "comm.h"
#include "sis.h"
#include "options.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>


static int tct;               //total number of subtitle tracks
static int tcur;              //currently playing subtitle track
static UNICODE *cur_page = 0; //current page of subtitle track 
static int no_page_break = 0;
static int track_pos_changed = 0; // set whenever  ff, frev is enabled
static TFB_SoundChain *cur_text_chain = NULL;  //current link w/ subbies

#define MAX_CLIPS 50
static TFB_SoundSample *sound_sample = NULL;
TFB_SoundChain *first_chain = NULL; //first decoder in linked list
static TFB_SoundChain *last_chain = NULL;  //last decoder in linked list
static TFB_SoundChain *last_ts_chain = NULL; //last element in the chain with a subtitle

static Mutex track_mutex; //protects tcur and tct
void recompute_track_pos (TFB_SoundSample *sample, 
						  TFB_SoundChain *first_chain, sint32 offset);
bool is_sample_playing(TFB_SoundSample* samp);

// stream callbacks
static bool OnTrackStart (TFB_SoundSample* sample);
static bool OnChunkEnd (TFB_SoundSample* sample, audio_Object buffer);
static void OnTrackEnd (TFB_SoundSample* sample);
static void OnTrackTag (TFB_SoundSample* sample, TFB_SoundTag* tag);

static TFB_SoundCallbacks trackCBs =
{
	OnTrackStart,
	OnChunkEnd,
	OnTrackEnd,
	OnTrackTag,
	NULL
};

//JumpTrack currently aborts the current track. However, it doesn't clear the
//data-structures as StopTrack does.  this allows for rewind even after the
//track has finished playing
//Question:  Should 'abort' call StopTrack?
void
JumpTrack (void)
{
	TFB_SoundChainData* scd;
	uint32 cur_time;
	sint32 total_length = (sint32)((last_chain->start_time + 
			last_chain->decoder->length) * (float)ONE_SECOND);

	if (!sound_sample)
		return;

	scd = (TFB_SoundChainData*) sound_sample->data;

	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	PauseStream (SPEECH_SOURCE);
	cur_time = GetTimeCounter();
	soundSource[SPEECH_SOURCE].start_time = 
			(sint32)cur_time - total_length;
	track_pos_changed = 1;
	scd->play_chain_ptr = last_chain;
	recompute_track_pos(sound_sample, first_chain, total_length + 1);
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	PlayingTrack();
}

//advance to the next track and start playing
// we no longer support advancing tracks this way.  Instead this should just start playing
// a stream.
void
PlayTrack (void)
{
	TFB_SoundChainData* scd;

	if (!sound_sample)
		return;

	scd = (TFB_SoundChainData*) sound_sample->data;

	if (scd->read_chain_ptr || sound_sample->decoder)
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
	TFB_SoundChainData* scd;

	if (!sound_sample)
		return;

	scd = (TFB_SoundChainData*) sound_sample->data;

	if (scd->read_chain_ptr || sound_sample->decoder)
	{
		// Only try to start the track if there is something to play
		audio_IntVal state;
		BOOLEAN playing;

		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		audio_GetSourcei (soundSource[SPEECH_SOURCE].handle, audio_SOURCE_STATE, &state);
		playing = PlayingStream (SPEECH_SOURCE);
		if (!track_pos_changed && !playing && state == audio_PAUSED)
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
		audio_DeleteBuffers (sound_sample->num_buffers, sound_sample->buffer);
		HFree (sound_sample->buffer);
		HFree (sound_sample->buffer_tag);
		HFree (sound_sample->data);
		HFree (sound_sample);
		sound_sample = NULL;
	}
	tct = tcur = 0;
	cur_page = 0;
	do_subtitles ((void *)~0);
}

bool is_sample_playing(TFB_SoundSample* sample)
{
	TFB_SoundChainData* scd = (TFB_SoundChainData*) sample->data;

	return (scd->read_chain_ptr && scd->play_chain_ptr)
			|| (!scd->read_chain_ptr && sample->decoder);
}

static void
DoTrackTag (TFB_SoundChain *chain)
{
	LockMutex (track_mutex);
	if (chain->callback)
		chain->callback ();
	tcur = chain->track_num;
	cur_page = (UNICODE *) chain->text;
	UnlockMutex (track_mutex);
}

static bool
OnTrackStart (TFB_SoundSample* sample)
{
	TFB_SoundChainData* scd = (TFB_SoundChainData*) sample->data;

	if (!scd->read_chain_ptr && !sample->decoder)
		return false;

	if (scd->read_chain_ptr)
	{
		sample->decoder = scd->read_chain_ptr->decoder;
		sample->offset = (sint32) (scd->read_chain_ptr->start_time * (float)ONE_SECOND);
	}
	else
		sample->offset = 0;

	scd->play_chain_ptr = scd->read_chain_ptr;

	if (scd->read_chain_ptr && scd->read_chain_ptr->tag_me)
		DoTrackTag(scd->read_chain_ptr);

	return true;
}

static bool
OnChunkEnd (TFB_SoundSample* sample, audio_Object buffer)
{
	TFB_SoundChainData* scd = (TFB_SoundChainData*) sample->data;

	if (!scd->read_chain_ptr || !scd->read_chain_ptr->next)
		return false;

	scd->read_chain_ptr = scd->read_chain_ptr->next;
	sample->decoder = scd->read_chain_ptr->decoder;
	SoundDecoder_Rewind (sample->decoder);
	log_add (log_Info, "Switching to stream %s at pos %d",
			sample->decoder->filename, sample->decoder->start_sample);
	
	if (sample->buffer_tag && scd->read_chain_ptr->tag_me)
	{
		TFB_TagBuffer (sample, buffer, scd->read_chain_ptr);
	}

	return true;
}

static void
OnTrackEnd (TFB_SoundSample* sample)
{
	TFB_SoundChainData* scd = (TFB_SoundChainData*) sample->data;

	sample->decoder = NULL;
	scd->read_chain_ptr = NULL;
}

static void
OnTrackTag (TFB_SoundSample* sample, TFB_SoundTag* tag)
{
	TFB_SoundChainData* scd = (TFB_SoundChainData*) sample->data;
	TFB_SoundChain* chain = (TFB_SoundChain*) tag->data;

	TFB_ClearBufferTag (tag);
	DoTrackTag (chain);

	scd->play_chain_ptr = scd->read_chain_ptr;
}

int
GetTimeStamps(UNICODE *TimeStamps, sint32 *time_stamps)
{
	int pos;
	int num = 0;
	while (*TimeStamps && (pos = strcspn (TimeStamps, ",\r\n")))
	{
		UNICODE valStr[32];
		uint32 val;
		
		strncpy (valStr, TimeStamps, pos);
		valStr[pos] = '\0';
		val = strtoul (valStr, NULL, 10);
		if (val)
		{
			*time_stamps = (sint32)val;
			num++;
			time_stamps++;
		}
		TimeStamps += pos;
		TimeStamps += strspn (TimeStamps, ",\r\n");
	}
	return (num);
}

#define TEXT_SPEED 80
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
			// XXX: this will only work when ASCII punctuation and spaces
			//   are used exclusively
			if (!ispunct (text[pos - 1]) && !isspace (text[pos - 1]))
			{
				split_text[page] = HMalloc (sizeof (UNICODE) * (pos + ellips + 4));
				if (ellips)
					strcpy (split_text[page], "..");
				strncpy (split_text[page] + ellips, text, pos);
				strcpy (split_text[page] + ellips + pos, "...");
				timestamp[page] = - pos * TEXT_SPEED;
				if (timestamp[page] > -1000)
					timestamp[page] = -1000;
				ellips = 2;
				text = text + pos;
				pos = 0;
				page ++;
			}
			else
			{
				split_text[page] = HMalloc (sizeof (UNICODE) * (pos + ellips + 1));
				if (ellips)
					strcpy (split_text[page], "..");
				strncpy (split_text[page] + ellips, text, pos);
				*(split_text[page] + ellips + pos) = 0;
				timestamp[page] = - pos * TEXT_SPEED;
				if (timestamp[page] > -1000)
					timestamp[page] = -1000;
				ellips = 0;
				text = text + pos;
				pos = 0;
				page ++;
			}
			while (text[pos] == '\n' || text[pos] == '\r')
				text++;
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
			strcpy (split_text[page], "..");
		strncpy (split_text[page] + ellips, text, pos);
		*(split_text[page] + ellips + pos) = 0;
		timestamp[page] = - pos * TEXT_SPEED;
		if (timestamp[page] > -1000)
			timestamp[page] = -1000;
		timestamp[page] += -1000;
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
		log_add (log_Debug, "SpliceMultiTrack(): no track text");
		return;
	}

	if (tct >= MAX_CLIPS)
	{
		log_add (log_Warning, "SpliceMultiTrack(): no more clip slots (%d)",
				MAX_CLIPS);
		return;
	}

	if (! sound_sample)
	{
		log_add (log_Warning, "SpliceMultiTrack(): Cannot be called before SpliceTrack()");
		return;
	}

	log_add (log_Info, "SpliceMultiTrack(): loading...");
	for (tracks = 0; *TrackNames && tracks < MAX_MULTI_TRACKS; TrackNames++, tracks++)
	{
		track_decs[tracks] = SoundDecoder_Load (contentDir, *TrackNames,
				32768, 0, - 3 * TEXT_SPEED);
		if (track_decs[tracks])
		{
			log_add (log_Info, "  track: %s, decoder: %s, rate %d format %x",
					*TrackNames,
					SoundDecoder_GetName (track_decs[tracks]),
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
			log_add (log_Warning, "SpliceMultiTrack(): couldn't load %s\n",
					*TrackNames);
			tracks--;
		}
	}
	track_decs[tracks] = 0; // term

	if (tracks == 0)
	{
		log_add (log_Warning, "SpliceMultiTrack(): no tracks loaded");
		return;
	}

	slen = strlen (TrackText);
	if (tct)
	{
		int slen1 = strlen ((UNICODE *)cur_text_chain->text);
		cur_text_chain->text = HRealloc (
				(UNICODE *)cur_text_chain->text, slen1 + slen + 1);
		strcpy (&((UNICODE *)cur_text_chain->text)[slen1], TrackText);
	}
	else
	{
		begin_chain->text = HMalloc (slen + 1);
		strcpy ((UNICODE *)begin_chain->text, TrackText);
		cur_text_chain = begin_chain;
		begin_chain->tag_me = 1;
		begin_chain->track_num = tct;
		tct++;
	}
	no_page_break = 1;
	
}

void
SpliceTrack (UNICODE *TrackName, UNICODE *TrackText, UNICODE *TimeStamp, TFB_TrackCB cb)
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

				if (!last_ts_chain || !last_ts_chain->text)
				{
					log_add (log_Warning, "SpliceTrack(): Tried to append"
							" a subtitle to a NULL string");
					return;
				}
				split_text = SplitSubPages (TrackText, time_stamps, &num_pages);
				if (! split_text)
				{
					log_add (log_Warning, "SpliceTrack(): Failed to parse sutitles");
					return;
				}
				oTT = (UNICODE *)last_ts_chain->text;
				slen1 = strlen (oTT);
				slen2 = strlen (split_text[0]);
				last_ts_chain->text = HRealloc (oTT, slen1 + slen2 + 1);
				strcpy (&((UNICODE *)last_ts_chain->text)[slen1], split_text[0]);
				HFree (split_text[0]);
				for (page_counter = 1; page_counter < num_pages; page_counter++)
				{
					if (! last_ts_chain->next)
					{
						log_add (log_Warning, "SpliceTrack(): More text pages than timestamps!");
						break;
					}
					last_ts_chain = last_ts_chain->next;
					last_ts_chain->text = split_text[page_counter];
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
				log_add (log_Warning, "SpliceTrack(): Failed to parse sutitles");
				return;
			}
			if (no_page_break && tct)
			{
				int slen1, slen2;
				UNICODE *oTT;

				oTT = (UNICODE *)cur_text_chain->text;
				slen1 = strlen (oTT);
				slen2 = strlen (split_text[0]);
				cur_text_chain->text = HRealloc (oTT, slen1 + slen2 + 1);
				strcpy (&((UNICODE *)cur_text_chain->text)[slen1], split_text[0]);
				HFree (split_text[0]);
			}
			else
				tct++;

			log_add (log_Info, "SpliceTrack(): loading %s", TrackName);

			if (TimeStamp)
			{
				num_timestamps = GetTimeStamps (TimeStamp, time_stamps) + 1;
				if (num_timestamps < num_pages)
					log_add (log_Warning, "SpliceTrack(): number of timestamps"
							" doesn't match number of pages!");
			}
			else
				num_timestamps = num_pages;
			startTime = 0;
			for (page_counter = 0; page_counter < num_timestamps; page_counter++)
			{
				if (! sound_sample)
				{
					TFB_SoundDecoder *decoder;
					TFB_SoundChainData* scd;

					track_mutex = CreateMutex("trackplayer mutex", SYNC_CLASS_TOPLEVEL | SYNC_CLASS_AUDIO);
					sound_sample = (TFB_SoundSample *) HMalloc (sizeof (TFB_SoundSample));
					scd = (TFB_SoundChainData *) HCalloc (sizeof (TFB_SoundChainData));
					sound_sample->data = scd;
					sound_sample->callbacks = trackCBs;
					sound_sample->num_buffers = 8;
					sound_sample->buffer_tag = HCalloc (sizeof (TFB_SoundTag) * sound_sample->num_buffers);
					sound_sample->buffer = HMalloc (sizeof (audio_Object) * sound_sample->num_buffers);
					audio_GenBuffers (sound_sample->num_buffers, sound_sample->buffer);
					decoder = SoundDecoder_Load (contentDir, TrackName, 4096,
							startTime, time_stamps[page_counter]);
					scd->read_chain_ptr = create_soundchain (decoder, 0.0);
					sound_sample->decoder = decoder;
					first_chain = last_chain = scd->read_chain_ptr;
					sound_sample->length = 0;
					scd->play_chain_ptr = 0;
				}
				else
				{
					TFB_SoundDecoder *decoder;
					decoder =  SoundDecoder_Load (contentDir, TrackName, 4096,
							startTime, time_stamps[page_counter]);
					last_chain->next = create_soundchain (decoder, sound_sample->length);
					last_chain = last_chain->next;
				}
				startTime += abs (time_stamps[page_counter]);
			
#if 0
				log_add (log_Debug, "page (%d of %d): %d ts: %d",
						page_counter, num_pages,
						startTime, time_stamps[page_counter]);
#endif
				if (last_chain->decoder)
				{
					static float old_volume = 0.0f;
					if (last_chain->decoder->is_null)
					{
						if (speechVolumeScale != 0.0f)
						{
							/* No voice ogg available so zeroing speech volume to
							ensure proper operation of oscilloscope and music fading */
							old_volume = speechVolumeScale;
							speechVolumeScale = 0.0f;
							log_add (log_Warning, "SpliceTrack(): no voice ogg"
									" available so setting speech volume to zero");
						}
					}
					else if (old_volume != 0.0f && speechVolumeScale != old_volume)
					{
						/* This time voice ogg is there */
						log_add (log_Warning, "SpliceTrack(): restoring speech volume");
						speechVolumeScale = old_volume;
						old_volume = 0.0f;
					}

					sound_sample->length += last_chain->decoder->length;
					if (! no_page_break)
					{
						last_chain->tag_me = 1;
						// last_chain->tag.value = (void *)(((tct - 1) << 8) | page_counter);
						last_chain->track_num = tct - 1;
						if (page_counter < num_pages)
						{
							last_chain->text = (void *)split_text[page_counter];
							last_ts_chain = last_chain;
						}
						last_chain->callback = cb;
						cur_text_chain = last_chain;
					}
					no_page_break = 0;
				}
				else
				{
					log_add (log_Warning, "SpliceTrack(): couldn't load %s", TrackName);
					audio_DeleteBuffers (sound_sample->num_buffers, sound_sample->buffer);
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
	TFB_SoundChainData* scd;
	TFB_SoundChain *cur_chain = first_chain;

	if (! sample)
		return;

	scd = (TFB_SoundChainData*) sample->data;

	while (cur_chain->next &&
			(sint32)(cur_chain->next->start_time * (float)ONE_SECOND) < offset)
	{
		if (cur_chain->tag_me)
			DoTrackTag (cur_chain);
		cur_chain = cur_chain->next;
	}
	if (cur_chain->tag_me)
		DoTrackTag (cur_chain);
	if ((sint32)((cur_chain->start_time + cur_chain->decoder->length) * (float)ONE_SECOND) < offset)
	{
		scd->read_chain_ptr = NULL;
		sample->decoder = NULL;
	}
	else
	{
		scd->read_chain_ptr = cur_chain;
		SoundDecoder_Seek(scd->read_chain_ptr->decoder,
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
		TFB_SoundChainData* scd = (TFB_SoundChainData*) sound_sample->data;
		TFB_SoundChain *prev_chain;

		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		prev_chain = get_previous_chain (first_chain, scd->play_chain_ptr);
		if (prev_chain)
		{
			scd->read_chain_ptr = prev_chain;
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
				(sint32)cur_time - total_length - 1;
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
		TFB_SoundChainData* scd = (TFB_SoundChainData*) sound_sample->data;
		TFB_SoundChain *cur_ptr = scd->play_chain_ptr;

		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		while (cur_ptr->next && !cur_ptr->next->tag_me)
			cur_ptr = cur_ptr->next;
		if (cur_ptr->next)
		{
			scd->read_chain_ptr = cur_ptr->next;
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
			assert (soundSource[SPEECH_SOURCE].sample->decoder->format == audio_FORMAT_MONO16);

			if (delta < 0)
			{
				log_add (log_Debug, "GetSoundData(): something's messed"
						" with timing, delta %ld", delta);
				delta = 0;
			}
			else if (delta > (int)(soundSource[SPEECH_SOURCE].sbuf_size * 2))
			{
#if 0
				log_add (log_Debug, "GetSoundData(): something's messed"
						" with timing, delta %d", delta);
#endif
				delta = 0;
			}
#if 0
			log_add (log_Debug, "played_data %d total_decoded %d delta %d",
					played_data, soundSource[SPEECH_SOURCE].total_decoded,
					delta);
#endif
			pos = soundSource[SPEECH_SOURCE].sbuf_offset + delta;
			if (pos % 2 == 1)
				pos++;

			// pos is an unsigned data type; this assertion cannot fail!
			// assert (pos >= 0);

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
			assert (soundSource[MUSIC_SOURCE].sample->decoder->format == audio_FORMAT_STEREO16);

			step = soundSource[MUSIC_SOURCE].sample->decoder->frequency / 11025 * 16;
			if (step % 2 == 1)
				step++;

			if (delta < 0)
			{
#if 0				
				log_add (log_Debug, "GetSoundData(): something's messed"
						" with timing, delta %d", delta);
#endif
				delta = 0;
			}
			else if (delta > (int)(soundSource[MUSIC_SOURCE].sbuf_size * 2))
			{
#if 0				
				log_add (log_Debug, "GetSoundData(): something's messed"
						" with timing, delta %d", delta);
#endif
				delta = 0;
			}
#if 0			
			log_add (log_Debug, "played_data %d total_decoded %d delta %d",
					played_data, soundSource[MUSIC_SOURCE].total_decoded,
					delta);
#endif
			pos = soundSource[MUSIC_SOURCE].sbuf_offset + delta;
			if (pos % 2 == 1)
				pos++;

			// pos is an unsigned type; this assertion cannot fail!
			// assert (pos >= 0);

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

TFB_SoundChain *
create_soundchain (TFB_SoundDecoder *decoder, float startTime)
{
	TFB_SoundChain *chain;
	chain = (TFB_SoundChain *)HMalloc (sizeof (TFB_SoundChain));
	chain->decoder = decoder;
	chain->next = NULL;
	chain->start_time = startTime;
	chain->tag_me = 0;
	chain->text = 0;
	return (chain);
}

void
destroy_soundchain (TFB_SoundChain *chain)
{
	while (chain->next)
	{
		TFB_SoundChain *tmp_chain = chain->next;
		chain->next = chain->next->next;
		if (tmp_chain->decoder)
			SoundDecoder_Free (tmp_chain->decoder);
		if (tmp_chain->text)
			HFree (tmp_chain->text);
		HFree (tmp_chain);
	}
	SoundDecoder_Free (chain->decoder);
	HFree (chain);
}

TFB_SoundChain *
get_previous_chain (TFB_SoundChain *first_chain, TFB_SoundChain *current_chain)
{
	TFB_SoundChain *prev_chain, *last_valid = NULL;
	prev_chain = first_chain;
	if (prev_chain == current_chain)
		return (prev_chain);
	while (prev_chain->next)
	{
		if (prev_chain->tag_me)
			last_valid = prev_chain;
		if (prev_chain->next == current_chain)
			return (last_valid);
		prev_chain = prev_chain->next;
	}
	return (first_chain);
}
