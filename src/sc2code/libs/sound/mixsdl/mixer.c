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

/* Simple mixer for use with SDL_audio.
 * some initialization code is borrowed
 * from SDL_mixer
 */

#ifdef SOUNDMODULE_MIXSDL

#include "starcon.h"
#include "mixer.h"
#include "libs/misc.h"
#include "SDL.h"
#include "SDL_types.h"
#include "SDL_thread.h"
#include "SDL_byteorder.h"
#include "SDL_audio.h"
#include "SDL_version.h"

#define DEBUG

static uint32 audio_opened = 0;
static SDL_AudioSpec mixer_spec;
static uint32 last_error = MIX_NO_ERROR;
static uint32 mixer_format; /* format in mixer enumeration */
static uint32 mixer_chansize;
static uint32 mixer_sampsize;
static sint32 mixer_silence;
static uint32 mixer_freq;

/* this is purely private */
/* Reentrant mutex */
typedef struct
{
	Semaphore sem;
	uint32 thread_id;
	uint32 locks;

} mixSDL_Mutex_info;
typedef mixSDL_Mutex_info *mixSDL_Mutex;

mixSDL_Mutex_info mixer_mutexes[3];
/* when locking more than one mutex
 * you must lock them in this order
 */
static mixSDL_Mutex src_mutex = mixer_mutexes + 0;
static mixSDL_Mutex buf_mutex = mixer_mutexes + 1;
static mixSDL_Mutex act_mutex = mixer_mutexes + 2;

#define MAX_SOURCES 8
mixSDL_Source *active_sources[MAX_SOURCES];

static void mixSDL_mix_channels (void *userdata, uint8 *stream,
		sint32 len);
static void mixSDL_ConvertBuffer_internal (mixSDL_Convertion *conv);
static void mixSDL_Resample_frac (mixSDL_Convertion *conv);
static void mixSDL_ResampleFlat (mixSDL_Convertion *conv);
static void mixSDL_ResampleUp_fast (mixSDL_Convertion *conv);
static void mixSDL_ResampleDown_fast (mixSDL_Convertion *conv);
static void mixSDL_SourceUnqueueAll (mixSDL_Source *src);
static void mixSDL_SourceStop_internal (mixSDL_Source *src);
static void mixSDL_SourceActivate (mixSDL_Source* src);
static void mixSDL_SourceDeactivate (mixSDL_Source* src);

static __inline__ bool mixSDL_SourceGetNextSample (mixSDL_Source *src,
		sint32* samp);

static __inline__ sint32 mixSDL_GetSampleExt (void *src, uint32 bpc);
static __inline__ sint32 mixSDL_GetSampleInt (void *src, uint32 bpc);
static __inline__ void mixSDL_PutSampleInt (void *dst, uint32 bpc,
		sint32 samp);
static __inline__ void mixSDL_PutSampleExt (void *dst, uint32 bpc,
		sint32 samp);


/*************************************************
 *  Internals
 */

static void
mixSDL_InitMutex (mixSDL_Mutex mtx, char* name)
{
	mtx->thread_id = 0;
	mtx->sem = CreateSemaphore (1, name);
	mtx->locks = 0;
}

static void
mixSDL_TermMutex (mixSDL_Mutex mtx)
{
	DestroySemaphore (mtx->sem);
	mtx->sem = 0;
	mtx->thread_id = 0;
}

static void
mixSDL_LockMutex (mixSDL_Mutex mtx)
{
	uint32 thread_id = SDL_ThreadID();
	if (mtx->thread_id != thread_id)
	{
		SetSemaphore (mtx->sem);
		mtx->thread_id = thread_id;
	}
	mtx->locks++;
}

static void
mixSDL_UnlockMutex (mixSDL_Mutex mtx)
{
	uint32 thread_id = SDL_ThreadID();
	if (mtx->thread_id != thread_id)
	{
#ifdef DEBUG
		fprintf (stderr, "%8x attempted to unlock the mutex "
				"when it didn't hold it\n", thread_id);
#endif
	}
	else
	{
		mtx->locks--;
		if (!mtx->locks)
		{
			mtx->thread_id = 0;
			ClearSemaphore (mtx->sem);
		}
	}
}

static void
mixSDL_SetError (uint32 error)
{
	last_error = error;
}

/*************************************************
 *  General interface
 */

uint32
mixSDL_GetError (void)
{
	uint32 error = last_error;
	last_error = MIX_NO_ERROR;
	return error;
}

/* Open the mixer with a certain desired audio format */
bool
mixSDL_OpenAudio (uint32 frequency, uint32 format, uint32 chunksize)
{
	SDL_AudioSpec desired;

	/* If the mixer is already opened, increment open count */
	if (audio_opened)
	{
	    ++audio_opened;
	    return true;
	}

	last_error = MIX_NO_ERROR;
	memset (active_sources, 0, sizeof(mixSDL_Source*) * MAX_SOURCES);

	desired.channels = MIX_FORMAT_CHANS (format);
	if (MIX_FORMAT_BPC (format) == 1)
		desired.format = AUDIO_U8;
	else if (MIX_FORMAT_BPC (format) == 2)
		desired.format = AUDIO_S16SYS;
	else
	{
		mixSDL_SetError (MIX_INVALID_VALUE);
		fprintf (stderr, "mixSDL_OpenAudio: invalid format\n");
		return false;
	}

	/* Set the desired format and frequency */
	desired.freq = frequency;
	desired.samples = chunksize;
	desired.callback = mixSDL_mix_channels;
	desired.userdata = NULL;

	/* Accept nearly any audio format */
	if (SDL_OpenAudio (&desired, &mixer_spec) < 0)
	{
		mixSDL_SetError (MIX_SDL_FAILURE);
		return false;
	}

	if (mixer_spec.format == AUDIO_U8)
	{
		mixer_chansize = 1;
		mixer_silence = 128;
	}
	else if (mixer_spec.format == AUDIO_S16SYS)
	{
		mixer_chansize = 2;
		mixer_silence = 0;
	}
	else
		mixer_chansize = 0;

	if (mixer_chansize == 0 ||
			mixer_spec.channels < 1 || mixer_spec.channels > 2)
	{
		mixSDL_SetError (MIX_SDL_FAILURE);
		SDL_CloseAudio ();
		fprintf (stderr, "mixSDL_OpenAudio: unable to aquire "
				"desired format\n");
		return false;
	}

	mixer_format = MIX_FORMAT_MAKE (
			mixer_chansize, mixer_spec.channels);
	mixer_sampsize = mixer_chansize * mixer_spec.channels;
	mixer_freq = mixer_spec.freq;

	mixSDL_InitMutex (src_mutex, "mixSDL_SourceMutex");
	mixSDL_InitMutex (buf_mutex, "mixSDL_BufferMutex");
	mixSDL_InitMutex (act_mutex, "mixSDL_ActiveMutex");

	audio_opened = 1;
	SDL_PauseAudio (0);

	return true;
}

/* Close the mixer, halting all playing audio */
void
mixSDL_CloseAudio (void)
{
	if (audio_opened)
	{
		if (audio_opened == 1)
		{
			SDL_CloseAudio();

			mixSDL_TermMutex (src_mutex);
			mixSDL_TermMutex (buf_mutex);
			mixSDL_TermMutex (act_mutex);
		}
		--audio_opened;
	}
}

/* Return the actual mixer parameters */
bool
mixSDL_QuerySpec (uint32 *frequency, uint32 *format, uint32 *channels)
{
	if (!audio_opened)
	{
#ifdef DEBUG
		fprintf (stderr, "mixSDL_QuerySpec() called when audio closed\n");
#endif
		return false;
	}

	if (frequency)
		*frequency = mixer_spec.freq;
	if (format)
		*format = mixer_spec.format;
	if (channels)
		*channels = mixer_spec.channels;

	return true;
}


/*************************************************
 *  Sources interface
 */

/* generate n sources */
void
mixSDL_GenSources (uint32 n, mixSDL_Object *psrcobj)
{
	if (n == 0)
		return; /* do nothing per OpenAL */

	if (!psrcobj)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_GenSources() called with null ptr\n");
#endif
		return;
	}
	for (; n; n--, psrcobj++)
	{
		mixSDL_Source *src;

		src = (mixSDL_Source *) HMalloc (sizeof (mixSDL_Source));
		src->magic = mixSDL_srcMagic;
		src->locked = false;
		src->state = MIX_INITIAL;
		src->looping = false;
		src->gain = MIX_GAIN_ADJ;
		src->cqueued = 0;
		src->cprocessed = 0;
		src->firstqueued = 0;
		src->nextqueued = 0;
		src->lastqueued = 0;
		src->curbufofs = 0;

		*psrcobj = (mixSDL_Object) src;
	}
}

/* delete n sources */
void
mixSDL_DeleteSources (uint32 n, mixSDL_Object *psrcobj)
{
	uint32 i;
	mixSDL_Object *pcurobj;

	if (n == 0)
		return; /* do nothing per OpenAL */

	if (!psrcobj)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_DeleteSources() called with null ptr\n");
#endif
		return;
	}

	mixSDL_LockMutex (src_mutex);

	/* check to make sure we can delete all sources */
	for (i = n, pcurobj = psrcobj; i && pcurobj; i--, pcurobj++)
	{
		mixSDL_Source *src = (mixSDL_Source *) *pcurobj;

		if (!src)
			continue;

		if (src->magic != mixSDL_srcMagic)
			break;
	}

	if (i)
	{	/* some source failed */
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_DeleteSources(): not a source\n");
#endif
	}
	else
	{	/* all sources checked out */
		for (; n; n--, psrcobj++)
		{
			mixSDL_Source *src = (mixSDL_Source *) *psrcobj;

			if (!src)
				continue;

			/* stopping should not be necessary
			 * under ideal circumstances
			 */
			if (src->state != MIX_INITIAL)
				mixSDL_SourceStop_internal (src);

			/* unqueueing should not be necessary
			 * under ideal circumstances
			 */
			mixSDL_SourceUnqueueAll (src);
			HFree (src);
			*psrcobj = 0;
		}
	}

	mixSDL_UnlockMutex (src_mutex);
}

/* check if really is a source */
bool
mixSDL_IsSource (mixSDL_Object srcobj)
{
	mixSDL_Source *src = (mixSDL_Source *) srcobj;
	bool ret;

	if (!src)
		return false;

	mixSDL_LockMutex (src_mutex);
	ret = src->magic == mixSDL_srcMagic;
	mixSDL_UnlockMutex (src_mutex);

	return ret;
}

/* set source integer property */
void
mixSDL_Sourcei (mixSDL_Object srcobj, mixSDL_SourceProp pname,
		mixSDL_IntVal value)
{
	mixSDL_Source *src = (mixSDL_Source *) srcobj;

	if (!src)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_Sourcei() called with null source\n");
#endif
		return;
	}

	mixSDL_LockMutex (src_mutex);

	if (src->magic != mixSDL_srcMagic)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_Sourcei(): not a source\n");
#endif
	}
	else
	{
		switch (pname)
		{
		case MIX_LOOPING:
			src->looping = value;
			break;
		case MIX_BUFFER:
			if (src->cqueued > 0)
				mixSDL_SourceUnqueueAll (src);

			src->firstqueued = (mixSDL_Buffer *) value;
			src->nextqueued = src->firstqueued;
			src->lastqueued = src->nextqueued;
			if (src->lastqueued)
				src->lastqueued->next = 0;
			break;
		case MIX_SOURCE_STATE:
#ifdef DEBUG
			fprintf (stderr, "mixSDL_Sourcei() called "
					"with MIX_SOURCE_STATE. call ignored\n");
#endif
			break;
		default:
			mixSDL_SetError (MIX_INVALID_ENUM);
			fprintf (stderr, "mixSDL_Sourcei() called "
					"with unsupported property %u\n", pname);
		}
	}

	mixSDL_UnlockMutex (src_mutex);
}

/* set source float property */
void
mixSDL_Sourcef (mixSDL_Object srcobj, mixSDL_SourceProp pname, float value)
{
	mixSDL_Source *src = (mixSDL_Source *) srcobj;
	
	if (!src)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_Sourcef() called with null source\n");
#endif
		return;
	}

	mixSDL_LockMutex (src_mutex);

	if (src->magic != mixSDL_srcMagic)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_Sourcef(): not a source\n");
#endif
	}
	else
	{
		switch (pname)
		{
		case MIX_GAIN:
			src->gain = value * MIX_GAIN_ADJ;
			break;
		default:
			fprintf (stderr, "mixSDL_Sourcei() called "
				"with unsupported property %u\n", pname);
		}
	}

	mixSDL_UnlockMutex (src_mutex);
}

/* get source integer property */
void
mixSDL_GetSourcei (mixSDL_Object srcobj, mixSDL_SourceProp pname,
		mixSDL_IntVal *value)
{
	mixSDL_Source *src = (mixSDL_Source *) srcobj;

	if (!src || !value)
	{
		mixSDL_SetError (src ? MIX_INVALID_VALUE : MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_GetSourcei() called with null param\n");
#endif
		return;
	}

	mixSDL_LockMutex (src_mutex);

	if (src->magic != mixSDL_srcMagic)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_GetSourcei(): not a source\n");
#endif
	}
	else
	{
		switch (pname)
		{
		case MIX_LOOPING:
			*value = src->looping;
			break;
		case MIX_BUFFER:
			*value = (mixSDL_IntVal) src->firstqueued;
			break;
		case MIX_SOURCE_STATE:
			*value = src->state;
			break;
		case MIX_BUFFERS_QUEUED:
			*value = src->cqueued;
			break;
		case MIX_BUFFERS_PROCESSED:
			*value = src->cprocessed;
			break;
		default:
			mixSDL_SetError (MIX_INVALID_ENUM);
			fprintf (stderr, "mixSDL_GetSourcei() called "
					"with unsupported property %u\n", pname);
		}
	}

	mixSDL_UnlockMutex (src_mutex);
}

/* get source float property */
void
mixSDL_GetSourcef (mixSDL_Object srcobj, mixSDL_SourceProp pname,
		float *value)
{
	mixSDL_Source *src = (mixSDL_Source *) srcobj;

	if (!src || !value)
	{
		mixSDL_SetError (src ? MIX_INVALID_VALUE : MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_GetSourcef() called with null param\n");
#endif
		return;
	}

	mixSDL_LockMutex (src_mutex);

	if (src->magic != mixSDL_srcMagic)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_GetSourcef(): not a source\n");
#endif
	}
	else
	{
		switch (pname)
		{
		case MIX_GAIN:
			*value = src->gain / MIX_GAIN_ADJ;
			break;
		default:
			fprintf (stderr, "mixSDL_GetSourcef() called "
					"with unsupported property %u\n", pname);
		}
	}

	mixSDL_UnlockMutex (src_mutex);
}

/* start the source; add it to active array */
void
mixSDL_SourcePlay (mixSDL_Object srcobj)
{
	mixSDL_Source *src = (mixSDL_Source *) srcobj;

	if (!src)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_SourcePlay() called with null source\n");
#endif
		return;
	}

	mixSDL_LockMutex (src_mutex);

	if (src->magic != mixSDL_srcMagic)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_SourcePlay(): not a source\n");
#endif
	}
	else /* should make the source active */
	{
		if (src->state < MIX_PLAYING)
			mixSDL_SourceActivate (src);
		src->state = MIX_PLAYING;
	}

	mixSDL_UnlockMutex (src_mutex);
}

/* stop the source; remove it from active array and requeue buffers */
void
mixSDL_SourceRewind (mixSDL_Object srcobj)
{
	mixSDL_Source *src = (mixSDL_Source *) srcobj;

	if (!src)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_SourceRewind() called with null source\n");
#endif
		return;
	}

	mixSDL_LockMutex (src_mutex);

	if (src->magic != mixSDL_srcMagic)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_SourcePlay(): not a source\n");
#endif
	}
	else
	{
		/* should change the processed buffers to queued */
		mixSDL_Buffer *buf;

		if (src->state >= MIX_PLAYING)
			mixSDL_SourceDeactivate (src);

		mixSDL_LockMutex (buf_mutex);

		for (buf = src->firstqueued;
				buf && buf->state != MIX_BUF_QUEUED;
				buf = buf->next)
		{
			buf->state = MIX_BUF_QUEUED;
		}

		mixSDL_UnlockMutex (buf_mutex);

		src->curbufofs = 0;
		src->cprocessed = 0;
		src->state = MIX_INITIAL;
	}

	mixSDL_UnlockMutex (src_mutex);
}

/* pause the source; keep in active array */
void
mixSDL_SourcePause (mixSDL_Object srcobj)
{
	mixSDL_Source *src = (mixSDL_Source *) srcobj;

	if (!src)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_SourcePause() called with null source\n");
#endif
		return;
	}

	mixSDL_LockMutex (src_mutex);

	if (src->magic != mixSDL_srcMagic)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_SourcePause(): not a source\n");
#endif
	}
	else /* should keep all buffers and offsets */
	{
		if (src->state < MIX_PLAYING)
			mixSDL_SourceActivate (src);
		src->state = MIX_PAUSED;
	}

	mixSDL_UnlockMutex (src_mutex);
}

/* stop the source; remove it from active array
 * and unqueue 'queued' buffers
 */
void
mixSDL_SourceStop (mixSDL_Object srcobj)
{
	mixSDL_Source *src = (mixSDL_Source *) srcobj;

	if (!src)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_SourceStop() called with null source\n");
#endif
		return;
	}

	mixSDL_LockMutex (src_mutex);

	if (src->magic != mixSDL_srcMagic)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_SourceStop(): not a source\n");
#endif
	}
	else /* should remove queued buffers */
	{
		if (src->state >= MIX_PLAYING)
			mixSDL_SourceDeactivate (src);
		mixSDL_SourceStop_internal (src);
		src->state = MIX_STOPPED;
	}

	mixSDL_UnlockMutex (src_mutex);
}

/* queue buffers on the source */
void
mixSDL_SourceQueueBuffers (mixSDL_Object srcobj, uint32 n,
		mixSDL_Object* pbufobj)
{
	uint32 i;
	mixSDL_Object* pobj;
	mixSDL_Source *src = (mixSDL_Source *) srcobj;

	if (!src || !pbufobj)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_SourceQueueBuffers() called "
				"with null param\n");
#endif
		return;
	}

	mixSDL_LockMutex (buf_mutex);
	/* check to make sure we can safely queue all buffers */
	for (i = n, pobj = pbufobj; i; i--, pobj++)
	{
		mixSDL_Buffer *buf = (mixSDL_Buffer *) *pobj;
		if (!buf || buf->magic != mixSDL_bufMagic)
		{
			mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
			fprintf (stderr, "mixSDL_SourceQueueBuffers(): not a buffer\n");
#endif
			break;
		}
		else if (buf->locked)
		{
			mixSDL_SetError (MIX_INVALID_OPERATION);
#ifdef DEBUG
			fprintf (stderr, "mixSDL_SourceQueueBuffers(): "
					"locked buffer attempted\n");
#endif
			break;
		}
		else if (buf->state != MIX_BUF_FILLED)
		{
			mixSDL_SetError (MIX_INVALID_OPERATION);
#ifdef DEBUG
			fprintf (stderr, "mixSDL_SourceQueueBuffers(): "
					"invalid buffer attempted\n");
#endif
			break;
		}
	}
	mixSDL_UnlockMutex (buf_mutex);

	if (i == 0)
	{	/* all buffers checked out */
		mixSDL_LockMutex (src_mutex);
		mixSDL_LockMutex (buf_mutex);

		if (src->magic != mixSDL_srcMagic)
		{
			mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
			fprintf (stderr, "mixSDL_SourceQueueBuffers(): not a source\n");
#endif
		}
		else
		{
			for (i = n, pobj = pbufobj; i; i--, pobj++)
			{
				mixSDL_Buffer *buf = (mixSDL_Buffer *) *pobj;

				/* add buffer to the chain */
				if (src->lastqueued)
					src->lastqueued->next = buf;
				src->lastqueued = buf;

				if (!src->firstqueued)
				{
					src->firstqueued = buf;
					src->nextqueued = buf;
				}
				src->cqueued++;
				buf->state = MIX_BUF_QUEUED;
			}
		}

		mixSDL_UnlockMutex (buf_mutex);
		mixSDL_UnlockMutex (src_mutex);
	}
}

/* unqueue buffers from the source */
void
mixSDL_SourceUnqueueBuffers (mixSDL_Object srcobj, uint32 n,
		mixSDL_Object* pbufobj)
{
	uint32 i;
	mixSDL_Source *src = (mixSDL_Source *) srcobj;
	mixSDL_Buffer *curbuf = 0;

	if (!src || !pbufobj)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_SourceUnqueueBuffers() called "
				"with null source\n");
#endif
		return;
	}

	mixSDL_LockMutex (src_mutex);

	if (src->magic != mixSDL_srcMagic)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_SourceUnqueueBuffers(): not a source\n");
#endif
	}
	else if (n > src->cqueued)
	{
		mixSDL_SetError (MIX_INVALID_OPERATION);
	}
	else
	{
		mixSDL_LockMutex (buf_mutex);

		/* check to make sure we can unqueue all buffers */
		for (i = n, curbuf = src->firstqueued;
				i && curbuf && curbuf->state != MIX_BUF_PLAYING;
				i--, curbuf = curbuf->next)
			;

		if (i)
		{
			mixSDL_SetError (MIX_INVALID_OPERATION);
#ifdef DEBUG
			fprintf (stderr, "mixSDL_SourceUnqueueBuffers(): "
					"active buffer attempted\n");
#endif
		}
		else
		{	/* all buffers checked out */
			for (i = n; i; i--, pbufobj++)
			{
				mixSDL_Buffer *buf = src->firstqueued;

				/* remove buffer from the chain */
				if (src->nextqueued == buf)
					src->nextqueued = buf->next;
				if (src->lastqueued == buf)
					src->lastqueued = 0;
				src->firstqueued = buf->next;
				src->cqueued--;

				if (buf->state == MIX_BUF_PROCESSED)
					src->cprocessed--;
				
				buf->state = MIX_BUF_FILLED;
				buf->next = 0;
				*pbufobj = (mixSDL_Object) buf;
			}
		}

		mixSDL_UnlockMutex (buf_mutex);
	}

	mixSDL_UnlockMutex (src_mutex);
}

/*************************************************
 *  Sources internals
 */

void
mixSDL_SourceUnqueueAll (mixSDL_Source *src)
{
	mixSDL_Buffer *buf;
	mixSDL_Buffer *nextbuf;

	if (!src)
	{
#ifdef DEBUG
		fprintf (stderr, "mixSDL_SourceUnqueueAll() called "
				"with null source\n");
#endif
		return;
	}

	mixSDL_LockMutex (buf_mutex);

	for (buf = src->firstqueued; buf; buf = nextbuf)
	{
#ifdef DEBUG
		if (buf->state == MIX_BUF_PLAYING)
		{
			fprintf (stderr, "mixSDL_SourceUnqueueAll(): "
					"attempted on active buffer\n");
		}
#endif
		nextbuf = buf->next;
		buf->state = MIX_BUF_FILLED;
		buf->next = 0;
	}

	mixSDL_UnlockMutex (buf_mutex);

	src->firstqueued = 0;
	src->nextqueued = 0;
	src->lastqueued = 0;
	src->cqueued = 0;
	src->cprocessed = 0;
	src->curbufofs = 0;
}

/* add the source to the active array */
void
mixSDL_SourceActivate (mixSDL_Source* src)
{
	uint32 i;

	mixSDL_LockMutex (act_mutex);

#ifdef DEBUG
	/* check active sources, see if this source is there already */
	for (i = 0; i < MAX_SOURCES && active_sources[i] != src; i++)
		;
	if (i < MAX_SOURCES)
	{	/* source found */
		fprintf (stderr, "mixSDL_SourceActivate(): "
				"source already active in slot %u\n", i);
		mixSDL_UnlockMutex (act_mutex);
		return;
	}
#endif

	/* find an empty slot */
	for (i = 0; i < MAX_SOURCES && active_sources[i] != 0; i++)
		;
	if (i < MAX_SOURCES)
	{	/* slot found */
		active_sources[i] = src;
	}
#ifdef DEBUG
	else
	{
		fprintf (stderr, "mixSDL_SourceActivate(): "
				"no more slots available (max=%d)\n", MAX_SOURCES);
	}
#endif

	mixSDL_UnlockMutex (act_mutex);
}

/* remove the source from the active array */
void
mixSDL_SourceDeactivate (mixSDL_Source* src)
{
	uint32 i;

	mixSDL_LockMutex (act_mutex);

	/* check active sources, see if this source is there */
	for (i = 0; i < MAX_SOURCES && active_sources[i] != src; i++)
		;
	if (i < MAX_SOURCES)
	{	/* source found */
		active_sources[i] = 0;
	}
#ifdef DEBUG
	else
	{	/* source not found */
		fprintf (stderr, "mixSDL_SourceDeactivate(): source not active\n");
	}
#endif

	mixSDL_UnlockMutex (act_mutex);
}

void
mixSDL_SourceStop_internal (mixSDL_Source *src)
{
	mixSDL_Buffer *buf;
	mixSDL_Buffer *nextbuf;

	if (!src->firstqueued)
		return;

#ifdef DEBUG
	/* assert the source buffers state */
	if (!src->lastqueued)
	{	
		fprintf (stderr, "mixSDL_SourceStop_internal(): "
				"desynced source state\n");
		return;
	}
#endif

	mixSDL_LockMutex (buf_mutex);

	/* find last 'processed' buffer */
	for (buf = src->firstqueued;
			buf && buf->next && buf->next != src->nextqueued;
			buf = buf->next)
		;
	src->lastqueued = buf;
	if (buf)
		buf->next = 0; /* break the chain */

	/* unqueue all 'queued' buffers */
	for (buf = src->nextqueued; buf; buf = nextbuf)
	{
		nextbuf = buf->next;
		buf->state = MIX_BUF_FILLED;
		buf->next = 0;
		src->cqueued--;
	}

	if (src->cqueued == 0)
	{	/* all buffers were removed */
		src->firstqueued = 0;
		src->lastqueued = 0;
	}
	src->nextqueued = 0;
	src->curbufofs = 0;

	mixSDL_UnlockMutex (buf_mutex);
}

/* get the sample next in queue in internal format */
__inline__ bool
mixSDL_SourceGetNextSample (mixSDL_Source *src, sint32* psamp)
{
	while (src->nextqueued)
	{
		mixSDL_Buffer *buf = src->nextqueued;
		uint8* data;
		double samp;

		if (!buf->data || buf->size < mixer_sampsize)
		{
			/* buffer invalid, go next */
			buf->state = MIX_BUF_PROCESSED;
			src->curbufofs = 0;
			src->nextqueued = src->nextqueued->next;
			src->cprocessed++;
			continue;
		}

		data = buf->data + src->curbufofs;
		samp = mixSDL_GetSampleInt (data, mixer_chansize);
		src->curbufofs += mixer_chansize;
		samp *= src->gain;

		if (mixer_chansize == 2)
		{
			/* check S16 clipping */
			if (samp > MIX_S16_MAX)
				*psamp = SINT16_MAX;
			else if (samp < MIX_S16_MIN)
				*psamp = SINT16_MIN;
			else
				*psamp = (sint32)samp;
		}
		else
		{
			/* check S8 clipping */
			if (samp > MIX_S8_MAX)
				*psamp = SINT8_MAX;
			else if (samp < MIX_S8_MIN)
				*psamp = SINT8_MIN;
			else
				*psamp = (sint32)samp;
		}

		if (src->curbufofs >= buf->size)
		{	
			/* buffer exhausted, go next */
			buf->state = MIX_BUF_PROCESSED;
			src->curbufofs = 0;
			src->nextqueued = src->nextqueued->next;
			src->cprocessed++;
		}
		else
		{
			buf->state = MIX_BUF_PLAYING;
		}
		
		return true;
	}
	
	/* no more playable buffers */
	if (src->state >= MIX_PLAYING)
		mixSDL_SourceDeactivate (src);

	src->state = MIX_STOPPED;

	return false;
}

/*************************************************
 *  Buffers interface
 */

/* generate n buffer objects */
void
mixSDL_GenBuffers (uint32 n, mixSDL_Object *pbufobj)
{
	if (n == 0)
		return; /* do nothing per OpenAL */

	if (!pbufobj)
	{
		mixSDL_SetError (MIX_INVALID_VALUE);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_GenBuffers() called with null ptr\n");
#endif
		return;
	}
	for (; n; n--, pbufobj++)
	{
		mixSDL_Buffer *buf;

		buf = (mixSDL_Buffer *) HMalloc (sizeof (mixSDL_Buffer));
		buf->magic = mixSDL_bufMagic;
		buf->locked = false;
		buf->state = MIX_BUF_INITIAL;
		buf->data = 0;
		buf->size = 0;
		buf->next = 0;
		buf->orgdata = 0;
		buf->orgfreq = 0;
		buf->orgsize = 0;
		buf->orgchannels = 0;
		buf->orgchansize = 0;

		*pbufobj = (mixSDL_Object) buf;
	}
}

/* delete n buffer objects */
void
mixSDL_DeleteBuffers (uint32 n, mixSDL_Object *pbufobj)
{
	uint32 i;
	mixSDL_Object *pcurobj;

	if (n == 0)
		return; /* do nothing per OpenAL */

	if (!pbufobj)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_DeleteBuffers() called with null ptr\n");
#endif
		return;
	}
	
	mixSDL_LockMutex (buf_mutex);

	/* check to make sure we can delete all buffers */
	for (i = n, pcurobj = pbufobj; i && pcurobj; i--, pcurobj++)
	{
		mixSDL_Buffer *buf = (mixSDL_Buffer *) *pcurobj;

		if (!buf)
			continue;

		if (buf->magic != mixSDL_bufMagic)
		{
			mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
			fprintf (stderr, "mixSDL_DeleteBuffers(): not a buffer\n");
#endif
			break;
		}
		else if (buf->locked)
		{
			mixSDL_SetError (MIX_INVALID_OPERATION);
#ifdef DEBUG
			fprintf (stderr, "mixSDL_DeleteBuffers(): locked buffer\n");
#endif
			break;
		}
		else if (buf->state >= MIX_BUF_QUEUED)
		{
			mixSDL_SetError (MIX_INVALID_OPERATION);
#ifdef DEBUG
			fprintf (stderr, "mixSDL_DeleteBuffers(): "
					"attempted on queued/active buffer\n");
#endif
			break;
		}
	}

	if (i == 0)
	{
		/* all buffers check out */
		for (; n; n--, pbufobj++)
		{
			mixSDL_Buffer *buf = (mixSDL_Buffer *) *pbufobj;

			if (!buf)
				continue;

			if (buf->data)
				HFree (buf->data);
			HFree (buf);

			*pbufobj = 0;
		}
	}
	mixSDL_UnlockMutex (buf_mutex);
}

/* check if really a buffer object */
bool
mixSDL_IsBuffer (mixSDL_Object bufobj)
{
	mixSDL_Buffer *buf = (mixSDL_Buffer *) bufobj;
	bool ret;

	if (!buf)
		return false;

	mixSDL_LockMutex (buf_mutex);
	ret = buf->magic == mixSDL_bufMagic;
	mixSDL_UnlockMutex (buf_mutex);

	return ret;
}

/* get buffer property */
void
mixSDL_GetBufferi (mixSDL_Object bufobj, mixSDL_BufferProp pname,
		mixSDL_IntVal *value)
{
	mixSDL_Buffer *buf = (mixSDL_Buffer *) bufobj;

	if (!buf || !value)
	{
		mixSDL_SetError (buf ? MIX_INVALID_VALUE : MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_GetBufferi() called with null param\n");
#endif
		return;
	}

	mixSDL_LockMutex (buf_mutex);
	
	if (buf->locked)
	{
		mixSDL_UnlockMutex (buf_mutex);
		mixSDL_SetError (MIX_INVALID_OPERATION);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_GetBufferi() called with locked buffer\n");
#endif
		return;
	}

	if (buf->magic != mixSDL_bufMagic)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_GetBufferi(): not a buffer\n");
#endif
	}
	else
	{
		/* Return original buffer values
		 */
		switch (pname)
		{
		case MIX_FREQUENCY:
			*value = buf->orgfreq;
			break;
		case MIX_BITS:
			*value = buf->orgchansize << 3;
			break;
		case MIX_CHANNELS:
			*value = buf->orgchannels;
			break;
		case MIX_SIZE:
			*value = buf->orgsize;
			break;
		case MIX_DATA:
			*value = (mixSDL_IntVal) buf->orgdata;
			break;
		default:
			mixSDL_SetError (MIX_INVALID_ENUM);
			fprintf (stderr, "mixSDL_GetBufferi() called "
					"with invalid property %u\n", pname);
		}
	}

	mixSDL_UnlockMutex (buf_mutex);
}

/* fill buffer with external data */
void
mixSDL_BufferData (mixSDL_Object bufobj, uint32 format, void* data,
		uint32 size, uint32 freq)
{
	mixSDL_Buffer *buf = (mixSDL_Buffer *) bufobj;
	mixSDL_Convertion conv;
	uint32 dstsize;

	if (!buf || !data || !size)
	{
		mixSDL_SetError (buf ? MIX_INVALID_VALUE : MIX_INVALID_NAME);
#ifdef DEBUG
//		fprintf (stderr, "mixSDL_BufferData() called with bad param\n");
#endif
		return;
	}

	mixSDL_LockMutex (buf_mutex);
	
	if (buf->locked)
	{
		mixSDL_UnlockMutex (buf_mutex);
		mixSDL_SetError (MIX_INVALID_OPERATION);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_BufferData() called "
				"with locked buffer\n");
#endif
		return;
	}

	if (buf->magic != mixSDL_bufMagic)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_BufferData(): not a buffer\n");
#endif
	}
	else if (buf->state > MIX_BUF_FILLED)
	{
		mixSDL_SetError (MIX_INVALID_OPERATION);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_BufferData() attempted "
				"on in-use buffer\n");
#endif
	}
	else
	{
		if (buf->data)
			HFree (buf->data);
		buf->data = 0;
		buf->size = 0;
		/* Store original buffer values for OpenAL compatibility */
		buf->orgdata = data;
		buf->orgfreq = freq;
		buf->orgsize = size;
		buf->orgchannels = MIX_FORMAT_CHANS (format);
		buf->orgchansize = MIX_FORMAT_BPC (format);

		conv.srcsamples = size / MIX_FORMAT_SAMPSIZE (format);
		conv.dstsamples = (uint32)
				((double)conv.srcsamples * mixer_freq / freq);
				
		if (conv.dstsamples >
				UINT32_MAX / MIX_FORMAT_SAMPSIZE (mixer_format))
		{
			mixSDL_SetError (MIX_INVALID_VALUE);
		}
		else
		{
			dstsize = conv.dstsamples *
					MIX_FORMAT_SAMPSIZE (mixer_format);

			buf->size = dstsize;
			buf->data = HMalloc (dstsize);
			
			if (format == mixer_format && freq == mixer_freq)
			{
				/* format identical to internal */
				buf->locked = true;
				mixSDL_UnlockMutex (buf_mutex);

				memcpy (buf->data, data, size);
				if (MIX_FORMAT_SAMPSIZE (mixer_format) == 1)
				{
					/* convert buffer to S8 format internally */
					uint8* dst;
					for (dst = buf->data; dstsize; dstsize--, dst++)
						*dst ^= 0x80;
				}

				mixSDL_LockMutex (buf_mutex);
				buf->locked = false;
			}
			else 
			{
				/* needs convertion */
				conv.srcfmt = format;
				conv.srcdata = data;
				conv.srcsize = size;
				conv.srcfreq = freq;
				conv.dstfmt = mixer_format;
				conv.dstdata = buf->data;
				conv.dstsize = dstsize;
				conv.dstfreq = mixer_freq;

				buf->locked = true;
				mixSDL_UnlockMutex (buf_mutex);

				mixSDL_ConvertBuffer_internal (&conv);
				
				mixSDL_LockMutex (buf_mutex);
				buf->locked = false;
			}

			buf->state = MIX_BUF_FILLED;
		}
	}

	mixSDL_UnlockMutex (buf_mutex);
}

/* convert a user buffer from one format to another */
void
mixSDL_ConvertBuffer (uint32 srcfmt, void* srcdata, uint32 srcsize,
		uint32 srcfreq, uint32 dstfmt, void* dstdata,
		uint32 dstfreq, uint32 dstsize)
{
	mixSDL_Convertion conv;
	uint32 reqsize;

	if (!srcdata || !dstdata)
	{
		mixSDL_SetError (MIX_INVALID_VALUE);
		fprintf (stderr, "mixSDL_ConvertBuffer(): called "
				"with null buffer\n");
		return;
	}


	conv.srcsamples = srcsize / MIX_FORMAT_SAMPSIZE (srcfmt);
	conv.dstsamples = (uint32)
			((double)conv.srcsamples * dstfreq / srcfreq);
			
	if (conv.dstsamples > UINT32_MAX / MIX_FORMAT_SAMPSIZE (dstfmt))
	{
		mixSDL_SetError (MIX_INVALID_VALUE);
		return;
	}

	reqsize = conv.dstsamples * MIX_FORMAT_SAMPSIZE (mixer_format);

	if (reqsize > dstsize)
	{
		mixSDL_SetError (MIX_INVALID_VALUE);
		fprintf (stderr, "mixSDL_ConvertBuffer(): "
				"destination buffer too small\n");
		return;
	}

	if (srcfmt == dstfmt && srcfreq == dstfreq)
	{
		/* user buffer, no S8->U8 convertion */
		memcpy (dstdata, srcdata, srcsize);
	}
	else
	{
		conv.srcfmt = srcfmt;
		conv.srcdata = srcdata;
		conv.srcsize = srcsize;
		conv.srcfreq = srcfreq;
		conv.dstfmt = dstfmt;
		conv.dstdata = dstdata;
		conv.dstsize = dstsize;
		conv.dstfreq = dstfreq;

		mixSDL_ConvertBuffer_internal (&conv);

		if (conv.dstbpc == 1)
		{
			/* converted buffer is in S8 format
			 * have to make it U8
			 */
			uint8* dst;
			for (dst = conv.dstdata; dstsize; dstsize--, dst++)
				*dst ^= 0x80;
		}
	}
}

/*************************************************
 *  Buffer internals
 */

void
mixSDL_ConvertBuffer_internal (mixSDL_Convertion *conv)
{
	conv->srcbpc = MIX_FORMAT_BPC (conv->srcfmt);
	conv->srcchans = MIX_FORMAT_CHANS (conv->srcfmt);
	conv->dstbpc = MIX_FORMAT_BPC (conv->dstfmt);
	conv->dstchans = MIX_FORMAT_CHANS (conv->dstfmt);

	conv->flags = 0;
	if (conv->srcbpc > conv->dstbpc)
		conv->flags |= mixConvSizeDown;
	else if (conv->srcbpc < conv->dstbpc)
		conv->flags |= mixConvSizeUp;
	if (conv->srcchans > conv->dstchans)
		conv->flags |= mixConvStereoDown;
	else if (conv->srcchans < conv->dstchans)
		conv->flags |= mixConvStereoUp;

	if (conv->srcfreq == conv->dstfreq)
	{
		mixSDL_ResampleFlat (conv);
	}
	else if (conv->dstfreq > conv->srcfreq)
	{
		if (conv->dstfreq % conv->srcfreq == 0)
			mixSDL_ResampleUp_fast (conv);
		else
			mixSDL_Resample_frac (conv);
	}
	else
	{
		if (conv->srcfreq % conv->dstfreq == 0)
			mixSDL_ResampleDown_fast (conv);
		else
			mixSDL_Resample_frac (conv);
	}
}

/*************************************************
 *  Resampling routines
 */

/* get a sample from external buffer
 * in internal format
 */
__inline__ sint32
mixSDL_GetSampleExt (void *src, uint32 bpc)
{
	if (bpc == 2)
		return *(sint16 *)src;
	else
		return (*(uint8 *)src) - 128;
}

/* get a sample from internal buffer */
__inline__ sint32
mixSDL_GetSampleInt (void *src, uint32 bpc)
{
	if (bpc == 2)
		return *(sint16 *)src;
	else
		return *(sint8 *)src;
}

/* put a sample into an external buffer
 * from internal format
 */
__inline__ void
mixSDL_PutSampleExt (void *dst, uint32 bpc, sint32 samp)
{
	if (bpc == 2)
		*(sint16 *)dst = samp;
	else
		*(uint8 *)dst = samp ^ 0x80;
}

/* put a sample into an internal buffer
 * in internal format
 */
__inline__ void
mixSDL_PutSampleInt (void *dst, uint32 bpc, sint32 samp)
{
	if (bpc == 2)
		*(sint16 *)dst = samp;
	else
		*(sint8 *)dst = samp;
}

/* get next sample from external buffer
 * in internal format, while performing
 * convertion if necessary
 */
__inline__ sint32
mixSDL_GetConvSample (uint8 **psrc, uint32 bpc, uint32 flags)
{
	register sint32 samp;
	
	samp = mixSDL_GetSampleExt (*psrc, bpc);
	*psrc += bpc;
	if (flags & mixConvStereoDown)
	{
		/* downmix to mono - average up channels */
		samp = (samp + mixSDL_GetSampleExt (*psrc, bpc)) / 2;
		*psrc += bpc;
	}

	if (flags & mixConvSizeUp)
	{
		/* convert S8 to S16 */
		samp <<= 8;
	}
	else if (flags & mixConvSizeDown)
	{
		/* convert S16 to S8
		 * if arithmetic shift is available to the compiler
		 * it will use it to optimize this
		 */
		samp /= 0x100;
	}

	return samp;
}

/* put next sample into an internal buffer
 * in internal format, while performing
 * convertion if necessary
 */
__inline__ void
mixSDL_PutConvSample (uint8 **pdst, uint32 bpc, uint32 flags, sint32 samp)
{
	mixSDL_PutSampleInt (*pdst, bpc, samp);
	*pdst += bpc;
	if (flags & mixConvStereoUp)
	{
		mixSDL_PutSampleInt (*pdst, bpc, samp);
		*pdst += bpc;
	}
}

/* resampling with fractional rate change */
void
mixSDL_Resample_frac (mixSDL_Convertion *conv)
{
	mixSDL_ConvFlags flags = conv->flags;
	uint8 *src = conv->srcdata;
	uint8 *dst = conv->dstdata;
	uint32 srcsamples = conv->srcsamples;
	uint32 srcbpc = conv->srcbpc;
	uint32 dstbpc = conv->dstbpc;
	double convrate = (double)conv->srcfreq / conv->dstfreq;
	double dstpos = 0;
	uint32 nextsrcpos; /* next position already read */
	sint32 sampl, sampr, nextsampl, nextsampr;
	uint32 srcjump = srcbpc * conv->srcchans;
	bool stereo = conv->srcchans == 2 && !(flags & mixConvStereoDown);
	uint32 i;

	nextsampl = mixSDL_GetConvSample (&src, srcbpc, flags);
	if (stereo)
		nextsampr = mixSDL_GetConvSample (&src, srcbpc, flags);
	else
		nextsampr = 0;
	nextsrcpos = 0;

	for (i = conv->dstsamples; i; i--)
	{
		double delta, intdstpos;
		uint32 srcpos;
		uint32 samp;

		delta = modf (dstpos, &intdstpos);
		srcpos = (uint32) intdstpos;

		if (srcpos >= nextsrcpos)
		{
			if (srcpos == nextsrcpos)
			{
				sampl = nextsampl;
				sampr = nextsampr;
				nextsrcpos++;
			}
			else
			{
				src += srcjump * (srcpos - nextsrcpos - 1);
				sampl = mixSDL_GetConvSample (&src, srcbpc, flags);
				if (stereo)
					sampr = mixSDL_GetConvSample (&src, srcbpc, flags);
				nextsrcpos = srcpos + 1;
			}

			if (nextsrcpos < srcsamples)
			{
				nextsampl = mixSDL_GetConvSample (&src, srcbpc, flags);
				if (stereo)
					nextsampr = mixSDL_GetConvSample (&src, srcbpc, flags);
			}
			else
			{
				nextsampl = nextsampr = 0;
			}
		}

		/* iterpolate */
		samp = sampl + (sint32) (delta * (nextsampl - sampl));
		mixSDL_PutConvSample (&dst, dstbpc, flags, samp);
		if (stereo)
		{
			samp = sampr + (sint32) (delta * (nextsampr - sampr));
			mixSDL_PutConvSample (&dst, dstbpc, flags, sampr);
		}
		
		dstpos += convrate;
	}
}

/* resampling with respect to sample size only */
void
mixSDL_ResampleFlat (mixSDL_Convertion *conv)
{
	mixSDL_ConvFlags flags = conv->flags;
	uint8 *src = conv->srcdata;
	uint8 *dst = conv->dstdata;
	uint32 srcbpc = conv->srcbpc;
	uint32 dstbpc = conv->dstbpc;
	uint32 i;

	for (i = conv->srcsamples; i; i--)
	{
		register sint32 samp;

		samp = mixSDL_GetConvSample (&src, srcbpc, flags);
		mixSDL_PutConvSample (&dst, dstbpc, flags, samp);
	}
}

/* integer-periodic resampling with rate increase */
void
mixSDL_ResampleUp_fast (mixSDL_Convertion *conv)
{
	mixSDL_ConvFlags flags = conv->flags;
	uint8 *src = conv->srcdata;
	uint8 *dst = conv->dstdata;
	uint32 srcbpc = conv->srcbpc;
	uint32 dstbpc = conv->dstbpc;
	uint32 convrate = conv->dstfreq / conv->srcfreq;
	sint32 nextsampl, nextsampr;
	bool stereo = conv->srcchans == 2 && !(flags & mixConvStereoDown);
	uint32 i;
	
	/* read one sample ahead */
	nextsampl = mixSDL_GetConvSample (&src, srcbpc, flags);
	if (stereo)
		nextsampr = mixSDL_GetConvSample (&src, srcbpc, flags);
	else
		nextsampr = 0;

	for (i = conv->srcsamples; i; i--)
	{
		sint32 sampl = nextsampl;
		sint32 sampr = nextsampr;
		sint32 di;
		
		if (i > 1)
		{
			nextsampl = mixSDL_GetConvSample (&src, srcbpc, flags);
			if (stereo)
				nextsampr = mixSDL_GetConvSample (&src, srcbpc, flags);
		}
		else
		{
			nextsampl = nextsampr = 0;
		}
		
		for (di = convrate; di > 0; di--)
		{
			mixSDL_PutConvSample (&dst, dstbpc, flags, sampl);
			sampl += (nextsampl - sampl) / di;
			if (stereo)
			{
				mixSDL_PutConvSample (&dst, dstbpc, flags, sampr);
				sampr += (nextsampr - sampr) / di;
			}
		}
	}
}

/* integer-periodic resampling with rate decrease */
void
mixSDL_ResampleDown_fast (mixSDL_Convertion *conv)
{
	mixSDL_ConvFlags flags = conv->flags;
	uint8 *src = conv->srcdata;
	uint8 *dst = conv->dstdata;
	uint32 srcbpc = conv->srcbpc;
	uint32 dstbpc = conv->dstbpc;
	uint32 convrate = conv->srcfreq / conv->dstfreq;
	uint32 srcjump = (convrate - 1) * srcbpc * conv->srcchans;
	bool stereo = conv->srcchans == 2 && !(flags & mixConvStereoDown);
	uint32 i;

	for (i = conv->dstsamples; i; i--)
	{
		sint32 samp;
		
		samp = mixSDL_GetConvSample (&src, srcbpc, flags);
		mixSDL_PutConvSample (&dst, dstbpc, flags, samp);
		if (stereo)
		{
			samp = mixSDL_GetConvSample (&src, srcbpc, flags);
			mixSDL_PutConvSample (&dst, dstbpc, flags, samp);
		}
		
		src += srcjump;
	}
}


/*************************************************
 * THE mixer - suprisingly simple in the end
 *
 * This could use some optimization perhaps
 */

void mixSDL_mix_channels (void *userdata, uint8 *stream, sint32 len)
{
	uint8 *end_stream = stream + len;

	/* keep this order or die */
	mixSDL_LockMutex (src_mutex);
	mixSDL_LockMutex (buf_mutex);
	mixSDL_LockMutex (act_mutex);

	for (; stream < end_stream; stream += mixer_chansize)
	{
		uint32 i;
		sint32 fullsamp;
		
		fullsamp = 0;

		for (i = 0; i < MAX_SOURCES; i++)
		{
			mixSDL_Source *src;
			sint32 samp;
			
			/* find next source */
			for (; i < MAX_SOURCES && (
					(src = active_sources[i]) == 0
					|| src->state != MIX_PLAYING
					|| !mixSDL_SourceGetNextSample (src, &samp));
					i++)
				;

			if (i < MAX_SOURCES)
			{
				/* sample aquired */
				fullsamp += samp;
			}
		}

		/* clip the sample */
		if (mixer_chansize == 2)
		{
			/* check S16 clipping */
			if (fullsamp > SINT16_MAX)
				fullsamp = SINT16_MAX;
			else if (fullsamp < SINT16_MIN)
				fullsamp = SINT16_MIN;
		}
		else
		{
			/* check S8 clipping */
			if (fullsamp > SINT8_MAX)
				fullsamp = SINT8_MAX;
			else if (fullsamp < SINT8_MIN)
				fullsamp = SINT8_MIN;
		}

		mixSDL_PutSampleExt (stream, mixer_chansize, fullsamp);
	}

	/* keep this order or die */
	mixSDL_UnlockMutex (act_mutex);
	mixSDL_UnlockMutex (buf_mutex);
	mixSDL_UnlockMutex (src_mutex);

	(void)userdata; // satisfying compiler
}

#endif
