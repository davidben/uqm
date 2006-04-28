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

