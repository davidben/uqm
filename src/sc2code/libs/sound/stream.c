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
#include <string.h>
#include "sound.h"
#include "libs/tasklib.h"
#include "libs/log.h"


void
PlayStream (TFB_SoundSample *sample, uint32 source, bool looping, bool scope,
		bool rewind)
{	
	uint32 i, pos = 0;
	sint32 offset = 0;

	if (!sample)
		return;

	StopStream (source);
	if (sample->callbacks.OnStartStream &&
		!sample->callbacks.OnStartStream (sample))
		return; // callback failed

	if (sample->buffer_tag)
		memset (sample->buffer_tag, 0,
				sample->num_buffers * sizeof (sample->buffer_tag[0]));

	offset = sample->offset;
	if (rewind)
		SoundDecoder_Rewind (sample->decoder);
	else
		offset += (sint32)(SoundDecoder_GetTime (sample->decoder) *
				(float)ONE_SECOND);
	soundSource[source].sample = sample;
	soundSource[source].sample->decoder->looping = looping;
	audio_Sourcei (soundSource[source].handle, audio_LOOPING, false);

	if (scope)
	{
		soundSource[source].sbuffer = HMalloc (PAD_SCOPE_BYTES);
	}

	for (i = 0; i < sample->num_buffers; ++i)
	{
		uint32 decoded_bytes;

		decoded_bytes = SoundDecoder_Decode (sample->decoder);
#if 0		
		log_add (log_Debug, "PlayStream(): source:%d filename:%s start:%d position:%d bytes:%d\n",
				source, sample->decoder->filename, sample->decoder->start_sample,
				sample->decoder->pos, decoded_bytes);
#endif
		if (decoded_bytes == 0)
			break;
		
		audio_BufferData (sample->buffer[i], sample->decoder->format,
				sample->decoder->buffer, decoded_bytes,
				sample->decoder->frequency);
		audio_SourceQueueBuffers (soundSource[source].handle, 1,
				&sample->buffer[i]);
		if (sample->callbacks.OnQueueBuffer)
			sample->callbacks.OnQueueBuffer (sample, sample->buffer[i]);

		if (scope)
		{
			UBYTE *p;
			soundSource[source].sbuffer = HRealloc (
					soundSource[source].sbuffer,
					pos + decoded_bytes + PAD_SCOPE_BYTES);
			p = (UBYTE *)soundSource[source].sbuffer;
			memcpy (&p[pos], sample->decoder->buffer, decoded_bytes);
			pos += decoded_bytes;
		}

		if (sample->decoder->error)
		{
			if (sample->decoder->error != SOUNDDECODER_EOF ||
					!sample->callbacks.OnEndChunk ||
					!sample->callbacks.OnEndChunk (sample, sample->buffer[i]))
				break;
		}
	}

	soundSource[source].sbuf_size = pos + PAD_SCOPE_BYTES;
	soundSource[source].sbuf_start = pos;
	soundSource[source].sbuf_lasttime = GetTimeCounter ();
	soundSource[source].start_time = (sint32)GetTimeCounter () - offset;
	soundSource[source].stream_should_be_playing = TRUE;
	audio_SourcePlay (soundSource[source].handle);
}

void
StopStream (uint32 source)
{
	soundSource[source].stream_should_be_playing = FALSE;
	soundSource[source].sample = NULL;

	if (soundSource[source].sbuffer)
	{
		void *sbuffer = soundSource[source].sbuffer;
		soundSource[source].sbuffer = NULL;
		HFree (sbuffer);
	}
	soundSource[source].sbuf_start = 0;
	soundSource[source].sbuf_size = 0;
	soundSource[source].sbuf_offset = 0;

	StopSource (source);
}

void
PauseStream (uint32 source)
{
	soundSource[source].stream_should_be_playing = FALSE;
	audio_SourcePause (soundSource[source].handle);
}

void
ResumeStream (uint32 source)
{
	soundSource[source].stream_should_be_playing = TRUE;
	audio_SourcePlay (soundSource[source].handle);
}

void
SeekStream (uint32 source, uint32 pos)
{
	TFB_SoundSample* sample = soundSource[source].sample;
	bool looping;
	bool scope;

	if (!sample)
		return;
	looping = sample->decoder->looping;
	scope = soundSource[source].sbuffer != NULL;

	StopSource (source);
	SoundDecoder_Seek (sample->decoder, pos);
	PlayStream (sample, source, looping, scope, false);
}

BOOLEAN
PlayingStream (uint32 source)
{	
	return soundSource[source].stream_should_be_playing;
}

TFB_SoundTag*
TFB_FindTaggedBuffer (TFB_SoundSample* sample, audio_Object buffer)
{
	uint32 buf_num;

	for (buf_num = 0;
			buf_num < sample->num_buffers &&
			(!sample->buffer_tag[buf_num].in_use ||
			 sample->buffer_tag[buf_num].buf_name != buffer
			);
			buf_num++)
		;
	
	return buf_num < sample->num_buffers ?
			&sample->buffer_tag[buf_num] : NULL;
}

void
TFB_TagBuffer (TFB_SoundSample* sample, audio_Object buffer, void* data)
{
	uint32 buf_num;

	for (buf_num = 0;
			buf_num < sample->num_buffers &&
			sample->buffer_tag[buf_num].in_use &&
			sample->buffer_tag[buf_num].buf_name != buffer;
			buf_num++)
		;

	if (buf_num >= sample->num_buffers)
		return; // no empty slot

	sample->buffer_tag[buf_num].in_use = 1;
	sample->buffer_tag[buf_num].buf_name = buffer;
	sample->buffer_tag[buf_num].data = data;
}

void
TFB_ClearBufferTag (TFB_SoundTag* ptag)
{
	ptag->in_use = 0;
	ptag->buf_name = 0;
}

int
StreamDecoderTaskFunc (void *data)
{
	Task task = (Task)data;
	int i;
	audio_Object last_buffer[NUM_SOUNDSOURCES];

	for (i = 0; i < NUM_SOUNDSOURCES; i++)
		last_buffer[i] = 0;

	while (!Task_ReadState (task, TASK_EXIT))
	{

		TaskSwitch ();

		for (i = MUSIC_SOURCE; i < NUM_SOUNDSOURCES; ++i)
		{
			audio_IntVal processed, queued;
			audio_IntVal state;

			LockMutex (soundSource[i].stream_mutex);

			if (!soundSource[i].sample ||
				!soundSource[i].sample->decoder ||
				!soundSource[i].stream_should_be_playing ||
				soundSource[i].sample->decoder->error == SOUNDDECODER_ERROR)
			{
				UnlockMutex (soundSource[i].stream_mutex);
				continue;
			}

			audio_GetSourcei (soundSource[i].handle, audio_BUFFERS_PROCESSED,
					&processed);
			audio_GetSourcei (soundSource[i].handle, audio_BUFFERS_QUEUED,
					&queued);

			if (processed == 0)
			{
				audio_GetSourcei (soundSource[i].handle, audio_SOURCE_STATE,
						&state);			
				if (state != audio_PLAYING)
				{
					if (queued == 0 && soundSource[i].sample->decoder->error
							== SOUNDDECODER_EOF)
					{
						log_add (log_Info, "StreamDecoderTaskFunc(): "
								"finished playing %s, source %d",
								soundSource[i].sample->decoder->filename, i);
						soundSource[i].stream_should_be_playing = FALSE;
						if (soundSource[i].sample->callbacks.OnEndStream)
							soundSource[i].sample->callbacks.OnEndStream (
									soundSource[i].sample);
 					}
					else
 					{
						log_add (log_Warning, "StreamDecoderTaskFunc(): buffer "
								"underrun when playing %s, source %d",
								soundSource[i].sample->decoder->filename, i);
						audio_SourcePlay (soundSource[i].handle);
					}
				}
			}
            
#if 0
			log_add (log_Debug, "StreamDecoderTaskFunc(): source %d, processed %d queued %d",
					i, processed, queued);
#endif

			while (processed)
			{
				uint32 error;
				audio_Object buffer;
				uint32 decoded_bytes;

				audio_GetError (); // clear error state

				audio_SourceUnqueueBuffers (soundSource[i].handle, 1, &buffer);

				error = audio_GetError();
				if (error != audio_NO_ERROR)
				{
					log_add (log_Warning, "StreamDecoderTaskFunc(): OpenAL "
							"error after alSourceUnqueueBuffers: %x, "
							"file %s, source %d", error,
							soundSource[i].sample->decoder->filename, i);
					break;
				}

				if (soundSource[i].sample->callbacks.OnTaggedBuffer)
				{
					TFB_SoundTag* tag = TFB_FindTaggedBuffer (
							soundSource[i].sample, buffer);
					if (tag)
					{
						soundSource[i].sample->callbacks.OnTaggedBuffer (
								soundSource[i].sample, tag);
					}
				}
				{
					audio_IntVal buf_size;
					soundSource[i].sbuf_lasttime = GetTimeCounter ();
					audio_GetBufferi(buffer, audio_SIZE, &buf_size);
					soundSource[i].sbuf_offset += buf_size;
					if (soundSource[i].sbuf_offset > soundSource[i].sbuf_size)
						soundSource[i].sbuf_offset -=
								soundSource[i].sbuf_size;
				}
				//soundSource[i].total_decoded += soundSource[i].sample->decoder->buffer_size;

				if (soundSource[i].sample->decoder->error)
				{
					if (soundSource[i].sample->decoder->error ==
							SOUNDDECODER_EOF)
					{
						if (!soundSource[i].sample->callbacks.OnEndChunk ||
								!soundSource[i].sample->callbacks.OnEndChunk (
									soundSource[i].sample, last_buffer[i]))
						{
#if 0
							log_add (log_Debug, "StreamDecoderTaskFunc(): decoder->error is eof for %s",
									soundSource[i].sample->decoder->filename);
#endif
							processed--;
							continue;
						}
					}
					else
					{
#if 0						
						log_add (log_Debug, "StreamDecoderTaskFunc(): decoder->error is %d for %s",
								soundSource[i].sample->decoder->error,
								soundSource[i].sample->decoder->filename);
#endif
						processed--;
						continue;
					}
				}

				decoded_bytes = SoundDecoder_Decode (
						soundSource[i].sample->decoder);
				if (soundSource[i].sample->decoder->error ==
						SOUNDDECODER_ERROR)
				{
					log_add (log_Warning, "StreamDecoderTaskFunc(): "
							"SoundDecoder_Decode error %d, file %s, "
							"source %d",
							soundSource[i].sample->decoder->error,
							soundSource[i].sample->decoder->filename, i);
					soundSource[i].stream_should_be_playing = FALSE;
					processed--;
					continue;
				}

				if (decoded_bytes > 0)
				{
					audio_BufferData (buffer,
							soundSource[i].sample->decoder->format,
							soundSource[i].sample->decoder->buffer,
							decoded_bytes,
							soundSource[i].sample->decoder->frequency);

					error = audio_GetError();
					if (error != audio_NO_ERROR)
					{
						log_add (log_Warning, "StreamDecoderTaskFunc(): "
								"TFBSound error after audio_BufferData: "
								"%x, file %s, source %d, decoded_bytes %d",
							error, soundSource[i].sample->decoder->filename,
							i, decoded_bytes);
					}
					else
					{
						last_buffer[i] = buffer;
						audio_SourceQueueBuffers (soundSource[i].handle, 1,
								&buffer);
						// do OnQueue callback
						if (soundSource[i].sample->callbacks.OnQueueBuffer)
							soundSource[i].sample->callbacks.OnQueueBuffer (
									soundSource[i].sample, buffer);
						
						if (soundSource[i].sbuffer)
						{
							// copies decoded data to oscilloscope buffer

							uint32 j, remaining_bytes = 0;
							UBYTE *sbuffer = (UBYTE *) soundSource[i].sbuffer;
							UBYTE *decoder_buffer = (UBYTE *) soundSource[i].
									sample->decoder->buffer;
														
							if (soundSource[i].sbuf_start + decoded_bytes >
									soundSource[i].sbuf_size)
							{
								j = soundSource[i].sbuf_size;
								remaining_bytes = decoded_bytes -
										(j - soundSource[i].sbuf_start);
							}
							else
							{
								j = soundSource[i].sbuf_start + decoded_bytes;
							}
							
							if (j - soundSource[i].sbuf_start >= 1)
							{
								memcpy (&sbuffer[soundSource[i].sbuf_start],
										decoder_buffer,
										j - soundSource[i].sbuf_start);
							}

							if (remaining_bytes)
							{
								memcpy (sbuffer, &decoder_buffer[
										j - soundSource[i].sbuf_start],
										remaining_bytes);
								soundSource[i].sbuf_start = remaining_bytes;
							}
							else
							{
								soundSource[i].sbuf_start += decoded_bytes;
							}
						}

						error = audio_GetError();
						if (error != audio_NO_ERROR)
						{
							log_add (log_Warning, "StreamDecoderTaskFunc(): "
									"TFBSound error after "
									"audio_SourceQueueBuffers: %x, file %s, "
									"source %d, decoded_bytes %d", error,
									soundSource[i].sample->decoder->filename,
									i, decoded_bytes);
						}
					}
				}

				processed--;
			}

			UnlockMutex (soundSource[i].stream_mutex);
		}
	}

	FinishTask (task);
	return 0;
}

inline sint32
readSoundSample (void *ptr, int sample_size)
{
	if (sample_size == sizeof (uint8))
		return (*(uint8*)ptr - 128) << 8;
	else
		return *(sint16*)ptr;
}

// Graphs the current sound data for the oscilloscope.
// Includes a rudimentary automatic gain control (AGC) to properly graph
// the streams at different gain levels (based on running average).
// We use AGC because different pieces of music and speech can easily be
// at very different gain levels, because the game is moddable.
int
GraphForegroundStream (uint8 *data, sint32 width, sint32 height)
{
	int source_num;
	TFB_SoundSource *source;
	TFB_SoundDecoder *decoder;
	int channels;
	int sample_size;
	int full_sample;
	int step;
	long played_time;
	long delta;
	uint8 *sbuffer;
	unsigned long pos;
	int scale;
	sint32 i;
	// AGC variables
#define DEF_PAGE_MAX    28000
#define AGC_PAGE_COUNT  16
	static int page_sum = DEF_PAGE_MAX * AGC_PAGE_COUNT;
	static int pages[AGC_PAGE_COUNT] =
	{
		DEF_PAGE_MAX, DEF_PAGE_MAX, DEF_PAGE_MAX, DEF_PAGE_MAX,
		DEF_PAGE_MAX, DEF_PAGE_MAX, DEF_PAGE_MAX, DEF_PAGE_MAX,
		DEF_PAGE_MAX, DEF_PAGE_MAX, DEF_PAGE_MAX, DEF_PAGE_MAX,
		DEF_PAGE_MAX, DEF_PAGE_MAX, DEF_PAGE_MAX, DEF_PAGE_MAX,
	};
	static int page_head;
#define AGC_FRAME_COUNT  8
	static int frame_sum;
	static int frames;
	static int avg_amp = DEF_PAGE_MAX; // running amplitude (sort of) average
	int target_amp;
	int max_a;
#define VAD_MIN_ENERGY  100
	long energy;


	if (speechVolumeScale != 0.0f)
	{	// Use speech waveform when speech is enabled
		source_num = SPEECH_SOURCE;
		// Step is picked experimentally. Using step of 1 sample at 11025Hz,
		// because human speech is mostly in the low frequencies, and it looks
		// better this way.
		step = 1;

	}
	else if (musicVolumeScale != 0.0f)
	{	// Use music waveform when speech is disabled
		source_num = MUSIC_SOURCE;
		// Step is picked experimentally. Using step of 4 samples at 11025Hz.
		// It looks better this way.
		step = 4;
	}
	else
	{
		return 0;
	}

	source = &soundSource[source_num];
	LockMutex (source->stream_mutex);
	if (!PlayingStream (source_num) || !source->sample
			|| !source->sample->decoder || !source->sbuffer
			|| source->sbuf_size == 0)
	{	// We don't have data to return, oh well.
		UnlockMutex (source->stream_mutex);
		return 0;
	}
	decoder = source->sample->decoder;

	assert (audio_GetFormatInfo (decoder->format, &channels, &sample_size));
	full_sample = channels * sample_size;

	// See how far into the buffer we should be now
	played_time = GetTimeCounter () - source->sbuf_lasttime;
	delta = played_time * decoder->frequency * full_sample / ONE_SECOND;
	// align delta to sample start
	delta = delta & ~(full_sample - 1);

	if (delta < 0)
	{
		log_add (log_Debug, "GraphForegroundStream(): something is messed"
				" with timing, delta %ld", delta);
		delta = 0;
	}
	else if (delta > (long)source->sbuf_size)
	{	// Stream decoder task has just had a heart attack, not much we can do
		delta = 0;
	}

	// Step is in 11025 Hz units, so we need to adjust to source frequency
	step = decoder->frequency * step / 11025;
	if (step == 0)
		step = 1;
	step *= full_sample;

	sbuffer = source->sbuffer;
	pos = source->sbuf_offset + delta;

	// We are not basing the scaling factor on signal energy, because we
	// want it to *look* pretty instead of sounding nice and even
	target_amp = (height >> 1) >> 1;
	scale = avg_amp / target_amp;

	max_a = 0;
	energy = 0;
	for (i = 0; i < width; ++i, pos += step)
	{
		sint32 s;
		int t;

		pos %= source->sbuf_size;

		s = readSoundSample (sbuffer + pos, sample_size);
		if (channels > 1)
			s += readSoundSample (sbuffer + pos + sample_size, sample_size);

		energy += (s * s) / 0x10000;
		t = abs(s);
		if (t > max_a)
			max_a = t;

		s = (s / scale) + (height >> 1);
		if (s < 0)
			s = 0;
		else if (s > height - 1)
			s = height - 1;
		
		data[i] = s;
	}
	energy /= width;

	// Very basic VAD. We don't want to count speech pauses in the average
	if (energy > VAD_MIN_ENERGY)
	{
		// Record the maximum amplitude (sort of)
		frame_sum += max_a;
		++frames;
		if (frames == AGC_FRAME_COUNT)
		{	// Got a full page
			frame_sum /= AGC_FRAME_COUNT;
			// Record the page
			page_sum -= pages[page_head];
			page_sum += frame_sum;
			pages[page_head] = frame_sum;
			page_head = (page_head + 1) % AGC_PAGE_COUNT;

			frame_sum = 0;
			frames = 0;

			avg_amp = page_sum / AGC_PAGE_COUNT;
		}
	}

	UnlockMutex (source->stream_mutex);
	return 1;
}

