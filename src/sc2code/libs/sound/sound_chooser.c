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

/* Mixer abstraction layer for MixSDL
 */

#ifdef HAVE_OPENAL

#include "libs/sound/sound_common.h"
#include "libs/sound/sound_chooser.h"
#include <stdio.h>

static unsigned int tfb_enum_lookup[TFBSOUND_ENUMSIZE];
unsigned int TFBSOUND_NO_ERROR;
int TFBSOUND_PAUSED;
int TFBSOUND_PLAYING;
unsigned int TFBSOUND_FORMAT_MONO16;
unsigned int TFBSOUND_FORMAT_STEREO16;
unsigned int TFBSOUND_FORMAT_STEREO8;
unsigned int TFBSOUND_FORMAT_MONO8;

/*************************************************
 *  General interface
 */
uint32
TFBSound_GetError (void)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		return (alGetError ());
	else
		return (mixSDL_GetError ());
}

/*************************************************
 *  Sources
 */

void
TFBSound_GenSources (uint32 n, mixSDL_Object *psrcobj)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alGenSources (n, (ALuint *)psrcobj);
	else
		mixSDL_GenSources (n, (mixSDL_Object *)psrcobj);
}

void TFBSound_DeleteSources (uint32 n, TFBSound_Object *psrcobj)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alDeleteSources (n, (ALuint *)psrcobj);
	else
		mixSDL_DeleteSources (n, (mixSDL_Object *)psrcobj);
}

bool
TFBSound_IsSource (TFBSound_Object srcobj)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		return (alIsSource ((ALuint)srcobj));
	else
		return (mixSDL_IsSource ((mixSDL_Object)srcobj));
}

void
TFBSound_Sourcei (TFBSound_Object srcobj, TFBSound_SourceProp pname,
		TFBSound_IntVal value)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alSourcei ((ALuint) srcobj, (ALenum) tfb_enum_lookup[pname], (ALint) value);
	else
		mixSDL_Sourcei ((mixSDL_Object) srcobj, 
				(mixSDL_SourceProp) tfb_enum_lookup[pname], (mixSDL_IntVal) value);
}

void
TFBSound_Sourcef (TFBSound_Object srcobj, TFBSound_SourceProp pname, float value)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alSourcef ((ALuint) srcobj, (ALenum) tfb_enum_lookup[pname], value);
	else
		mixSDL_Sourcef ((mixSDL_Object) srcobj, 
				(mixSDL_SourceProp) tfb_enum_lookup[pname], value);
}

void
TFBSound_GetSourcei (TFBSound_Object srcobj, TFBSound_SourceProp pname,
		TFBSound_IntVal *value)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alGetSourcei ((ALuint) srcobj, (ALenum) tfb_enum_lookup[pname], (ALint *)value);
	else
		mixSDL_GetSourcei ((mixSDL_Object) srcobj, 
				(mixSDL_SourceProp) tfb_enum_lookup[pname], (mixSDL_IntVal *)value);
}

void
TFBSound_GetSourcef (TFBSound_Object srcobj, TFBSound_SourceProp pname,
		float *value)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alGetSourcef ((ALuint) srcobj, (ALenum) tfb_enum_lookup[pname], value);
	else
		mixSDL_GetSourcef ((mixSDL_Object) srcobj, 
				(mixSDL_SourceProp) tfb_enum_lookup[pname], value);
}

void
TFBSound_SourceRewind (TFBSound_Object srcobj)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alSourceRewind ((ALuint) srcobj);
	else
		mixSDL_SourceRewind ((mixSDL_Object) srcobj);
}

void
TFBSound_SourcePlay (TFBSound_Object srcobj)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alSourcePlay ((ALuint) srcobj);
	else
		mixSDL_SourcePlay ((mixSDL_Object) srcobj);
}

void
TFBSound_SourcePause (TFBSound_Object srcobj)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alSourcePause ((ALuint) srcobj);
	else
		mixSDL_SourcePause ((mixSDL_Object) srcobj);
}

void
TFBSound_SourceStop (TFBSound_Object srcobj)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alSourceStop ((ALuint) srcobj);
	else
		mixSDL_SourceStop ((mixSDL_Object) srcobj);
}

void
TFBSound_SourceQueueBuffers (TFBSound_Object srcobj, uint32 n,
		TFBSound_Object* pbufobj)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alSourceQueueBuffers ((ALuint) srcobj, n,(ALuint *) pbufobj);
	else
		mixSDL_SourceQueueBuffers ((mixSDL_Object) srcobj, n,
				(mixSDL_Object*) pbufobj);
}

void
TFBSound_SourceUnqueueBuffers (TFBSound_Object srcobj, uint32 n,
		TFBSound_Object* pbufobj)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alSourceUnqueueBuffers ((ALuint) srcobj, n, (ALuint *) pbufobj);
	else
		mixSDL_SourceUnqueueBuffers ((mixSDL_Object) srcobj, n,
				(mixSDL_Object*) pbufobj);
}

/*************************************************
 *  Buffers
 */
void
TFBSound_GenBuffers (uint32 n, TFBSound_Object *pbufobj)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alGenBuffers (n, (ALuint *)pbufobj);
	else
		mixSDL_GenBuffers (n, (mixSDL_Object *)pbufobj);
}

void
TFBSound_DeleteBuffers (uint32 n, TFBSound_Object *pbufobj)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alDeleteBuffers (n, (ALuint *)pbufobj);
	else
		mixSDL_DeleteBuffers (n, (mixSDL_Object *)pbufobj);
}

bool
TFBSound_IsBuffer (TFBSound_Object bufobj)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		return (alIsBuffer ((ALuint) bufobj));
	else
		return (mixSDL_IsBuffer ((mixSDL_Object) bufobj));
}

void
TFBSound_GetBufferi (TFBSound_Object bufobj, TFBSound_BufferProp pname,
		TFBSound_IntVal *value)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alGetBufferi ((ALuint) bufobj, (ALenum) tfb_enum_lookup[pname], (ALint *)value);
	else
		mixSDL_GetBufferi ((mixSDL_Object) bufobj, 
				(mixSDL_BufferProp) tfb_enum_lookup[pname], (mixSDL_IntVal *)value);
}

void
TFBSound_BufferData (TFBSound_Object bufobj, uint32 format, void* data,
		uint32 size, uint32 freq)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		alBufferData ((ALuint) bufobj, (ALenum) format, data, size, freq);
	else
		mixSDL_BufferData ((mixSDL_Object) bufobj, format, data, size, freq);
}

void
TFBSound_ConvertBuffer (uint32 srcfmt, void* srcdata, uint32 srcsize,
		uint32 srcfreq, uint32 dstfmt, void* dstdata,
		uint32 dstsize, uint32 dstfreq)
{
	if (SoundDriver == TFB_SOUNDDRIVER_OPENAL)
		fprintf (stderr, "ConvertBuffer is not supported bu openAL\n");
	else
		mixSDL_ConvertBuffer (srcfmt, srcdata, srcsize, srcfreq, dstfmt, dstdata,
				dstsize, dstfreq);
}

int 
TFB_choose_InitSound (int driver, int flags)
{
	if (driver == TFB_SOUNDDRIVER_OPENAL)
	{
		SoundDriver = driver;
		tfb_enum_lookup[TFBSOUND_GAIN]              = AL_GAIN;
		tfb_enum_lookup[TFBSOUND_BUFFER]            = AL_BUFFER;
		tfb_enum_lookup[TFBSOUND_SOURCE_STATE]      = AL_SOURCE_STATE;
		tfb_enum_lookup[TFBSOUND_LOOPING]           = AL_LOOPING;
		tfb_enum_lookup[TFBSOUND_BUFFERS_PROCESSED] = AL_BUFFERS_PROCESSED;
		tfb_enum_lookup[TFBSOUND_BUFFERS_QUEUED]    = AL_BUFFERS_QUEUED;
		tfb_enum_lookup[TFBSOUND_SIZE]              = AL_SIZE;
		TFBSOUND_NO_ERROR = AL_NO_ERROR;
		TFBSOUND_PAUSED = AL_PAUSED;
		TFBSOUND_PLAYING = AL_PLAYING;
		TFBSOUND_FORMAT_MONO16 = AL_FORMAT_MONO16;
		TFBSOUND_FORMAT_STEREO16 = AL_FORMAT_STEREO16;
		TFBSOUND_FORMAT_MONO8 = AL_FORMAT_MONO8;
		TFBSOUND_FORMAT_STEREO8 = AL_FORMAT_STEREO8;
		return (TFB_alInitSound (driver, flags));
	}
	else
	{
		SoundDriver = driver;
		tfb_enum_lookup[TFBSOUND_GAIN]              = MIX_GAIN;
		tfb_enum_lookup[TFBSOUND_BUFFER]            = MIX_BUFFER;
		tfb_enum_lookup[TFBSOUND_SOURCE_STATE]      = MIX_SOURCE_STATE;
		tfb_enum_lookup[TFBSOUND_LOOPING]           = MIX_LOOPING;
		tfb_enum_lookup[TFBSOUND_BUFFERS_PROCESSED] = MIX_BUFFERS_PROCESSED;
		tfb_enum_lookup[TFBSOUND_BUFFERS_QUEUED]    = MIX_BUFFERS_QUEUED;
		tfb_enum_lookup[TFBSOUND_SIZE]              = MIX_SIZE;
		TFBSOUND_NO_ERROR = MIX_NO_ERROR;
		TFBSOUND_PAUSED = MIX_PAUSED;
		TFBSOUND_PLAYING = MIX_PLAYING;
		TFBSOUND_FORMAT_MONO16 = MIX_FORMAT_MONO16;
		TFBSOUND_FORMAT_STEREO16 = MIX_FORMAT_STEREO16;
		TFBSOUND_FORMAT_MONO8 = MIX_FORMAT_MONO8;
		TFBSOUND_FORMAT_STEREO8 = MIX_FORMAT_STEREO8;
		return (TFB_mixSDL_InitSound (driver, flags));
	}
}

#endif
