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

/* OpenAL specific code by Mika Kolehmainen, 2002-10-23
 */

#ifdef SOUNDMODULE_OPENAL

#include "sound.h"

ALCcontext *alcContext = NULL;
ALCdevice *alcDevice = NULL;
ALfloat listenerPos[] = {0.0f, 0.0f, 0.0f};
ALfloat listenerVel[] = {0.0f, 0.0f, 0.0f};
ALfloat listenerOri[] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};
TFB_SoundSource soundSource[NUM_SOUNDSOURCES];
static Task StreamDecoderTask;


int 
TFB_InitSound (int driver, int flags, int frequency)
{
	int i;
	
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
    fprintf (stderr, "    extensions:  %s\n", alGetString (AL_EXTENSIONS));

	fprintf (stderr, "Initializing SDL_sound.\n");
	if (!Sound_Init())
	{
		fprintf(stderr, "SDL_sound initialization failed: %s\n", Sound_GetError());
		exit (-1);
	}
	else
	{
		const Sound_DecoderInfo **sdi;
		Sound_Version compiled, linked;

		SOUND_VERSION (&compiled);
		Sound_GetLinkedVersion (&linked);

		fprintf (stderr, "SDL_sound initialized.\n");
		fprintf (stderr, "    compiled version: %d.%d.%d linked version: %d.%d.%d\n",
			compiled.major, compiled.minor, compiled.patch, 
			linked.major, linked.minor, linked.patch);

		for (sdi = Sound_AvailableDecoders(); *sdi != NULL; ++sdi)
		{
			fprintf (stderr, "    supported format: %s\n", 
				(*sdi)->description);
		}
	}
		
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
		soundSource[i].stream_looping = FALSE;
		soundSource[i].stream_should_be_playing = FALSE;
		soundSource[i].stream_mutex = CreateMutex ();
	}
	
	SetSFXVolume (sfxVolumeScale);
	SetSpeechVolume (speechVolumeScale);
	SetMusicVolume (musicVolume);
		
	// NOTE: change this if implementing 3D sound stuff
	alDistanceModel (AL_NONE);

	StreamDecoderTask = AssignTask (StreamDecoderTaskFunc, 1024, 
		"audio stream decoder");

	return 0;
}

void
TFB_UninitSound (void)
{
	ConcludeTask (StreamDecoderTask);
	alcMakeContextCurrent (NULL);
	alcDestroyContext (alcContext);
	alcContext = NULL;
	alcCloseDevice (alcDevice);
	alcDevice = NULL;
}

void
StopSound (void)
{
	int i;

	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		alSourceRewind (soundSource[i].handle);
	}
}

BOOLEAN
SoundPlaying (void)
{
	int i;

	for (i = 0; i < NUM_SOUNDSOURCES; ++i)
	{		
		if (soundSource[i].sample && soundSource[i].sample->decoder)
		{
			BOOLEAN result;
			LockMutex (soundSource[i].stream_mutex);
			result = PlayingStream (i);
			UnlockMutex (soundSource[i].stream_mutex);
			if (result)
				return TRUE;
		}
		else
		{
			ALint state;
			alGetSourcei (soundSource[i].handle, AL_SOURCE_STATE, &state);
			if (state == AL_PLAYING)
				return TRUE;
		}
	}

	return FALSE;
}

// Status: Ignored
BOOLEAN
InitSound (int argc, char* argv[])
{
	return TRUE;
}

// Status: Ignored
void
UninitSound (void)
{
}

ALenum
DetermineALFormat (Sound_Sample *sample)
{
	ALenum format;

	if (sample->actual.channels == 1)
	{
		if (sample->actual.format == AUDIO_U8 ||
			sample->actual.format == AUDIO_S8)
		{
			format = AL_FORMAT_MONO8;
		}
		else
		{
			format = AL_FORMAT_MONO16;
		}
	}
	else
	{
		if (sample->actual.format == AUDIO_U8 ||
			sample->actual.format == AUDIO_S8)
		{
			format = AL_FORMAT_STEREO8;
		}
		else
		{
			format = AL_FORMAT_STEREO16;
		}
	}

	return format;
}

void SetSFXVolume (float volume)
{
	int i;
	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		alSourcef (soundSource[i].handle, AL_GAIN, sfxVolumeScale);
	}	
}

void SetSpeechVolume (float volume)
{
	alSourcef (soundSource[SPEECH_SOURCE].handle, AL_GAIN, speechVolumeScale);
}


#endif
