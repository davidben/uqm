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

#ifdef SOUNDMODULE_OPENAL

#include "libs/sound/sound.h"

ALCcontext *alcContext = NULL;
ALCdevice *alcDevice = NULL;
ALfloat listenerPos[] = {0.0f, 0.0f, 0.0f};
ALfloat listenerVel[] = {0.0f, 0.0f, 0.0f};
ALfloat listenerOri[] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};
static Task StreamDecoderTask;


int 
TFB_InitSound (int driver, int flags)
{
	int i;
	TFB_DecoderFormats formats =
	{
		MIX_IS_BIG_ENDIAN, MIX_WANT_BIG_ENDIAN,
		AL_FORMAT_MONO8, AL_FORMAT_STEREO8,
		AL_FORMAT_MONO16, AL_FORMAT_STEREO16
	};
	
	fprintf (stderr, "Initializing OpenAL.\n");
	atexit (TFB_UninitSound);

#ifdef WIN32
	alcDevice = alcOpenDevice ((ALubyte*)"DirectSound3D");
#else
	alcDevice = alcOpenDevice (NULL);
#endif

	if (!alcDevice)
	{
		fprintf (stderr,"Couldn't initialize OpenAL: %d\n", alcGetError (NULL));
		exit (-1);
	}

	alcContext = alcCreateContext (alcDevice, NULL);
	if (!alcContext)
	{
		fprintf (stderr, "Couldn't create OpenAL context: %d\n", alcGetError (alcDevice));
	}

	alcMakeContextCurrent (alcContext);

	fprintf (stderr, "OpenAL initialized.\n");
	fprintf (stderr, "    version:     %s\n", alGetString (AL_VERSION));
	fprintf (stderr, "    vendor:      %s\n", alGetString (AL_VENDOR));
	fprintf (stderr, "    renderer:    %s\n", alGetString (AL_RENDERER));
	fprintf (stderr, "    device:      %s\n",
		alcGetString (alcDevice, ALC_DEFAULT_DEVICE_SPECIFIER));
    //fprintf (stderr, "    extensions:  %s\n", alGetString (AL_EXTENSIONS));
		
	fprintf (stderr, "Initializing sound decoders.\n");
	SoundDecoder_Init (flags, &formats);
	fprintf (stderr, "Sound decoders initialized.\n");

	alListenerfv (AL_POSITION, listenerPos);
	alListenerfv (AL_VELOCITY, listenerVel);
	alListenerfv (AL_ORIENTATION, listenerOri);

	for (i = 0; i < NUM_SOUNDSOURCES; ++i)
	{
		float zero[3] = {0.0f, 0.0f, 0.0f};
		
		alGenSources (1, &soundSource[i].handle);
		alSourcei (soundSource[i].handle, AL_LOOPING, AL_FALSE);
		alSourcefv (soundSource[i].handle, AL_POSITION, zero);
		alSourcefv (soundSource[i].handle, AL_VELOCITY, zero);
		alSourcefv (soundSource[i].handle, AL_DIRECTION, zero);
		
		soundSource[i].sample = NULL;
		soundSource[i].stream_should_be_playing = FALSE;
		soundSource[i].stream_mutex = CreateMutex ();
		soundSource[i].sbuffer = NULL;
		soundSource[i].sbuf_start = 0;
		soundSource[i].sbuf_size = 0;
		soundSource[i].sbuf_offset = 0;
	}
	
	SetSFXVolume (sfxVolumeScale);
	SetSpeechVolume (speechVolumeScale);
	SetMusicVolume ((COUNT) musicVolume);
		
	// NOTE: change this if implementing 3D sound stuff
	alDistanceModel (AL_NONE);

	StreamDecoderTask = AssignTask (StreamDecoderTaskFunc, 1024, 
		"audio stream decoder");

	return 0;
}

void
TFB_UninitSound (void)
{
	int i;

	ConcludeTask (StreamDecoderTask);

	for (i = 0; i < NUM_SOUNDSOURCES; ++i)
	{
		if (soundSource[i].sample && soundSource[i].sample->decoder)
		{
			StopStream (i);
		}
		if (soundSource[i].sbuffer)
		{
			void *sbuffer = soundSource[i].sbuffer;
			soundSource[i].sbuffer = NULL;
			HFree (sbuffer);
		}
		DestroyMutex (soundSource[i].stream_mutex);
	}

	alcMakeContextCurrent (NULL);
	alcDestroyContext (alcContext);
	alcContext = NULL;
	alcCloseDevice (alcDevice);
	alcDevice = NULL;

	SoundDecoder_Uninit ();
}

#endif