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

/* Sound file decoder for .wav, .mod, .ogg (to be used with OpenAL)
 * API is heavily influenced by SDL_sound.
 * By Mika Kolehmainen, 2002-10-27
 */

#ifndef DECODER_H
#define DECODER_H

#ifdef WIN32
#include <al.h>
#pragma comment (lib, "vorbisfile.lib")
#else
#include <AL/al.h>
#endif

typedef struct tfb_sounddecoder
{
	// public
	void *buffer;
	ALuint buffer_size;
	ALenum format;
	ALuint frequency;
	ALboolean looping;
	ALint error;
	float length; // total length in seconds
	char *decoder_info;

	// semi-private
	ALint type;
	char *filename;
	void *data;
	unsigned long pos;
	unsigned long start_sample;
	unsigned long end_sample;
} TFB_SoundDecoder;

// return values
enum
{
	SOUNDDECODER_OK,
	SOUNDDECODER_ERROR,
	SOUNDDECODER_EOF,
};

// types
enum
{
	SOUNDDECODER_NONE,
	SOUNDDECODER_WAV,
	SOUNDDECODER_MOD,
	SOUNDDECODER_OGG,
	SOUNDDECODER_BUF,
};

ALint SoundDecoder_Init (int flags);
void SoundDecoder_Uninit (void);
TFB_SoundDecoder* SoundDecoder_Load (char *filename, ALuint buffer_size, unsigned long startTime, unsigned long runTime);
ALuint SoundDecoder_Decode (TFB_SoundDecoder *decoder);
ALuint SoundDecoder_DecodeAll (TFB_SoundDecoder *decoder);
float SoundDecoder_GetTime (TFB_SoundDecoder *decoder);
void SoundDecoder_Seek (TFB_SoundDecoder *decoder, ALuint pcm_pos);
void SoundDecoder_Rewind (TFB_SoundDecoder *decoder);
void SoundDecoder_Free (TFB_SoundDecoder *decoder);

#endif
