/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2000 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

/* NOTE by Mika: Wav loader, (c) Creative, taken from their OpenAL
 *               implementation and modified.
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "libs/misc.h"
#include "wav.h"
#include "decoder.h"

#if defined _MSC_VER
	#pragma pack (push,1) 							/* Turn off alignment */
#elif defined __GNUC__
	#define PADOFF_VAR __attribute__((packed))
#endif

#ifndef PADOFF_VAR
	#define PADOFF_VAR
#endif

typedef struct                                  /* WAV File-header */
{
  uint8  Id[4]			PADOFF_VAR;
  sint32  Size				PADOFF_VAR;
  uint8  Type[4]			PADOFF_VAR;
} WAVFileHdr_Struct;

typedef struct                                  /* WAV Fmt-header */
{
  uint16 Format			PADOFF_VAR;
  uint16 Channels			PADOFF_VAR;
  uint32   SamplesPerSec	PADOFF_VAR;
  uint32   BytesPerSec		PADOFF_VAR;
  uint16 BlockAlign		PADOFF_VAR;
  uint16 BitsPerSample	PADOFF_VAR;
} WAVFmtHdr_Struct;

typedef struct									/* WAV FmtEx-header */
{
  uint16 Size				PADOFF_VAR;
  uint16 SamplesPerBlock	PADOFF_VAR;
} WAVFmtExHdr_Struct;

typedef struct                                  /* WAV Smpl-header */
{
  uint32   Manufacturer		PADOFF_VAR;
  uint32   Product			PADOFF_VAR;
  uint32   SamplePeriod		PADOFF_VAR;
  uint32   Note				PADOFF_VAR;
  uint32   FineTune			PADOFF_VAR;
  uint32   SMPTEFormat		PADOFF_VAR;
  uint32   SMPTEOffest		PADOFF_VAR;
  uint32   Loops			PADOFF_VAR;
  uint32   SamplerData		PADOFF_VAR;
  struct
  {
    uint32 Identifier		PADOFF_VAR;
    uint32 Type				PADOFF_VAR;
    uint32 Start			PADOFF_VAR;
    uint32 End				PADOFF_VAR;
    uint32 Fraction			PADOFF_VAR;
    uint32 Count			PADOFF_VAR;
  }      Loop[1]			PADOFF_VAR;
} WAVSmplHdr_Struct;

typedef struct                                  /* WAV Chunk-header */
{
  uint8  Id[4]			PADOFF_VAR;
  uint32   Size				PADOFF_VAR;
} WAVChunkHdr_Struct;


#ifdef PADOFF_VAR			    				/* Default alignment */
	#undef PADOFF_VAR
#endif

#if defined _MSC_VER
	#pragma pack (pop)
#endif

/* loads the WAV file in little endian byte order */
void LoadWAVFile (char *file, uint32 *format, void **data, uint32 *size, uint32 *freq, bool *loop)
{
	WAVChunkHdr_Struct ChunkHdr;
	WAVFmtExHdr_Struct FmtExHdr;
	WAVFileHdr_Struct FileHdr;
	WAVSmplHdr_Struct SmplHdr;
	WAVFmtHdr_Struct FmtHdr;
	FILE *Stream;
	
	*format=decoder_formats.mono16;
	*data=NULL;
	*size=0;
	*freq=22050;
	*loop=false;
	if (file)
	{
		Stream=fopen(file,"rb");
		if (Stream)
		{
			fread(&FileHdr,1,sizeof(WAVFileHdr_Struct),Stream);
			FileHdr.Size=((FileHdr.Size+1)&~1)-4;
			while ((FileHdr.Size!=0)&&(fread(&ChunkHdr,1,sizeof(WAVChunkHdr_Struct),Stream)))
			{
				if (!memcmp(ChunkHdr.Id,"fmt ",4))
				{
					fread(&FmtHdr,1,sizeof(WAVFmtHdr_Struct),Stream);
					if (FmtHdr.Format==0x0001)
					{
						*format=(FmtHdr.Channels == 1 ?
								(FmtHdr.BitsPerSample == 8 ?
									decoder_formats.mono8
									: decoder_formats.mono16)
								: (FmtHdr.BitsPerSample == 8 ?
									decoder_formats.stereo8
									: decoder_formats.stereo16)
								);
						*freq=FmtHdr.SamplesPerSec;
						fseek(Stream,ChunkHdr.Size-sizeof(WAVFmtHdr_Struct),SEEK_CUR);
					} 
					else
					{
						fread(&FmtExHdr,1,sizeof(WAVFmtExHdr_Struct),Stream);
						fseek(Stream,ChunkHdr.Size-sizeof(WAVFmtHdr_Struct)-
								sizeof(WAVFmtExHdr_Struct),SEEK_CUR);
					}
				}
				else if (!memcmp(ChunkHdr.Id,"data",4))
				{
					if (FmtHdr.Format==0x0001)
					{
						*size=ChunkHdr.Size;
						*data=HMalloc(ChunkHdr.Size+31);
						if (*data)
							fread(*data,FmtHdr.BlockAlign,
									ChunkHdr.Size/FmtHdr.BlockAlign,Stream);
						memset(((char *)*data)+ChunkHdr.Size,0,31);
					}
					else if (FmtHdr.Format==0x0011)
					{
						//IMA ADPCM
					}
					else if (FmtHdr.Format==0x0055)
					{
						//MP3 WAVE
					}
				}
				else if (!memcmp(ChunkHdr.Id,"smpl",4))
				{
					fread(&SmplHdr,1,sizeof(WAVSmplHdr_Struct),Stream);
					*loop = (SmplHdr.Loops ? true : false);
					fseek(Stream,ChunkHdr.Size-sizeof(WAVSmplHdr_Struct),SEEK_CUR);
				}
				else
					fseek(Stream,ChunkHdr.Size,SEEK_CUR);

				fseek(Stream,ChunkHdr.Size&1,SEEK_CUR);
				FileHdr.Size-=(((ChunkHdr.Size+1)&~1)+8);
			}
			fclose(Stream);
		}		
	}
}
