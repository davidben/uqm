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

#ifndef _UQM_VIDEO_H
#define _UQM_VIDEO_H

#include "starcon.h"
#include "options.h"
#include "misc.h"
#include "vidlib.h"
#include "libs/graphics/tfb_draw.h"
#include "videodec.h"
#include "libs/sound/sound.h"

typedef struct tfb_videoclip
{
	TFB_VideoDecoder *decoder; // decoder to read from
	float length; // total length of clip seconds
	uint32 w, h;
	RECT dst_rect; // destination screen rect
	RECT src_rect; // source rect
	
	MUSIC_REF hAudio;
	Task play_task;
	uint32 frame_time; // time when next frame should be rendered
	TFB_Image* frame;
	uint32 cur_frame;
	uint32 max_frame_wait;
	bool playing;

	Mutex guard;
	CondVar frame_lock;
	uint32 want_frame;

	void* data; // user-defined data

} TFB_VideoClip;

#endif // _UQM_VIDEO_H
