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

#include "libs/strings/strintrn.h"
#include "libs/sound/sndintrn.h"
#include "libs/graphics/sdl/sdl_common.h"
#include "libs/sound/sound_common.h"

#ifdef WIN32
#include <al.h>
#include <alc.h>
#include <SDL_sound.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <SDL/SDL_sound.h>
#endif


#define NUM_SOUNDBUFFERS 8
#define FIRST_SFX_SOURCE 0
#define LAST_SFX_SOURCE 4
#define MUSIC_SOURCE (LAST_SFX_SOURCE + 1)
#define SPEECH_SOURCE (MUSIC_SOURCE + 1)
#define NUM_SOUNDSOURCES (SPEECH_SOURCE + 1)

// audio data
typedef struct tfb_soundsample
{
	Sound_Sample *decoder; // if != NULL, sound is streamed
	ALuint buffer[NUM_SOUNDBUFFERS]; // if not streamed, only buffer[0] is used
} TFB_SoundSample;

// equivalent to channel in legacy sound code
typedef struct tfb_soundsource
{
	TFB_SoundSample *sample;
	ALuint handle;
	BOOLEAN stream_looping;
	BOOLEAN stream_should_be_playing;
	Mutex stream_mutex;
} TFB_SoundSource;


extern ALCcontext *alcContext; // OpenAL context
extern ALCdevice *alcDevice; // OpenAL device
extern ALfloat listenerPos[]; // listener position (3 floats)
extern ALfloat listenerVel[]; // listener velocity (3 floats)
extern ALfloat listenerOri[]; // listener orientation (two 3 float vectors, "look at" and "up")
extern TFB_SoundSource soundSource[];

ALenum DetermineALFormat (Sound_Sample *sample);
void SetSFXVolume (float volume);
void SetSpeechVolume (float volume);

#include "stream.h"
