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

#ifndef MIXER_H
#define MIXER_H

#include "types.h"
#include "SDL_byteorder.h"

/**
 * The interface heavily influenced by OpenAL
 * to the point where you should use OpenAL's
 * documentation when programming the mixer.
 * (some source properties are not supported)
 *
 * EXCEPTION: You may not queue the same buffer
 * on more than one source
 */

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#	define MIX_IS_BIG_ENDIAN   true
#	define MIX_WANT_BIG_ENDIAN true
#else
#	define MIX_IS_BIG_ENDIAN   false
#	define MIX_WANT_BIG_ENDIAN false
#endif

/**
 * Mixer errors (see OpenAL errors)
 */
enum
{
	MIX_NO_ERROR = 0,
	MIX_INVALID_NAME = 0xA001U,
	MIX_INVALID_ENUM = 0xA002U,
	MIX_INVALID_VALUE = 0xA003U,
	MIX_INVALID_OPERATION = 0xA004U,
	MIX_OUT_OF_MEMORY = 0xA005U,

	MIX_SDL_FAILURE = 0xA101U
};

/**
 * Source properties (see OpenAL)
 */
typedef enum
{
	MIX_LOOPING = 0x1007,
	MIX_BUFFER = 0x1009,
	MIX_GAIN = 0x100A,
	MIX_SOURCE_STATE = 0x1010,

	MIX_BUFFERS_QUEUED = 0x1015,
	MIX_BUFFERS_PROCESSED = 0x1016

} mixSDL_SourceProp;

/**
 * Source state information
 */
typedef enum
{
	MIX_INITIAL = 0,
	MIX_STOPPED,
	MIX_PLAYING,
	MIX_PAUSED,

} mixSDL_SourceState;

/** 
 * Sound buffer properties
 */
typedef enum
{
	MIX_FREQUENCY = 0x2001,
	MIX_BITS = 0x2002,
	MIX_CHANNELS = 0x2003,
	MIX_SIZE = 0x2004,
	MIX_DATA = 0x2005

} mixSDL_BufferProp;

/**
 * Buffer states: semi-private
 */
typedef enum
{
	MIX_BUF_INITIAL = 0,
	MIX_BUF_FILLED,
	MIX_BUF_QUEUED,
	MIX_BUF_PLAYING,
	MIX_BUF_PROCESSED

} mixSDL_BufferState;

/** Sound buffers: format specifier.
 * bits 00..07: bytes per sample
 * bits 08..15: channels
 * bits 15..31: meaningless
 */
#define MIX_FORMAT_DUMMYID     0x00170000
#define MIX_FORMAT_BPC(f)      ((f) & 0xff)
#define MIX_FORMAT_CHANS(f)    (((f) >> 8) & 0xff)
#define MIX_FORMAT_BPC_MAX     2
#define MIX_FORMAT_CHANS_MAX   2
#define MIX_FORMAT_MAKE(b, c) \
		( MIX_FORMAT_DUMMYID | ((b) & 0xff) | (((c) & 0xff) << 8) )

#define MIX_FORMAT_SAMPSIZE(f) \
		( MIX_FORMAT_BPC(f) * MIX_FORMAT_CHANS(f) )

typedef enum
{
	MIX_FORMAT_MONO8 = MIX_FORMAT_MAKE (1, 1),
	MIX_FORMAT_STEREO8 = MIX_FORMAT_MAKE (1, 2),
	MIX_FORMAT_MONO16 = MIX_FORMAT_MAKE (2, 1),
	MIX_FORMAT_STEREO16 = MIX_FORMAT_MAKE (2, 2)

} mixSDL_Format;


typedef int mixSDL_Object;

typedef struct _mixSDL_Buffer
{
	uint32 magic;
	bool locked;
	mixSDL_BufferState state;
	uint8 *data;
	uint32 size;
	/* original buffer values for OpenAL compat */
	void* orgdata;
	uint32 orgfreq;
	uint32 orgsize;
	uint32 orgchannels;
	uint32 orgchansize;
	/* next buffer in chain */
	struct _mixSDL_Buffer *next;

} mixSDL_Buffer;

#define mixSDL_bufMagic 0x4258494DU /* MIXB in LSB */

typedef struct
{
	uint32 magic;
	bool locked;
	mixSDL_SourceState state;
	bool looping;
	float gain;
	uint32 cqueued;
	uint32 cprocessed;
	mixSDL_Buffer *firstqueued; /* first buf in the queue */
	mixSDL_Buffer *nextqueued;  /* next to play, or 0 */
	mixSDL_Buffer *lastqueued;  /* last in queue */
	uint32 curbufofs;

} mixSDL_Source;

#define mixSDL_srcMagic 0x5358494DU /* MIXS in LSB */

/*************************************************
 *  General interface
 */
uint32 mixSDL_GetError (void);

bool mixSDL_OpenAudio (uint32 freq, uint32 format, uint32 chunksize);
void mixSDL_CloseAudio (void);
bool mixSDL_QuerySpec (uint32 *freq, uint32 *format, uint32 *channels);

/*************************************************
 *  Sources
 */
void mixSDL_GenSources (uint32 n, mixSDL_Object *psrcobj);
void mixSDL_DeleteSources (uint32 n, mixSDL_Object *psrcobj);
bool mixSDL_IsSource (mixSDL_Object srcobj);
void mixSDL_Sourcei (mixSDL_Object srcobj, mixSDL_SourceProp pname,
		mixSDL_Object value);
void mixSDL_Sourcef (mixSDL_Object srcobj, mixSDL_SourceProp pname,
		float value);
void mixSDL_GetSourcei (mixSDL_Object srcobj, mixSDL_SourceProp pname,
		mixSDL_Object *value);
void mixSDL_GetSourcef (mixSDL_Object srcobj, mixSDL_SourceProp pname,
		float *value);
void mixSDL_SourceRewind (mixSDL_Object srcobj);
void mixSDL_SourcePlay (mixSDL_Object srcobj);
void mixSDL_SourcePause (mixSDL_Object srcobj);
void mixSDL_SourceStop (mixSDL_Object srcobj);
void mixSDL_SourceQueueBuffers (mixSDL_Object srcobj, uint32 n,
		mixSDL_Object* pbufobj);
void mixSDL_SourceUnqueueBuffers (mixSDL_Object srcobj, uint32 n,
		mixSDL_Object* pbufobj);

/*************************************************
 *  Buffers
 */
void mixSDL_GenBuffers (uint32 n, mixSDL_Object *pbufobj);
void mixSDL_DeleteBuffers (uint32 n, mixSDL_Object *pbufobj);
bool mixSDL_IsBuffer (mixSDL_Object bufobj);
void mixSDL_GetBufferi (mixSDL_Object bufobj, mixSDL_BufferProp pname,
		mixSDL_Object *value);
void mixSDL_BufferData (mixSDL_Object bufobj, uint32 format, void* data,
		uint32 size, uint32 freq);

void mixSDL_ConvertBuffer (uint32 srcfmt, void* srcdata, uint32 srcsize,
		uint32 srcfreq, uint32 dstfmt, void* dstdata,
		uint32 dstsize, uint32 dstfreq);

/*************************************************
 *  Internals
 */

/* private conversion info types */
typedef enum
{
	mixConvNone = 0,
	mixConvStereoUp = 1,
	mixConvStereoDown = 2,
	mixConvSizeUp = 4,
	mixConvSizeDown = 8

} mixSDL_ConvFlags;

typedef struct
{
	uint32 srcfmt;
	void *srcdata;
	uint32 srcsize;
	uint32 srcfreq;
	uint32 srcbpc; /* bytes/sample for 1 chan */
	uint32 srcchans;
	uint32 srcsamples;
	
	uint32 dstfmt;
	void *dstdata;
	uint32 dstsize;
	uint32 dstfreq;
	uint32 dstbpc; /* bytes/sample for 1 chan */
	uint32 dstchans;
	uint32 dstsamples;

	mixSDL_ConvFlags flags;

} mixSDL_Convertion;

/* Make sure the prop-value type is of suitable size
 * it must be able to store both int and void*
 * Adapted from SDL
 * This will generate "negative subscript or subscript is too large"
 * error during compile, if the actual size of a type is wrong
 */
#define MIX_COMPILE_TIME_ASSERT(name, x) \
	typedef int mixSDL_dummy_##name [(x) * 2 - 1]

MIX_COMPILE_TIME_ASSERT(mixSDL_Object, sizeof(mixSDL_Object) >= sizeof(void*));

#undef MIX_COMPILE_TIME_ASSERT

/* Clipping boundaries */
#define MIX_S16_MAX ((double) SINT16_MAX)
#define MIX_S16_MIN ((double) SINT16_MIN)
#define MIX_S8_MAX  ((double) SINT8_MAX)
#define MIX_S8_MIN  ((double) SINT8_MIN)

/* Channel gain adjustment for clipping reduction */
#define MIX_GAIN_ADJ (0.7f)

#endif /* MIXER_H */
