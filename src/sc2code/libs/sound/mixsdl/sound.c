//Copyright Paul Reiche, Fred Ford. 1992-2002

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

#ifdef SOUNDMODULE_MIXSDL

#include "libs/graphics/sdl/sdl_common.h"
#include "libs/sound/sound_common.h"
#include "sound.h"

TFB_SoundSource soundSource[NUM_SOUNDSOURCES];
static Task StreamDecoderTask;

int 
TFB_InitSound (int driver, int flags)
{
	int i;
	char SoundcardName[256];
	uint32 audio_rate, audio_channels, audio_bufsize, audio_format;
	TFB_DecoderFormats formats =
	{
		MIX_IS_BIG_ENDIAN, MIX_WANT_BIG_ENDIAN,
		MIX_FORMAT_MONO8, MIX_FORMAT_STEREO8,
		MIX_FORMAT_MONO16, MIX_FORMAT_STEREO16
	};

	fprintf (stderr, "Initializing SDL audio subsystem.\n");
	if ((SDL_InitSubSystem(SDL_INIT_AUDIO)) == -1)
	{
		fprintf (stderr, "Couldn't initialize audio subsystem: %s\n", SDL_GetError());
		exit(-1);
	}
	fprintf (stderr, "SDL audio subsystem initialized.\n");
	
	if (flags & TFB_SOUNDFLAGS_HQAUDIO)
	{
		audio_rate = 44100;
		audio_bufsize = 2048;
	}
	else if (flags & TFB_SOUNDFLAGS_LQAUDIO)
	{
		audio_rate = 22050;
		audio_bufsize = 1024;
	}
	else
	{
		audio_rate = 44100;
		audio_bufsize = 2048;
	}

	fprintf (stderr, "Initializing internal Mixer for SDL_audio.\n");
	if (!mixSDL_OpenAudio(audio_rate, MIX_FORMAT_STEREO16, audio_bufsize))
	{
		fprintf (stderr, "Unable to open audio: %x, %s\n",
				mixSDL_GetError (), SDL_GetError ());
		exit (-1);
	}
	fprintf (stderr, "Mixer for SDL_audio initialized.\n");
	
	atexit (TFB_UninitSound);
	
	SDL_AudioDriverName (SoundcardName, sizeof (SoundcardName));
	mixSDL_QuerySpec (&audio_rate, &audio_format, &audio_channels);
	fprintf (stderr, "    opened %s at %d Hz %d bit %s, %d samples audio buffer\n",
			SoundcardName, audio_rate, audio_format & 0xFF,
			audio_channels > 1 ? "stereo" : "mono", audio_bufsize);
	
	fprintf (stderr, "Initializing sound decoders.\n");
	SoundDecoder_Init (flags, &formats);
	fprintf (stderr, "Sound decoders initialized.\n");

	for (i = 0; i < NUM_SOUNDSOURCES; ++i)
	{
		mixSDL_GenSources (1, &soundSource[i].handle);
		
		soundSource[i].sample = NULL;
		soundSource[i].stream_should_be_playing = FALSE;
		soundSource[i].stream_mutex = CreateMutex ();
		soundSource[i].sbuffer = NULL;
		soundSource[i].sbuf_start = 0;
		soundSource[i].sbuf_size = 0;
		soundSource[i].total_decoded = 0;
	}
	
	SetSFXVolume (sfxVolumeScale);
	SetSpeechVolume (speechVolumeScale);
	SetMusicVolume ((COUNT)musicVolume);
		
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

		mixSDL_DeleteSources (1, &soundSource[i].handle);
	}

	SoundDecoder_Uninit ();
	mixSDL_CloseAudio ();
}

void
StopSound (void)
{
	int i;

	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		mixSDL_SourceRewind (soundSource[i].handle);
	}
}

BOOLEAN
SoundPlaying (void)
{
	int i;

	for (i = 0; i < NUM_SOUNDSOURCES; ++i)
	{
		TFB_SoundSample *sample;
		sample = soundSource[i].sample;
		if (sample && sample->decoder)
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
			uint32 state;
			mixSDL_GetSourcei (soundSource[i].handle, MIX_SOURCE_STATE, &state);
			if (state == MIX_PLAYING)
				return TRUE;
		}
	}

	return FALSE;
}

TFB_SoundChain *
create_soundchain (TFB_SoundDecoder *decoder, float startTime)
{
	TFB_SoundChain *chain;
	chain = (TFB_SoundChain *)HMalloc (sizeof (TFB_SoundChain));
	chain->decoder = decoder;
	chain->next =NULL;
	chain->start_time = startTime;
	chain->tag.buf_name = 0;
	chain->tag.type = 0;
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
		HFree (tmp_chain);
	}
	SoundDecoder_Free (chain->decoder);
	HFree (chain);
}

TFB_SoundChain *
get_previous_chain (TFB_SoundChain *first_chain, TFB_SoundChain *current_chain)
{
	TFB_SoundChain *prev_chain;
	prev_chain = first_chain;
	if (prev_chain == current_chain)
		return (prev_chain);
	while (prev_chain->next)
	{
		if (prev_chain->next == current_chain)
			return (prev_chain);
		prev_chain = prev_chain->next;
	}
	return (first_chain);
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

void SetSFXVolume (float volume)
{
	int i;
	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		mixSDL_Sourcef (soundSource[i].handle, MIX_GAIN, volume);
	}	
}

void SetSpeechVolume (float volume)
{
	mixSDL_Sourcef (soundSource[SPEECH_SOURCE].handle, MIX_GAIN, volume);
}

#endif
