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

#include "starcon.h"
#include "misc.h"
#include "libs/strings/strintrn.h"
#include "libs/sound/sndintrn.h"
#include "libs/sound/sound_common.h"
#include "mixer.h"
#include "decoders/decoder.h"


#define FIRST_SFX_SOURCE 0
#define LAST_SFX_SOURCE 4
#define MUSIC_SOURCE (LAST_SFX_SOURCE + 1)
#define SPEECH_SOURCE (MUSIC_SOURCE + 1)
#define NUM_SOUNDSOURCES (SPEECH_SOURCE + 1)

// audio data
typedef struct tfb_soundsample
{
	TFB_SoundDecoder *decoder; // if != NULL, sound is streamed
	float length; // total length in seconds
	mixSDL_Object *buffer;
	uint32 num_buffers;
} TFB_SoundSample;

// equivalent to channel in legacy sound code
typedef struct tfb_soundsource
{
	TFB_SoundSample *sample;
	mixSDL_Object handle;
	bool stream_should_be_playing;
	Mutex stream_mutex;
	uint32 start_time;

	// for oscilloscope
	void *sbuffer; 
	uint32 sbuf_start;
	uint32 sbuf_size;
	uint32 total_decoded;
} TFB_SoundSource;


extern TFB_SoundSource soundSource[];

void SetSFXVolume (float volume);
void SetSpeechVolume (float volume);

#include "stream.h"
