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

#ifndef _UQM_SOUND_H // try avoiding collisions on id
#define _UQM_SOUND_H

#include "misc.h"
#include "libs/strings/strintrn.h"
#include "sndintrn.h"
#include "audiocore.h"
#include "decoders/decoder.h"
#include "libs/threadlib.h"


#define FIRST_SFX_SOURCE 0
#define LAST_SFX_SOURCE  (FIRST_SFX_SOURCE + NUM_SFX_CHANNELS - 1)
#define MUSIC_SOURCE (LAST_SFX_SOURCE + 1)
#define SPEECH_SOURCE (MUSIC_SOURCE + 1)
#define NUM_SOUNDSOURCES (SPEECH_SOURCE + 1)
#define PAD_SCOPE_BYTES 8096

typedef struct
{
	int in_use;
	audio_Object buf_name;
	void *data; // user-defined data
} TFB_SoundTag;

// forward-declare
typedef struct tfb_soundsample TFB_SoundSample;

typedef struct tfb_soundcallbacks
{
	// return TRUE to continue, FALSE to abort
	bool (* OnStartStream) (TFB_SoundSample*);
	// return TRUE to continue, FALSE to abort
	bool (* OnEndChunk) (TFB_SoundSample*, audio_Object);
	// return TRUE to continue, FALSE to abort
	void (* OnEndStream) (TFB_SoundSample*);
	// tagged buffer callback
	void (* OnTaggedBuffer) (TFB_SoundSample*, TFB_SoundTag*);
	// buffer just queued
	void (* OnQueueBuffer) (TFB_SoundSample*, audio_Object);
} TFB_SoundCallbacks;

// audio data
struct tfb_soundsample
{
	TFB_SoundDecoder *decoder; // decoder to read from
	float length; // total length of decoder chain in seconds
	audio_Object *buffer;
	uint32 num_buffers;
	TFB_SoundTag *buffer_tag;
	sint32 offset; // initial offset
	void* data; // user-defined data
	TFB_SoundCallbacks callbacks; // user-defined callbacks
};

// equivalent to channel in legacy sound code
typedef struct tfb_soundsource
{
	TFB_SoundSample *sample;
	audio_Object handle;
	bool stream_should_be_playing;
	Mutex stream_mutex;
	sint32 start_time;
	void *positional_object;

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

extern int musicVolume;
extern float musicVolumeScale;
extern float sfxVolumeScale;
extern float speechVolumeScale;

void StopSource (int iSource);
void CleanSource (int iSource);

void SetSFXVolume (float volume);
void SetSpeechVolume (float volume);

#include "stream.h"

#endif // _UQM_SOUND_H
