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

/* Loader for .wav files
 * Code is based on Creative's Win32 OpenAL implementation.
 */

#include <stdio.h>
#include <memory.h>
#include "libs/misc.h"
#include "wav.h"
#include "decoder.h"
#include "endian_uqm.h"

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


void
LoadWAVFile (const char *file, uint32 *format, void **data, uint32 *size,
			 uint32 *freq, bool want_big_endian)
{
	WAVChunkHdr_Struct ChunkHdr;
	WAVFileHdr_Struct FileHdr;
	WAVFmtHdr_Struct FmtHdr;
	FILE *fp;
	
	*format = decoder_formats.mono16;
	*data = NULL;
	*size = 0;
	*freq = 22050;
	
	fp = fopen (file, "rb");
	if (fp)
	{
		fread(&FileHdr.Id, 4, 1, fp);
		FileHdr.Id = UQM_SwapLE32 (FileHdr.Id);
		fread(&FileHdr.Size, 4, 1, fp);
		FileHdr.Size = UQM_SwapLE32 (FileHdr.Size);
		fread(&FileHdr.Type, 4, 1, fp);
		FileHdr.Type = UQM_SwapLE32 (FileHdr.Type);

		FileHdr.Size = ((FileHdr.Size + 1) & ~1) - 4;
		while (FileHdr.Size != 0)
		{
			if (!fread (&ChunkHdr.Id, 4, 1, fp))
				break;
			ChunkHdr.Id = UQM_SwapLE32 (ChunkHdr.Id);
			if (!fread (&ChunkHdr.Size, 4, 1, fp))
				break;
			ChunkHdr.Size = UQM_SwapLE32 (ChunkHdr.Size);

			if (ChunkHdr.Id == FMT)
			{
				fread (&FmtHdr.Format, 2, 1, fp);
				FmtHdr.Format = UQM_SwapLE16 (FmtHdr.Format);
				fread (&FmtHdr.Channels, 2, 1, fp);
				FmtHdr.Channels = UQM_SwapLE16 (FmtHdr.Channels);
				fread (&FmtHdr.SamplesPerSec, 4, 1, fp);
				FmtHdr.SamplesPerSec = UQM_SwapLE32 (FmtHdr.SamplesPerSec);
				fread (&FmtHdr.BytesPerSec, 4, 1, fp);
				FmtHdr.BytesPerSec = UQM_SwapLE32 (FmtHdr.BytesPerSec);
				fread (&FmtHdr.BlockAlign, 2, 1, fp);
				FmtHdr.BlockAlign = UQM_SwapLE16 (FmtHdr.BlockAlign);
				fread (&FmtHdr.BitsPerSample, 2, 1, fp);
				FmtHdr.BitsPerSample = UQM_SwapLE16 (FmtHdr.BitsPerSample);

				if (FmtHdr.Format == 0x0001)
				{
					*format=(FmtHdr.Channels == 1 ?
						(FmtHdr.BitsPerSample == 8 ?
						decoder_formats.mono8
						: decoder_formats.mono16)
						: (FmtHdr.BitsPerSample == 8 ?
						decoder_formats.stereo8
						: decoder_formats.stereo16)
						);
					*freq = FmtHdr.SamplesPerSec;
					fseek (fp, ChunkHdr.Size - 16, SEEK_CUR);
				} 
				else
				{
					fprintf (stderr, "LoadWAVFile(): unsupported format %x\n", FmtHdr.Format);
					fclose (fp);
					return;
				}
			}
			else if (ChunkHdr.Id == DATA)
			{
				*size = ChunkHdr.Size;
				if ((*data = HMalloc (ChunkHdr.Size + 31)))
					fread (*data, FmtHdr.BlockAlign, ChunkHdr.Size / FmtHdr.BlockAlign, fp);
				memset(((char *)*data) + ChunkHdr.Size, 0, 31);
			}
			else
			{
				fseek(fp, ChunkHdr.Size, SEEK_CUR);
			}

			fseek (fp, ChunkHdr.Size & 1, SEEK_CUR);
			FileHdr.Size -= (((ChunkHdr.Size + 1) & ~1) + 8);
		}
		fclose(fp);
	}

	if (want_big_endian &&
		(*format == decoder_formats.stereo16 || *format == decoder_formats.mono16))
	{
		SoundDecoder_SwapWords (*data, *size);
	}

	if (*size == 0)
		fprintf (stderr, "LoadWAVFile(): loading %s failed!\n", file);
}
