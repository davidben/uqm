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
#include "videodec.h"
#include "dukvid.h"

static bool vd_inited = 0;
static TFB_VideoFormat vd_vidfmt;

static struct vd_DecoderInfo
{
	const char* ext;
	const char* name;
	TFB_VideoDecoderFuncs* funcs;
}
vd_decoders[] = 
{
	{"duk", "DukVid", &dukv_DecoderVtbl},
	{NULL, NULL, NULL}	// null term
};

static void vd_computeMasks (uint32 mask, uint32* shift, uint32* loss);

bool
VideoDecoder_Init (int flags, int depth, uint32 Rmask, uint32 Gmask,
		uint32 Bmask, uint32 Amask)
{
	vd_inited = 0;

	if (depth < 15 || depth > 32)
	{
		fprintf (stderr, "VideoDecoder_Init: Unsupported video depth %d\n", depth);
		return false;
	}

	if ((Rmask & Gmask) || (Rmask & Bmask) || (Rmask & Amask) ||
			(Gmask & Bmask) || (Gmask & Amask) || (Bmask & Amask))
	{
		fprintf (stderr, "VideoDecoder_Init: Invalid channel masks\n");
		return false;
	}

	// BEGIN: adapted from SDL
	vd_vidfmt.BitsPerPixel = depth;
	vd_vidfmt.BytesPerPixel = (depth + 7) / 8;
	vd_vidfmt.Rmask = Rmask;
	vd_vidfmt.Gmask = Gmask;
	vd_vidfmt.Bmask = Bmask;
	vd_vidfmt.Amask = Amask;
	vd_computeMasks (Rmask, &vd_vidfmt.Rshift, &vd_vidfmt.Rloss);
	vd_computeMasks (Gmask, &vd_vidfmt.Gshift, &vd_vidfmt.Gloss);
	vd_computeMasks (Bmask, &vd_vidfmt.Bshift, &vd_vidfmt.Bloss);
	vd_computeMasks (Amask, &vd_vidfmt.Ashift, &vd_vidfmt.Aloss);
	// END: adapted from SDL
	
	vd_inited = 1;
	(void)flags; // dodge compiler warning

	return 1;
}

void
VideoDecoder_Uninit (void)
{
	vd_inited = 0;
	// go for a strall in a park
}

TFB_VideoDecoder*
VideoDecoder_Load (uio_DirHandle *dir, const char *filename)
{
	const char* pext;
	struct vd_DecoderInfo* dinfo;
	TFB_VideoDecoder* decoder;
	

	if (!vd_inited)
		return NULL;

	pext = strrchr (filename, '.');
	if (!pext)
	{
		fprintf (stderr, "VideoDecoder_Load: Unknown file type\n");
		return NULL;
	}
	++pext;

	for (dinfo = vd_decoders;
			dinfo->ext && strcmp(dinfo->ext, pext) != 0;
			++dinfo)
		;
	if (!dinfo->ext)
	{
		fprintf (stderr, "VideoDecoder_Load: Unsupported file type\n");
		return NULL;
	}

	decoder = (TFB_VideoDecoder*) HCalloc (sizeof (*decoder));
	decoder->funcs = dinfo->funcs;
	if (!decoder->funcs->Init (decoder, &vd_vidfmt))
	{
		fprintf (stderr, "VideoDecoder_Load: Cannot init '%s' decoder, code %d\n",
				dinfo->name, decoder->funcs->GetError (decoder));
		HFree (decoder);
		return NULL;
	}

	decoder->decoder_info = dinfo->name;
	decoder->dir = dir;
	decoder->filename = (char *) HMalloc (strlen (filename) + 1);
	strcpy (decoder->filename, filename);
	decoder->error = VIDEODECODER_OK;

	if (!decoder->funcs->Open (decoder, dir, filename))
	{
		fprintf (stderr, "VideoDecoder_Load: '%s' decoder did not load %s, code %d\n",
				dinfo->name, filename, decoder->funcs->GetError (decoder));
		
		VideoDecoder_Free (decoder);
		return NULL;
	}

	return decoder;
}

// return: >0 = OK, 0 = EOF, <0 = Error
int
VideoDecoder_Decode (TFB_VideoDecoder *decoder)
{
	int ret;

	if (!decoder)
		return 0;

	decoder->cur_frame = decoder->funcs->GetFrame (decoder);
	decoder->pos = decoder->funcs->GetTime (decoder);

	ret = decoder->funcs->DecodeNext (decoder);
	if (ret == 0)
		decoder->error = VIDEODECODER_EOF;
	else if (ret < 0)
		decoder->error = VIDEODECODER_ERROR;
	else
		decoder->error = VIDEODECODER_OK;

	return ret;
}

float
VideoDecoder_Seek (TFB_VideoDecoder *decoder, float pos)
{
	if (!decoder)
		return 0.0;

	decoder->pos = decoder->funcs->SeekTime (decoder, pos);
	decoder->cur_frame = decoder->funcs->GetFrame (decoder);

	return decoder->pos;
}

uint32
VideoDecoder_SeekFrame (TFB_VideoDecoder *decoder, uint32 frame)
{
	if (!decoder)
		return 0;

	decoder->cur_frame = decoder->funcs->SeekFrame (decoder, frame);
	decoder->pos = decoder->funcs->GetTime (decoder);

	return decoder->cur_frame;
}

void
VideoDecoder_Rewind (TFB_VideoDecoder *decoder)
{
	if (!decoder)
		return;

	VideoDecoder_Seek (decoder, 0);
}

void
VideoDecoder_Free (TFB_VideoDecoder *decoder)
{
	if (!decoder)
		return;
	
	decoder->funcs->Close (decoder);
	decoder->funcs->Term (decoder);

	HFree (decoder->filename);
	HFree (decoder);
}

// BEGIN: adapted from SDL
static void
vd_computeMasks (uint32 mask, uint32* shift, uint32* loss)
{
	*shift = 0;
	*loss = 8;
	if (mask)
	{
		for (; !(mask & 1); mask >>= 1 )
			++*shift;

		for (; (mask & 1); mask >>= 1 )
			--*loss;
	}
}
// END: adapted from SDL
