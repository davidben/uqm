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

//#include "libs/graphics/sdl/sdl_common.h"
#include "SDL.h"
#include "libs/sound/sound.h"
#include "libs/tasklib.h"

static Task StreamDecoderTask;

/*******************************************************************
 *  NoSound driver for MixSDL - declarations
 */

static const char* NoSound_GetDriverName (void);
static const char* NoSound_GetError (void);
static int NoSound_OpenAudio (void *desired, void *obtained);
static void NoSound_CloseAudio (void);
static void NoSound_PauseAudio (int pause_on);

/* The nosound driver */
static const
mixSDL_DriverInfo NoSound_driver = 
{
	NoSound_GetDriverName,
	NoSound_GetError,
	NoSound_OpenAudio,
	NoSound_CloseAudio,
	NoSound_PauseAudio
};
static SDL_AudioSpec NoSound_spec;
static Task NoSound_PlaybackTask;
static bool NoSound_Paused = true;
static const char* NoSound_ErrorStr = "";

/*******************************************************************
 *  Other stuff
 */
static int current_driver = 0;

int TFB_NoSound_InitSound (int driver, int flags);

/*******************************************************************
 *  Normal MixSDL sound
 */

int 
TFB_mixSDL_InitSound (int driver, int flags)
{
	int i;
	char SoundcardName[256];
	uint32 audio_rate, audio_channels, audio_bufsize, audio_format;
	mixSDL_Quality audio_quality;
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
		return -1;
	}
	fprintf (stderr, "SDL audio subsystem initialized.\n");
	
	if (flags & TFB_SOUNDFLAGS_HQAUDIO)
	{
		audio_quality = MIX_QUALITY_HIGH;
		audio_rate = 44100;
		audio_bufsize = 2048;
	}
	else if (flags & TFB_SOUNDFLAGS_LQAUDIO)
	{
		audio_quality = MIX_QUALITY_LOW;
		audio_rate = 22050;
		audio_bufsize = 1024;
	}
	else
	{
		audio_quality = MIX_QUALITY_DEFAULT;
		audio_rate = 44100;
		audio_bufsize = 2048;
	}

	fprintf (stderr, "Initializing MixSDL mixer.\n");
	if (!mixSDL_OpenAudio(audio_rate, MIX_FORMAT_STEREO16,
			audio_bufsize, audio_quality))
	{
		fprintf (stderr, "Unable to open audio: %x, %s\n",
				mixSDL_GetError (), SDL_GetError ());
		SDL_QuitSubSystem (SDL_INIT_AUDIO);
		return -1;
	}
	fprintf (stderr, "MixSDL Mixer initialized.\n");
	
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

	current_driver = driver;
	
	return 0;
}

void
TFB_mixSDL_UninitSound (void)
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

/*******************************************************************
 *  NoSound driver for MixSDL
 */

int 
TFB_NoSound_InitSound (int driver, int flags)
{
	int i;
	uint32 audio_rate, audio_channels, audio_bufsize, audio_format;
	TFB_DecoderFormats formats =
	{
		0, 0, /* do not care about endianness */
		MIX_FORMAT_MONO8, MIX_FORMAT_STEREO8,
		MIX_FORMAT_MONO16, MIX_FORMAT_STEREO16
	};

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

	fprintf (stderr, "Initializing MixSDL mixer.\n");
	mixSDL_UseDriver (&NoSound_driver, MIX_DRIVER_FAKE_ALL);
	if (!mixSDL_OpenAudio (audio_rate, MIX_FORMAT_STEREO16,
			audio_bufsize, MIX_QUALITY_DEFAULT))
	{
		fprintf (stderr, "Unable to open audio: %x, %s\n",
				mixSDL_GetError (), SDL_GetError ());
		return -1;
	}
	fprintf (stderr, "MixSDL mixer initialized.\n");
	
	atexit (TFB_UninitSound);
	
	mixSDL_QuerySpec (&audio_rate, &audio_format, &audio_channels);
	fprintf (stderr, "    opened 'fake' "
			"at %d Hz %d bit %s, %d samples audio buffer\n",
			audio_rate, audio_format & 0xFF,
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

	current_driver = driver;
	
	return 0;
}

void
TFB_NoSound_UninitSound (void)
{
	TFB_mixSDL_UninitSound ();
}

int
NoSound_PlaybackTaskFunc (void *data)
{
	Task task = (Task)data;
	uint8 *stream;
	uint32 entryTime;
	sint32 period, delay;
	
	stream = (uint8 *) HMalloc (NoSound_spec.size);
	period = 1000 * NoSound_spec.samples / NoSound_spec.freq;

	while (!Task_ReadState (task, TASK_EXIT))
	{
		entryTime = SDL_GetTicks ();

		if (!NoSound_Paused)
		{
			NoSound_spec.callback (NoSound_spec.userdata,
					stream, NoSound_spec.size);
		}
		
		delay = period - (SDL_GetTicks () - entryTime);
		if (delay > 0)
			SDL_Delay (delay);
	}

	FinishTask (task);
	return 0;
}

static const char*
NoSound_GetDriverName ()
{
	return "NoSound";
}

static const char*
NoSound_GetError ()
{
	return NoSound_ErrorStr;
}

static int
NoSound_OpenAudio (void *desired, void *obtained)
{
	/* we accept anything - copy the format verbatim */
	memcpy (&NoSound_spec, desired, sizeof (SDL_AudioSpec));
	/* calculate the requested PCM-buffer size and silence val */
	NoSound_spec.size = (NoSound_spec.format & 0x003f) / 8
			* NoSound_spec.channels * NoSound_spec.samples;
	NoSound_spec.silence = ((NoSound_spec.format >> 8) & 0x80) ^ 0x80;

	memcpy (obtained, &NoSound_spec, sizeof (SDL_AudioSpec));

	NoSound_PlaybackTask = AssignTask (NoSound_PlaybackTaskFunc, 1024, 
		"nosound audio playback");
	if (!NoSound_PlaybackTask)
	{
		NoSound_ErrorStr = "Could not start Playback Task";
		return -1;
	}

	return 0;
}

static void
NoSound_CloseAudio (void)
{
	if (NoSound_PlaybackTask)
	{
		ConcludeTask (NoSound_PlaybackTask);
		NoSound_PlaybackTask = 0;
	}
}

static void
NoSound_PauseAudio (int pause_on)
{
	NoSound_Paused = pause_on;
}
