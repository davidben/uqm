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
#include "mixer.h"

#define TFBSound_Object mixSDL_Object

/*************************************************
 *  General interface
 */
#define TFBSound_GetError         mixSDL_GetError
#define TFBSound_OpenAudio        mixSDL_OpenAudio
#define TFBSound_CloseAudio       mixSDL_CloseAudio
#define TFBSound_QuerySpec        mixSDL_QuerySpec

/*************************************************
 *  Sources
 */
#define TFBSound_GenSources       mixSDL_GenSources
#define TFBSound_DeleteSources    mixSDL_DeleteSources
#define TFBSound_IsSource         mixSDL_IsSource
#define TFBSound_Sourcei          mixSDL_Sourcei
#define TFBSound_Sourcef          mixSDL_Sourcef
#define TFBSound_GetSourcei       mixSDL_GetSourcei
#define TFBSound_GetSourcef       mixSDL_GetSourcef
#define TFBSound_SourceRewind     mixSDL_SourceRewind
#define TFBSound_SourcePlay       mixSDL_SourcePlay
#define TFBSound_SourcePause      mixSDL_SourcePause
#define TFBSound_SourceStop       mixSDL_SourceStop
#define TFBSound_SourceQueueBuffers     mixSDL_SourceQueueBuffers
#define TFBSound_SourceUnqueueBuffers   mixSDL_SourceUnqueueBuffers

/*************************************************
 *  Buffers
 */
#define TFBSound_GenBuffers       mixSDL_GenBuffers
#define TFBSound_DeleteBuffers    mixSDL_DeleteBuffers
#define TFBSound_IsBuffer         mixSDL_IsBuffer
#define TFBSound_GetBufferi       mixSDL_GetBufferi
#define TFBSound_BufferData       mixSDL_BufferData
#define TFBSound_ConvertBuffer    mixSDL_ConvertBuffer

#define TFBSOUND_GAIN              MIX_GAIN
#define TFBSOUND_BUFFER            MIX_BUFFER
#define TFBSOUND_SOURCE_STATE      MIX_SOURCE_STATE
#define TFBSOUND_PLAYING           MIX_PLAYING
#define TFBSOUND_PAUSED            MIX_PAUSED
#define TFBSOUND_FORMAT_MONO16     MIX_FORMAT_MONO16
#define TFBSOUND_FORMAT_STEREO16   MIX_FORMAT_STEREO16
#define TFBSOUND_FORMAT_STEREO8    MIX_FORMAT_STEREO8
#define TFBSOUND_LOOPING           MIX_LOOPING
#define TFBSOUND_BUFFERS_PROCESSED MIX_BUFFERS_PROCESSED
#define TFBSOUND_BUFFERS_QUEUED    MIX_BUFFERS_QUEUED
#define TFBSOUND_NO_ERROR          MIX_NO_ERROR
#define TFBSOUND_SIZE              MIX_SIZE
