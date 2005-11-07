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

/* MikMod decoder (.mod adapter)
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "libs/misc.h"
#include "port.h"
#include "types.h"
#include "endian_uqm.h"
#include "uio.h"
#include "decoder.h"
#include "mikmod/mikmod.h"
#include "mikmod/drv_openal.h"
#include "libs/sound/audiocore.h"
#include "modaud.h"

#define THIS_PTR TFB_SoundDecoder* This

static const char* moda_GetName (void);
static bool moda_InitModule (int flags, const TFB_DecoderFormats*);
static void moda_TermModule (void);
static uint32 moda_GetStructSize (void);
static int moda_GetError (THIS_PTR);
static bool moda_Init (THIS_PTR);
static void moda_Term (THIS_PTR);
static bool moda_Open (THIS_PTR, uio_DirHandle *dir, const char *filename);
static void moda_Close (THIS_PTR);
static int moda_Decode (THIS_PTR, void* buf, sint32 bufsize);
static uint32 moda_Seek (THIS_PTR, uint32 pcm_pos);
static uint32 moda_GetFrame (THIS_PTR);

TFB_SoundDecoderFuncs moda_DecoderVtbl = 
{
	moda_GetName,
	moda_InitModule,
	moda_TermModule,
	moda_GetStructSize,
	moda_GetError,
	moda_Init,
	moda_Term,
	moda_Open,
	moda_Close,
	moda_Decode,
	moda_Seek,
	moda_GetFrame,
};

typedef struct tfb_modsounddecoder
{
	// always the first member
	TFB_SoundDecoder decoder;

	// private
	sint32 last_error;
	MODULE* module;

} TFB_ModSoundDecoder;

static const TFB_DecoderFormats* moda_formats = NULL;

// MikMod READER interface
//  we provide our own so that we can do loading via uio 
//
typedef struct MUIOREADER
{
	MREADER     core;
	uio_Stream* file;

} MUIOREADER;

static BOOL
moda_uioReader_Eof (MREADER* reader)
{
	return uio_feof (((MUIOREADER*)reader)->file);
}

static BOOL
moda_uioReader_Read (MREADER* reader, void* ptr, size_t size)
{
	return uio_fread (ptr, size, 1, ((MUIOREADER*)reader)->file);
}

static int
moda_uioReader_Get (MREADER* reader)
{
	return uio_fgetc (((MUIOREADER*)reader)->file);
}

static BOOL
moda_uioReader_Seek (MREADER* reader, long offset, int whence)
{
	return uio_fseek (((MUIOREADER*)reader)->file, offset, whence);
}

static long
moda_uioReader_Tell (MREADER* reader)
{
	return uio_ftell (((MUIOREADER*)reader)->file);
}

MREADER*
moda_new_uioReader (uio_Stream* fp)
{
	MUIOREADER* reader = (MUIOREADER*) HMalloc (sizeof(MUIOREADER));
	if (reader)
	{
		reader->core.Eof  = &moda_uioReader_Eof;
		reader->core.Read = &moda_uioReader_Read;
		reader->core.Get  = &moda_uioReader_Get;
		reader->core.Seek = &moda_uioReader_Seek;
		reader->core.Tell = &moda_uioReader_Tell;
		reader->file = fp;
	}
	return (MREADER*)reader;
}

void
moda_delete_uioReader (MREADER* reader)
{
	if (reader)
		HFree (reader);
}


static const char*
moda_GetName (void)
{
	return "MikMod";
}

static bool
moda_InitModule (int flags, const TFB_DecoderFormats* fmts)
{
    MikMod_RegisterDriver (&drv_openal);
    MikMod_RegisterAllLoaders ();

	if (flags & audio_QUALITY_HIGH)
	{
#ifndef WORDS_BIGENDIAN
		md_mode = DMODE_HQMIXER|DMODE_STEREO|DMODE_16BITS|DMODE_INTERP|DMODE_SURROUND;
#else
		// disable hqmixer on big endian machines (workaround for bug #166)
		md_mode = DMODE_SOFT_MUSIC|DMODE_STEREO|DMODE_16BITS|DMODE_INTERP|DMODE_SURROUND;
#endif
	    md_mixfreq = 44100;
		md_reverb = 1;
	}
	else if (flags & audio_QUALITY_LOW)
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
		fprintf (stderr, "MikMod_Init() failed, %s\n", 
			MikMod_strerror (MikMod_errno));
		return false;
	}

	moda_formats = fmts;

	return true;
}

static void
moda_TermModule (void)
{
	MikMod_Exit ();
}

static uint32
moda_GetStructSize (void)
{
	return sizeof (TFB_ModSoundDecoder);
}

static int
moda_GetError (THIS_PTR)
{
	TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*) This;
	int ret = moda->last_error;
	moda->last_error = 0;
	return ret;
}

static bool
moda_Init (THIS_PTR)
{
	//TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*) This;
	This->need_swap =
			moda_formats->big_endian != moda_formats->want_big_endian;
	return true;
}

static void
moda_Term (THIS_PTR)
{
	//TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*) This;
	moda_Close (This); // ensure cleanup
}

static bool
moda_Open (THIS_PTR, uio_DirHandle *dir, const char *filename)
{
	TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*) This;
	uio_Stream *fp;
	MREADER* reader;
	MODULE* mod;

	fp = uio_fopen (dir, filename, "rb");
	if (!fp)
	{
		moda->last_error = errno;
		return false;
	}

	reader = moda_new_uioReader (fp);
	if (!reader)
	{
		moda->last_error = -1;
		uio_fclose (fp);
		return false;
	}

	mod = Player_LoadGeneric (reader, 8, 0);
	
	// can already dispose of reader and fileh
	moda_delete_uioReader (reader);
	uio_fclose (fp);
	if (!mod)
	{
		fprintf (stderr, "moda_Open(): could not load %s\n", filename);
		return false;
	}

	moda->module = mod;
	mod->extspd = 1;
	mod->panflag = 1;
	mod->wrap = 0;
	mod->loop = 1;

	This->format = moda_formats->stereo16;
	This->frequency = md_mixfreq;
	This->length = 0; // FIXME way to obtain this from mikmod?

	moda->last_error = 0;

	return true;
}

static void
moda_Close (THIS_PTR)
{
	TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*) This;
	
	if (moda->module)
	{
		Player_Free (moda->module);
		moda->module = NULL;
	}
}

static int
moda_Decode (THIS_PTR, void* buf, sint32 bufsize)
{
	TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*) This;
	volatile ULONG* poutsize;

	Player_Start (moda->module);
	if (!Player_Active())
		return 0;

	poutsize = ALDRV_SetOutputBuffer (buf, bufsize);
	MikMod_Update ();
	
	return *poutsize;
}

static uint32
moda_Seek (THIS_PTR, uint32 pcm_pos)
{
	TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*) This;
	
	Player_Start (moda->module);
	if (pcm_pos)
		fprintf (stderr, "moda_Seek(): "
				"non-zero seek positions not supported for mod\n");
	Player_SetPosition (0);

	return 0;
}

static uint32
moda_GetFrame (THIS_PTR)
{
	TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*) This;
	return moda->module->sngpos;
}
