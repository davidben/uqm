//Copyright Paul Reiche, Fred Ford. 1992-2002

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

#include "video.h"
#include "sdl/sdlvideo.h"
#include "vidplayer.h"

#define NULL_VIDEO_REF	(0)
static VIDEO_REF _cur_video = NULL_VIDEO_REF;

BOOLEAN
InitVideo (BOOLEAN useCDROM)
		//useCDROM doesn't really apply to us
{
	TFB_VideoFormat fmt;
	TFB_GetScreenFormat (&fmt);
	return VideoDecoder_Init (0, fmt.BitsPerPixel, fmt.Rmask,
			fmt.Gmask, fmt.Bmask, fmt.Amask);
	
	(void)useCDROM;  /* dodge compiler warning */
}

void
UninitVideo ()
{
	VideoDecoder_Uninit ();
}

void
VidStop ()
{
	if (_cur_video)
	{
		TFB_StopVideo (_cur_video);
		TFB_FadeClearScreen ();
	}

	_cur_video = NULL_VIDEO_REF;
}

VIDEO_REF
VidPlaying ()
		// this should just probably return BOOLEAN
{
	if (!_cur_video)
		return NULL_VIDEO_REF;
	
	if (TFB_VideoPlaying (_cur_video))
		return _cur_video;

	return NULL_VIDEO_REF;
}

VIDEO_TYPE
VidPlay (VIDEO_REF VidRef, char *loopname, BOOLEAN uninit)
		// uninit was used to uninit the game kernel
		// before spawning duck exe
{
	VIDEO_TYPE ret;
	TFB_VideoClip* vid = (TFB_VideoClip*) VidRef;

	if (!vid)
		return NO_FMV;

	if (_cur_video)
		TFB_StopVideo (_cur_video);
	_cur_video = NULL_VIDEO_REF;

	TFB_FadeClearScreen ();
	FlushInput ();
	LockCrossThreadMutex (GraphicsLock);
	SetContext (ScreenContext);
	// play video in the center of the screen
	if (TFB_PlayVideo (VidRef, (SCREEN_WIDTH - vid->w) / 2,
			(SCREEN_HEIGHT - vid->h) / 2))
	{
		_cur_video = VidRef;
		ret = SOFTWARE_FMV;
	}
	else
	{
		ret = NO_FMV;
	}
	UnlockCrossThreadMutex (GraphicsLock);

	/* dodge compiler warnings */
	(void) loopname;
	(void) uninit;

	return ret;
}

void
VidDoInput (void)
{
	if (_cur_video)
		TFB_VideoInput (_cur_video);
}

VIDEO_REF
_init_video_file(PVOID pStr)
{
	TFB_VideoClip* vid;
	TFB_VideoDecoder* dec;

	dec = VideoDecoder_Load (contentDir, (const char*)pStr);
	if (!dec)
		return NULL_VIDEO_REF;

	vid = (TFB_VideoClip*) HCalloc (sizeof (*vid));
	vid->decoder = dec;
	vid->length = dec->length;
	vid->w = vid->decoder->w;
	vid->h = vid->decoder->h;
	vid->guard = CreateMutex ();
	vid->frame_lock = CreateCondVar ();

	return (VIDEO_REF) vid;
}

BOOLEAN
DestroyVideo (VIDEO_REF VideoRef)
{
	TFB_VideoClip* vid = (TFB_VideoClip*) VideoRef;

	if (!vid)
		return FALSE;

	VideoDecoder_Free (vid->decoder);
	DestroyMutex (vid->guard);
	DestroyCondVar (vid->frame_lock);
	HFree (vid);
	
	return TRUE;
}
