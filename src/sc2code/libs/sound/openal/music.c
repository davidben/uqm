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

/* OpenAL specific code by Mika Kolehmainen, 2002-10-23
 */

#ifdef SOUNDMODULE_OPENAL

#include "sound.h"
#include "options.h"
#include "file.h"

static MUSIC_REF curMusicRef;

void
PLRPlaySong (MUSIC_REF MusicRef, BOOLEAN Continuous, BYTE Priority)
{
	TFB_SoundSample **pmus;

	LockMusicData (MusicRef, &pmus);
	if (pmus)
	{
		LockMutex (soundSource[MUSIC_SOURCE].stream_mutex);
		PlayStream ((*pmus), MUSIC_SOURCE, Continuous, 
			speechVolumeScale == 0.0f ? AL_TRUE : AL_FALSE);
		UnlockMutex (soundSource[MUSIC_SOURCE].stream_mutex);
		
		curMusicRef = MusicRef;
	}

	(void) Priority;  /* Satisfy compiler because of unused variable */
}

void
PLRStop (MUSIC_REF MusicRef)
{
	if (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0)
	{
		LockMutex (soundSource[MUSIC_SOURCE].stream_mutex);
		StopStream (MUSIC_SOURCE);
		UnlockMutex (soundSource[MUSIC_SOURCE].stream_mutex);
		UnlockMusicData (curMusicRef);

		curMusicRef = 0;
	}
}

BOOLEAN
PLRPlaying (MUSIC_REF MusicRef)
{
	if (curMusicRef && (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0))
	{
		BOOLEAN playing;

		LockMutex (soundSource[MUSIC_SOURCE].stream_mutex);
		playing = PlayingStream (MUSIC_SOURCE);
		UnlockMutex (soundSource[MUSIC_SOURCE].stream_mutex);

		return playing;
	}

	return FALSE;
}

void
PLRPause (MUSIC_REF MusicRef)
{	
	if (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0)
	{
		LockMutex (soundSource[MUSIC_SOURCE].stream_mutex);
		PauseStream (MUSIC_SOURCE);
		UnlockMutex (soundSource[MUSIC_SOURCE].stream_mutex);
	}
}

void
PLRResume (MUSIC_REF MusicRef)
{
	if (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0)
	{
		LockMutex (soundSource[MUSIC_SOURCE].stream_mutex);
		ResumeStream (MUSIC_SOURCE);
		UnlockMutex (soundSource[MUSIC_SOURCE].stream_mutex);
	}
}

BOOLEAN
DestroyMusic (MUSIC_REF MusicRef)
{
	return (FreeMusicData (MusicRef));
}

void
SetMusicVolume (COUNT Volume)
{
	float f = (Volume / (ALfloat)MAX_VOLUME) * musicVolumeScale;
	musicVolume = Volume;
	alSourcef (soundSource[MUSIC_SOURCE].handle, AL_GAIN, f);
}

MEM_HANDLE
_GetMusicData (FILE *fp, DWORD length)
{
	MEM_HANDLE h;
	extern char *_cur_resfile_name;

	h = 0;
	if (_cur_resfile_name && (h = AllocMusicData (sizeof (void *))))
	{
		TFB_SoundSample **pmus;

		LockMusicData (h, &pmus);
		if (!pmus)
		{
			UnlockMusicData (h);
			mem_release (h);
			h = 0;
		}		
		else
		{
            char filename[1000];

            *pmus = (TFB_SoundSample *) HMalloc (sizeof (TFB_SoundSample));
            strcpy (filename, _cur_resfile_name);

            switch (optWhichMusic)
            {
                default:
                case OPT_3DO:
                {
                    char threedoname[1000];
                    
                    strcpy (threedoname, filename);
                    strcpy (&threedoname[strlen (threedoname)-3], "ogg");
                    if (fileExists (threedoname))
                    {
                        strcpy (filename, threedoname);
                    }                    
                    break;   
                }
                case OPT_PC:
                {
                    break;
                }            
            }

            fprintf (stderr, "_GetMusicData(): loading %s\n", filename);                        			
			if (((*pmus)->decoder = SoundDecoder_Load (filename, 4096)) == 0)
			{
				fprintf (stderr, "_GetMusicData(): couldn't load %s\n", filename);

				UnlockMusicData (h);
				mem_release (h);
				h = 0;
			}
			else
			{
				fprintf (stderr, "    decoder: %s, rate %d format %x\n", (*pmus)->decoder->decoder_info,
					(*pmus)->decoder->frequency, (*pmus)->decoder->format);

				(*pmus)->num_buffers = 64;
				(*pmus)->buffer = (ALuint *) HMalloc (sizeof (ALuint) * (*pmus)->num_buffers);
				alGenBuffers ((*pmus)->num_buffers, (*pmus)->buffer);
			}
		}

		UnlockMusicData (h);
	}

	(void) fp;  /* satisfy compiler (unused parameter) */
	(void) length;  /* satisfy compiler (unused parameter) */
	return (h);
}

BOOLEAN
_ReleaseMusicData (MEM_HANDLE handle)
{
	TFB_SoundSample **pmus;

	LockMusicData (handle, &pmus);
	if (pmus == 0)
		return (FALSE);

	if ((*pmus)->decoder)
	{
		TFB_SoundDecoder *decoder = (*pmus)->decoder;
		LockMutex (soundSource[MUSIC_SOURCE].stream_mutex);
		if (soundSource[MUSIC_SOURCE].sample == (*pmus))
		{
			StopStream (MUSIC_SOURCE);
		}
		UnlockMutex (soundSource[MUSIC_SOURCE].stream_mutex);

		(*pmus)->decoder = NULL;
		SoundDecoder_Free (decoder);
		alDeleteBuffers ((*pmus)->num_buffers, (*pmus)->buffer);
		HFree ((*pmus)->buffer);
	}
	HFree (*pmus);

	UnlockMusicData (handle);
	mem_release (handle);

	return (TRUE);
}

#endif
