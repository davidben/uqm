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
#include "libs/sound/sound.h"
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
		soundSource[i].sbuf_offset = 0;
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

#endif
