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


/****************************************

  Star Control II: 3DO => SDL Port

  Copyright Toys for Bob, 2002

  Programmer: Chris Nelson
  
*****************************************/

#if defined (GFXMODULE_SDL_OPENGL) || defined (GFXMODULE_SDL_PURE)

#include "libs/graphics/sdl/sdl_common.h"

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
GetSoundData(char* data) //Returns the data size.
{
	int ret;

	printf("Unimplemented function activated: GetSoundData()\n");
	ret = 0;
	return (ret);
}

// Status: Unimplemented
int
GetSoundInfo(int* length, int* offset)
		// Umm... How does it know which sound?
{
	int ret;

	printf ("Unimplemented function activated: GetSoundInfo()\n");
	ret = 0;
	return (ret);
}

// Status: Unimplemented
BOOLEAN
ChannelPlaying(COUNT WhichChannel)
		// SDL will have a nice interface for this, I hope.
{
	// printf("Unimplemented function activated: ChannelPlaying()\n");
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

// printf ("Maybe Implemented function activated: GetSampleLength()\n");
	return(ret);
}

// Status: Unimplemented
void
SetChannelRate (COUNT channel, DWORD rate_hz, unsigned char priority)
		// in hz
{
// printf ("Unimplemented function activated: SetChannelRate()\n");
}

// Status: Unimplemented
COUNT
GetSampleRate (SOUND sound)
{
	COUNT ret;

// printf("Unimplemented function activated: GetSampleRate()\n");
	ret=0;
	return(ret);
}

// Status: Unimplemented
void
SetChannelVolume (COUNT channel, COUNT volume, BYTE priority)
		// I wonder what this whole priority business is...
		// I can probably ignore it.
{
// printf ("Unimplemented function activated: SetChannelVolume()\n");
}

//Status: Ignored
BOOLEAN
InitSound (int argc, char* argv[])
		// Odd things to pass the InitSound function...
{
	BOOLEAN ret;

// printf("Unimplemented function activated: InitSound()\n");
	ret = TRUE;
	return (ret);
}

// Status: Unimplemented
void
UninitSound ()  //Maybe I should call StopSound() first?
{
	//printf("Unimplemented function activated: UninitSound()\n");
}

#endif
