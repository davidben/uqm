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

#include "starcon.h"
#include <math.h>
#include "mixer.h"
#include "libs/misc.h"
#include "SDL.h"
#include "SDL_types.h"
#include "SDL_thread.h"
#include "SDL_byteorder.h"
#include "SDL_audio.h"
#include "SDL_version.h"
#include "mixerint.h"

#define DEBUG

/* stanard SDL_audio driver */
static const
mixSDL_DriverInfo mixer_SDLdriver = 
{
	mixSDL_DriverGetName,
	mixSDL_DriverGetError,
	mixSDL_DriverOpenAudio,
	SDL_CloseAudio,
	SDL_PauseAudio
};
/* SDL_audio driver is the default */
static mixSDL_Driver mixer_driver = &mixer_SDLdriver;
static mixSDL_DriverFlags mixer_driverflags = 0;

static uint32 audio_opened = 0;
static SDL_AudioSpec mixer_spec;
static uint32 last_error = MIX_NO_ERROR;
static uint32 mixer_format; /* format in mixer enumeration */
static uint32 mixer_chansize;
static uint32 mixer_sampsize;
static sint32 mixer_silence;
static uint32 mixer_freq;
static mixSDL_Quality mixer_quality;

/* mixer interim work-buffer; size in one-channel samples */
static uint32 mixer_datasize;
static sint32 *mixer_data = 0;

/* when locking more than one mutex
 * you must lock them in this order
 */
static RecursiveMutex src_mutex;
static RecursiveMutex buf_mutex;
static RecursiveMutex act_mutex;

#define MAX_SOURCES 8
mixSDL_Source *active_sources[MAX_SOURCES];


/*************************************************
 *  Internals
 */

static void
mixSDL_SetError (uint32 error)
{
	last_error = error;
}

static const char*
mixSDL_DriverGetName ()
{
	return "SDL_audio";
}

static const char*
mixSDL_DriverGetError (void)
{
	return SDL_GetError();
}

static int
mixSDL_DriverOpenAudio (void *desired, void *obtained)
{
	return SDL_OpenAudio ((SDL_AudioSpec *) desired,
			(SDL_AudioSpec *) obtained);
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

void
mixSDL_UseDriver (mixSDL_Driver driver, mixSDL_DriverFlags flags)
{
	if (!driver)
		return;

	if (driver == (mixSDL_Driver)~0)
		driver = &mixer_SDLdriver; /* default */

	mixer_driver = driver;
	mixer_driverflags = flags;
}

/* Open the mixer with a certain desired audio format */
bool
mixSDL_OpenAudio (uint32 frequency, uint32 format, uint32 samples_buf,
		mixSDL_Quality quality)
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

	fprintf (stderr, "MixSDL using driver '%s'\n",
			mixer_driver->GetDriverName());

	/* Set the desired format and frequency */
	desired.freq = frequency;
	desired.samples = samples_buf;
	if (mixer_driverflags & MIX_DRIVER_FAKE_PLAY)
		desired.callback = mixSDL_mix_fake;
	else if (quality == MIX_QUALITY_LOW)
		desired.callback = mixSDL_mix_lowq;
	else
		desired.callback = mixSDL_mix_channels;
	desired.userdata = NULL;

	/* Accept nearly any audio format */
	if (mixer_driver->OpenAudio (&desired, &mixer_spec) < 0)
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
		mixer_driver->CloseAudio ();
		fprintf (stderr, "mixSDL_OpenAudio: unable to aquire "
				"desired format\n");
		return false;
	}

	mixer_format = MIX_FORMAT_MAKE (
			mixer_chansize, mixer_spec.channels);
	mixer_sampsize = mixer_chansize * mixer_spec.channels;
	mixer_freq = mixer_spec.freq;
	mixer_quality = quality;

	/* 2x size the sound playback buffer should be enough for anything */
	mixer_datasize = samples_buf * mixer_spec.channels * 2;
	mixer_data = (sint32 *) HMalloc (sizeof (uint32) * mixer_datasize);

	src_mutex = CreateRecursiveMutex("mixSDL_SourceMutex", SYNC_CLASS_AUDIO);
	buf_mutex = CreateRecursiveMutex("mixSDL_BufferMutex", SYNC_CLASS_AUDIO);
	act_mutex = CreateRecursiveMutex("mixSDL_ActiveMutex", SYNC_CLASS_AUDIO);

	audio_opened = 1;
	mixer_driver->PauseAudio (0);

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

			if (mixer_data)
			{
				HFree (mixer_data);
				mixer_data = 0;
			}

			DestroyRecursiveMutex (src_mutex);
			DestroyRecursiveMutex (buf_mutex);
			DestroyRecursiveMutex (act_mutex);
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
		src->prevqueued = 0;
		src->lastqueued = 0;
		src->curbufofs = 0;
		src->curbufdelta = 0;

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

	LockRecursiveMutex (src_mutex);

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

	UnlockRecursiveMutex (src_mutex);
}

/* check if really is a source */
bool
mixSDL_IsSource (mixSDL_Object srcobj)
{
	mixSDL_Source *src = (mixSDL_Source *) srcobj;
	bool ret;

	if (!src)
		return false;

	LockRecursiveMutex (src_mutex);
	ret = src->magic == mixSDL_srcMagic;
	UnlockRecursiveMutex (src_mutex);

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

	LockRecursiveMutex (src_mutex);

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
			{
				mixSDL_Buffer *buf = (mixSDL_Buffer *) value;

				if (src->cqueued > 0)
					mixSDL_SourceUnqueueAll (src);
				
				if (buf && !mixSDL_CheckBufferState (buf, "mixSDL_Sourcei"))
					break;

				src->firstqueued = buf;
				src->nextqueued = src->firstqueued;
				src->prevqueued = 0;
				src->lastqueued = src->nextqueued;
				if (src->lastqueued)
					src->lastqueued->next = 0;
				src->cqueued = 1;
			}
			break;
		case MIX_SOURCE_STATE:
			if (value == MIX_INITIAL)
			{
				mixSDL_SourceRewind_internal (src);
			}
			else
			{
				fprintf (stderr, "mixSDL_Sourcei(MIX_SOURCE_STATE): "
						"unsupported state, call ignored\n");
			}
			break;
		default:
			mixSDL_SetError (MIX_INVALID_ENUM);
			fprintf (stderr, "mixSDL_Sourcei() called "
					"with unsupported property %u\n", pname);
		}
	}

	UnlockRecursiveMutex (src_mutex);
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

	LockRecursiveMutex (src_mutex);

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

	UnlockRecursiveMutex (src_mutex);
}

/* set source float array property (CURRENTLY NOT IMPLEMENTED) */
void mixSDL_Sourcefv (mixSDL_Object srcobj, mixSDL_SourceProp pname, float *value)
{
	(void)srcobj;
	(void)pname;
	(void)value;
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

	LockRecursiveMutex (src_mutex);

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

	UnlockRecursiveMutex (src_mutex);
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

	LockRecursiveMutex (src_mutex);

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

	UnlockRecursiveMutex (src_mutex);
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

	LockRecursiveMutex (src_mutex);

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
		{
			if (src->firstqueued && !src->nextqueued)
				mixSDL_SourceRewind_internal (src);
			mixSDL_SourceActivate (src);
		}
		src->state = MIX_PLAYING;
	}

	UnlockRecursiveMutex (src_mutex);
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

	LockRecursiveMutex (src_mutex);

	if (src->magic != mixSDL_srcMagic)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "mixSDL_SourcePlay(): not a source\n");
#endif
	}
	else
	{
		mixSDL_SourceRewind_internal (src);
	}

	UnlockRecursiveMutex (src_mutex);
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

	LockRecursiveMutex (src_mutex);

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

	UnlockRecursiveMutex (src_mutex);
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

	LockRecursiveMutex (src_mutex);

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

	UnlockRecursiveMutex (src_mutex);
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

	LockRecursiveMutex (buf_mutex);
	/* check to make sure we can safely queue all buffers */
	for (i = n, pobj = pbufobj; i; i--, pobj++)
	{
		mixSDL_Buffer *buf = (mixSDL_Buffer *) *pobj;
		if (!buf || !mixSDL_CheckBufferState (buf,
				"mixSDL_SourceQueueBuffers"))
		{
			break;
		}
	}
	UnlockRecursiveMutex (buf_mutex);

	if (i == 0)
	{	/* all buffers checked out */
		LockRecursiveMutex (src_mutex);
		LockRecursiveMutex (buf_mutex);

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
					src->prevqueued = 0;
				}
				src->cqueued++;
				buf->state = MIX_BUF_QUEUED;
			}
		}

		UnlockRecursiveMutex (buf_mutex);
		UnlockRecursiveMutex (src_mutex);
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

	LockRecursiveMutex (src_mutex);

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
		LockRecursiveMutex (buf_mutex);

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
				if (src->prevqueued == buf)
					src->prevqueued = 0;
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

		UnlockRecursiveMutex (buf_mutex);
	}

	UnlockRecursiveMutex (src_mutex);
}

/*************************************************
 *  Sources internals
 */

static void
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

	LockRecursiveMutex (buf_mutex);

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

	UnlockRecursiveMutex (buf_mutex);

	src->firstqueued = 0;
	src->nextqueued = 0;
	src->prevqueued = 0;
	src->lastqueued = 0;
	src->cqueued = 0;
	src->cprocessed = 0;
	src->curbufofs = 0;
	src->curbufdelta = 0;
}

/* add the source to the active array */
static void
mixSDL_SourceActivate (mixSDL_Source* src)
{
	uint32 i;

	LockRecursiveMutex (act_mutex);

#ifdef DEBUG
	/* check active sources, see if this source is there already */
	for (i = 0; i < MAX_SOURCES && active_sources[i] != src; i++)
		;
	if (i < MAX_SOURCES)
	{	/* source found */
		fprintf (stderr, "mixSDL_SourceActivate(): "
				"source already active in slot %u\n", i);
		UnlockRecursiveMutex (act_mutex);
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

	UnlockRecursiveMutex (act_mutex);
}

/* remove the source from the active array */
static void
mixSDL_SourceDeactivate (mixSDL_Source* src)
{
	uint32 i;

	LockRecursiveMutex (act_mutex);

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

	UnlockRecursiveMutex (act_mutex);
}

static void
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

	LockRecursiveMutex (buf_mutex);

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
	src->prevqueued = 0;
	src->curbufofs = 0;
	src->curbufdelta = 0;

	UnlockRecursiveMutex (buf_mutex);
}

static void
mixSDL_SourceRewind_internal (mixSDL_Source *src)
{
	/* should change the processed buffers to queued */
	mixSDL_Buffer *buf;

	if (src->state >= MIX_PLAYING)
		mixSDL_SourceDeactivate (src);

	LockRecursiveMutex (buf_mutex);

	for (buf = src->firstqueued;
			buf && buf->state != MIX_BUF_QUEUED;
			buf = buf->next)
	{
		buf->state = MIX_BUF_QUEUED;
	}

	UnlockRecursiveMutex (buf_mutex);

	src->curbufofs = 0;
	src->curbufdelta = 0;
	src->cprocessed = 0;
	src->nextqueued = src->firstqueued;
	src->prevqueued = 0;
	src->state = MIX_INITIAL;
}

/* get the sample next in queue in internal format */
static __inline__ bool
mixSDL_SourceGetNextSample (mixSDL_Source *src, sint32* psamp, bool left)
{
	/* fake the data if requested */
	if (mixer_driverflags & MIX_DRIVER_FAKE_DATA)
		return mixSDL_SourceGetFakeSample (src, psamp, left);

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

		if (mixer_freq == buf->orgfreq)
		{
			data = buf->data + (uint32)src->curbufofs;
			samp = mixSDL_GetSampleInt (data, mixer_chansize);
			src->curbufofs += mixer_chansize;
		}
		else if (mixer_freq > buf->orgfreq)
		{
			if (mixer_quality == MIX_QUALITY_DEFAULT)
				samp = mixSDL_GetResampledInt_linear (src, left);
			else if (mixer_quality == MIX_QUALITY_HIGH)
				samp = mixSDL_GetResampledInt_cubic (src, left);
			else
				samp = mixSDL_GetResampledInt_nearest (src, left);
		}
		else
		{
			/* because currently linear and cubic resamplers do not work
			   for downsampling */
			samp = mixSDL_GetResampledInt_nearest (src, left);
		}
		
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
			src->prevqueued = src->nextqueued;
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

/* fake the next sample, but process buffers and states */
static __inline__ bool
mixSDL_SourceGetFakeSample (mixSDL_Source *src, sint32* psamp, bool left)
{
	while (src->nextqueued)
	{
		mixSDL_Buffer *buf = src->nextqueued;

		if (mixer_freq == buf->orgfreq)
		{
			src->curbufofs += mixer_chansize;
		}
		else
		{
			double offset, intoffset;
			if (MIX_FORMAT_CHANS (mixer_format) == 2)
			{
				if (!left)
				{
					offset = src->curbufdelta +
						(double)src->nextqueued->orgfreq / mixer_freq;
					src->curbufdelta = modf (offset, &intoffset);
					src->curbufofs += (uint32)intoffset * mixer_sampsize;
				}
			}
			else
			{
				offset = src->curbufdelta +
					(double)src->nextqueued->orgfreq / mixer_freq;
				src->curbufdelta = modf (offset, &intoffset);
				src->curbufofs += (uint32)intoffset * mixer_sampsize;
			}
		}

		*psamp = 0;
		
		if (src->curbufofs >= buf->size)
		{	
			/* buffer exhausted, go next */
			buf->state = MIX_BUF_PROCESSED;
			src->curbufofs = 0;
			src->curbufdelta = 0;
			src->prevqueued = src->nextqueued;
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
	
	LockRecursiveMutex (buf_mutex);

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
	UnlockRecursiveMutex (buf_mutex);
}

/* check if really a buffer object */
bool
mixSDL_IsBuffer (mixSDL_Object bufobj)
{
	mixSDL_Buffer *buf = (mixSDL_Buffer *) bufobj;
	bool ret;

	if (!buf)
		return false;

	LockRecursiveMutex (buf_mutex);
	ret = buf->magic == mixSDL_bufMagic;
	UnlockRecursiveMutex (buf_mutex);

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

	LockRecursiveMutex (buf_mutex);
	
	if (buf->locked)
	{
		UnlockRecursiveMutex (buf_mutex);
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

	UnlockRecursiveMutex (buf_mutex);
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

	LockRecursiveMutex (buf_mutex);
	
	if (buf->locked)
	{
		UnlockRecursiveMutex (buf_mutex);
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

		conv.srcsamples = conv.dstsamples =
			size / MIX_FORMAT_SAMPSIZE (format);
				
		if (conv.dstsamples >
				UINT32_MAX / MIX_FORMAT_SAMPSIZE (format))
		{
			mixSDL_SetError (MIX_INVALID_VALUE);
		}
		else
		{
			dstsize = conv.dstsamples *
					MIX_FORMAT_SAMPSIZE (mixer_format);

			buf->size = dstsize;
			/* only copy/convert the data if not faking */
			if (! (mixer_driverflags & MIX_DRIVER_FAKE_DATA))
			{
				buf->data = HMalloc (dstsize);
			
				if (format == mixer_format)
				{
					/* format identical to internal */
					buf->locked = true;
					UnlockRecursiveMutex (buf_mutex);

					memcpy (buf->data, data, size);
					if (MIX_FORMAT_SAMPSIZE (mixer_format) == 1)
					{
						/* convert buffer to S8 format internally */
						uint8* dst;
						for (dst = buf->data; dstsize; dstsize--, dst++)
							*dst ^= 0x80;
					}

					LockRecursiveMutex (buf_mutex);
					buf->locked = false;
				}
				else 
				{
					/* needs convertion */
					conv.srcfmt = format;
					conv.srcdata = data;
					conv.srcsize = size;
					conv.dstfmt = mixer_format;
					conv.dstdata = buf->data;
					conv.dstsize = dstsize;

					buf->locked = true;
					UnlockRecursiveMutex (buf_mutex);

					mixSDL_ConvertBuffer_internal (&conv);
					
					LockRecursiveMutex (buf_mutex);
					buf->locked = false;
				}
			}

			buf->state = MIX_BUF_FILLED;
		}
	}

	UnlockRecursiveMutex (buf_mutex);
}


/*************************************************
 *  Buffer internals
 */

static __inline__ bool
mixSDL_CheckBufferState (mixSDL_Buffer *buf, const char* FuncName)
{
	if (!buf)
		return false;

	if (buf->magic != mixSDL_bufMagic)
	{
		mixSDL_SetError (MIX_INVALID_NAME);
#ifdef DEBUG
		fprintf (stderr, "%s(): not a buffer\n", FuncName);
#endif
		return false;
	}

	if (buf->locked)
	{
		mixSDL_SetError (MIX_INVALID_OPERATION);
#ifdef DEBUG
		fprintf (stderr, "%s(): locked buffer attempted\n", FuncName);
#endif
		return false;
	}

	if (buf->state != MIX_BUF_FILLED)
	{
		mixSDL_SetError (MIX_INVALID_OPERATION);
#ifdef DEBUG
		fprintf (stderr, "%s: invalid buffer attempted\n", FuncName);
#endif
		return false;
	}
	return true;
}

static void
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

	mixSDL_ResampleFlat (conv);
}

/*************************************************
 *  Resampling routines
 */

/* get a sample from external buffer
 * in internal format
 */
static __inline__ sint32
mixSDL_GetSampleExt (void *src, uint32 bpc)
{
	if (bpc == 2)
		return *(sint16 *)src;
	else
		return (*(uint8 *)src) - 128;
}

/* get a sample from internal buffer */
static __inline__ sint32
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
static __inline__ void
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
static __inline__ void
mixSDL_PutSampleInt (void *dst, uint32 bpc, sint32 samp)
{
	if (bpc == 2)
		*(sint16 *)dst = samp;
	else
		*(sint8 *)dst = samp;
}

/* get a resampled sample from internal buffer (nearest neighbor) */
static __inline__ sint32
mixSDL_GetResampledInt_nearest (mixSDL_Source *src, bool left)
{	
	uint8 *d0 = src->nextqueued->data + src->curbufofs;
	double offset, intoffset;

	if (MIX_FORMAT_CHANS (mixer_format) == 2)
	{
		if (!left)
		{
			d0 += mixer_chansize;
			offset = src->curbufdelta +
					(double)src->nextqueued->orgfreq / mixer_freq;
			src->curbufdelta = modf (offset, &intoffset);
			src->curbufofs += (uint32)intoffset * mixer_sampsize;
		}
	}
	else
	{
		offset = src->curbufdelta +
				(double)src->nextqueued->orgfreq / mixer_freq;
		src->curbufdelta = modf (offset, &intoffset);
		src->curbufofs += (uint32)intoffset * mixer_sampsize;
	}

	return mixSDL_GetSampleInt (d0, mixer_chansize);
}

/* get a resampled sample from internal buffer (linear interpolation) */
static __inline__ sint32
mixSDL_GetResampledInt_linear (mixSDL_Source *src, bool left)
{
	// TODO: support for downsampling

	mixSDL_Buffer *curr = src->nextqueued;
	mixSDL_Buffer *next = src->nextqueued->next;
	uint8 *d0, *d1;
	sint32 s0, s1, samp;
	double offset, intoffset, delta;

	delta = src->curbufdelta;
	d0 = curr->data + src->curbufofs;
	
	if (MIX_FORMAT_CHANS (mixer_format) == 2)
	{
		if (!left)
		{
			d0 += mixer_chansize;
			offset = src->curbufdelta +
					(double)src->nextqueued->orgfreq / mixer_freq;
			src->curbufdelta = modf (offset, &intoffset);
			src->curbufofs += (uint32)intoffset * mixer_sampsize;
		}
	}
	else
	{
		offset = src->curbufdelta +
				(double)src->nextqueued->orgfreq / mixer_freq;
		src->curbufdelta = modf (offset, &intoffset);
		src->curbufofs += (uint32)intoffset * mixer_sampsize;
	}

	if (d0 + mixer_sampsize >= curr->data + curr->size)
	{
		if (next && next->data && next->size >= mixer_sampsize)
		{
			d1 = next->data;
			if (!left)
				d1 += mixer_chansize;
		}
		else
			d1 = d0;
	}
	else
		d1 = d0 + mixer_sampsize;

	s0 = mixSDL_GetSampleInt (d0, mixer_chansize);
	s1 = mixSDL_GetSampleInt (d1, mixer_chansize);
	samp = s0 + (sint32)(delta * (s1 - s0));

	return samp;
}

/* get a resampled sample from internal buffer (cubic interpolation) */
static __inline__ sint32
mixSDL_GetResampledInt_cubic (mixSDL_Source *src, bool left)
{
	// TODO: support for downsampling

	mixSDL_Buffer *prev = src->prevqueued;
	mixSDL_Buffer *curr = src->nextqueued;
	mixSDL_Buffer *next = src->nextqueued->next;
	uint8 *d0, *d1, *d2, *d3; /* prev, curr, next, next + 1 */
	sint32 samp;
	double offset, intoffset;
	float delta, delta2, a, b, c, s0, s1, s2, s3;

	delta = (float)src->curbufdelta;
	delta2 = delta * delta;
	d1 = curr->data + src->curbufofs;
	
	if (MIX_FORMAT_CHANS (mixer_format) == 2)
	{
		if (!left)
		{
			d1 += mixer_chansize;
			offset = src->curbufdelta +
					(double)src->nextqueued->orgfreq / mixer_freq;
			src->curbufdelta = modf (offset, &intoffset);
			src->curbufofs += (uint32)intoffset * mixer_sampsize;
		}
	}
	else
	{
		offset = src->curbufdelta +
				(double)src->nextqueued->orgfreq / mixer_freq;
		src->curbufdelta = modf (offset, &intoffset);
		src->curbufofs += (uint32)intoffset * mixer_sampsize;
	}

	if (d1 - mixer_sampsize < curr->data)
	{
		if (prev && prev->data && prev->size >= mixer_sampsize)
		{
			d0 = prev->data + prev->size - mixer_sampsize;
			if (!left)
				d0 += mixer_chansize;
		}
		else
			d0 = d1;
	}
	else
		d0 = d1 - mixer_sampsize;

	if (d1 + mixer_sampsize >= curr->data + curr->size)
	{
		if (next && next->data && next->size >= mixer_sampsize * 2)
		{
			d2 = next->data;
			if (!left)
				d2 += mixer_chansize;
			d3 = d2 + mixer_sampsize;
		}
		else
			d2 = d3 = d1;
	}
	else
	{
		d2 = d1 + mixer_sampsize;
		if (d2 + mixer_sampsize >= curr->data + curr->size)
		{
			if (next && next->data && next->size >= mixer_sampsize)
			{
				d3 = next->data;
				if (!left)
					d3 += mixer_chansize;
			}
			else
				d3 = d2;
		}
		else
			d3 = d2 + mixer_sampsize;
	}

	s0 = (float)mixSDL_GetSampleInt (d0, mixer_chansize);
	s1 = (float)mixSDL_GetSampleInt (d1, mixer_chansize);
	s2 = (float)mixSDL_GetSampleInt (d2, mixer_chansize);
	s3 = (float)mixSDL_GetSampleInt (d3, mixer_chansize);

	a = (3.0f * (s1 - s2) - s0 + s3) * 0.5f;
	b = 2.0f * s2 + s0 - ((5.0f * s1 + s3) * 0.5f);
	c = (s2 - s0) * 0.5f;
	
	samp = (sint32)(a * delta2 * delta + b * delta2 + c * delta + s1);
	return samp;
}

/* get next sample from external buffer
 * in internal format, while performing
 * convertion if necessary
 */
static __inline__ sint32
mixSDL_GetConvSample (uint8 **psrc, uint32 bpc, uint32 flags)
{
	sint32 samp;
	
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
static __inline__ void
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

/* resampling with respect to sample size only */
static void
mixSDL_ResampleFlat (mixSDL_Convertion *conv)
{
	mixSDL_ConvFlags flags = conv->flags;
	uint8 *src = conv->srcdata;
	uint8 *dst = conv->dstdata;
	uint32 srcbpc = conv->srcbpc;
	uint32 dstbpc = conv->dstbpc;
	uint32 samples;

	samples = conv->srcsamples;
	if ( !(conv->flags & (mixConvStereoUp | mixConvStereoDown)))
		samples *= conv->srcchans;

	for (; samples; samples--)
	{
		sint32 samp;

		samp = mixSDL_GetConvSample (&src, srcbpc, flags);
		mixSDL_PutConvSample (&dst, dstbpc, flags, samp);
	}
}


/**********************************************************
 * THE mixer - higher quality; smoothed-out clipping
 *
 * This could use some optimization perhaps
 */

static void
mixSDL_mix_channels (void *userdata, uint8 *stream, sint32 len)
{
	uint32 samples = len / mixer_chansize;
	sint32 *end_data = mixer_data + samples;
	sint32 *data;
	uint32 step;
	bool left = true;
	uint32 chans = MIX_FORMAT_CHANS (mixer_format);

	/* mixer_datasize < samples should not happen ever, but for now.. */
	if (mixer_datasize < samples)
	{
#ifdef DEBUG
		fprintf (stderr, "mixSDL_mix_channels(): "
				"WARNING: work-buffer too small\n");
#endif
		mixSDL_mix_lowq (userdata, stream, len);
	}

	/* keep this order or die */
	LockRecursiveMutex (src_mutex);
	LockRecursiveMutex (buf_mutex);
	LockRecursiveMutex (act_mutex);

	/* first, collect data from sources and put into work-buffer */
	for (data = mixer_data; data < end_data; ++data)
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
					|| !mixSDL_SourceGetNextSample (src, &samp, left));
					i++)
				;

			if (i < MAX_SOURCES)
			{
				/* sample aquired */
				fullsamp += samp;
			}
		}
		
		*data = fullsamp;
		if (chans == 2)
			left = !left;
	}

	/* unclip work-buffer */
	step = mixer_spec.channels;
	mixSDL_UnclipWorkBuffer (mixer_data, end_data, step);
	if (step == 2)
	{	/* also, for the second channel */
		mixSDL_UnclipWorkBuffer (mixer_data + 1, end_data, step);
	}

	/* copy data into driver buffer */
	for (data = mixer_data; data < end_data; ++data)
	{
		mixSDL_PutSampleExt (stream, mixer_chansize, *data);
		stream += mixer_chansize;
	}

	/* keep this order or die */
	UnlockRecursiveMutex (act_mutex);
	UnlockRecursiveMutex (buf_mutex);
	UnlockRecursiveMutex (src_mutex);

	(void) userdata; // satisfying compiler - unused arg
}

/* data unclipping - smooth out the areas that need to be clipped */
static void
mixSDL_UnclipWorkBuffer (sint32 *data, sint32 *end_data, uint32 step)
{
	while (data < end_data)
	{
		uint32 len;
		sint32 extremum;
		sint32 *chunk;
		sint32 threshold, origin, range_end;
		double gain_mult;
		sint32 samp = *data;

		if (mixer_chansize == 2)
		{	/* S16 */
			if (samp < MIX_UNCLIP_S16_MIN)
			{
				origin = MIX_UNCLIP_S16_MIN;
				range_end = -SINT16_MIN;
				threshold = -MIX_UNCLIP_S16_MIN;
			}
			else if (samp > MIX_UNCLIP_S16_MAX)
			{
				origin = MIX_UNCLIP_S16_MAX;
				range_end = SINT16_MAX;
				threshold = MIX_UNCLIP_S16_MAX;
			}
			else
			{
				data += step;
				continue;
			}
		}
		else
		{	/* S8 */
			if (samp < MIX_UNCLIP_S8_MIN)
			{
				origin = MIX_UNCLIP_S8_MIN;
				range_end = -SINT8_MIN;
				threshold = -MIX_UNCLIP_S8_MIN;
			}
			else if (samp > MIX_UNCLIP_S8_MAX)
			{
				origin = MIX_UNCLIP_S8_MAX;
				range_end = SINT8_MAX;
				threshold = MIX_UNCLIP_S8_MAX;
			}
			else
			{
				data += step;
				continue;
			}
		}

		chunk = data;

		/* seek to next sample not in clipping area */
		extremum = 0;
		for (len = 0; data < end_data; data += step, ++len)
		{
			samp = *data;
			if (samp < 0)
				samp = -samp;
			if (samp > extremum)
				extremum = samp;
			if (samp <= threshold)
				break;
		}

		if (extremum < range_end)
			continue;	/* nothing to do really */

		gain_mult = (double) (range_end - threshold)
				/ (double) (extremum - threshold);
		
		/* apply unclipping filter - clipping smooth-out */
		for (data = chunk; len; data += step, --len)
		{
			*data = origin + (sint32) (gain_mult * (*data - origin));
		}
	}
}

/* low quality faster version */
static void
mixSDL_mix_lowq (void *userdata, uint8 *stream, sint32 len)
{
	uint8 *end_stream = stream + len;
	bool left = true;
	uint32 chans = MIX_FORMAT_CHANS (mixer_format);

	/* keep this order or die */
	LockRecursiveMutex (src_mutex);
	LockRecursiveMutex (buf_mutex);
	LockRecursiveMutex (act_mutex);

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
					|| !mixSDL_SourceGetNextSample (src, &samp, left));
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
		if (chans == 2)
			left = !left;
	}

	/* keep this order or die */
	UnlockRecursiveMutex (act_mutex);
	UnlockRecursiveMutex (buf_mutex);
	UnlockRecursiveMutex (src_mutex);

	(void) userdata; // satisfying compiler - unused arg
}

/* fake mixer -- only process buffer and source states */
static void
mixSDL_mix_fake (void *userdata, uint8 *stream, sint32 len)
{
	uint8 *end_stream = stream + len;
	bool left = true;
	uint32 chans = MIX_FORMAT_CHANS (mixer_format);

	/* keep this order or die */
	LockRecursiveMutex (src_mutex);
	LockRecursiveMutex (buf_mutex);
	LockRecursiveMutex (act_mutex);

	for (; stream < end_stream; stream += mixer_chansize)
	{
		uint32 i;

		for (i = 0; i < MAX_SOURCES; i++)
		{
			mixSDL_Source *src;
			sint32 samp;
			
			/* find next source */
			for (; i < MAX_SOURCES && (
					(src = active_sources[i]) == 0
					|| src->state != MIX_PLAYING
					|| !mixSDL_SourceGetFakeSample (src, &samp, left));
					i++)
				;
		}
		if (chans == 2)
			left = !left;
	}

	/* keep this order or die */
	UnlockRecursiveMutex (act_mutex);
	UnlockRecursiveMutex (buf_mutex);
	UnlockRecursiveMutex (src_mutex);

	(void) userdata; // satisfying compiler - unused arg
}
