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

/* By Mika Kolehmainen, 2002-10-23
 */

#ifdef SOUNDMODULE_MIXSDL

#include <assert.h>
#include "sound.h"
#include "mixer.h"

bool speech_advancetrack = FALSE;


void
PlayStream (TFB_SoundSample *sample, uint32 source, bool looping, bool scope)
{	
	uint32 i, pos = 0;

	if (!sample)
		return;

	StopStream (source);
	SoundDecoder_Rewind (sample->decoder);
	soundSource[source].sample = sample;
	soundSource[source].sample->decoder->looping = looping;
	mixSDL_Sourcei (soundSource[source].handle, MIX_LOOPING, false);

	if (scope)
	{
		soundSource[source].sbuffer = HMalloc (sample->num_buffers * sample->decoder->buffer_size);
	}

	for (i = 0; i < sample->num_buffers; ++i)
	{
		uint32 decoded_bytes;

		decoded_bytes = SoundDecoder_Decode (sample->decoder);
		//fprintf (stderr, "PlayStream(): source %d sample %x decoded_bytes %d\n", source, sample, decoded_bytes);
		if (decoded_bytes == 0)
			break;

		mixSDL_BufferData (sample->buffer[i], sample->decoder->format, sample->decoder->buffer,
				decoded_bytes, sample->decoder->frequency);
		
		mixSDL_SourceQueueBuffers (soundSource[source].handle, 1, &sample->buffer[i]);

		if (scope)
		{
			UBYTE *p = (UBYTE *)soundSource[source].sbuffer;
			assert (pos + decoded_bytes <= sample->num_buffers * sample->decoder->buffer_size);
			memcpy (&p[pos], sample->decoder->buffer, decoded_bytes);
			pos += decoded_bytes;
		}

		if (sample->decoder->error)
			break;
	}
	
	soundSource[source].sbuf_size = pos;
	soundSource[source].start_time = GetTimeCounter ();
	soundSource[source].stream_should_be_playing = TRUE;
	mixSDL_SourcePlay (soundSource[source].handle);
}

// decoders array is NULL-terminated
int
PreDecodeClips (TFB_SoundSample *sample, TFB_SoundDecoder *decoders[], bool autofree)
{	
	uint32 i;
	uint32 org_buffers;
	TFB_SoundDecoder **pdecoder;

	if (!sample)
		return 0;

	sample->length = 0.0;
	org_buffers = sample->num_buffers;

	for (i = 0, pdecoder = decoders;
			i < sample->num_buffers && *pdecoder;
			i++, pdecoder++)
	{
		TFB_SoundDecoder *decoder = *pdecoder;
		uint32 decoded_bytes;

		decoded_bytes = SoundDecoder_DecodeAll (decoder);
		if (decoded_bytes == 0)
		{
			// decoder error, will reuse the buffer
			i--;
		}
		else
		{
			mixSDL_BufferData (sample->buffer[i], decoder->format, decoder->buffer,
						  decoded_bytes, decoder->frequency);

			sample->length += decoder->length;
		}

		if (autofree)
		{
			SoundDecoder_Free (*pdecoder);
			*pdecoder = 0;
		}
	}

	if (autofree && i < sample->num_buffers)
	{
		mixSDL_DeleteBuffers (sample->num_buffers - i, sample->buffer + i);
		sample->num_buffers = i;
	}

	if (i == 0)
		fprintf (stderr, "EnqueueClips(): no clips decoded\n");

	return i;
}

void
PlayDecodedClip (TFB_SoundSample *sample, uint32 source, bool scope)
{	
	uint32 i, pos = 0;

	if (!sample)
		return;

	StopStream (source);
	soundSource[source].sample = sample;
	mixSDL_Sourcei (soundSource[source].handle, MIX_LOOPING, false);

	mixSDL_SourceQueueBuffers (soundSource[source].handle,
			sample->num_buffers, sample->buffer);

	for (i = 0; i < sample->num_buffers; i++)
	{
		mixSDL_Object size;

		size = 0;
		mixSDL_GetBufferi (sample->buffer[i], MIX_SIZE, &size);
		//soundSource[source].total_decoded += size;
	}
	
	soundSource[source].sbuf_size = pos;
	soundSource[source].start_time = GetTimeCounter ();
	soundSource[source].stream_should_be_playing = TRUE;
	mixSDL_SourcePlay (soundSource[source].handle);
}

void
StopStream (uint32 source)
{
	uint32 queued, processed;
	uint32 *buffer;

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
	soundSource[source].total_decoded = 0;

	mixSDL_SourceStop (soundSource[source].handle);
	mixSDL_GetSourcei (soundSource[source].handle, MIX_BUFFERS_PROCESSED, &processed);
	mixSDL_GetSourcei (soundSource[source].handle, MIX_BUFFERS_QUEUED, &queued);

	//fprintf (stderr, "StopStream(): source %d processed %d queued %d num_buffers %d\n", source, processed, queued, soundSource[source].sample->num_buffers);
	
	if (processed != 0)
	{
		buffer = (uint32 *) HMalloc (sizeof (uint32) * processed);
		mixSDL_SourceUnqueueBuffers (soundSource[source].handle, processed, buffer);
		HFree (buffer);
	}
}

void
PauseStream (uint32 source)
{
	// TODO: how to handle start_time
	soundSource[source].stream_should_be_playing = FALSE;
	mixSDL_SourcePause (soundSource[source].handle);
}

void
ResumeStream (uint32 source)
{
	// TODO: how to handle start_time
	soundSource[source].stream_should_be_playing = TRUE;
	mixSDL_SourcePlay (soundSource[source].handle);
}

BOOLEAN
PlayingStream (uint32 source)
{	
	return soundSource[source].stream_should_be_playing;
}

int
StreamDecoderTaskFunc (void *data)
{
	Task task = (Task)data;

	while (!Task_ReadState (task, TASK_EXIT))
	{
		int i;

		TaskSwitch ();

		for (i = MUSIC_SOURCE; i < NUM_SOUNDSOURCES; ++i)
		{
			uint32 processed, queued;
			mixSDL_Object state;
			bool do_speech_advancetrack = false;
			int streaming;

			LockMutex (soundSource[i].stream_mutex);

			if (!soundSource[i].sample ||
				!soundSource[i].stream_should_be_playing ||
				(soundSource[i].sample->decoder &&
				soundSource[i].sample->decoder->error == SOUNDDECODER_ERROR))
			{
				UnlockMutex (soundSource[i].stream_mutex);
				continue;
			}

			streaming = soundSource[i].sample->decoder != 0;

			mixSDL_GetSourcei (soundSource[i].handle, MIX_BUFFERS_PROCESSED, &processed);
			mixSDL_GetSourcei (soundSource[i].handle, MIX_BUFFERS_QUEUED, &queued);

			if (processed == 0)
			{
				mixSDL_GetSourcei (soundSource[i].handle, MIX_SOURCE_STATE, &state);
				if (state != MIX_PLAYING)
				{
					if (queued == 0 && (!streaming ||
							soundSource[i].sample->decoder->error == SOUNDDECODER_EOF))
					{
						fprintf (stderr, "StreamDecoderTaskFunc(): finished playing %s, source %d\n",
								streaming ? soundSource[i].sample->decoder->filename : "(decoded)", i);
						soundSource[i].stream_should_be_playing = false;
						if (i == SPEECH_SOURCE && speech_advancetrack)
						{
							do_speech_advancetrack = true;
						}
					}
					else if (streaming)
					{
						fprintf (stderr, "StreamDecoderTaskFunc(): buffer underrun when playing %s, source %d\n",
								soundSource[i].sample->decoder->filename, i);
						mixSDL_SourcePlay (soundSource[i].handle);
					}
				}
			}
            
			//fprintf (stderr, "StreamDecoderTaskFunc(): source %d, processed %d queued %d\n", i, processed, queued);

			while (processed)
			{
				uint32 error;
				mixSDL_Object buffer;
				uint32 decoded_bytes;

				mixSDL_GetError (); // clear error state

				mixSDL_SourceUnqueueBuffers (soundSource[i].handle, 1, &buffer);
				if ((error = mixSDL_GetError()) != MIX_NO_ERROR)
				{
					fprintf (stderr, "StreamDecoderTaskFunc(): OpenAL error after alSourceUnqueueBuffers: %x, file %s, source %d\n", error, soundSource[i].sample->decoder->filename, i);
					break;
				}

				if (!streaming)
				{
					processed--;
					continue;
				}

				soundSource[i].total_decoded += soundSource[i].sample->decoder->buffer_size;

				if (soundSource[i].sample->decoder->error)
				{
					if (soundSource[i].sample->decoder->error == SOUNDDECODER_EOF)
					{
						//fprintf (stderr, "StreamDecoderTaskFunc(): decoder->error is eof for %s\n", soundSource[i].sample->decoder->filename);
					}
					else
					{
						//fprintf (stderr, "StreamDecoderTaskFunc(): decoder->error is %d for %s\n", soundSource[i].sample->decoder->error, soundSource[i].sample->decoder->filename);
					}
					processed--;
					continue;
				}

				decoded_bytes = SoundDecoder_Decode (soundSource[i].sample->decoder);
				if (soundSource[i].sample->decoder->error == SOUNDDECODER_ERROR)
				{
					fprintf (stderr, "StreamDecoderTaskFunc(): SoundDecoder_Decode error %d, file %s, source %d\n", soundSource[i].sample->decoder->error, soundSource[i].sample->decoder->filename, i);
					soundSource[i].stream_should_be_playing = FALSE;
					processed--;
					continue;
				}

				if (decoded_bytes > 0)
				{
					mixSDL_BufferData (buffer, soundSource[i].sample->decoder->format,
							soundSource[i].sample->decoder->buffer, decoded_bytes,
							soundSource[i].sample->decoder->frequency);

					if ((error = mixSDL_GetError()) != MIX_NO_ERROR)
					{
						fprintf (stderr, "StreamDecoderTaskFunc(): OpenAL error after alBufferData: %x, file %s, source %d, decoded_bytes %d\n", 
								error, soundSource[i].sample->decoder->filename, i, decoded_bytes);
					}
					else
					{
						mixSDL_SourceQueueBuffers (soundSource[i].handle, 1, &buffer);
						
						if (soundSource[i].sbuffer)
						{
							// copies decoded data to oscilloscope buffer

							uint32 j, remaining_bytes = 0;
							UBYTE *sbuffer = (UBYTE *) soundSource[i].sbuffer;
							UBYTE *decoder_buffer = (UBYTE *) soundSource[i].sample->decoder->buffer;
														
							if (soundSource[i].sbuf_start + decoded_bytes > soundSource[i].sbuf_size)
							{
								j = soundSource[i].sbuf_size;
								remaining_bytes = decoded_bytes - (j - soundSource[i].sbuf_start);
							}
							else
							{
								j = soundSource[i].sbuf_start + decoded_bytes;
							}
							
							if (j - soundSource[i].sbuf_start >= 1)
							{
								//fprintf (stderr, "copying_a to %d - %d, %d bytes\n", soundSource[i].sbuf_start, j, j - soundSource[i].sbuf_start);
								memcpy (&sbuffer[soundSource[i].sbuf_start], decoder_buffer, j - soundSource[i].sbuf_start);
							}

							if (remaining_bytes)
							{
								//fprintf (stderr, "copying_b to 0 - %d\n", remaining_bytes);
								memcpy (sbuffer, &decoder_buffer[j - soundSource[i].sbuf_start], remaining_bytes);
								soundSource[i].sbuf_start = remaining_bytes;
							}
							else
							{
								soundSource[i].sbuf_start += decoded_bytes;
							}
						}

						if ((error = mixSDL_GetError()) != MIX_NO_ERROR)
						{
							fprintf (stderr, "StreamDecoderTaskFunc(): OpenAL error after alSourceQueueBuffers: %x, file %s, source %d, decoded_bytes %d\n", 
									error, i, soundSource[i].sample->decoder->filename, decoded_bytes);
						}
					}
				}

				processed--;
			}

			UnlockMutex (soundSource[i].stream_mutex);

			if (i == SPEECH_SOURCE)
			{
				if (do_speech_advancetrack)
				{
					//fprintf (stderr, "StreamDecoderTaskFunc(): calling advance_track\n");
					do_speech_advancetrack = false;
					advance_track (0);
				}
			}
		}
	}

	FinishTask (task);
	return 0;
}

#endif
