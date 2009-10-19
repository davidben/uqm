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

#include "vidplayer.h"

#include "vidintrn.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/log.h"
#include "libs/memlib.h"
#include "libs/sndlib.h"

// video callbacks
static void vp_BeginFrame (TFB_VideoDecoder*);
static void vp_EndFrame (TFB_VideoDecoder*);
static void* vp_GetCanvasLine (TFB_VideoDecoder*, uint32 line);
static uint32 vp_GetTicks (TFB_VideoDecoder*);
static bool vp_SetTimer (TFB_VideoDecoder*, uint32 msecs);


static const TFB_VideoCallbacks vp_DecoderCBs =
{
	vp_BeginFrame,
	vp_EndFrame,
	vp_GetCanvasLine,
	vp_GetTicks,
	vp_SetTimer
};

// audio stream callbacks
static bool vp_AudioStart (TFB_SoundSample* sample);
static void vp_AudioEnd (TFB_SoundSample* sample);
static void vp_BufferTag (TFB_SoundSample* sample, TFB_SoundTag* tag);
static void vp_QueueBuffer (TFB_SoundSample* sample, audio_Object buffer);

static const TFB_SoundCallbacks vp_AudioCBs =
{
	vp_AudioStart,
	NULL,
	vp_AudioEnd,
	vp_BufferTag,
	vp_QueueBuffer
};

// inter-thread param guarded by mutex
static Semaphore vp_interthread_lock = 0;
static void* vp_interthread_clip = NULL;

bool
TFB_InitVideoPlayer (void)
{
	// creation probably needs better handling
	vp_interthread_lock = CreateSemaphore (1, "inter-thread param lock",
			SYNC_CLASS_VIDEO);
	return vp_interthread_lock != 0;
}

void
TFB_UninitVideoPlayer (void)
{
	DestroySemaphore (vp_interthread_lock);
}

// audio-synced video playback task
// the frame rate and timing is dictated by the audio
static int
as_video_play_task (void *data)
{
	Task task = (Task) data;
	volatile TFB_VideoClip* vid;
	int ret;
	uint32 want_frame;
	TimeCount TimeOut;
	int bTerm;
	uint32 clagged;

	// snatch the clip pointer and release
	vid = vp_interthread_clip;
	vp_interthread_clip = NULL;
	ClearSemaphore (vp_interthread_lock);

	LockMutex (vid->guard);
	want_frame = vid->want_frame;
	if (vid->hAudio)
		PLRPlaySong (vid->hAudio, FALSE, 1);
	UnlockMutex (vid->guard);

	clagged = 0;

	// this works like so (audio-synced):
	//  1. you call VideoDecoder_Seek() [when necessary] and
	//     VideoDecoder_Decode()
	//  2. wait till the audio signals it's time for this frame
	//     or the maximum inter-frame timeout elapses; the timeout
	//     is necessary because the audio signaling is not precise
	//	   (see vp_AudioStart, vp_AudioEnd, vp_BufferTag)
	//  3. output the frame; if the timeout elapsed, increment the
	//     the lag counter
	//  4. set the next frame timeout; lag counter increases the
	//     timeout to allow audio to catch up
	//  5. on a seek operation, the audio stream is moved to the
	//     correct position and then the audio signals the frame
	//     that should be rendered
	//  The system of timeouts and lag counts should make the video
	//  *relatively* smooth
	//
	ret = VideoDecoder_Decode (vid->decoder);
	TimeOut = GetTimeCounter () + vid->decoder->max_frame_wait *
			ONE_SECOND / 1000;

	while (!(bTerm = Task_ReadState (task, TASK_EXIT)) && ret > 0)
	{
		// wait till its time to render next frame
		while (!(bTerm = Task_ReadState (task, TASK_EXIT))
				&& want_frame == vid->cur_frame - clagged
				&& TimeOut > GetTimeCounter ())
		{
			TaskSwitch ();
			LockMutex (vid->guard);
			want_frame = vid->want_frame;
			UnlockMutex (vid->guard);
		}

		if (bTerm)
			break;
		
		if (want_frame == vid->cur_frame - clagged)
		{	// timed out - draw the next frame
			++clagged;
		}
		else if (want_frame > vid->cur_frame - clagged
				&& want_frame <= vid->cur_frame)
		{
			// catching up
			clagged = vid->cur_frame - want_frame;
			// try again
			continue;
		}
		else if (want_frame != vid->cur_frame + 1)
		{	// out of sequence frame, let's get it
			vid->cur_frame = VideoDecoder_SeekFrame (
					vid->decoder, want_frame);
			ret = VideoDecoder_Decode (vid->decoder);
			clagged = 0;
		}
		else
		{
			clagged = 0;
		}
		vid->cur_frame = vid->decoder->cur_frame;

		// draw the frame
		if (ret > 0)
		{
			CONTEXT oldContext;
			
			LockMutex (GraphicsLock);
			// We have the cliprect precalculated and don't need the rest
			oldContext = SetContext (NULL);
			TFB_DrawScreen_Image (vid->frame,
					vid->dst_rect.corner.x, vid->dst_rect.corner.y,
					GSCALE_IDENTITY, NULL, TFB_SCREEN_MAIN);
			SetContext (oldContext);
			UnlockMutex (GraphicsLock);
			FlushGraphics (); // needed to prevent half-frame updates
		}

		// increase timeout with lag-count to allow audio to catch up
		TimeOut = GetTimeCounter () + vid->decoder->max_frame_wait *
				ONE_SECOND / 1000 + clagged * ONE_SECOND / 100;

		ret = VideoDecoder_Decode (vid->decoder);
	}
	if (vid->hAudio)
		PLRStop (vid->hAudio);
	vid->playing = false;

	FinishTask (task);

	return 0;
}

// audio-independent video playback task
// the frame rate and timing is dictated by the video decoder
static int
video_play_task (void *data)
{
	Task task = (Task) data;
	TFB_VideoClip* vid;
	int ret;

	// snatch the clip pointer and release
	vid = vp_interthread_clip;
	vp_interthread_clip = NULL;
	ClearSemaphore (vp_interthread_lock);

	LockMutex (vid->guard);
	if (vid->hAudio)
		PLRPlaySong (vid->hAudio, (vid->loop_frame != VID_NO_LOOP), 1);

	UnlockMutex (vid->guard);

	// this works like so:
	//  1. you call VideoDecoder_Seek() [when necessary] and
	//     VideoDecoder_Decode()
	//  2. the decoder calls back vp_GetTicks() and vp_SetTimer()
	//     to figure out and tell you when to render the frame
	//     being decoded
	//  On a seek operation, the decoder should reset its internal
	//  clock and call vp_GetTicks() again
	//
	ret = VideoDecoder_Decode (vid->decoder);

	while (!Task_ReadState (task, TASK_EXIT) && ret > 0)
	{
		CONTEXT oldContext;
		
		// wait till its time to render next frame
		SleepThreadUntil (vid->frame_time);
		vid->cur_frame = vid->decoder->cur_frame;

		LockMutex (GraphicsLock);
		// We have the cliprect precalculated and don't need the rest
		oldContext = SetContext (NULL);
		TFB_DrawScreen_Image (vid->frame,
				vid->dst_rect.corner.x, vid->dst_rect.corner.y,
				GSCALE_IDENTITY, NULL, TFB_SCREEN_MAIN);
		SetContext (oldContext);
		UnlockMutex (GraphicsLock);
		FlushGraphics (); // needed to prevent half-frame updates

		if (vid->cur_frame == vid->loop_frame)
			VideoDecoder_SeekFrame (vid->decoder, vid->loop_to);

		ret = VideoDecoder_Decode (vid->decoder);
	}
	vid->playing = false;
	if (vid->hAudio)
		PLRStop (vid->hAudio);

	FinishTask (task);

	return 0;
}

bool
TFB_PlayVideo (VIDEO_REF vid, uint32 x, uint32 y)
{
	RECT scrn_r;
	RECT clip_r = {{0, 0}, {vid->w, vid->h}};
	RECT vid_r = {{0, 0}, {ScreenWidth, ScreenHeight}};
	RECT dr = {{x, y}, {vid->w, vid->h}};
	RECT sr;

	if (!vid)
		return false;

	// calculate the frame-source and screen-destination rects
	GetContextClipRect (&scrn_r);
	if (scrn_r.extent.width <= 0 || scrn_r.extent.height <= 0)
	{	// bad or empty rect
		scrn_r = vid_r;
	}
	else if (!BoxIntersect(&scrn_r, &vid_r, &scrn_r))
		return false; // drawing outside visible

	sr = dr;
	sr.corner.x = -sr.corner.x;
	sr.corner.y = -sr.corner.y;
	if (!BoxIntersect (&clip_r, &sr, &sr))
		return false; // drawing outside visible

	dr.corner.x += scrn_r.corner.x;
	dr.corner.y += scrn_r.corner.y;
	if (!BoxIntersect (&scrn_r, &dr, &vid->dst_rect))
		return false; // drawing outside visible

	vid->src_rect = vid->dst_rect;
	vid->src_rect.corner.x = sr.corner.x;
	vid->src_rect.corner.y = sr.corner.y;

	vid->decoder->callbacks = vp_DecoderCBs;
	vid->decoder->data = vid;
	
	vid->frame = TFB_DrawImage_CreateForScreen (vid->w, vid->h, FALSE);
	vid->cur_frame = -1;
	vid->want_frame = -1;

	if (!vid->hAudio)
	{
		vid->hAudio = LoadMusicFile (vid->decoder->filename);
		vid->own_audio = true;
	}

	if (vid->decoder->audio_synced)
	{
		if (!vid->hAudio)
		{
			log_add (log_Warning, "TFB_PlayVideo: "
					"Cannot load sound-track for audio-synced video");
			return false;
		}

		TFB_SetSoundSampleCallbacks (*vid->hAudio, &vp_AudioCBs);
		TFB_SetSoundSampleData (*vid->hAudio, (intptr_t)vid);
	}

	SetSemaphore (vp_interthread_lock);
	vp_interthread_clip = vid;

	vid->playing = true;
	if (vid->decoder->audio_synced)
		vid->play_task = AssignTask (
				as_video_play_task, 4096, "a/s video player");
	else
		vid->play_task = AssignTask (
				video_play_task, 4096, "video player");

	if (!vid->play_task)
	{
		vid->playing = false;
		ClearSemaphore (vp_interthread_lock);
		TFB_StopVideo (vid);

		return false;
	}

	return true;
}

void
TFB_StopVideo (VIDEO_REF vid)
{
	if (!vid)
		return;

	vid->playing = false;
	if (vid->play_task)
		ConcludeTask (vid->play_task);
	
	if (vid->hAudio)
	{
		PLRStop (vid->hAudio);
		if (vid->own_audio)
		{
			DestroyMusic (vid->hAudio);
			vid->hAudio = 0;
			vid->own_audio = false;
		}
	}
	if (vid->frame) 
	{
		TFB_DrawScreen_DeleteImage (vid->frame);
		vid->frame = NULL;
	}
}

bool
TFB_VideoPlaying (VIDEO_REF vid)
{
	if (!vid)
		return false;

	return vid->playing;
}

uint32
TFB_GetVideoPosition (VIDEO_REF vid)
{
	uint32 pos;

	if (!TFB_VideoPlaying (vid))
		return 0;

	LockMutex (vid->guard);
	pos = (uint32) (vid->decoder->pos * 1000);
	UnlockMutex (vid->guard);

	return pos;
}

bool
TFB_SeekVideo (VIDEO_REF vid, uint32 pos)
{
	if (!TFB_VideoPlaying (vid))
		return false;

	if (vid->decoder->audio_synced)
	{
		PLRSeek (vid->hAudio, pos);
		TaskSwitch ();
		return true;
	}
	else
	{	// TODO: Non-a/s decoder seeking is not supported yet
		//   Decide what to do with these. Seeking this kind of
		//   video is trivial, but we may not want to do it.
		//   The only non-a/s videos right now are ship spins.
		return false;
	}
}

static void
vp_BeginFrame (TFB_VideoDecoder* decoder)
{
	TFB_VideoClip* vid = decoder->data;

	if (vid)
		TFB_DrawCanvas_Lock (vid->frame->NormalImg);
}

static void
vp_EndFrame (TFB_VideoDecoder* decoder)
{
	TFB_VideoClip* vid = decoder->data;

	if (vid)
		TFB_DrawCanvas_Unlock (vid->frame->NormalImg);
}

static void*
vp_GetCanvasLine (TFB_VideoDecoder* decoder, uint32 line)
{
	TFB_VideoClip* vid = decoder->data;

	if (!vid)
		return NULL;
	
	return TFB_DrawCanvas_GetLine (vid->frame->NormalImg, line);
}

static uint32
vp_GetTicks (TFB_VideoDecoder* decoder)
{
	uint32 ctr = GetTimeCounter ();
	return (ctr / ONE_SECOND) * 1000 + ((ctr % ONE_SECOND) * 1000) / ONE_SECOND;

	(void)decoder; // gobble up compiler warning
}

static bool
vp_SetTimer (TFB_VideoDecoder* decoder, uint32 msecs)
{
	TFB_VideoClip* vid = decoder->data;

	if (!vid)
		return false;

	// time when next frame should be displayed
	vid->frame_time = GetTimeCounter () + msecs * ONE_SECOND / 1000;
	return true;
}

static bool
vp_AudioStart (TFB_SoundSample* sample)
{
	TFB_VideoClip* vid = (TFB_VideoClip*) TFB_GetSoundSampleData (sample);
	TFB_SoundDecoder *decoder;

	assert (sizeof (intptr_t) >= sizeof (vid));
	assert (vid != NULL);

	decoder = TFB_GetSoundSampleDecoder (sample);

	LockMutex (vid->guard);
	vid->want_frame = SoundDecoder_GetFrame (decoder);
	UnlockMutex (vid->guard);

	return true;
}

static void
vp_AudioEnd (TFB_SoundSample* sample)
{
	TFB_VideoClip* vid = (TFB_VideoClip*) TFB_GetSoundSampleData (sample);

	assert (vid != NULL);

	LockMutex (vid->guard);
	vid->want_frame = vid->decoder->frame_count; // end it
	UnlockMutex (vid->guard);
}

static void
vp_BufferTag (TFB_SoundSample* sample, TFB_SoundTag* tag)
{
	TFB_VideoClip* vid = (TFB_VideoClip*) TFB_GetSoundSampleData (sample);
	uint32 frame = (uint32) tag->data;

	assert (sizeof (tag->data) >= sizeof (frame));
	assert (vid != NULL);

	LockMutex (vid->guard);
	vid->want_frame = frame; // let it go!
	UnlockMutex (vid->guard);
}

static void
vp_QueueBuffer (TFB_SoundSample* sample, audio_Object buffer)
{
	//TFB_VideoClip* vid = (TFB_VideoClip*) TFB_GetSoundSampleData (sample);
	TFB_SoundDecoder *decoder = TFB_GetSoundSampleDecoder (sample);

	TFB_TagBuffer (sample, buffer,
			(intptr_t) SoundDecoder_GetFrame (decoder));
}

