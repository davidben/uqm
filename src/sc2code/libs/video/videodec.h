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

#ifndef _VIDEODEC_H
#define _VIDEODEC_H

#include "vidlib.h"
#include "libs/video/video.h"
#include "reslib.h"

// forward-declare
typedef struct tfb_videodecoder TFB_VideoDecoder;

#define _THIS TFB_VideoDecoder*

typedef struct tfb_videodecoderfunc
{
	int (* GetError) (_THIS);
	bool (* Init) (_THIS, TFB_PixelFormat* fmt);
	void (* Term) (_THIS);
	bool (* Open) (_THIS, uio_DirHandle *dir, const char *filename);
	void (* Close) (_THIS);
	int (* DecodeNext) (_THIS);
	uint32 (* SeekFrame) (_THIS, uint32 frame);
	float (* SeekTime) (_THIS, float time);
	uint32 (* GetFrame) (_THIS);
	float (* GetTime) (_THIS);

} TFB_VideoDecoderFuncs;

// decoder will call these to get info
// from the player
typedef struct tfb_videocallbacks
{
	// any decoder calls this
	void* (* GetCanvasLine) (_THIS, uint32 line);
	// non-audio-driven decoders call this to figure out
	// when the next frame should be drawn
	uint32 (* GetTicks) (_THIS);
	// non-audio-driven decoders call this to inform
	// the player when the next frame should be drawn
	bool (* SetTimer) (_THIS, uint32 msecs);

} TFB_VideoCallbacks;

#undef _THIS

struct tfb_videodecoder
{
	// decoder virtual funcs - R/O
	const TFB_VideoDecoderFuncs *funcs;
	// video formats - R/O
	const TFB_PixelFormat *format;
	// decoder-set data - R/O
	uint32 w, h;
	float length; // total length in seconds
	uint32 frame_count;
	uint32 max_frame_wait; // maximum interframe delay in msecs
	bool audio_synced;
	// decoder callbacks
	TFB_VideoCallbacks callbacks;
	// decoder-defined data - don't mess with
	void *dec_data;

	// other - public
	bool looping;
	void* data; // user-defined data
	// info - public R/O
	sint32 error;
	float pos; // position in seconds
	uint32 cur_frame;
	const char *decoder_info;

	// semi-private
	uio_DirHandle *dir;
	char *filename;

};

// return values
enum
{
	VIDEODECODER_OK,
	VIDEODECODER_ERROR,
	VIDEODECODER_EOF,
};

bool VideoDecoder_Init (int flags, int depth, uint32 Rmask, uint32 Gmask,
		uint32 Bmask, uint32 Amask);
void VideoDecoder_Uninit (void);
TFB_VideoDecoder* VideoDecoder_Load (uio_DirHandle *dir,
		const char *filename);
int VideoDecoder_Decode (TFB_VideoDecoder *decoder);
float VideoDecoder_Seek (TFB_VideoDecoder *decoder, float time_pos);
uint32 VideoDecoder_SeekFrame (TFB_VideoDecoder *decoder, uint32 frame_pos);
void VideoDecoder_Rewind (TFB_VideoDecoder *decoder);
void VideoDecoder_Free (TFB_VideoDecoder *decoder);


#endif // _VIDEODEC_H
