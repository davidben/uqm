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

/* .duk sound track decoder
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "libs/misc.h"
#include "dukaud.h"
#include "decoder.h"
#include "endian_uqm.h"

#define DATA_BUF_SIZE	  0x8000
#define DUCK_GENERAL_FPS  14.622f

static sint32 duka_error = 0;

typedef struct
{
	uint32 audsize;
	uint32 vidsize;
} DukAud_FrameHeader;

typedef struct
{
	uint16 magic;  // always 0xf77f
	uint16 numsamples;
	uint16 tag;
	uint16 indices[2];  // initial indices for channels
} DukAud_AudSubframe;

sint32
duka_getError ()
{
	sint32 ret = duka_error;
	duka_error = dukae_None;
	return ret;
}

sint32
duka_readAudFrameHeader (DukAud_Track* track, uint32 iframe,
		DukAud_AudSubframe* aud)
{
	DukAud_FrameHeader hdr;

	uio_fseek (track->duk, track->frames[iframe], SEEK_SET);
	if (uio_fread (&hdr, sizeof(hdr), 1, track->duk) != 1)
	{
		duka_error = errno;
		return dukae_BadFile;
	}
	hdr.audsize = UQM_SwapBE32 (hdr.audsize);

	if (uio_fread (aud, sizeof(*aud), 1, track->duk) != 1)
	{
		duka_error = errno;
		return dukae_BadFile;
	}

	aud->magic = UQM_SwapBE16 (aud->magic);
	if (aud->magic != 0xf77f)
		return duka_error = dukae_BadFile;
	
	aud->numsamples = UQM_SwapBE16 (aud->numsamples);
	aud->tag = UQM_SwapBE16 (aud->tag);
	aud->indices[0] = UQM_SwapBE16 (aud->indices[0]);
	aud->indices[1] = UQM_SwapBE16 (aud->indices[1]);

	return 0;
}

DukAud_Track*
duka_openTrack (uio_DirHandle *dir, const char *file)
{
	DukAud_Track* track;
	uio_Stream* duk;
	uio_Stream* frm;
	DukAud_AudSubframe aud;
	char filename[256];
	uint32 filelen;
	size_t cread;
	uint32 i;

	filelen = strlen (file);
	if (filelen > sizeof (filename) - 1)
		return NULL;
	strcpy (filename, file);

	duk = uio_fopen (dir, filename, "rb");
	if (!duk)
	{
		duka_error = errno;
		return NULL;
	}

	strcpy (filename + filelen - 3, "frm");
	frm = uio_fopen (dir, filename, "rb");
	if (!frm)
	{
		duka_error = errno;
		uio_fclose (duk);
		return NULL;
	}

	track = (DukAud_Track *) HCalloc (sizeof (DukAud_Track));
	track->duk = duk;

	uio_fseek (frm, 0, SEEK_END);
	track->cframes = uio_ftell (frm) / sizeof (uint32);
	uio_fseek (frm, 0, SEEK_SET);
	if (!track->cframes)
	{
		duka_error = dukae_BadFile;
		uio_fclose (frm);
		duka_closeTrack (track);
		return NULL;
	}
	
	track->frames = (uint32*) HMalloc (track->cframes * sizeof (uint32));
	cread = uio_fread (track->frames, sizeof (uint32), track->cframes, frm);
	uio_fclose (frm);
	if (cread != track->cframes)
	{
		duka_error = dukae_BadFile;
		duka_closeTrack (track);
		return NULL;
	}

	for (i = 0; i < track->cframes; ++i)
		track->frames[i] = UQM_SwapBE32 (track->frames[i]);

	if (duka_readAudFrameHeader (track, 0, &aud) < 0)
	{
		duka_closeTrack (track);
		return NULL;
	}

	track->rate = 22050;
	track->channels = 2;
	track->pcm_frame = aud.numsamples;
	track->data = HMalloc (DATA_BUF_SIZE);
	track->maxdata = DATA_BUF_SIZE;

	// estimate
	track->length = (float) track->cframes / DUCK_GENERAL_FPS;

	duka_error = 0;

	return track;
}

void
duka_closeTrack (DukAud_Track* track)
{
	if (track->data)
		HFree (track->data);
	if (track->duk)
		uio_fclose (track->duk);
	HFree (track);
	duka_error = 0;
}

bool
duka_seekTrack (DukAud_Track* track, float pos /* in seconds */)
{
	if (pos < 0 || pos > track->length)
		return false;

	track->iframe = (uint32) (pos * DUCK_GENERAL_FPS);
	track->cbdata = 0;
	track->dataofs = 0;
	track->predictors[0] = 0;
	track->predictors[1] = 0;

	return true;
}

// This table is from one of the files that came with the original 3do source
// It's slightly different from the data used by MPlayer.
static int adpcm_step[89] = {
		0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xF,
		0x10, 0x12, 0x13, 0x15, 0x17, 0x1A, 0x1C, 0x1F,
		0x22, 0x26, 0x29, 0x2E, 0x32, 0x37, 0x3D, 0x43,
		0x4A, 0x51, 0x59, 0x62, 0x6C, 0x76, 0x82, 0x8F,
		0x9E, 0xAD, 0xBF, 0xD2, 0xE7, 0xFE, 0x117, 0x133,
		0x152, 0x174, 0x199, 0x1C2, 0x1EF, 0x220, 0x256, 0x292,
		0x2D4, 0x31D, 0x36C, 0x3C4, 0x424, 0x48E, 0x503, 0x583,
		0x610, 0x6AC, 0x756, 0x812, 0x8E1, 0x9C4, 0xABE, 0xBD1,
		0xCFF, 0xE4C, 0xFBA, 0x114D, 0x1308, 0x14EF, 0x1707, 0x1954,
		0x1BDD, 0x1EA6, 0x21B7, 0x2516,
		0x28CB, 0x2CDF, 0x315C, 0x364C,
		0x3BBA, 0x41B2, 0x4844, 0x4F7E,
		0x5771, 0x6030, 0x69CE, 0x7463,
		0x7FFF
		};


// *** BEGIN part copied from MPlayer ***
// (some little changes)

#if 0
// pertinent tables for IMA ADPCM
static int adpcm_step[89] = {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
		19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
		50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
		130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
		337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
		876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
		2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
		5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
		};
#endif

static int adpcm_index[16] = {
		-1, -1, -1, -1, 2, 4, 6, 8,
		-1, -1, -1, -1, 2, 4, 6, 8
		};

// clamp a number between 0 and 88
#define CLAMP_0_TO_88(x) \
		if ((x) < 0) (x) = 0; else if ((x) > 88) (x) = 88;

// clamp a number within a signed 16-bit range
#define CLAMP_S16(x) \
		if ((x) < -32768) \
			(x) = -32768; \
		else if ((x) > 32767) \
			(x) = 32767;

static void
decode_nibbles (sint16 *output, sint32 output_size, sint32 channels,
		sint32* predictors, uint16* indices)
{
	sint32 step[2];
	sint32 index[2];
	sint32 diff;
	sint32 i;
	int sign;
	sint32 delta;
	int channel_number = 0;

	channels -= 1;
	index[0] = indices[0];
	index[1] = indices[1];
	step[0] = adpcm_step[index[0]];
	step[1] = adpcm_step[index[1]];

	for (i = 0; i < output_size; i++)
	{
		delta = output[i];

		index[channel_number] += adpcm_index[delta];
		CLAMP_0_TO_88(index[channel_number]);

		sign = delta & 8;
		delta = delta & 7;

#if 0
		// fast approximation, used in most decoders
		diff = step[channel_number] >> 3;
		if (delta & 4) diff += step[channel_number];
		if (delta & 2) diff += step[channel_number] >> 1;
		if (delta & 1) diff += step[channel_number] >> 2;
#else
		// real thing
//		diff = ((signed)delta + 0.5) * step[channel_number] / 4;
		diff = (((delta << 1) + 1) * step[channel_number]) >> 3;
#endif

		if (sign)
			predictors[channel_number] -= diff;
		else
			predictors[channel_number] += diff;

		CLAMP_S16(predictors[channel_number]);
		output[i] = predictors[channel_number];
		step[channel_number] = adpcm_step[index[channel_number]];

		// toggle channel
		channel_number ^= channels;
	}
}
// *** END part copied from MPlayer ***

sint32
duka_decodeFrame (DukAud_Track* track, DukAud_AudSubframe* header,
		uint8* input)
{
	uint8* inend;
	sint16* output;
	sint16* outptr;
	sint32 outputsize;

	outputsize = header->numsamples * 2 * sizeof (sint16);
	outptr = output = (sint16*) ((uint8*)track->data + track->cbdata);
	
	for (inend = input + header->numsamples; input < inend; ++input)
	{
		*(outptr++) = *input >> 4;
		*(outptr++) = *input & 0x0f;
	}
	
	decode_nibbles (output, header->numsamples * 2, track->channels,
			track->predictors, header->indices);

	track->cbdata += outputsize;

	return outputsize;
}


sint32
duka_readNextFrame (DukAud_Track* track)
{
	DukAud_FrameHeader hdr;
	DukAud_AudSubframe* aud;
	uint8* p;

	uio_fseek (track->duk, track->frames[track->iframe], SEEK_SET);
	if (uio_fread (&hdr, sizeof(hdr), 1, track->duk) != 1)
	{
		duka_error = errno;
		return dukae_BadFile;
	}
	hdr.audsize = UQM_SwapBE32 (hdr.audsize);

	// dump encoded data at the end of the buffer aligned on 8-byte
	p = ((uint8*)track->data + track->maxdata - ((hdr.audsize + 7) & (-8)));
	if (uio_fread (p, 1, hdr.audsize, track->duk) != hdr.audsize)
	{
		duka_error = errno;
		return dukae_BadFile;
	}
	aud = (DukAud_AudSubframe*) p;
	p += sizeof(DukAud_AudSubframe);

	aud->magic = UQM_SwapBE16 (aud->magic);
	if (aud->magic != 0xf77f)
		return duka_error = dukae_BadFile;
	
	aud->numsamples = UQM_SwapBE16 (aud->numsamples);
	aud->tag = UQM_SwapBE16 (aud->tag);
	aud->indices[0] = UQM_SwapBE16 (aud->indices[0]);
	aud->indices[1] = UQM_SwapBE16 (aud->indices[1]);

	track->iframe++;

	return duka_decodeFrame (track, aud, p);
}

static sint32
duka_stuffBuffer (DukAud_Track* track, void* buf, sint32 bufsize)
{
	sint32 dataleft;

	dataleft = track->cbdata - track->dataofs;
	if (dataleft > 0)
	{
		if (dataleft > bufsize)
			dataleft = bufsize & (-4);
		memcpy (buf, (uint8*)track->data + track->dataofs, dataleft);
		track->dataofs += dataleft;
	}

	if (track->cbdata > 0 && track->dataofs >= track->cbdata)
		track->cbdata = track->dataofs = 0; // reset for new data

	return dataleft;
}

sint32
duka_readData (DukAud_Track* track, void* buf, sint32 bufsize)
{
	sint32 stuffed;
	sint32 total = 0;

	if (bufsize <= 0)
		return duka_error = dukae_BadArg;

	do
	{
		stuffed = duka_stuffBuffer (track, buf, bufsize);
		((uint8*)buf) += stuffed;
		bufsize -= stuffed;
		total += stuffed;
	
		if (bufsize > 0 && track->iframe < track->cframes)
		{
			stuffed = duka_readNextFrame (track);
			if (stuffed <= 0)
				return stuffed;
		}
	} while (bufsize > 0 && track->iframe < track->cframes);

	return total;
}

uint32
duka_getFrame (DukAud_Track* track)
{
	// if there is nothing buffered return the actual current frame
	//  otherwise return previous
	return track->dataofs == track->cbdata ?
			track->iframe : track->iframe - 1;
}

uint32
duka_getFrameSize (DukAud_Track* track)
{
	return track->pcm_frame * track->channels * sizeof (uint16);
}
