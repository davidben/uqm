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

#include "sound.h"

#if !defined (WIN32) // && !defined (MacOS and BeOS too)
#include <AL/alext.h>
#endif

BOOLEAN speech_advancetrack = FALSE;


void
PlayStream (TFB_SoundSample *sample, ALuint source, ALboolean looping)
{	
	ALuint i;

	if (!sample)
		return;

	StopStream (source);

	SoundDecoder_Rewind (sample->decoder);
	soundSource[source].sample = sample;
	soundSource[source].sample->decoder->looping = looping;

	alSourcei (soundSource[source].handle, AL_LOOPING, AL_FALSE);

	for (i = 0; i < sample->num_buffers; ++i)
	{
		ALuint decoded_bytes;

		decoded_bytes = SoundDecoder_Decode (sample->decoder);
		//fprintf (stderr, "PlayStream(): source %d sample %x decoded_bytes %d\n", source, sample, decoded_bytes);
		if (decoded_bytes == 0)
			break;

#if !defined (WIN32) // && !defined (MacOS and BeOS too)
		// *nix OpenAL implementation has to handle stereo data differently to avoid mixing it to mono
	   	
		if (sample->decoder->format == AL_FORMAT_STEREO8 ||
			sample->decoder->format == AL_FORMAT_STEREO16)
		{
			alBufferWriteData_LOKI (sample->buffer[i], sample->decoder->format, sample->decoder->buffer,
									decoded_bytes, sample->decoder->frequency,
									sample->decoder->format);
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

		if (sample->decoder->error)
			break;
	}

	soundSource[source].start_time = GetTimeCounter ();
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
	soundSource[source].start_time = GetTimeCounter () - soundSource[source].start_time;
	soundSource[source].stream_should_be_playing = FALSE;
	alSourcePause (soundSource[source].handle);
}

void
ResumeStream (ALuint source)
{
	soundSource[source].start_time = GetTimeCounter () - soundSource[source].start_time;
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

	while (!Task_ReadState (task, TASK_EXIT))
	{
		int i;

		SleepThread (1);

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
						if (i == SPEECH_SOURCE && speech_advancetrack)
						{
							do_speech_advancetrack = AL_TRUE;
						}
					}
					else
					{
						fprintf (stderr, "StreamDecoderTaskFunc(): buffer underrun when playing %s, source %d\n", soundSource[i].sample->decoder->filename, i);
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

				alGetError (); // clear error state

				alSourceUnqueueBuffers (soundSource[i].handle, 1, &buffer);
				if ((error = alGetError()) != AL_NO_ERROR)
				{
					fprintf (stderr, "StreamDecoderTaskFunc(): OpenAL error after alSourceUnqueueBuffers: %x, file %s, source %d\n", error, soundSource[i].sample->decoder->filename, i);
					break;
				}
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
					//fprintf (stderr, "StreamDecoderTaskFunc(): calling advance_track\n");
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
