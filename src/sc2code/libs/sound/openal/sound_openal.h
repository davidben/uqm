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

#ifdef WIN32
#include <al.h>
#include <alc.h>
#pragma comment (lib, "OpenAL32.lib")
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "mixer.h"

#define TFBSound_Object ALuint

/*************************************************
 *  General interface
 */
#define TFBSound_GetError         alGetError
#define TFBSound_OpenAudio        alOpenAudio
#define TFBSound_CloseAudio       alCloseAudio
#define TFBSound_QuerySpec        alQuerySpec

/*************************************************
 *  Sources
 */
#define TFBSound_GenSources       alGenSources
#define TFBSound_DeleteSources    alDeleteSources
#define TFBSound_IsSource         alIsSource
#define TFBSound_Sourcei          alSourcei
#define TFBSound_Sourcef          alSourcef
#define TFBSound_GetSourcei       alGetSourcei
#define TFBSound_GetSourcef       alGetSourcef
#define TFBSound_SourceRewind     alSourceRewind
#define TFBSound_SourcePlay       alSourcePlay
#define TFBSound_SourcePause      alSourcePause
#define TFBSound_SourceStop       alSourceStop
#define TFBSound_SourceQueueBuffers     alSourceQueueBuffers
#define TFBSound_SourceUnqueueBuffers   alSourceUnqueueBuffers

/*************************************************
 *  Buffers
 */
#define TFBSound_GenBuffers       alGenBuffers
#define TFBSound_DeleteBuffers    alDeleteBuffers
#define TFBSound_IsBuffer         alIsBuffer
#define TFBSound_GetBufferi       alGetBufferi
#define TFBSound_BufferData       alBufferData
#define TFBSound_ConvertBuffer    alConvertBuffer
#define TFBSound_BufferData_Linux alBufferWriteData_LOKI

#define TFBSOUND_GAIN              AL_GAIN
#define TFBSOUND_BUFFER            AL_BUFFER
#define TFBSOUND_SOURCE_STATE      AL_SOURCE_STATE
#define TFBSOUND_PLAYING           AL_PLAYING
#define TFBSOUND_PAUSED            AL_PAUSED
#define TFBSOUND_FORMAT_MONO16     AL_FORMAT_MONO16
#define TFBSOUND_FORMAT_STEREO16   AL_FORMAT_STEREO16
#define TFBSOUND_FORMAT_STEREO8    AL_FORMAT_STEREO8
#define TFBSOUND_LOOPING           AL_LOOPING
#define TFBSOUND_BUFFERS_PROCESSED AL_BUFFERS_PROCESSED
#define TFBSOUND_BUFFERS_QUEUED    AL_BUFFERS_QUEUED
#define TFBSOUND_NO_ERROR          AL_NO_ERROR
#define TFBSOUND_SIZE              AL_SIZE
