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

/* Simple mixer for use with SDL_audio
 */

#ifndef SOUNDCHOOSER_H
#define SOUNDCHOOSER_H
#ifdef WIN32
#include <al.h>
#include <alc.h>
#pragma comment (lib, "OpenAL32.lib")
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "mixsdl/mixer.h"
#include "openal/mixer.h"
#include "types.h"


int TFB_mixSDL_InitSound (int driver, int flags);
#ifdef HAVE_OPENAL
int TFB_choose_InitSound (int driver, int flags);
int TFB_alInitSound (int driver, int flags);
#endif

void TFB_mixSDL_UninitSound (void);
#ifdef HAVE_OPENAL
void TFB_alUninitSound (void);
#endif


 /*************************************************
 *  Interface Types
 */

typedef unsigned int TFBSound_Object;
typedef int TFBSound_IntVal;
typedef const int TFBSound_SourceProp;
typedef const int TFBSound_BufferProp;

/*************************************************
 *  General interface
 */
uint32 TFBSound_GetError (void);

bool TFBSound_OpenAudio (uint32 freq, uint32 format, uint32 chunksize);
void TFBSound_CloseAudio (void);
bool TFBSound_QuerySpec (uint32 *freq, uint32 *format, uint32 *channels);

/*************************************************
 *  Sources
 */
void TFBSound_GenSources (uint32 n, TFBSound_Object *psrcobj);
void TFBSound_DeleteSources (uint32 n, TFBSound_Object *psrcobj);
bool TFBSound_IsSource (TFBSound_Object srcobj);
void TFBSound_Sourcei (TFBSound_Object srcobj, TFBSound_SourceProp pname,
		TFBSound_IntVal value);
void TFBSound_Sourcef (TFBSound_Object srcobj, TFBSound_SourceProp pname,
		float value);
void TFBSound_GetSourcei (TFBSound_Object srcobj, TFBSound_SourceProp pname,
		TFBSound_IntVal *value);
void TFBSound_GetSourcef (TFBSound_Object srcobj, TFBSound_SourceProp pname,
		float *value);
void TFBSound_SourceRewind (TFBSound_Object srcobj);
void TFBSound_SourcePlay (TFBSound_Object srcobj);
void TFBSound_SourcePause (TFBSound_Object srcobj);
void TFBSound_SourceStop (TFBSound_Object srcobj);
void TFBSound_SourceQueueBuffers (TFBSound_Object srcobj, uint32 n,
		TFBSound_Object* pbufobj);
void TFBSound_SourceUnqueueBuffers (TFBSound_Object srcobj, uint32 n,
		TFBSound_Object* pbufobj);

/*************************************************
 *  Buffers
 */
void TFBSound_GenBuffers (uint32 n, TFBSound_Object *pbufobj);
void TFBSound_DeleteBuffers (uint32 n, TFBSound_Object *pbufobj);
bool TFBSound_IsBuffer (TFBSound_Object bufobj);
void TFBSound_GetBufferi (TFBSound_Object bufobj, TFBSound_BufferProp pname,
		TFBSound_IntVal *value);
void TFBSound_BufferData (TFBSound_Object bufobj, uint32 format, void* data,
		uint32 size, uint32 freq);

void TFBSound_ConvertBuffer (uint32 srcfmt, void* srcdata, uint32 srcsize,
		uint32 srcfreq, uint32 dstfmt, void* dstdata,
		uint32 dstsize, uint32 dstfreq);

#define TFBSound_BufferData_Linux alBufferWriteData_LOKI
enum
{

	TFBSOUND_GAIN = 0,
	TFBSOUND_BUFFER,
	TFBSOUND_SOURCE_STATE,
	TFBSOUND_LOOPING,
	TFBSOUND_BUFFERS_PROCESSED,
	TFBSOUND_BUFFERS_QUEUED,
	TFBSOUND_SIZE,
	TFBSOUND_ENUMSIZE
};

extern int TFBSOUND_PAUSED;
extern int TFBSOUND_PLAYING;
extern unsigned int TFBSOUND_NO_ERROR;
extern unsigned int TFBSOUND_FORMAT_MONO16;
extern unsigned int TFBSOUND_FORMAT_STEREO16;
extern unsigned int TFBSOUND_FORMAT_MONO8;
extern unsigned int TFBSOUND_FORMAT_STEREO8;

#endif /* SOUNDCHOOSER_H */
