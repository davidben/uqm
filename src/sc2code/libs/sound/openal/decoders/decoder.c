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

#ifdef SOUNDMODULE_OPENAL

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include "decoder.h"
#include "wav.h"
#include "libs/sound/sound_common.h"
#include "mikmod/mikmod.h"

MIKMODAPI extern struct MDRIVER drv_openal;

static char *decoder_info_wav = "Wav";
static char *decoder_info_mod = "MikMod";
static char *decoder_info_ogg = "Ogg Vorbis";


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

ALint SoundDecoder_Init (int flags)
{
    MikMod_RegisterDriver (&drv_openal);
    MikMod_RegisterAllLoaders ();

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
	
	md_pansep = 96;

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

TFB_SoundDecoder* SoundDecoder_Load (char *filename, ALuint buffer_size)
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

		decoder = (TFB_SoundDecoder *) malloc (sizeof (TFB_SoundDecoder));
		decoder->buffer = NULL;
		decoder->buffer_size = 0;
		decoder->format = 0;
		decoder->frequency = 0;
		decoder->looping = AL_FALSE;
		decoder->error = SOUNDDECODER_OK;
		decoder->decoder_info = decoder_info_wav;
		decoder->type = SOUNDDECODER_WAV;
		decoder->filename = (char *) malloc (strlen (filename) + 1);
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
		mod->loop = 0;
		
		decoder = (TFB_SoundDecoder *) malloc (sizeof (TFB_SoundDecoder));
		decoder->buffer = malloc (buffer_size);
		decoder->buffer_size = buffer_size;
		decoder->format = AL_FORMAT_STEREO16;
		decoder->frequency = md_mixfreq;
		decoder->looping = AL_FALSE;
		decoder->error = SOUNDDECODER_OK;
		decoder->decoder_info = decoder_info_mod;
		decoder->type = SOUNDDECODER_MOD;
		decoder->filename = (char *) malloc (strlen (filename) + 1);
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

		vf = (OggVorbis_File *) malloc (sizeof (OggVorbis_File));
		if (!vf)
		{
			fprintf (stderr, "SoundDecoder_Load(): couldn't allocate mem for OggVorbis_File\n");
			return NULL;
		}
		if ((vfp = fopen (filename, "r")) == NULL)
		{
			fprintf (stderr, "SoundDecoder_Load(): couldn't open %s\n", filename);
			free (vf);
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
			free (vf);
			return NULL;
		}

		vinfo = ov_info (vf, -1);
		if (!vinfo)
		{
			fprintf (stderr, "SoundDecoder_Load(): failed to retrieve ogg bitstream info for %s\n", filename);
	        ov_clear (vf);
		    free (vf);
			return NULL;
		}
		
		//fprintf (stderr, "SoundDecoder_Load(): ogg bitstream version %d, channels %d, rate %d, length %.2f seconds\n", vinfo->version, vinfo->channels, vinfo->rate, ov_time_total (vf, -1));

		decoder = (TFB_SoundDecoder *) malloc (sizeof (TFB_SoundDecoder));
		decoder->buffer = malloc (buffer_size);
		decoder->buffer_size = buffer_size;
		if (vinfo->channels == 1)
			decoder->format = AL_FORMAT_MONO16;
		else
			decoder->format = AL_FORMAT_STEREO16;
		decoder->frequency = vinfo->rate;
		decoder->looping = AL_FALSE;
		decoder->error = SOUNDDECODER_OK;
		decoder->decoder_info = decoder_info_ogg;
		decoder->type = SOUNDDECODER_OGG;
		decoder->filename = (char *) malloc (strlen (filename) + 1);
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

ALuint SoundDecoder_Decode (TFB_SoundDecoder *decoder)
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
			return ((ALuint) VC_WriteBytes (decoder->buffer, decoder->buffer_size));
		}
		case SOUNDDECODER_OGG:
		{
			OggVorbis_File *vf = (OggVorbis_File *) decoder->data;
			ALuint decoded_bytes = 0, count = 0;
			long rc;
			int bitstream;
			char *buffer = (char *) decoder->buffer;

			while (decoded_bytes < decoder->buffer_size)
			{	
				
				rc = ov_read (vf, &buffer[decoded_bytes], decoder->buffer_size - decoded_bytes, 0, 2, 1, &bitstream);
				if (rc < 0)
				{
					decoder->error = SOUNDDECODER_ERROR;
					fprintf (stderr, "SoundDecoder_Decode(): error decoding %s, code %d\n", decoder->filename, rc);
					return decoded_bytes;
				}
				if (rc == 0)
				{
					if (decoder->looping)
					{
						int err;
						if ((err = ov_pcm_seek (vf, 0)) != 0)
						{
							fprintf (stderr, "SoundDecoder_Decode(): tried to loop %s but couldn't rewind, error code %d\n", decoder->filename, err);
						}
						else
						{
							fprintf (stderr, "SoundDecoder_Decode(): looping %s\n", decoder->filename);
							count++;
							continue;
						}
					}

					fprintf (stderr, "SoundDecoder_Decode(): eof for %s\n", decoder->filename);
					decoder->error = SOUNDDECODER_EOF;
					return decoded_bytes;
				}

				decoded_bytes += rc;
				//fprintf (stderr, "iter %d rc %d decoded_bytes %d remaining %d\n", count, rc, decoded_bytes, decoder->buffer_size - decoded_bytes);
				count++;
			}

			decoder->error = SOUNDDECODER_OK;
			//fprintf (stderr, "SoundDecoder_Decode(): decoded %d bytes from %s\n", decoded_bytes, decoder->filename);
			return decoded_bytes;
		}
		default:
		{
			fprintf (stderr, "SoundDecoder_Decode(): unknown type %d\n", decoder->type);
			break;
		}
	}

	decoder->error = SOUNDDECODER_ERROR;
	return 0;
}

ALuint SoundDecoder_DecodeAll (TFB_SoundDecoder *decoder)
{
	switch (decoder->type)
	{
		case SOUNDDECODER_WAV:
		{
			ALboolean loop;

			LoadWAVFile(decoder->filename ,&decoder->format, &decoder->buffer, 
				&decoder->buffer_size, &decoder->frequency, &loop);
			
			if (decoder->buffer_size != 0)
				decoder->error = SOUNDDECODER_OK;
			else
				decoder->error = SOUNDDECODER_ERROR;
			
			return decoder->buffer_size;
		}
		case SOUNDDECODER_MOD:
		{
			fprintf (stderr, "SoundDecoder_DecodeAll(): unimplemented for mod\n");
			break;
		}
		case SOUNDDECODER_OGG:
		{
			fprintf (stderr, "SoundDecoder_DecodeAll(): unimplemented for ogg\n");
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
	switch (decoder->type)
	{
		case SOUNDDECODER_WAV:
		{
			fprintf (stderr, "SoundDecoder_Rewind(): unimplemented for wav\n");
			break;
		}
		case SOUNDDECODER_MOD:
		{
			MODULE *mod = (MODULE *)decoder->data;
			Player_Start (mod);
			Player_SetPosition (0);
			//fprintf (stderr, "SoundDecoder_Rewind(): rewound %s\n", decoder->filename);
			decoder->error = SOUNDDECODER_OK;
			return;
		}
		case SOUNDDECODER_OGG:
		{
			int err;
			OggVorbis_File *vf = (OggVorbis_File *) decoder->data;
			if ((err = ov_pcm_seek (vf, 0)) != 0)
			{
				fprintf (stderr, "SoundDecoder_Rewind(): couldn't rewind %s, error code %d\n", decoder->filename, err);
				break;
			}
			//fprintf (stderr, "SoundDecoder_Rewind(): rewound %s\n", decoder->filename);
			decoder->error = SOUNDDECODER_OK;
			return;
		}
		default:
		{
			fprintf (stderr, "SoundDecoder_Rewind(): unknown type %d\n", decoder->type);
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
			free (vf);
			break;
		}
		default:
		{
			fprintf (stderr, "SoundDecoder_Free(): unknown type %d\n", decoder->type);
			break;
		}
	}

	free (decoder->buffer);
	free (decoder->filename);
	free (decoder);
}

#endif
