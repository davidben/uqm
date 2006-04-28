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

/* Wave decoder (.wav adapter)
 * Code is based on Creative's Win32 OpenAL implementation.
 */

#include <stdio.h>
#include <errno.h>
#include "port.h"
#include "types.h"
#include "uio.h"
#include "endian_uqm.h"
#include "libs/misc.h"
#include "libs/log.h"
#include "wav.h"

#define RIFF 0x46464952 /* "RIFF" */
#define WAVE 0x45564157 /* "WAVE" */
#define FMT  0x20746D66 /* "fmt " */
#define DATA 0x61746164 /* "data" */

typedef struct
{
	uint32 Id;
	sint32 Size;
	uint32 Type;
} WAVFileHdr_Struct;

typedef struct
{
	uint16 Format;
	uint16 Channels;
	uint32 SamplesPerSec;
	uint32 BytesPerSec;
	uint16 BlockAlign;
	uint16 BitsPerSample;
} WAVFmtHdr_Struct;

typedef struct
{
	uint32 Id;
	uint32 Size;
} WAVChunkHdr_Struct;


#define THIS_PTR TFB_SoundDecoder* This

static const char* wava_GetName (void);
static bool wava_InitModule (int flags, const TFB_DecoderFormats*);
static void wava_TermModule (void);
static uint32 wava_GetStructSize (void);
static int wava_GetError (THIS_PTR);
static bool wava_Init (THIS_PTR);
static void wava_Term (THIS_PTR);
static bool wava_Open (THIS_PTR, uio_DirHandle *dir, const char *filename);
static void wava_Close (THIS_PTR);
static int wava_Decode (THIS_PTR, void* buf, sint32 bufsize);
static uint32 wava_Seek (THIS_PTR, uint32 pcm_pos);
static uint32 wava_GetFrame (THIS_PTR);

TFB_SoundDecoderFuncs wava_DecoderVtbl = 
{
	wava_GetName,
	wava_InitModule,
	wava_TermModule,
	wava_GetStructSize,
	wava_GetError,
	wava_Init,
	wava_Term,
	wava_Open,
	wava_Close,
	wava_Decode,
	wava_Seek,
	wava_GetFrame,
};

typedef struct tfb_wavesounddecoder
{
	// always the first member
	TFB_SoundDecoder decoder;

	// private
	sint32 last_error;
	uio_Stream *fp;
	WAVFmtHdr_Struct FmtHdr;
	uint32 data_ofs;
	uint32 data_size;
	uint32 max_pcm;
	uint32 cur_pcm;

} TFB_WaveSoundDecoder;

static const TFB_DecoderFormats* wava_formats = NULL;


static const char*
wava_GetName (void)
{
	return "Wave";
}

static bool
wava_InitModule (int flags, const TFB_DecoderFormats* fmts)
{
	wava_formats = fmts;
	return true;
	
	(void)flags;	// laugh at compiler warning
}

static void
wava_TermModule (void)
{
	// no specific module term
}

static uint32
wava_GetStructSize (void)
{
	return sizeof (TFB_WaveSoundDecoder);
}

static int
wava_GetError (THIS_PTR)
{
	TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;
	int ret = wava->last_error;
	wava->last_error = 0;
	return ret;
}

static bool
wava_Init (THIS_PTR)
{
	//TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;
	This->need_swap = wava_formats->want_big_endian;
	return true;
}

static void
wava_Term (THIS_PTR)
{
	//TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;
	wava_Close (This); // ensure cleanup
}

static bool
wava_readFileHeader (TFB_WaveSoundDecoder* wava, WAVFileHdr_Struct* hdr)
{
	if (!uio_fread (&hdr->Id, 4, 1, wava->fp) ||
			!uio_fread (&hdr->Size, 4, 1, wava->fp) ||
			!uio_fread (&hdr->Type, 4, 1, wava->fp))
	{
		wava->last_error = errno;
		return false;
	}
	hdr->Id   = UQM_SwapLE32 (hdr->Id);
	hdr->Size = UQM_SwapLE32 (hdr->Size);
	hdr->Type = UQM_SwapLE32 (hdr->Type);

	return true;
}

static bool
wava_readChunkHeader (TFB_WaveSoundDecoder* wava, WAVChunkHdr_Struct* chunk)
{
	if (!uio_fread (&chunk->Id, 4, 1, wava->fp) ||
			!uio_fread (&chunk->Size, 4, 1, wava->fp))
		return false;

	chunk->Id   = UQM_SwapLE32 (chunk->Id);
	chunk->Size = UQM_SwapLE32 (chunk->Size);

	return true;
}

static bool
wava_readFormatHeader (TFB_WaveSoundDecoder* wava, WAVFmtHdr_Struct* fmt)
{
	if (!uio_fread (&fmt->Format, 2, 1, wava->fp) ||
			!uio_fread (&fmt->Channels, 2, 1, wava->fp) ||
			!uio_fread (&fmt->SamplesPerSec, 4, 1, wava->fp) ||
			!uio_fread (&fmt->BytesPerSec, 4, 1, wava->fp) ||
			!uio_fread (&fmt->BlockAlign, 2, 1, wava->fp) ||
			!uio_fread (&fmt->BitsPerSample, 2, 1, wava->fp))
		return false;

	fmt->Format        = UQM_SwapLE16 (fmt->Format);
	fmt->Channels      = UQM_SwapLE16 (fmt->Channels);
	fmt->SamplesPerSec = UQM_SwapLE32 (fmt->SamplesPerSec);
	fmt->BytesPerSec   = UQM_SwapLE32 (fmt->BytesPerSec);
	fmt->BlockAlign    = UQM_SwapLE16 (fmt->BlockAlign);
	fmt->BitsPerSample = UQM_SwapLE16 (fmt->BitsPerSample);

	return true;
}

static bool
wava_Open (THIS_PTR, uio_DirHandle *dir, const char *filename)
{
	TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;
	WAVFileHdr_Struct FileHdr;
	WAVChunkHdr_Struct ChunkHdr;

	wava->fp = uio_fopen (dir, filename, "rb");
	if (!wava->fp)
	{
		wava->last_error = errno;
		return false;
	}

	wava->data_size = 0;
	wava->data_ofs = 0;

	// read wave header
	if (!wava_readFileHeader (wava, &FileHdr))
	{
		wava->last_error = errno;
		wava_Close (This);
		return false;
	}
	if (FileHdr.Id != RIFF || FileHdr.Type != WAVE)
	{
		log_add (log_Warning, "wava_Open(): "
				"not a wave file, ID 0x%08x, Type 0x%08x",
				FileHdr.Id, FileHdr.Type);
		wava_Close (This);
		return false;
	}

	for (FileHdr.Size = ((FileHdr.Size + 1) & ~1) - 4; FileHdr.Size != 0;
			FileHdr.Size -= (((ChunkHdr.Size + 1) & ~1) + 8))
	{
		if (!wava_readChunkHeader (wava, &ChunkHdr))
		{
			wava->last_error = errno;
			wava_Close (This);
			return false;
		}

		if (ChunkHdr.Id == FMT)
		{
			if (!wava_readFormatHeader (wava, &wava->FmtHdr))
			{
				wava->last_error = errno;
				wava_Close (This);
				return false;
			}
			uio_fseek (wava->fp, ChunkHdr.Size - 16, SEEK_CUR);
		}
		else
		{
			if (ChunkHdr.Id == DATA)
			{
				wava->data_size = ChunkHdr.Size;
				wava->data_ofs = uio_ftell (wava->fp);
			}
			uio_fseek (wava->fp, ChunkHdr.Size, SEEK_CUR);
		}

		uio_fseek (wava->fp, ChunkHdr.Size & 1, SEEK_CUR);
	}

	if (!wava->data_size || !wava->data_ofs)
	{
		log_add (log_Warning, "wava_Open(): bad wave file,"
				" no DATA chunk found");
		wava_Close (This);
		return false;
	}

	if (wava->FmtHdr.Format == 0x0001)
	{
		This->format = (wava->FmtHdr.Channels == 1 ?
				(wava->FmtHdr.BitsPerSample == 8 ?
					wava_formats->mono8 : wava_formats->mono16)
				:
				(wava->FmtHdr.BitsPerSample == 8 ?
					wava_formats->stereo8 : wava_formats->stereo16)
				);
		This->frequency = wava->FmtHdr.SamplesPerSec;
	} 
	else
	{
		log_add (log_Warning, "wava_Open(): unsupported format %x",
				wava->FmtHdr.Format);
		wava_Close (This);
		return false;
	}

	uio_fseek (wava->fp, wava->data_ofs, SEEK_SET);
	wava->max_pcm = wava->data_size / wava->FmtHdr.BlockAlign;
	wava->cur_pcm = 0;
	This->length = (float) wava->max_pcm / wava->FmtHdr.SamplesPerSec;
	wava->last_error = 0;

	return true;
}

static void
wava_Close (THIS_PTR)
{
	TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;

	if (wava->fp)
	{
		uio_fclose (wava->fp);
		wava->fp = NULL;
	}
}

static int
wava_Decode (THIS_PTR, void* buf, sint32 bufsize)
{
	TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;
	uint32 dec_pcm;

	dec_pcm = bufsize / wava->FmtHdr.BlockAlign;
	if (dec_pcm > wava->max_pcm - wava->cur_pcm)
		dec_pcm = wava->max_pcm - wava->cur_pcm;

	dec_pcm = uio_fread (buf, wava->FmtHdr.BlockAlign, dec_pcm, wava->fp);
	wava->cur_pcm += dec_pcm;
	
	return dec_pcm * wava->FmtHdr.BlockAlign;
}

static uint32
wava_Seek (THIS_PTR, uint32 pcm_pos)
{
	TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;

	if (pcm_pos > wava->max_pcm)
		pcm_pos = wava->max_pcm;
	wava->cur_pcm = pcm_pos;
	uio_fseek (wava->fp,
			wava->data_ofs + pcm_pos * wava->FmtHdr.BlockAlign,
			SEEK_SET);

	return pcm_pos;
}

static uint32
wava_GetFrame (THIS_PTR)
{
	//TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;
	return 0; // only 1 frame for now

	(void)This;	// laugh at compiler warning
}
