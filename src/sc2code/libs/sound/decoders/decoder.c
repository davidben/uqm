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

/* Sound file decoder for .wav, .mod, .ogg (to be used with OpenAL)
 * API is heavily influenced by SDL_sound.
 * By Mika Kolehmainen, 2002-10-27
 */

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include "libs/misc.h"
#include "decoder.h"
#include "wav.h"
#include "libs/sound/sound_common.h"
#include "mikmod/mikmod.h"

MIKMODAPI extern struct MDRIVER drv_openal;

static char *decoder_info_wav = "Wav";
static char *decoder_info_mod = "MikMod";
static char *decoder_info_ogg = "Ogg Vorbis";

TFB_DecoderFormats decoder_formats;

static int file_exists (char *filename)
{
    FILE *fp;
    if ((fp = fopen(filename, "rb")) == NULL)
        return 0;

    fclose (fp);
    return 1;
}

static size_t ogg_read (void *ptr, size_t size, size_t nmemb, void *datasource)
{
	return fread (ptr, size, nmemb, datasource);
}

static int ogg_seek (void *datasource, ogg_int64_t offset, int whence)
{
	long off = (long) offset;
	return fseek (datasource, off, whence);
}

static int ogg_close (void *datasource)
{
	return fclose (datasource);
}

static long ogg_tell(void *datasource)
{
	return ftell (datasource);
}

static const ov_callbacks ogg_callbacks = 
{
	ogg_read,
	ogg_seek,
	ogg_close, 
	ogg_tell,
};

/* change endianness of 16bit words
 * Only works optimal when 'data' is aligned on a 32 bits boundary.
 */
void
SoundDecoder_SwapWords (uint16* data, uint32 size)
{
	uint32 fsize = size & (~3U);

	size -= fsize;
	fsize >>= 2;
	for (; fsize; fsize--, data += 2)
	{
		uint32 v = *(uint32*)data;
		*(uint32*)data = ((v & 0x00ff00ff) << 8)
				| ((v & 0xff00ff00) >> 8);
	}
	if (size)
	{
		/* leftover word */
		*data = ((*data & 0x00ff) << 8) | ((*data & 0xff00) >> 8);
	}
}

sint32 SoundDecoder_Init (int flags, TFB_DecoderFormats *formats)
{
    MikMod_RegisterDriver (&drv_openal);
    MikMod_RegisterAllLoaders ();

	if (!formats)
	{
		fprintf (stderr, "SoundDecoder_Init(): missing decoder formats\n");
		return 1;		
	}
	decoder_formats = *formats;

	if (flags & TFB_SOUNDFLAGS_HQAUDIO)
	{
		md_mode = DMODE_HQMIXER|DMODE_STEREO|DMODE_16BITS|DMODE_INTERP|DMODE_SURROUND;
	    md_mixfreq = 44100;
		md_reverb = 1;
	}
	else if (flags & TFB_SOUNDFLAGS_LQAUDIO)
	{
		md_mode = DMODE_SOFT_MUSIC|DMODE_STEREO|DMODE_16BITS;
		md_mixfreq = 22050;
		md_reverb = 0;
	}
	else
	{
		md_mode = DMODE_SOFT_MUSIC|DMODE_STEREO|DMODE_16BITS|DMODE_INTERP;
		md_mixfreq = 44100;
		md_reverb = 0;
	}
	
	md_pansep = 64;

	if (MikMod_Init (""))
	{
		fprintf (stderr, "SoundDecoder_Init(): MikMod_Init failed, %s\n", 
			MikMod_strerror (MikMod_errno));
		return 1;		
	}

	return 0;
}

void SoundDecoder_Uninit (void)
{
	MikMod_Exit ();
}

TFB_SoundDecoder* SoundDecoder_Load (char *filename, uint32 buffer_size, 
									 uint32 startTime, uint32 runTime)
{	
	int i;

	if (!file_exists (filename))
	{
		fprintf (stderr, "SoundDecoder_Load(): %s doesn't exist\n", filename);
		return NULL;
	}

	i = strlen (filename) - 3;
	if (!strcmp (&filename[i], "wav"))
	{
		TFB_SoundDecoder *decoder;
		
		//fprintf (stderr, "SoundDecoder_Load(): %s interpreted as wav\n", filename);

		decoder = (TFB_SoundDecoder *) HMalloc (sizeof (TFB_SoundDecoder));
		decoder->buffer = NULL;
		decoder->buffer_size = 0;
		decoder->format = 0;
		decoder->frequency = 0;
		decoder->looping = false;
		decoder->error = SOUNDDECODER_OK;
		decoder->length = 0; // FIXME should be calculated
		decoder->decoder_info = decoder_info_wav;
		decoder->type = SOUNDDECODER_WAV;
		decoder->filename = (char *) HMalloc (strlen (filename) + 1);
		decoder->start_sample = 0;
		decoder->end_sample = 0;
		decoder->pos = 0;
		strcpy (decoder->filename, filename);

		return decoder;
	}
	else if (!strcmp (&filename[i], "mod"))
	{
		TFB_SoundDecoder *decoder;
		MODULE *mod;
		
		//fprintf (stderr, "SoundDecoder_Load(): %s interpreted as mod\n", filename);

		mod = Player_Load (filename, 4, 0);
		if (!mod)
		{
			fprintf (stderr, "SoundDecoder_Load(): couldn't load %s\n", filename);
			return NULL;
		}

		mod->extspd = 1;
		mod->panflag = 1;
		mod->wrap = 0;
		mod->loop = 1;
		
		decoder = (TFB_SoundDecoder *) HMalloc (sizeof (TFB_SoundDecoder));
		decoder->buffer = HMalloc (buffer_size);
		decoder->buffer_size = buffer_size;
		decoder->format = decoder_formats.stereo16;
		decoder->frequency = md_mixfreq;
		decoder->looping = false;
		decoder->error = SOUNDDECODER_OK;
		decoder->length = 0; // FIXME way to obtain this from mikmod?
		decoder->decoder_info = decoder_info_mod;
		decoder->type = SOUNDDECODER_MOD;
		decoder->filename = (char *) HMalloc (strlen (filename) + 1);
		decoder->start_sample = 0;
		decoder->end_sample = 0;
		decoder->pos = 0;
		strcpy (decoder->filename, filename);
		decoder->data = mod;

		return decoder;
	}
	else if (!strcmp (&filename[i], "ogg"))
	{
		int rc;
		FILE *vfp;
		OggVorbis_File *vf;
		vorbis_info *vinfo;
		TFB_SoundDecoder *decoder;
		
		//fprintf (stderr, "SoundDecoder_Load(): %s interpreted as ogg\n", filename);

		vf = (OggVorbis_File *) HMalloc (sizeof (OggVorbis_File));
		if (!vf)
		{
			fprintf (stderr, "SoundDecoder_Load(): couldn't allocate mem for OggVorbis_File\n");
			return NULL;
		}
		if ((vfp = fopen (filename, "r")) == NULL)
		{
			fprintf (stderr, "SoundDecoder_Load(): couldn't open %s\n", filename);
			HFree (vf);
			return NULL;
		}

#ifdef WIN32
		_setmode( _fileno(vfp), _O_BINARY);
#endif

		rc = ov_open_callbacks (vfp, vf, NULL, 0, ogg_callbacks);
		if (rc != 0)
		{
			fprintf (stderr, "SoundDecoder_Load(): ov_open_callbacks failed for %s, error code %d\n", filename, rc);
			fclose (vfp);
			HFree (vf);
			return NULL;
		}

		vinfo = ov_info (vf, -1);
		if (!vinfo)
		{
			fprintf (stderr, "SoundDecoder_Load(): failed to retrieve ogg bitstream info for %s\n", filename);
	        ov_clear (vf);
		    HFree (vf);
			return NULL;
		}
		
		//fprintf (stderr, "SoundDecoder_Load(): ogg bitstream version %d, channels %d, rate %d, length %.2f seconds\n", vinfo->version, vinfo->channels, vinfo->rate, ov_time_total (vf, -1));

		decoder = (TFB_SoundDecoder *) HMalloc (sizeof (TFB_SoundDecoder));
		decoder->buffer = HMalloc (buffer_size);
		decoder->buffer_size = buffer_size;
		decoder->frequency = vinfo->rate;
		decoder->looping = false;
		decoder->error = SOUNDDECODER_OK;
		decoder->length = (float) ov_time_total (vf, -1) - (startTime / 1000.0f);
		if (runTime && runTime / 1000.0 < decoder->length)
			decoder->length = (float)(runTime / 1000.0);

		decoder->start_sample = decoder->frequency * startTime / 1000;
		decoder->end_sample = decoder->start_sample + 
				(unsigned long)(decoder->length * decoder->frequency);
		if (decoder->start_sample)
			ov_pcm_seek (vf, decoder->start_sample);

		if (vinfo->channels == 1)
		{
			decoder->format = decoder_formats.mono16;
			decoder->pos = decoder->start_sample * 2;
		}
		else
		{
			decoder->format = decoder_formats.stereo16;
			decoder->pos = decoder->start_sample * 4;
		}

		decoder->decoder_info = decoder_info_ogg;
		decoder->type = SOUNDDECODER_OGG;
		decoder->filename = (char *) HMalloc (strlen (filename) + 1);
		strcpy (decoder->filename, filename);
		decoder->data = vf;

		return decoder;
	}
	else
	{
		fprintf (stderr, "SoundDecoder_Load(): %s file format is unsupported\n", filename);
	}

	return NULL;
}

uint32 SoundDecoder_Decode (TFB_SoundDecoder *decoder)
{
	switch (decoder->type)
	{
		case SOUNDDECODER_WAV:
		{
			fprintf (stderr, "SoundDecoder_Decode(): unimplemented for wav\n");
			break;
		}
		case SOUNDDECODER_MOD:
		{
			MODULE *mod = (MODULE *) decoder->data;
			uint32 decoded_bytes;

			Player_Start (mod);
			if (!Player_Active())
			{
				if (decoder->looping)
				{
					fprintf (stderr, "SoundDecoder_Decode(): looping %s\n", decoder->filename);
					Player_SetPosition (0);
					Player_Start (mod);
				}
				else
				{
					fprintf (stderr, "SoundDecoder_Decode(): eof for %s\n", decoder->filename);
					decoder->error = SOUNDDECODER_EOF;
					return 0;
				}
			}
			decoder->error = SOUNDDECODER_OK;
			decoded_bytes = VC_WriteBytes (decoder->buffer, decoder->buffer_size);

			/* MikMod produces output in machine byte-order
			 * may need to do a word-swap 
			 */
			if (decoded_bytes > 0 &&
					decoder_formats.big_endian != decoder_formats.want_big_endian &&
					(decoder->format == decoder_formats.stereo16 ||
					decoder->format == decoder_formats.mono16))
			{
				SoundDecoder_SwapWords (
						decoder->buffer, decoded_bytes);
			}
			return decoded_bytes;
		}
		case SOUNDDECODER_OGG:
		{
			OggVorbis_File *vf = (OggVorbis_File *) decoder->data;
			uint32 decoded_bytes = 0, count = 0;
			long rc;
			int bitstream;
			uint32 max_bytes, buffer_size, close_on_done = 0;
			char *buffer = (char *) decoder->buffer;

			if (! decoder->looping)
			{
				if (decoder->format == decoder_formats.stereo16)
					max_bytes = decoder->end_sample * 4;
				else
					max_bytes = decoder->end_sample * 2;
				if (max_bytes - decoder->pos < decoder->buffer_size)
				{
					buffer_size = max_bytes - decoder->pos;
					close_on_done = 1;
				}
				else
				{
					buffer_size = decoder->buffer_size;
				}
			}
			else
				buffer_size = decoder->buffer_size;

			while (decoded_bytes < buffer_size)
			{	
				/* Produce output in requested byte-order */
				rc = ov_read (vf, &buffer[decoded_bytes],
						buffer_size - decoded_bytes,
						decoder_formats.want_big_endian, 2, 1, &bitstream);
				if (rc < 0)
				{
					decoder->error = SOUNDDECODER_ERROR;
					fprintf (stderr, "SoundDecoder_Decode(): error decoding %s, code %d\n", decoder->filename, rc);
					return decoded_bytes;
				}
				if (rc == 0 || close_on_done && buffer_size == decoded_bytes + rc)
				{
					if (close_on_done)
					{
						decoded_bytes += rc;
					}
					if (decoder->looping)
					{
						SoundDecoder_Rewind (decoder);
						if (decoder->error)
						{
							fprintf (stderr, "SoundDecoder_Decode(): tried to loop %s but couldn't rewind, error code %d\n", decoder->filename, decoder->error);
						}
						else
						{
							fprintf (stderr, "SoundDecoder_Decode(): looping %s\n", decoder->filename);
							count++;
							continue;
						}
					}
					else
						decoder->pos += decoded_bytes;

					fprintf (stderr, "SoundDecoder_Decode(): eof for %s\n", decoder->filename);
					decoder->error = SOUNDDECODER_EOF;
					return decoded_bytes;
				}

				decoded_bytes += rc;
				//fprintf (stderr, "iter %d rc %d decoded_bytes %d remaining %d\n", count, rc, decoded_bytes, decoder->buffer_size - decoded_bytes);
				count++;
			}
			decoder->pos += decoded_bytes;
			decoder->error = SOUNDDECODER_OK;
			//fprintf (stderr, "SoundDecoder_Decode(): decoded %d bytes from %s\n", decoded_bytes, decoder->filename);
			return decoded_bytes;
		}
		case SOUNDDECODER_BUF:
			decoder->buffer = (char *)decoder->data + decoder->pos;
			decoder->error = SOUNDDECODER_EOF;
			return (decoder->buffer_size - decoder->pos);
		default:
		{
			fprintf (stderr, "SoundDecoder_Decode(): unknown type %d\n", decoder->type);
			break;
		}
	}

	decoder->error = SOUNDDECODER_ERROR;
	return 0;
}

uint32 SoundDecoder_DecodeAll (TFB_SoundDecoder *decoder)
{
	switch (decoder->type)
	{
		case SOUNDDECODER_WAV:
		{
			bool loop;

			LoadWAVFile (decoder->filename ,&decoder->format, &decoder->buffer, 
				&decoder->buffer_size, &decoder->frequency, &loop);
			
			if (decoder->buffer_size != 0)
			{

				/* WAVs are loaded in little-endian order
				 * may need to do a word-swap 
				 */
				if (decoder_formats.want_big_endian &&
						(decoder->format == decoder_formats.stereo16
						|| decoder->format == decoder_formats.mono16))
				{
					SoundDecoder_SwapWords (
							decoder->buffer, decoder->buffer_size);
				}
				decoder->type = SOUNDDECODER_BUF;
				decoder->data = decoder->buffer;
				decoder->pos = 0;
				decoder->error = SOUNDDECODER_OK;
			}
			else
			{
				decoder->error = SOUNDDECODER_ERROR;
			}
			
			return decoder->buffer_size;
		}
		case SOUNDDECODER_MOD:
		{
			fprintf (stderr, "SoundDecoder_DecodeAll(): unimplemented for mod\n");
			break;
		}
		case SOUNDDECODER_OGG:
		{
			OggVorbis_File *vf = (OggVorbis_File *) decoder->data;
			uint32 decoded_bytes = 0;
			long rc;
			int bitstream;
			uint32 reqbufsize = decoder->buffer_size;
			
			if (decoder->looping)
			{
				fprintf (stderr, "SoundDecoder_DecodeAll(): "
						"called for %s with looping\n", decoder->filename);
				return 0;
			}

			if (reqbufsize < 4096)
				reqbufsize = 4096;

#ifdef DEBUG
			if (reqbufsize < 16384)
				fprintf (stderr, "SoundDecoder_DecodeAll(): WARNING, "
						"called with a small buffer (%u)\n", reqbufsize);
#endif

			for (rc = 1; rc > 0; )
			{	
				if (decoded_bytes >= decoder->buffer_size)
				{	// need to grow buffer
					decoder->buffer_size += reqbufsize;
					decoder->buffer = HRealloc (
							decoder->buffer, decoder->buffer_size);
				}

				rc = ov_read (vf, (char*)decoder->buffer + decoded_bytes,
						decoder->buffer_size - decoded_bytes,
						decoder_formats.want_big_endian, 2, 1, &bitstream);
				if (rc < 0)
				{
					decoder->error = SOUNDDECODER_ERROR;
					fprintf (stderr, "SoundDecoder_DecodeAll(): "
							"error decoding %s, code %d\n",
							decoder->filename, rc);
					return decoded_bytes;
				}

				decoded_bytes += rc;
			}

			decoder->buffer_size = decoded_bytes;
			ov_clear (vf);
			HFree (vf);
			decoder->type = SOUNDDECODER_BUF;
			decoder->data = decoder->buffer;
			decoder->pos = 0;

			decoder->error = SOUNDDECODER_OK;
			return decoded_bytes;
			break;
		}
		default:
		{
			fprintf (stderr, "SoundDecoder_DecodeAll(): unknown type %d\n", decoder->type);
			break;
		}
	}

	decoder->error = SOUNDDECODER_ERROR;
	return 0;
}

void SoundDecoder_Rewind (TFB_SoundDecoder *decoder)
{
	SoundDecoder_Seek (decoder, 0);
}

// seekTime is specified in mili-seconds
void SoundDecoder_Seek (TFB_SoundDecoder *decoder, uint32 seekTime)
{
	switch (decoder->type)
	{
		case SOUNDDECODER_WAV:
		{
			fprintf (stderr, "SoundDecoder_Seek(): unimplemented for wav\n");
			break;
		}
		case SOUNDDECODER_MOD:
		{
			MODULE *mod = (MODULE *)decoder->data;
			Player_Start (mod);
			if (seekTime)
				fprintf (stderr, "SoundDecoder_Seek(): non-zero seek positions not supported for mod\n");
			Player_SetPosition (0);
			//fprintf (stderr, "SoundDecoder_Rewind(): rewound %s\n", decoder->filename);
			decoder->error = SOUNDDECODER_OK;
			return;
		}
		case SOUNDDECODER_OGG:
		{
			int err;
			uint32 pcm_pos = 0;
			OggVorbis_File *vf = (OggVorbis_File *) decoder->data;
			if (seekTime)
				pcm_pos = seekTime * decoder->frequency / 1000;
			if ((err = ov_pcm_seek (vf, decoder->start_sample + pcm_pos)) != 0)
			{
				fprintf (stderr, "SoundDecoder_Seek(): couldn't seek %s, error code %d\n", decoder->filename, err);
				break;
			}
			if (decoder->format == decoder_formats.mono16)
				decoder->pos = (pcm_pos + decoder->start_sample) * 2;
			else
				decoder->pos = (pcm_pos + decoder->start_sample) * 4;

			//fprintf (stderr, "SoundDecoder_Seek(): seek %s\n", decoder->filename);
			decoder->error = SOUNDDECODER_OK;
			return;
		}
		case SOUNDDECODER_BUF:
		{
			decoder->pos = seekTime * decoder->frequency *
				(decoder->format == decoder_formats.mono16 ? 2 : 4) / 1000;
			decoder->error = SOUNDDECODER_OK;
			return;
		}
		default:
		{
			fprintf (stderr, "SoundDecoder_Seek(): unknown type %d\n", decoder->type);
			break;
		}
	}

	decoder->error = SOUNDDECODER_ERROR;
}

void SoundDecoder_Free (TFB_SoundDecoder *decoder)
{
	if (!decoder)
	{
		return;
	}

	switch (decoder->type)
	{
		case SOUNDDECODER_WAV:
		{
			//fprintf (stderr, "SoundDecoder_Free(): freeing %s\n", decoder->filename);
			break;
		}
		case SOUNDDECODER_MOD:
		{
			MODULE *mod = (MODULE *) decoder->data;
			//fprintf (stderr, "SoundDecoder_Free(): freeing %s\n", decoder->filename);
			Player_Free (mod);
			break;
		}
		case SOUNDDECODER_OGG:
		{
			OggVorbis_File *vf = (OggVorbis_File *) decoder->data;
			//fprintf (stderr, "SoundDecoder_Free(): freeing %s\n", decoder->filename);
			ov_clear (vf);
			HFree (vf);
			break;
		}
		case SOUNDDECODER_BUF:
			break;
		default:
		{
			fprintf (stderr, "SoundDecoder_Free(): unknown type %d\n", decoder->type);
			break;
		}
	}

	HFree (decoder->buffer);
	HFree (decoder->filename);
	HFree (decoder);
}

float SoundDecoder_GetTime (TFB_SoundDecoder *decoder)
{
	if (!decoder)
	{
		return 0.0f;
	}

	switch (decoder->type)
	{
		case SOUNDDECODER_WAV:
		{
			//fprintf (stderr, "SoundDecoder_GetTime not supported for wav\n");
			return 0.0f;
		}
		case SOUNDDECODER_MOD:
		{
			//fprintf (stderr, "SoundDecoder_GetTime not supported for mod\n");
			return 0.0f;
		}
		case SOUNDDECODER_OGG:
		case SOUNDDECODER_BUF:
		{
			//fprintf (stderr, "SoundDecoder_GetTime not supported for mod\n");
			return (
				(float)(
					(decoder->pos >> (decoder->format == decoder_formats.mono16 ? 1 : 2))
					- decoder->start_sample
				 ) / decoder->frequency);
		}
		default:
		{
			fprintf (stderr, "SoundDecoder_GetTime(): unknown type %d\n", decoder->type);
			return 0.0f;
		}
	}
}
