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

#include "types.h"

#ifdef WIN32
#pragma comment (lib, "vorbisfile.lib")
#endif

typedef struct tfb_decoderformats
{
	bool big_endian;
	bool want_big_endian;
	uint32 mono8;
	uint32 stereo8;
	uint32 mono16;
	uint32 stereo16;
} TFB_DecoderFormats;

typedef struct tfb_sounddecoder
{
	// public
	void *buffer;
	uint32 buffer_size;
	uint32 format;
	uint32 frequency;
	bool looping;
	sint32 error;
	float length; // total length in seconds
	char *decoder_info;

	// semi-private
	sint32 type;
	char *filename;
	void *data;
	uint32 pos;
	uint32 start_sample;
	uint32 end_sample;

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

extern TFB_DecoderFormats decoder_formats;

sint32 SoundDecoder_Init (int flags, TFB_DecoderFormats* formats);
void SoundDecoder_Uninit (void);
TFB_SoundDecoder* SoundDecoder_Load (char *filename, uint32 buffer_size, 
									 uint32 startTime, uint32 runTime);
uint32 SoundDecoder_Decode (TFB_SoundDecoder *decoder);
uint32 SoundDecoder_DecodeAll (TFB_SoundDecoder *decoder);
float SoundDecoder_GetTime (TFB_SoundDecoder *decoder);
void SoundDecoder_Seek (TFB_SoundDecoder *decoder, uint32 pcm_pos);
void SoundDecoder_Rewind (TFB_SoundDecoder *decoder);
void SoundDecoder_Free (TFB_SoundDecoder *decoder);

#endif
