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

#ifdef SOUNDMODULE_SDL

#include "libs/graphics/sdl/sdl_common.h"
#include "libs/sound/sound_common.h"

#include <SDL_mixer.h>
#ifdef WIN32
#pragma comment (lib, "SDL_mixer.lib")
#endif

int 
TFB_InitSound (int driver, int flags)
{
	char SoundcardName[256];
	int audio_rate, audio_channels, audio_bufsize;
	Uint16 audio_format;
	SDL_version compile_version;

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
		audio_channels = 2;
		audio_bufsize = 2048;
	}
	else if (flags & TFB_SOUNDFLAGS_LQAUDIO)
	{
		audio_rate = 22050;
		audio_channels = 2;
		audio_bufsize = 1024;
	}
	else
	{
		audio_rate = 44100;
		audio_channels = 2;
		audio_bufsize = 2048;
	}

	fprintf (stderr, "Initializing SDL_mixer.\n");
	if (Mix_OpenAudio(audio_rate, AUDIO_S16, audio_channels, audio_bufsize))
	{
		fprintf (stderr, "Unable to open audio: %s\n", Mix_GetError());
		exit (-1);
	}
	fprintf (stderr, "SDL_mixer initialized.\n");
	
	atexit (TFB_UninitSound);
	
	SDL_AudioDriverName (SoundcardName, sizeof (SoundcardName));
	Mix_QuerySpec (&audio_rate, &audio_format, &audio_channels);
	fprintf (stderr, "    opened %s at %d Hz %d bit %s, %d bytes audio buffer\n",
			SoundcardName, audio_rate, audio_format & 0xFF,
			audio_channels > 1 ? "stereo" : "mono", audio_bufsize);
	MIX_VERSION (&compile_version);
	fprintf (stderr, "    compiled with SDL_mixer version: %d.%d.%d\n",
			compile_version.major,
			compile_version.minor,
			compile_version.patch);
	fprintf (stderr, "    running with SDL_mixer version: %d.%d.%d\n",
			Mix_Linked_Version()->major,
			Mix_Linked_Version()->minor,
			Mix_Linked_Version()->patch);
	
	return 0;
}

void
TFB_UninitSound (void)
{
	Mix_CloseAudio ();
}

//Status: Unimplemented
void
StopSound()
		// So does this stop ALL sound? How about music?
{
	Mix_HaltChannel (-1);
}

int _music_volume = (MAX_VOLUME + 1) >> 1;

// Status: Unimplemented
void
SetMusicVolume(COUNT Volume) //Volume calibration... [0,255]?
{
	Mix_VolumeMusic ((_music_volume = Volume) * SDL_MIX_MAXVOLUME /
			MAX_VOLUME);
}

//Status: Unimplemented
BOOLEAN
SoundPlaying()
{
	return (Mix_Playing (-1) != 0);
}

int
GetSoundData (char* data)
{
   return 0;
}

// Status: Unimplemented
int
GetSoundInfo (int length)
		// Umm... How does it know which sound?
{
	//fprintf (stderr, "Unimplemented function activated: GetSoundInfo()\n");
	return 0;
}

// Status: Unimplemented
BOOLEAN
ChannelPlaying(COUNT WhichChannel)
		// SDL will have a nice interface for this, I hope.
{
	// fprintf (stderr, "Unimplemented function activated: ChannelPlaying()\n");
	return ((BOOLEAN) Mix_Playing (WhichChannel));
}

// Status: Unimplemented
void
PlayChannel(
		COUNT channel,
		PVOID sample,
		COUNT sample_length,
		COUNT loop_begin,
		COUNT loop_length,
		unsigned char priority
)  // This one's important.
{
	Mix_Chunk* Chunk;

	Chunk=*(Mix_Chunk**)sample;
	Mix_PlayChannel (
		channel, //-1 would be easiest
		Chunk,
		0
	);
}

//Status: Unimplemented
void
StopChannel(COUNT channel, unsigned char Priority)  // Easy w/ SDL.
{
	Mix_HaltChannel (channel);
}

// Status: Maybe Implemented
PBYTE
GetSampleAddress (SOUND sound)
		// I might be prototyping this wrong, type-wise.
{
	return ((PBYTE)GetSoundAddress (sound));
}

// Status: Unimplemented
COUNT
GetSampleLength (SOUND sound)
{
	COUNT ret;
	Mix_Chunk *Chunk;
		
	Chunk = (Mix_Chunk *) sound;
	ret = 0;  // Chunk->alen;

// fprintf (stderr, "Maybe Implemented function activated: GetSampleLength()\n");
	return(ret);
}

// Status: Unimplemented
void
SetChannelRate (COUNT channel, DWORD rate_hz, unsigned char priority)
		// in hz
{
// fprintf (stderr, "Unimplemented function activated: SetChannelRate()\n");
}

// Status: Unimplemented
COUNT
GetSampleRate (SOUND sound)
{
	COUNT ret;

// fprintf (stderr, "Unimplemented function activated: GetSampleRate()\n");
	ret=0;
	return(ret);
}

// Status: Unimplemented
void
SetChannelVolume (COUNT channel, COUNT volume, BYTE priority)
		// I wonder what this whole priority business is...
		// I can probably ignore it.
{
// fprintf (stderr, "Unimplemented function activated: SetChannelVolume()\n");
}

//Status: Ignored
BOOLEAN
InitSound (int argc, char* argv[])
		// Odd things to pass the InitSound function...
{
	BOOLEAN ret;

// fprintf (stderr, "Unimplemented function activated: InitSound()\n");
	ret = TRUE;
	return (ret);
}

// Status: Unimplemented
void
UninitSound ()  //Maybe I should call StopSound() first?
{
//	fprintf (stderr, "Unimplemented function activated: UninitSound()\n");
}

#endif
