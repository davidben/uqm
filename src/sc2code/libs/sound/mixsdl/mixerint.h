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
 * Internals
 */

#ifndef MIXERINT_H
#define MIXERINT_H

#include "types.h"

/*************************************************
 *  Internals
 */

/* Conversion info types and funcs */
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

static void mixSDL_ConvertBuffer_internal (mixSDL_Convertion *conv);
static void mixSDL_Resample_frac (mixSDL_Convertion *conv);
static void mixSDL_ResampleFlat (mixSDL_Convertion *conv);
static void mixSDL_ResampleUp_fast (mixSDL_Convertion *conv);
static void mixSDL_ResampleDown_fast (mixSDL_Convertion *conv);

static __inline__ sint32 mixSDL_GetSampleExt (void *src, uint32 bpc);
static __inline__ sint32 mixSDL_GetSampleInt (void *src, uint32 bpc);
static __inline__ void mixSDL_PutSampleInt (void *dst, uint32 bpc,
		sint32 samp);
static __inline__ void mixSDL_PutSampleExt (void *dst, uint32 bpc,
		sint32 samp);

/* Source manipulation */
static void mixSDL_SourceUnqueueAll (mixSDL_Source *src);
static void mixSDL_SourceStop_internal (mixSDL_Source *src);
static void mixSDL_SourceActivate (mixSDL_Source* src);
static void mixSDL_SourceDeactivate (mixSDL_Source* src);


/* Reentrant mutex */
typedef struct
{
	Semaphore sem;
	uint32 thread_id;
	uint32 locks;

} mixSDL_Mutex_info;
typedef mixSDL_Mutex_info *mixSDL_Mutex;

static void mixSDL_InitMutex (mixSDL_Mutex mtx, char* name);
static void mixSDL_TermMutex (mixSDL_Mutex mtx);
static void mixSDL_LockMutex (mixSDL_Mutex mtx);
static void mixSDL_UnlockMutex (mixSDL_Mutex mtx);

/* Clipping boundaries */
#define MIX_S16_MAX ((double) SINT16_MAX)
#define MIX_S16_MIN ((double) SINT16_MIN)
#define MIX_S8_MAX  ((double) SINT8_MAX)
#define MIX_S8_MIN  ((double) SINT8_MIN)

/* Channel gain adjustment for clipping reduction */
#define MIX_GAIN_ADJ (0.8f)

/* Clipping filter boundaries */
#define MIX_UNCLIP_AREA     30 /* percent */
#define MIX_UNCLIP_S16_AREA ((sint32) SINT16_MAX * MIX_UNCLIP_AREA / 100)
#define MIX_UNCLIP_S16_MAX  ((sint32) SINT16_MAX - MIX_UNCLIP_S16_AREA)
#define MIX_UNCLIP_S16_MIN  ((sint32) SINT16_MIN + MIX_UNCLIP_S16_AREA)
#define MIX_UNCLIP_S8_AREA  ((sint32) SINT8_MAX * MIX_UNCLIP_AREA / 100)
#define MIX_UNCLIP_S8_MAX   ((sint32) SINT8_MAX - MIX_UNCLIP_S8_AREA)
#define MIX_UNCLIP_S8_MIN   ((sint32) SINT8_MIN + MIX_UNCLIP_S8_AREA)

/* The Mixer */
static void mixSDL_mix_channels (void *userdata, uint8 *stream,
		sint32 len);
static void mixSDL_UnclipWorkBuffer (sint32 *data, sint32 *end_data,
		uint32 step);
static void mixSDL_mix_lowq (uint8 *stream, sint32 len);
static __inline__ bool mixSDL_SourceGetNextSample (mixSDL_Source *src,
		sint32* samp);

#endif /* MIXERINT_H */
