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
#include "options.h"
#include <ctype.h>
#include <stdlib.h>


static int track_count;       //total number of subtitle tracks
static int cur_track;         //currently playing subtitle track
static UNICODE *cur_page = 0; //current page of subtitle track 
static int no_page_break = 0;
static int track_pos_changed = 0; // set whenever  ff, frev is enabled

static TFB_SoundSample *sound_sample = NULL;
TFB_SoundChain *chain_head = NULL; //first decoder in linked list
static TFB_SoundChain *chain_tail = NULL;  //last decoder in linked list
static TFB_SoundChain *last_sub = NULL; //last element in the chain with a subtitle

static Mutex track_mutex; //protects cur_track and track_count
void recompute_track_pos (TFB_SoundSample *sample, TFB_SoundChain *head,
			sint32 offset);
bool is_sample_playing(TFB_SoundSample* samp);

void destroy_sound_sample (TFB_SoundSample *sample);

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
	sint32 total_length = (sint32)((chain_tail->start_time + 
			chain_tail->decoder->length) * (float)ONE_SECOND);

	if (!sound_sample)
		return;

	scd = (TFB_SoundChainData*) sound_sample->data;

	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	PauseStream (SPEECH_SOURCE);
	cur_time = GetTimeCounter();
	soundSource[SPEECH_SOURCE].start_time = 
			(sint32)cur_time - total_length;
	track_pos_changed = 1;
	scd->play_chain_ptr = chain_tail;
	recompute_track_pos (sound_sample, chain_head, total_length + 1);
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
ResumeTrack (void)
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
PlayingTrack (void)
{
	// this is not a great way to detect whether the track is playing,
	// but as it should work during fast-forward/rewind, 'PlayingStream' can't be used
//	if (track_count == 0)
//		return ((COUNT)~0);
	if (sound_sample && is_sample_playing (sound_sample))
	{
		int last_track;
		UNICODE *last_page;
		LockMutex (track_mutex);
		last_track = cur_track;
		last_page = cur_page;
		UnlockMutex (track_mutex);
		if (do_subtitles (last_page))
		{
			return cur_track + 1;
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
StopTrack (void)
{
	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	StopStream (SPEECH_SOURCE);
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);

	if (chain_head)
	{
		destroy_soundchain (chain_head);
		chain_head = NULL;
		chain_tail = NULL;
		last_sub = NULL;
	}
	if (sound_sample)
	{
		DestroyMutex (track_mutex);
		destroy_sound_sample (sound_sample);
		sound_sample = NULL;
	}
	track_count = 0;
	cur_track = 0;
	cur_page = 0;
	do_subtitles ((void *)~0);
}

static bool
is_sample_playing (TFB_SoundSample* sample)
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
	cur_track = chain->track_num;
	cur_page = chain->text;
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
GetTimeStamps (UNICODE *TimeStamps, sint32 *time_stamps)
{
	int pos;
	int num = 0;
	
	while (*TimeStamps && (pos = strcspn (TimeStamps, ",\r\n")))
	{
		UNICODE valStr[32];
		uint32 val;
		
		memcpy (valStr, TimeStamps, pos);
		valStr[pos] = '\0';
		val = strtoul (valStr, NULL, 10);
		if (val)
		{
			*time_stamps = val;
			num++;
			time_stamps++;
		}
		TimeStamps += pos;
		TimeStamps += strspn (TimeStamps, ",\r\n");
	}
	return (num);
}

#define TEXT_SPEED 80
// Returns number of parsed pages 
int
SplitSubPages (UNICODE *text, UNICODE *pages[], sint32 timestamp[], int size)
{
	int lead_ellips = 0;
	COUNT page;
	
	for (page = 0; page < size && *text != '\0'; ++page)
	{
		int aft_ellips;
		int pos;

		// seek to EOL or end of the string
		pos = strcspn (text, "\r\n");
		// XXX: this will only work when ASCII punctuation and spaces
		//   are used exclusively
		aft_ellips = 3 * (text[pos] != '\0' && pos > 0 &&
				!ispunct (text[pos - 1]) && !isspace (text[pos - 1]));
		pages[page] = HMalloc (sizeof (UNICODE) *
				(lead_ellips + pos + aft_ellips + 1));
		if (lead_ellips)
			strcpy (pages[page], "..");
		memcpy (pages[page] + lead_ellips, text, pos);
		pages[page][lead_ellips + pos] = '\0'; // string term
		if (aft_ellips)
			strcpy (pages[page] + lead_ellips + pos, "...");
		timestamp[page] = pos * TEXT_SPEED;
		if (timestamp[page] < 1000)
			timestamp[page] = 1000;
		lead_ellips = aft_ellips ? 2 : 0;
		text += pos;
		// Skip any EOL
		text += strspn (text, "\r\n");
	}

	return page;
}

// decodes several tracks into one and adds it to queue
// track list is NULL-terminated
// May only be called after at least one SpliceTrack(). This is a limitation
// for the sake of timestamps, but it does not have to be so.
void
SpliceMultiTrack (UNICODE *TrackNames[], UNICODE *TrackText)
{
#define MAX_MULTI_TRACKS  20
#define MAX_MULTI_BUFFERS 100
	TFB_SoundDecoder* track_decs[MAX_MULTI_TRACKS + 1];
	int tracks;
	int slen1, slen2;

	if (!TrackText)
	{
		log_add (log_Debug, "SpliceMultiTrack(): no track text");
		return;
	}

	if (!sound_sample || !chain_tail)
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

			chain_tail->next = create_soundchain (track_decs[tracks], sound_sample->length);
			chain_tail = chain_tail->next;
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

	slen1 = strlen (last_sub->text);
	slen2 = strlen (TrackText);
	last_sub->text = HRealloc (last_sub->text, slen1 + slen2 + 1);
	strcpy (last_sub->text + slen1, TrackText);

	no_page_break = 1;
}

// XXX: This code and the entire trackplayer are begging to be overhauled
void
SpliceTrack (UNICODE *TrackName, UNICODE *TrackText, UNICODE *TimeStamp, TFB_TrackCB cb)
{
	static UNICODE last_track_name[128] = "";
	static unsigned long startTime = 0;
#define MAX_PAGES 50
	UNICODE *pages[MAX_PAGES];
	sint32 time_stamps[MAX_PAGES];
	int num_pages;
	int page;

	if (!TrackText)
		return;

	if (!TrackName)
	{	// Appending a piece of subtitles to the last track
		int slen1, slen2;

		if (track_count == 0)
		{
			log_add (log_Warning, "SpliceTrack(): Tried to append a subtitle,"
					" but no current track");
			return;
		}

		if (!last_sub || !last_sub->text)
		{
			log_add (log_Warning, "SpliceTrack(): Tried to append a subtitle"
					" to a NULL string");
			return;
		}
		
		num_pages = SplitSubPages (TrackText, pages, time_stamps, MAX_PAGES);
		if (num_pages == 0)
		{
			log_add (log_Warning, "SpliceTrack(): Failed to parse subtitles");
			return;
		}
		// The last page's stamp is a suggested value. The track should
		// actually play to the end.
		time_stamps[num_pages - 1] = -time_stamps[num_pages - 1];

		// Add the first piece to the last subtitle page
		slen1 = strlen (last_sub->text);
		slen2 = strlen (pages[0]);
		last_sub->text = HRealloc (last_sub->text, slen1 + slen2 + 1);
		strcpy (last_sub->text + slen1, pages[0]);
		HFree (pages[0]);
		
		// Add the rest of the pages
		for (page = 1; page < num_pages; ++page)
		{
			if (last_sub->next)
			{	// nodes prepared by previous call, just fill in the subs
				last_sub = last_sub->next;
				last_sub->text = pages[page];
			}
			else
			{	// probably no timestamps were provided, so need more work
				TFB_SoundDecoder *decoder = SoundDecoder_Load (contentDir,
						last_track_name, 4096, startTime, time_stamps[page]);
				if (!decoder)
				{
					log_add (log_Warning, "SpliceTrack(): couldn't load %s", TrackName);
					break;
				}
				startTime += (unsigned long)(decoder->length * 1000);
				chain_tail->next = create_soundchain (decoder, sound_sample->length);
				chain_tail = chain_tail->next;
				chain_tail->tag_me = 1;
				chain_tail->track_num = track_count - 1;
				chain_tail->text = pages[page];
				chain_tail->callback = cb;
				// We have to tag only one page with a callback
				cb = NULL;
				last_sub = chain_tail;
				sound_sample->length += decoder->length;
			}
		}
	}
	else
	{	// Adding a new track
		int num_timestamps = 0;

		utf8StringCopy (last_track_name, sizeof (last_track_name), TrackName);

		num_pages = SplitSubPages (TrackText, pages, time_stamps, MAX_PAGES);
		if (num_pages == 0)
		{
			log_add (log_Warning, "SpliceTrack(): Failed to parse sutitles");
			return;
		}
		// The last page's stamp is a suggested value. The track should
		// actually play to the end.
		time_stamps[num_pages - 1] = -time_stamps[num_pages - 1];

		if (no_page_break && track_count)
		{
			int slen1, slen2;

			slen1 = strlen (last_sub->text);
			slen2 = strlen (pages[0]);
			last_sub->text = HRealloc (last_sub->text, slen1 + slen2 + 1);
			strcpy (last_sub->text + slen1, pages[0]);
			HFree (pages[0]);
		}
		else
			track_count++;

		log_add (log_Info, "SpliceTrack(): loading %s", TrackName);

		if (TimeStamp)
		{
			num_timestamps = GetTimeStamps (TimeStamp, time_stamps) + 1;
			if (num_timestamps < num_pages)
			{
				log_add (log_Warning, "SpliceTrack(): number of timestamps"
						" doesn't match number of pages!");
			}
			else if (num_timestamps > num_pages)
			{	// We most likely will get more subtitles appended later
				// Set the last page to the rest of the track
				time_stamps[num_timestamps - 1] = -100000;
			}
		}
		else
		{
			num_timestamps = num_pages;
		}
		
		startTime = 0;
		for (page = 0; page < num_timestamps; ++page)
		{
			static float old_volume = 0.0f;
			TFB_SoundDecoder *decoder = SoundDecoder_Load (contentDir,
					TrackName, 4096, startTime, time_stamps[page]);
			if (!decoder)
			{
				log_add (log_Warning, "SpliceTrack(): couldn't load %s", TrackName);
				break;
			}

			if (!sound_sample)
			{
				TFB_SoundChainData* scd = HCalloc (sizeof (TFB_SoundChainData));
				track_mutex = CreateMutex ("trackplayer mutex", SYNC_CLASS_TOPLEVEL | SYNC_CLASS_AUDIO);
				sound_sample = (TFB_SoundSample *) HMalloc (sizeof (TFB_SoundSample));
				sound_sample->data = scd;
				sound_sample->callbacks = trackCBs;
				sound_sample->num_buffers = 8;
				sound_sample->buffer_tag = HCalloc (sizeof (TFB_SoundTag) * sound_sample->num_buffers);
				sound_sample->buffer = HMalloc (sizeof (audio_Object) * sound_sample->num_buffers);
				sound_sample->decoder = decoder;
				sound_sample->length = 0;
				audio_GenBuffers (sound_sample->num_buffers, sound_sample->buffer);
				chain_head = create_soundchain (decoder, 0.0);
				chain_tail = chain_head;
				scd->read_chain_ptr = chain_head;
				scd->play_chain_ptr = NULL;
			}
			else
			{
				chain_tail->next = create_soundchain (decoder, sound_sample->length);
				chain_tail = chain_tail->next;
			}
			startTime += (unsigned long)(decoder->length * 1000);
#if 0
			log_add (log_Debug, "page (%d of %d): %d ts: %d",
					page, num_pages,
					startTime, time_stamps[page]);
#endif
			if (decoder->is_null)
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

			sound_sample->length += decoder->length;
			if (!no_page_break)
			{
				chain_tail->tag_me = 1;
				// chain_tail->tag.value = (void *)(((track_count - 1) << 8) | page);
				chain_tail->track_num = track_count - 1;
				if (page < num_pages)
				{
					chain_tail->text = pages[page];
					last_sub = chain_tail;
				}
				chain_tail->callback = cb;
				// We have to tag only one page with a callback
				cb = NULL;
			}
			no_page_break = 0;
		}
	}
}

void
PauseTrack (void)
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
recompute_track_pos (TFB_SoundSample *sample, TFB_SoundChain *head, sint32 offset)
{
	TFB_SoundChainData* scd;
	TFB_SoundChain *cur = head;

	if (! sample)
		return;

	scd = (TFB_SoundChainData*) sample->data;

	while (cur->next &&
			(sint32)(cur->next->start_time * (float)ONE_SECOND) < offset)
	{
		if (cur->tag_me)
			DoTrackTag (cur);
		cur = cur->next;
	}
	if (cur->tag_me)
		DoTrackTag (cur);
	if ((sint32)((cur->start_time + cur->decoder->length) * (float)ONE_SECOND) < offset)
	{
		scd->read_chain_ptr = NULL;
		sample->decoder = NULL;
	}
	else
	{
		scd->read_chain_ptr = cur;
		SoundDecoder_Seek(scd->read_chain_ptr->decoder,
				(uint32)(1000 * (offset / (float)ONE_SECOND - cur->start_time)));
	}
}

void
FastReverse_Smooth (void)
{
	if (sound_sample)
	{
		sint32 offset;
		uint32 cur_time;
		sint32 total_length = (sint32)((chain_tail->start_time + 
				chain_tail->decoder->length) * (float)ONE_SECOND);
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
		recompute_track_pos (sound_sample, chain_head, offset);
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		PlayingTrack();

	}
}

void
FastReverse_Page (void)
{
	if (sound_sample)
	{
		TFB_SoundChainData* scd = (TFB_SoundChainData*) sound_sample->data;
		TFB_SoundChain *prev;

		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		prev = get_chain_previous (chain_head, scd->play_chain_ptr);
		if (prev)
		{
			scd->read_chain_ptr = prev;
			PlayStream (sound_sample,
					SPEECH_SOURCE, false,
					speechVolumeScale != 0.0f, true);
		}
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	}
}

void
FastForward_Smooth (void)
{
	if (sound_sample)
	{
		sint32 offset;
		uint32 cur_time;
		sint32 total_length = (sint32)((chain_tail->start_time + 
				chain_tail->decoder->length) * (float)ONE_SECOND);
		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		PauseStream (SPEECH_SOURCE);
		cur_time = GetTimeCounter();
		soundSource[SPEECH_SOURCE].start_time -= ACCEL_SCROLL_SPEED;
		if ((sint32)cur_time - soundSource[SPEECH_SOURCE].start_time > total_length)
			soundSource[SPEECH_SOURCE].start_time = 
				(sint32)cur_time - total_length - 1;
		offset = cur_time - soundSource[SPEECH_SOURCE].start_time;
		track_pos_changed = 1;
		recompute_track_pos (sound_sample, chain_head, offset);
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		PlayingTrack ();
	}
}

int
FastForward_Page (void)
{
	if (sound_sample)
	{
		TFB_SoundChainData* scd = (TFB_SoundChainData*) sound_sample->data;
		TFB_SoundChain *cur = scd->play_chain_ptr;

		LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		while (cur->next && !cur->next->tag_me)
			cur = cur->next;
		if (cur->next)
		{
			scd->read_chain_ptr = cur->next;
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
	chain = HMalloc (sizeof (TFB_SoundChain));
	chain->decoder = decoder;
	chain->next = NULL;
	chain->start_time = startTime;
	chain->tag_me = 0;
	chain->text = 0;
	return chain;
}

void
destroy_soundchain (TFB_SoundChain *chain)
{
	TFB_SoundChain *next = NULL;
	for ( ; chain; chain = next)
	{
		next = chain->next;
		if (chain->decoder)
			SoundDecoder_Free (chain->decoder);
		HFree (chain->text);
		HFree (chain);
	}
}

void
destroy_sound_sample (TFB_SoundSample *sample)
{
	if (sample->buffer)
	{
		audio_DeleteBuffers (sample->num_buffers, sample->buffer);
		HFree (sample->buffer);
	}
	HFree (sample->buffer_tag);
	HFree (sample->data);
	HFree (sample);
}

TFB_SoundChain *
get_chain_previous (TFB_SoundChain *head, TFB_SoundChain *current)
{
	TFB_SoundChain *prev, *last_valid = NULL;
	prev = head;
	if (prev == current)
		return prev;
	while (prev->next)
	{
		if (prev->tag_me)
			last_valid = prev;
		if (prev->next == current)
			return last_valid;
		prev = prev->next;
	}
	return head;
}
