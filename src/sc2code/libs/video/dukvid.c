/* DUCK video player
 *
 * Status:	as raw as it gets
 *
 */

#include "video.h"
#include "libs/sndlib.h"
#include "libs/sound/sound.h"
#include "dukvid.h"
#include "endian_uqm.h"
#include <stdio.h>

#define _THIS    TFB_VideoDecoder* This

int dukv_GetError (_THIS);
bool dukv_Init (_THIS, TFB_VideoFormat* fmt);
void dukv_Term (_THIS);
bool dukv_Open (_THIS, uio_DirHandle *dir, const char *filename);
void dukv_Close (_THIS);
int dukv_DecodeNext (_THIS);
uint32 dukv_SeekFrame (_THIS, uint32 frame);
float dukv_SeekTime (_THIS, float time);
uint32 dukv_GetFrame (_THIS);
float dukv_GetTime (_THIS);

TFB_VideoDecoderFuncs dukv_DecoderVtbl = 
{
	dukv_GetError,
	dukv_Init,
	dukv_Term,
	dukv_Open,
	dukv_Close,
	dukv_DecodeNext,
	dukv_SeekFrame,
	dukv_SeekTime,
	dukv_GetFrame,
	dukv_GetTime,
};

typedef struct tfb_duckvideoheader
{
	uint32 version;
	uint32 scrn_x_ofs;   // horz screen offset in pixels
	uint32 scrn_y_ofs;   // vert screen offset in pixels 
	uint16 wb, hb;       // width + height in blocks
	sint16 lumas[8];     // future luminance deltas
	sint16 chromas[8];   // future chrominance deltas

} TFB_DuckVideoHeader;

#define NUM_VEC_ITEMS	0x010
#define NUM_VECTORS		0x100

typedef struct tfb_duckvideodeltas
{
	sint32 lumas[NUM_VECTORS][NUM_VEC_ITEMS];
	sint32 chromas[NUM_VECTORS][NUM_VEC_ITEMS];

} TFB_DuckVideoDeltas;

typedef struct tfb_duckvideo
{
	sint32 last_error;
	uio_DirHandle* basedir;
	char* basename;
	uio_Stream *stream;

// loaded from disk
	uint32* frames;
	uint32 cframes;
	uint32 iframe;
	uint32 version;
	uint32 wb, hb;     // width + height in blocks
	uint32 w, h;

// generated
	TFB_DuckVideoDeltas d;

	uint8* inbuf;
	uint32* decbuf;

} TFB_DuckVideo;

#define DUCK_GENERAL_FPS     14.622f
#define DUCK_MAX_FRAME_SIZE  0x8000U
#define DUCK_END_OF_SEQUENCE 1

void
dukv_DecodeFrame (uint8* src_p, uint32* dst_p, uint32 wb, uint32 hb,
		TFB_DuckVideoDeltas* deltas)
{
	int iVec;
	int iSeq;
	uint32 x, y;
	sint32 w;
	uint32 *d_p0, *d_p1;
	int i;

	w = wb * 4;

	iVec = *(src_p++);
	iSeq = 0;

	for (y = 0; y < hb; ++y)
	{
		sint32 accum0, accum1, corr, corr0, corr1, delta;
		sint32 pix[4];

		d_p0 = dst_p + y * w * 2;
		d_p1 = d_p0 + w;

		accum0 = 0;
		accum1 = 0;
		corr0 = 0;
		corr1 = 0;

		for (x = 0; x < wb; ++x)
		{
			if (y == 0)
			{
				pix[0] = pix[1] = pix[2] = pix[3] = 0;
			}
			else
			{
				uint32* p_p = d_p0 - w;
				pix[0] = p_p[0];
				pix[1] = p_p[1];
				pix[2] = p_p[2];
				pix[3] = p_p[3];
			}

			// start with chroma delta
			delta = deltas->chromas[iVec][iSeq++];
			iSeq++; // correctors ignored

			accum0 += delta >> 1;
			if (delta & 1)
			{
				iVec = *(src_p++);
				iSeq = 0;
			}

			// line 0
			for (i = 0; i < 4; ++i, ++d_p0)
			{
				delta = deltas->lumas[iVec][iSeq++];
				corr = deltas->lumas[iVec][iSeq++];

				accum0 += delta >> 1;
				corr0 ^= corr;
				pix[i] += accum0;
				pix[i] ^= corr0;

				if (delta & 1)
				{
					iVec = *(src_p++);
					iSeq = 0;
				}
				
				*d_p0 = pix[i];
			}

			// line 1
			for (i = 0; i < 4; ++i, ++d_p1)
			{
				delta = deltas->lumas[iVec][iSeq++];
				corr = deltas->lumas[iVec][iSeq++];

				accum1 += delta >> 1;
				corr1 ^= corr;
				pix[i] += accum1;
				pix[i] ^= corr1;

				if (delta & 1)
				{
					iVec = *(src_p++);
					iSeq = 0;
				}
				
				*d_p1 = pix[i];
			}
		}
	}
}

void
dukv_DecodeFrameV3 (uint8* src_p, uint32* dst_p, uint32 wb, uint32 hb,
		TFB_DuckVideoDeltas* deltas)
{
	int iVec;
	int iSeq;
	uint32 x, y;
	sint32 w;
	uint32* d_p;
	int i;

	iVec = *(src_p++);
	iSeq = 0;

	hb *= 2;
	w = wb * 4;

	for (y = 0; y < hb; ++y)
	{
		sint32 accum, delta, pix;

		d_p = dst_p + y * w;

		accum = 0;

		for (x = 0; x < wb; ++x)
		{
			// start with chroma delta
			delta = deltas->chromas[iVec][iSeq];
			iSeq += 2; // correctors ignored

			accum += delta >> 1;

			if (delta & DUCK_END_OF_SEQUENCE)
			{
				iVec = *(src_p++);
				iSeq = 0;
			}

			for (i = 0; i < 4; ++i, ++d_p)
			{
				if (y == 0)
					pix = 0;
				else
					pix = d_p[-w];

				// get next luma delta
				delta = deltas->lumas[iVec][iSeq];
				iSeq += 2; // correctors ignored

				accum += delta >> 1;
				pix += accum;

				if (delta & DUCK_END_OF_SEQUENCE)
				{
					iVec = *(src_p++);
					iSeq = 0;
				}
				
				*d_p = pix;
			}
		}
	}
}

bool
dukv_OpenStream (TFB_DuckVideo* dukv)
{
	char filename[280];

	strcat (strcpy (filename, dukv->basename), ".duk");

	return (dukv->stream =
			res_OpenResFile (dukv->basedir, filename, "rb")) != NULL;
}

bool
dukv_ReadFrames (TFB_DuckVideo* dukv)
{
	char filename[280];
	uint32 i;
	uio_Stream *fp;

	strcat (strcpy (filename, dukv->basename), ".frm");

	if (!(fp = res_OpenResFile (dukv->basedir, filename, "rb")))
		return false;

	dukv->cframes = LengthResFile (fp) / sizeof (uint32);
	dukv->frames = (uint32*) HMalloc (dukv->cframes * sizeof (uint32));

	if ((uint32)ReadResFile (dukv->frames, sizeof (uint32), dukv->cframes, fp)
			!= dukv->cframes)
	{
		HFree (dukv->frames);
		dukv->frames = 0;
		return 0;
	}
	res_CloseResFile (fp);

	for (i = 0; i < dukv->cframes; ++i)
		dukv->frames[i] = UQM_SwapBE32 (dukv->frames[i]);

	return true;
}

bool
dukv_ReadVectors (TFB_DuckVideo* dukv, uint8* vectors)
{
	uio_Stream *fp;
	char filename[280];
	int ret;

	strcat (strcpy (filename, dukv->basename), ".tbl");

	if (!(fp = res_OpenResFile (dukv->basedir, filename, "rb")))
		return false;
	
	ret = ReadResFile (vectors, NUM_VEC_ITEMS * NUM_VECTORS, 1, fp);
	res_CloseResFile (fp);

	return ret;
}

bool
dukv_ReadHeader (TFB_DuckVideo* dukv, sint32* pl, sint32* pc)
{
	uio_Stream *fp;
	char filename[280];
	int ret;
	int i;
	TFB_DuckVideoHeader hdr;

	strcat (strcpy (filename, dukv->basename), ".hdr");

	if (!(fp = res_OpenResFile (dukv->basedir, filename, "rb")))
		return false;
	
	ret = ReadResFile (&hdr, sizeof (hdr), 1, fp);
	res_CloseResFile (fp);
	if (!ret)
		return false;

	dukv->version = UQM_SwapBE32 (hdr.version);
	dukv->wb = UQM_SwapBE16 (hdr.wb);
	dukv->hb = UQM_SwapBE16 (hdr.hb);

	for (i = 0; i < 8; ++i)
	{
		pl[i] = (sint16) UQM_SwapBE16 (hdr.lumas[i]);
		pc[i] = (sint16) UQM_SwapBE16 (hdr.chromas[i]);
	}

	dukv->w = dukv->wb * 4;
	dukv->h = dukv->hb * 4;

	return true;
}

sint32
dukv_make_delta (sint32* protos, bool is_chroma, int i1, int i2)
{
	sint32 d1, d2;

	if (!is_chroma)
	{
		// 0x421 is (r,g,b)=(1,1,1) in 15bit pixel coding
		d1 = (protos[i1] >> 1) * 0x421;
		d2 = (protos[i2] >> 1) * 0x421;
		return ((d1 << 16) + d2) << 1;
	}
	else
	{
		d1 = (protos[i1] << 10) + protos[i2];
		return ((d1 << 16) + d1) << 1;
	}
}

sint32
dukv_make_corr (sint32* protos, bool is_chroma, int i1, int i2)
{
	sint32 d1, d2;

	if (!is_chroma)
	{
		d1 = (protos[i1] & 1) << 15;
		d2 = (protos[i2] & 1) << 15;
		return (d1 << 16) + d2;
	}
	else
	{
		return (i1 << 3) + i2;
	}
}

void
dukv_DecodeVector (uint8* vec, sint32* p, bool is_chroma, sint32* deltas)
{
	int citems = vec[0];
	int i;

	for (i = 0; i < citems; i += 2, vec += 2, deltas += 2)
	{
		sint32 d = dukv_make_delta (p, is_chroma, vec[1], vec[2]);

		if (i == citems - 2)
			d |= DUCK_END_OF_SEQUENCE;

		deltas[0] = d;
		deltas[1] = dukv_make_corr (p, is_chroma, vec[1], vec[2]);
	}
	
}

void
dukv_InitDeltas (TFB_DuckVideo* dukv, uint8* vectors, sint32* pl, sint32* pc)
{
	int i;

	for (i = 0; i < NUM_VECTORS; ++i)
	{
		uint8* vector = vectors + i * NUM_VEC_ITEMS;
		dukv_DecodeVector (vector, pl, false, dukv->d.lumas[i]);
		dukv_DecodeVector (vector, pc, true, dukv->d.chromas[i]);
	}
}

__inline__ uint32
dukv_PixelConv (uint16 pix, const TFB_VideoFormat* fmt)
{
	uint32 r, g, b;

	r = (pix >> 7) & 0xf8;
	g = (pix >> 2) & 0xf8;
	b = (pix << 3) & 0xf8;

	return
		((r >> fmt->Rloss) << fmt->Rshift) |
		((g >> fmt->Gloss) << fmt->Gshift) |
		((b >> fmt->Bloss) << fmt->Bshift);
}

void
dukv_RenderFrame (_THIS)
{
	TFB_DuckVideo* dukv = This->dec_data;
	const TFB_VideoFormat* fmt = This->format;
	uint32 h, x, y;
	uint32* dec = dukv->decbuf;

	h = dukv->h / 2;

	// separate bpp versions for speed
	switch (fmt->BytesPerPixel)
	{
	case 2:
	{
		for (y = 0; y < h; ++y)
		{
			uint16 *dst0, *dst1;

			dst0 = (uint16*) This->callbacks.GetCanvasLine (This, y * 2);
			dst1 = (uint16*) This->callbacks.GetCanvasLine (This, y * 2 + 1);

			for (x = 0; x < dukv->w; ++x, ++dec, ++dst0, ++dst1)
			{
				uint32 pair = *dec;
				*dst0 = dukv_PixelConv ((uint16)(pair >> 16), fmt);
				*dst1 = dukv_PixelConv ((uint16)(pair & 0xffff), fmt);
			}
		}
		break;
	}
	case 3:
	{
		for (y = 0; y < h; ++y)
		{
			uint8 *dst0, *dst1;

			dst0 = (uint8*) This->callbacks.GetCanvasLine (This, y * 2);
			dst1 = (uint8*) This->callbacks.GetCanvasLine (This, y * 2 + 1);

			for (x = 0; x < dukv->w; ++x, ++dec, dst0 += 3, dst1 += 3)
			{
				uint32 pair = *dec;
				*(uint32*)dst0 =
						dukv_PixelConv ((uint16)(pair >> 16), fmt);
				*(uint32*)dst1 =
						dukv_PixelConv ((uint16)(pair & 0xffff), fmt);
			}
		}
		break;
	}
	case 4:
	{
		for (y = 0; y < h; ++y)
		{
			uint32 *dst0, *dst1;

			dst0 = (uint32*) This->callbacks.GetCanvasLine (This, y * 2);
			dst1 = (uint32*) This->callbacks.GetCanvasLine (This, y * 2 + 1);

			for (x = 0; x < dukv->w; ++x, ++dec, ++dst0, ++dst1)
			{
				uint32 pair = *dec;
				*dst0 = dukv_PixelConv ((uint16)(pair >> 16), fmt);
				*dst1 = dukv_PixelConv ((uint16)(pair & 0xffff), fmt);
			}
		}
		break;
	}
	default:
		;
	}

}


int dukv_GetError (_THIS)
{
	return This->error;
}

bool
dukv_Init (_THIS, TFB_VideoFormat* fmt)
{
	This->format = fmt;
	This->audio_synced = 1;
	This->dec_data = HCalloc (sizeof (TFB_DuckVideo));
	return true;
}

void
dukv_Term (_THIS)
{
	//TFB_DuckVideo* dukv = This->dec_data;

	dukv_Close (This);
	HFree (This->dec_data);
}

bool
dukv_Open (_THIS, uio_DirHandle *dir, const char *filename)
{
	TFB_DuckVideo* dukv = This->dec_data;
	char* pext;
	sint32 lumas[8], chromas[8];
	uint8* vectors;
	
	dukv->basedir = dir;
	dukv->basename = HMalloc (strlen (filename) + 1);
	strcpy (dukv->basename, filename);
	pext = strrchr (dukv->basename, '.');
	if (pext) // strip extension
		*pext = 0;

	vectors = HMalloc (NUM_VEC_ITEMS * NUM_VECTORS);

	if (!dukv_OpenStream (dukv)
			|| !dukv_ReadFrames (dukv)
			|| !dukv_ReadHeader (dukv, lumas, chromas)
			|| !dukv_ReadVectors (dukv, vectors))
	{
		HFree (vectors);
		dukv_Close (This);
		dukv->last_error = dukve_BadFile;
		return false;
	}

	dukv_InitDeltas (dukv, vectors, lumas, chromas);
	HFree (vectors);

	This->w = dukv->w;
	This->h = dukv->h;
	This->length = (float) dukv->cframes / DUCK_GENERAL_FPS;
	This->frame_count = dukv->cframes;
	This->max_frame_wait =
			(uint32) (1000.0f / DUCK_GENERAL_FPS * 1.1);

	dukv->inbuf = HMalloc (DUCK_MAX_FRAME_SIZE);
	dukv->decbuf = HMalloc (dukv->w * dukv->h * sizeof (uint16));

	return true;
}

void
dukv_Close (_THIS)
{
	TFB_DuckVideo* dukv = This->dec_data;

	if (dukv->basename)
	{
		HFree (dukv->basename);
		dukv->basename = NULL;
	}
	if (dukv->frames)
	{
		HFree (dukv->frames);
		dukv->frames = NULL;
	}
	if (dukv->stream)
	{
		res_CloseResFile (dukv->stream);
		dukv->stream = NULL;
	}
	if (dukv->inbuf)
	{
		HFree (dukv->inbuf);
		dukv->inbuf = NULL;
	}
	if (dukv->decbuf)
	{
		HFree (dukv->decbuf);
		dukv->decbuf = NULL;
	}
}

int
dukv_DecodeNext (_THIS)
{
	TFB_DuckVideo* dukv = This->dec_data;
	uint32 fh[2];
	uint32 vofs;
	uint32 vsize;
	uint16 ver;

	if (!dukv->stream || dukv->iframe >= dukv->cframes)
		return 0;

	SeekResFile (dukv->stream, dukv->frames[dukv->iframe], SEEK_SET);
	if (ReadResFile (&fh, sizeof (fh), 1, dukv->stream) != 1)
	{
		dukv->last_error = dukve_EOF;
		return 0;
	}

	vofs = UQM_SwapBE32 (fh[0]);
	vsize = UQM_SwapBE32 (fh[1]);
	if (vsize > DUCK_MAX_FRAME_SIZE)
	{
		dukv->last_error = dukve_OutOfBuf;
		return -1;
	}

	SeekResFile (dukv->stream, vofs, SEEK_CUR);
	if ((uint32)ReadResFile (dukv->inbuf, 1, vsize, dukv->stream) != vsize)
	{
		dukv->last_error = dukve_EOF;
		return 0;
	}

	ver = UQM_SwapBE16 (*(uint16*)dukv->inbuf);
	if (ver == 0x0300)
		dukv_DecodeFrameV3 (dukv->inbuf + 0x10, dukv->decbuf,
				dukv->wb, dukv->hb, &dukv->d);
	else
		dukv_DecodeFrame (dukv->inbuf + 0x10, dukv->decbuf,
				dukv->wb, dukv->hb, &dukv->d);

	dukv->iframe++;

	dukv_RenderFrame (This);

	return 1;
}

uint32
dukv_SeekFrame (_THIS, uint32 frame)
{
	TFB_DuckVideo* dukv = This->dec_data;
	
	if (frame > dukv->cframes)
		frame = dukv->cframes; // EOS

	return dukv->iframe = frame;
}

float
dukv_SeekTime (_THIS, float time)
{
	//TFB_DuckVideo* dukv = This->dec_data;
	uint32 frame = (uint32) (time * DUCK_GENERAL_FPS);
	
	return (float) dukv_SeekFrame (This, frame) / DUCK_GENERAL_FPS;
}

uint32
dukv_GetFrame (_THIS)
{
	TFB_DuckVideo* dukv = This->dec_data;

	return dukv->iframe;
}

float
dukv_GetTime (_THIS)
{
	TFB_DuckVideo* dukv = This->dec_data;
	
	return (float) dukv->iframe / DUCK_GENERAL_FPS;
}
