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

#include "starcon.h"
#include "misc.h"
#include "libs/strings/strintrn.h"
#include "libs/sound/sndintrn.h"
#include "libs/sound/sound_common.h"
#ifdef HAVE_OPENAL
#include "libs/sound/sound_chooser.h"
#else
#include "mixsdl/sound_mixsdl.h"
#endif
#include "decoders/decoder.h"


#define FIRST_SFX_SOURCE 0
#define LAST_SFX_SOURCE 4
#define MUSIC_SOURCE (LAST_SFX_SOURCE + 1)
#define SPEECH_SOURCE (MUSIC_SOURCE + 1)
#define NUM_SOUNDSOURCES (SPEECH_SOURCE + 1)
#define PAD_SCOPE_BYTES 8096

typedef struct
{
	TFBSound_Object buf_name;
	int type;
	uint32 value;
	void *data;
	void (*callback) (void);
} TFB_SoundTag;

enum
{
	MIX_BUFFER_TAG_TEXT = 1,
	MIX_BUFFER_TAG_TEXTPAGE
};

typedef struct tfb_soundchain
{
	TFB_SoundDecoder *decoder; // points at the decoder to read from
	float start_time;
	TFB_SoundTag tag;
	struct tfb_soundchain *next;
} TFB_SoundChain;

// audio data
typedef struct tfb_soundsample
{
	TFB_SoundDecoder *decoder; // decoder to read from
	TFB_SoundChain *read_chain_ptr; // points to chain read poistion
	TFB_SoundChain *play_chain_ptr; // points to chain playing position
	float length; // total length of decoder chain in seconds
	TFBSound_Object *buffer;
	uint32 num_buffers;
	TFB_SoundTag **buffer_tag;
} TFB_SoundSample;

// equivalent to channel in legacy sound code
typedef struct tfb_soundsource
{
	TFB_SoundSample *sample;
	TFBSound_Object handle;
	bool stream_should_be_playing;
	Mutex stream_mutex;
	sint32 start_time;

	// for oscilloscope
	void *sbuffer; 
	uint32 sbuf_start;
	uint32 sbuf_size;
	uint32 sbuf_offset;
	uint32 sbuf_lasttime;
	// keep track for paused tracks
	uint32 pause_time;
} TFB_SoundSource;


extern TFB_SoundSource soundSource[];

void StopSource (int iSource);
void CleanSource (int iSource);

void SetSFXVolume (float volume);
void SetSpeechVolume (float volume);
void DoTrackTag (TFB_SoundTag *tag);

TFB_SoundChain *create_soundchain (TFB_SoundDecoder *decoder, float startTime);
void destroy_soundchain (TFB_SoundChain *chain);
TFB_SoundChain *get_previous_chain (TFB_SoundChain *first_chain, TFB_SoundChain *current_chain);

#include "stream.h"
