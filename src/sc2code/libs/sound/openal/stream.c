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

BOOLEAN speech_advancetrack = FALSE;


void
PlayStream (TFB_SoundSample *sample, ALuint source, BOOLEAN looping)
{	
	int i;
	ALenum format = DetermineALFormat (sample->decoder);

	StopStream (source);

	Sound_Rewind (sample->decoder);
	soundSource[source].sample = sample;

	alSourcei (soundSource[source].handle, AL_LOOPING, AL_FALSE);

	for (i = 0; i < NUM_SOUNDBUFFERS; ++i)
	{
		Uint32 decoded_bytes;

		if (sample->decoder->flags & SOUND_SAMPLEFLAG_EOF ||
			sample->decoder->flags & SOUND_SAMPLEFLAG_ERROR)
			break;

		decoded_bytes = Sound_Decode (sample->decoder);
		//fprintf (stderr, "PlayStream(): source %d sample %x decoded_bytes %d\n", source, sample, decoded_bytes);
		if (decoded_bytes == 0)
			break;

		alBufferData (sample->buffer[i], format, sample->decoder->buffer, 
			decoded_bytes, sample->decoder->actual.rate);

		alSourceQueueBuffers (soundSource[source].handle, 1, &sample->buffer[i]);
	}

	soundSource[source].stream_looping = looping;
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

	//fprintf (stderr, "StopStream(): source %d processed %d queued %d NUM_SOUNDBUFFERS %d\n", 
	//	source, processed, queued, NUM_SOUNDBUFFERS);

	buffer = (ALuint *) HMalloc (sizeof (ALuint) * processed);
	alSourceUnqueueBuffers (soundSource[source].handle, processed, buffer);
	HFree (buffer);
}

void
PauseStream (ALuint source)
{
	soundSource[source].stream_should_be_playing = FALSE;
	alSourcePause (soundSource[source].handle);
}

void
ResumeStream (ALuint source)
{
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
			ALuint processed;
			ALuint queued;
			ALint state;
			ALboolean finished = AL_FALSE;
			ALboolean do_speech_advancetrack = AL_FALSE;

			LockMutex (soundSource[i].stream_mutex);

			if (!soundSource[i].sample ||
				!soundSource[i].sample->decoder ||
				!soundSource[i].stream_should_be_playing ||
				soundSource[i].sample->decoder->flags & SOUND_SAMPLEFLAG_EOF ||
				soundSource[i].sample->decoder->flags & SOUND_SAMPLEFLAG_ERROR)
			{
				UnlockMutex (soundSource[i].stream_mutex);
				continue;
			}

			alGetSourcei (soundSource[i].handle, AL_BUFFERS_PROCESSED, &processed);
			if (processed == 0)
			{
				alGetSourcei (soundSource[i].handle, AL_SOURCE_STATE, &state);			
				if (state != AL_PLAYING)
				{
					//fprintf (stderr, "StreamDecoderTaskFunc(): forcing alSourcePlay on %d\n", i);
					alSourcePlay (soundSource[i].handle);
				}
			}

			alGetSourcei (soundSource[i].handle, AL_BUFFERS_QUEUED, &queued);
            if (queued != NUM_SOUNDBUFFERS)
            {
                fprintf (stderr, "StreamDecoderTaskFunc(): warning, queued %d != NUM_SOUNDBUFFERS %d\n",
                    queued, NUM_SOUNDBUFFERS);
            }
            
			//fprintf (stderr, "StreamDecoderTaskFunc(): source %d, processed %d queued %d\n", i, processed, queued);

			while (processed && !finished)
			{
				ALenum format, error;
				ALuint buffer;
				Uint32 decoded_bytes;

				alGetError (); // clear error state

				alSourceUnqueueBuffers (soundSource[i].handle, 1, &buffer);
				if ((error = alGetError()) != AL_NO_ERROR)
				{
					fprintf (stderr, "StreamDecoderTaskFunc(): OpenAL error after alSourceUnqueueBuffers: %x, source %d\n", error, i);
					break;
				}

				decoded_bytes = Sound_Decode (soundSource[i].sample->decoder);
                //fprintf (stderr, "StreamDecoderTaskFunc(): decoded_bytes %d, source %d\n", decoded_bytes, i);

				if (soundSource[i].sample->decoder->flags & SOUND_SAMPLEFLAG_EOF)
				{
					if (i == SPEECH_SOURCE && speech_advancetrack)
					{
						finished = AL_TRUE;
						soundSource[i].stream_should_be_playing = FALSE;
						do_speech_advancetrack = AL_TRUE;
					}
					else
					{
						if (soundSource[i].stream_looping)
						{
							if ((Sound_Rewind (soundSource[i].sample->decoder)) == 0)
							{
								fprintf (stderr, "StreamDecoderTaskFunc(): Sound_Rewind error %s, source %d\n",
									Sound_GetError(), i);
								finished = AL_TRUE;
								soundSource[i].stream_should_be_playing = FALSE;
							}
							else if (decoded_bytes == 0)
							{
								decoded_bytes = Sound_Decode (soundSource[i].sample->decoder);
							}
						}
						else
						{
							finished = AL_TRUE;
							soundSource[i].stream_should_be_playing = FALSE;
						}
					}
				}
				else if (soundSource[i].sample->decoder->flags & SOUND_SAMPLEFLAG_ERROR)
				{
					fprintf (stderr, "StreamDecoderTaskFunc(): Sound_Decode error %s, source %d\n",
						Sound_GetError(), i);
					soundSource[i].stream_should_be_playing = FALSE;
					break;
				}

				if (decoded_bytes > 0)
				{
					format = DetermineALFormat (soundSource[i].sample->decoder);
					alBufferData (buffer, format, soundSource[i].sample->decoder->buffer,
						decoded_bytes, soundSource[i].sample->decoder->actual.rate);

					if ((error = alGetError()) != AL_NO_ERROR)
					{
						fprintf (stderr, "StreamDecoderTaskFunc(): OpenAL error after alBufferData: %x, source %d, decoded_bytes %d\n", error, i, decoded_bytes);
						finished = AL_TRUE;
					}
					else
					{
						alSourceQueueBuffers (soundSource[i].handle, 1, &buffer);

						if ((error = alGetError()) != AL_NO_ERROR)
						{
							fprintf (stderr, "StreamDecoderTaskFunc(): OpenAL error after alSourceQueueBuffers: %x, source %d, decoded_bytes %d\n", error, i, decoded_bytes);
							finished = AL_TRUE;
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
