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

#ifdef SOUNDMODULE_OPENAL

#include <assert.h>
#include "sound.h"

#if !defined (WIN32) // && !defined (MacOS and BeOS too)
#include <AL/alext.h>
#endif

BOOLEAN speech_advancetrack = FALSE;

void
PlayStream (TFB_SoundSample *sample, ALuint source, ALboolean looping, ALboolean scope)
{	
	ALuint i, pos = 0;
	ALuint offset = 0;
	if (!sample || (!sample->read_chain_ptr && !sample->decoder))
		return;

	StopStream (source);
	if (sample->read_chain_ptr)
	{
		sample->decoder = sample->read_chain_ptr->decoder;
		offset = (int) (sample->read_chain_ptr->start_time * (float)ONE_SECOND);
	}
	else
		offset= 0;
	SoundDecoder_Rewind (sample->decoder);
	soundSource[source].sample = sample;
	soundSource[source].sample->play_chain_ptr = sample->read_chain_ptr;
	soundSource[source].sample->decoder->looping = looping;
	alSourcei (soundSource[source].handle, AL_LOOPING, AL_FALSE);

	if (scope)
	{
		soundSource[source].sbuffer = NULL;
	}

	if (sample->read_chain_ptr && sample->read_chain_ptr->tag.type)
	{
		DoTrackTag(&sample->read_chain_ptr->tag);
	}
	for (i = 0; i < sample->num_buffers; ++i)
	{
		ALuint decoded_bytes;
		if (sample->buffer_tag)
			sample->buffer_tag[i] = 0;
		decoded_bytes = SoundDecoder_Decode (sample->decoder);
		//fprintf (stderr, "PlayStream(): source %d filename:%s start: %d position:%d bytes %d\n", source, 
		//		sample->decoder->filename, sample->decoder->start_sample, 
		//		sample->decoder->pos, decoded_bytes);
		if (decoded_bytes == 0)
			break;

#if !defined (WIN32) // && !defined (MacOS and BeOS too)
		// *nix OpenAL implementation has to handle stereo data differently to avoid mixing it to mono
	   	
		if (sample->decoder->format == AL_FORMAT_STEREO8 ||
			sample->decoder->format == AL_FORMAT_STEREO16)
		{
			alBufferWriteData_LOKI (sample->buffer[i], sample->decoder->format, sample->decoder->buffer,
									decoded_bytes, sample->decoder->frequency, sample->decoder->format);
		}
		else
		{
			alBufferData (sample->buffer[i], sample->decoder->format, sample->decoder->buffer, 
						  decoded_bytes, sample->decoder->frequency);
		}
#else
		alBufferData (sample->buffer[i], sample->decoder->format, sample->decoder->buffer, 
			decoded_bytes, sample->decoder->frequency);
#endif
		alSourceQueueBuffers (soundSource[source].handle, 1, &sample->buffer[i]);

		if (scope)
		{
			UBYTE *p;
			if (! soundSource[source].sbuffer)
				soundSource[source].sbuffer = HMalloc (decoded_bytes);
			else
				soundSource[source].sbuffer = HRealloc (soundSource[source].sbuffer, 
						pos + decoded_bytes);
			p = (UBYTE *)soundSource[source].sbuffer;
			memcpy (&p[pos], sample->decoder->buffer, decoded_bytes);
			pos += decoded_bytes;
		}

		if (sample->decoder->error)
		{
			if (sample->decoder->error == SOUNDDECODER_EOF && sample->read_chain_ptr->next)
			{
				sample->read_chain_ptr = sample->read_chain_ptr->next;
				sample->decoder = sample->read_chain_ptr->decoder;
				SoundDecoder_Rewind (sample->decoder);
				fprintf (stderr, "Switching to stream %s at pos %d\n",
						sample->decoder->filename, sample->decoder->start_sample);
				if (sample->buffer_tag && sample->read_chain_ptr->tag.type)
				{
					sample->buffer_tag[i] = &sample->read_chain_ptr->tag;
					sample->read_chain_ptr->tag.buf_name = sample->buffer[i];
				}
			}
			else
				break;
		}
	}
	if (sample->buffer_tag)
		for ( ; i <sample->num_buffers; ++i)
			sample->buffer_tag[i] = 0;
	soundSource[source].sbuf_size = pos;
	soundSource[source].start_time = GetTimeCounter () - offset;
	soundSource[source].stream_should_be_playing = TRUE;
	alSourcePlay (soundSource[source].handle);
}

void
StopStream (ALuint source)
{
	ALuint queued, processed;
	ALuint *buffer;

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

	alSourceStop (soundSource[source].handle);
	alGetSourcei (soundSource[source].handle, AL_BUFFERS_PROCESSED, &processed);
	alGetSourcei (soundSource[source].handle, AL_BUFFERS_QUEUED, &queued);

	//fprintf (stderr, "StopStream(): source %d processed %d queued %d num_buffers %d\n", source, processed, queued, soundSource[source].sample->num_buffers);
	
	if (processed != 0)
	{
		buffer = (ALuint *) HMalloc (sizeof (ALuint) * processed);
		alSourceUnqueueBuffers (soundSource[source].handle, processed, buffer);
		HFree (buffer);
	}
}

void
PauseStream (ALuint source)
{
	// TODO: how to handle start_time
	soundSource[source].stream_should_be_playing = FALSE;
	alSourcePause (soundSource[source].handle);
}

void
ResumeStream (ALuint source)
{
	// TODO: how to handle start_time
	soundSource[source].stream_should_be_playing = TRUE;
	alSourcePlay (soundSource[source].handle);
}

BOOLEAN
PlayingStream (ALuint source)
{	
	return soundSource[source].stream_should_be_playing;
}

int
StreamDecoderTaskFunc (void *data)
{
	Task task = (Task)data;
	int i;
	ALuint last_buffer[NUM_SOUNDSOURCES];

	for (i = 0; i < NUM_SOUNDSOURCES; i++)
		last_buffer[i] = 0;

	while (!Task_ReadState (task, TASK_EXIT))
	{

		TaskSwitch ();

		for (i = MUSIC_SOURCE; i < NUM_SOUNDSOURCES; ++i)
		{
			ALuint processed, queued;
			ALint state;
			ALboolean do_speech_advancetrack = AL_FALSE;

			LockMutex (soundSource[i].stream_mutex);

			if (!soundSource[i].sample ||
				!soundSource[i].sample->decoder ||
				!soundSource[i].stream_should_be_playing ||
				soundSource[i].sample->decoder->error == SOUNDDECODER_ERROR)
			{
				UnlockMutex (soundSource[i].stream_mutex);
				continue;
			}

			alGetSourcei (soundSource[i].handle, AL_BUFFERS_PROCESSED, &processed);
			alGetSourcei (soundSource[i].handle, AL_BUFFERS_QUEUED, &queued);

			if (processed == 0)
			{
				alGetSourcei (soundSource[i].handle, AL_SOURCE_STATE, &state);			
				if (state != AL_PLAYING)
				{
					if (queued == 0 && soundSource[i].sample->decoder->error == SOUNDDECODER_EOF)
					{
						fprintf (stderr, "StreamDecoderTaskFunc(): finished playing %s, source %d\n", soundSource[i].sample->decoder->filename, i);
						soundSource[i].stream_should_be_playing = FALSE;
						if (i == SPEECH_SOURCE)
						{
							soundSource[i].sample->decoder = NULL;
							soundSource[i].sample->read_chain_ptr = NULL;
 							if (speech_advancetrack)
								do_speech_advancetrack = AL_TRUE;
 						}
 					}
					else
 					{
						fprintf (stderr, "StreamDecoderTaskFunc(): buffer underrun when playing %s, source %d\n",
								soundSource[i].sample->decoder->filename, i);
						alSourcePlay (soundSource[i].handle);
					}
				}
			}
            
			//fprintf (stderr, "StreamDecoderTaskFunc(): source %d, processed %d queued %d\n", i, processed, queued);

			while (processed)
			{
				ALenum error;
				ALuint buffer;
				ALuint decoded_bytes;
				ALuint buf_num;

				alGetError (); // clear error state

				alSourceUnqueueBuffers (soundSource[i].handle, 1, &buffer);

				if ((error = alGetError()) != AL_NO_ERROR)
				{
					fprintf (stderr, "StreamDecoderTaskFunc(): OpenAL error after alSourceUnqueueBuffers: %x, file %s, source %d\n", error, soundSource[i].sample->decoder->filename, i);
					break;
				}

				if (i == SPEECH_SOURCE)
				{
					for (buf_num = 0; buf_num < soundSource[i].sample->num_buffers; buf_num++)
					{
						TFB_SoundTag *cur_tag = soundSource[i].sample->buffer_tag[buf_num];
						if (cur_tag && cur_tag->buf_name == buffer)
						{
							//	...do tag stuff ...
							DoTrackTag (cur_tag);
							soundSource[i].sample->buffer_tag[buf_num] = NULL;
							soundSource[i].sample->play_chain_ptr = soundSource[i].sample->read_chain_ptr;
							cur_tag->buf_name = 0;
							break;
						}
					}
				}

				soundSource[i].total_decoded += soundSource[i].sample->decoder->buffer_size;

				if (soundSource[i].sample->decoder->error)
				{
					if (soundSource[i].sample->decoder->error == SOUNDDECODER_EOF)
					{
						if (soundSource[i].sample->read_chain_ptr && 
								soundSource[i].sample->read_chain_ptr->next)
						{
							soundSource[i].sample->read_chain_ptr = soundSource[i].sample->read_chain_ptr->next;
							soundSource[i].sample->decoder = soundSource[i].sample->read_chain_ptr->decoder;
							SoundDecoder_Rewind (soundSource[i].sample->decoder);
							fprintf (stderr, "Switching to stream %s at pos %d\n",
									soundSource[i].sample->decoder->filename,
									soundSource[i].sample->decoder->start_sample);
							if (i == SPEECH_SOURCE)
							{
								for (buf_num = 0; buf_num < soundSource[i].sample->num_buffers; buf_num++)
								{
									if (soundSource[i].sample->buffer_tag[buf_num] == 0)
									{
										soundSource[i].sample->buffer_tag[buf_num] = &soundSource[i].sample->read_chain_ptr->tag;
										soundSource[i].sample->read_chain_ptr->tag.buf_name = last_buffer[i];
										break;
									}
								}
							}
						}
						else
						{
							//fprintf (stderr, "StreamDecoderTaskFunc(): decoder->error is eof for %s\n", soundSource[i].sample->decoder->filename);
							processed--;
							continue;
						}
					}
					else
					{
						//fprintf (stderr, "StreamDecoderTaskFunc(): decoder->error is %d for %s\n", soundSource[i].sample->decoder->error, soundSource[i].sample->decoder->filename);
						processed--;
						continue;
					}
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
#if !defined (WIN32) // && !defined (MacOS and BeOS too)
					// *nix OpenAL implementation has to handle stereo data differently to avoid mixing it to mono
	   	
					if (soundSource[i].sample->decoder->format == AL_FORMAT_STEREO8 ||
						soundSource[i].sample->decoder->format == AL_FORMAT_STEREO16)
					{
						alBufferWriteData_LOKI (buffer, soundSource[i].sample->decoder->format, soundSource[i].sample->decoder->buffer,
												decoded_bytes, soundSource[i].sample->decoder->frequency,
												soundSource[i].sample->decoder->format);
					}
					else
					{
						alBufferData (buffer, soundSource[i].sample->decoder->format, 
									  soundSource[i].sample->decoder->buffer, decoded_bytes, 
									  soundSource[i].sample->decoder->frequency);
					}
#else
					alBufferData (buffer, soundSource[i].sample->decoder->format, 
						soundSource[i].sample->decoder->buffer, decoded_bytes, 
						soundSource[i].sample->decoder->frequency);
#endif

					if ((error = alGetError()) != AL_NO_ERROR)
					{
						fprintf (stderr, "StreamDecoderTaskFunc(): OpenAL error after alBufferData: %x, file %s, source %d, decoded_bytes %d\n", 
							error, soundSource[i].sample->decoder->filename, i, decoded_bytes);
					}
					else
					{
						alSourceQueueBuffers (soundSource[i].handle, 1, &buffer);
						last_buffer[i] = buffer;
						
						if (soundSource[i].sbuffer)
						{
							// copies decoded data to oscilloscope buffer

							ALuint j, remaining_bytes = 0;
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

						if ((error = alGetError()) != AL_NO_ERROR)
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
					fprintf (stderr, "StreamDecoderTaskFunc(): calling advance_track\n");
					do_speech_advancetrack = AL_FALSE;
					advance_track (0);
				}
			}
		}
	}

	FinishTask (task);
	return 0;
}

#endif
