//Copyright Paul Reiche, Fred Ford. 1992-2002

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

#ifdef SOUNDMODULE_SDL

#include <fcntl.h>
#include "libs/strings/strintrn.h"
#include "libs/sound/sndintrn.h"
#include "libs/graphics/sdl/sdl_common.h"
#include "libs/sound/sound_common.h"
#include "options.h"

#ifdef WIN32
#include <io.h>
#include <SDL_mixer.h>
#elif defined __FreeBSD__
#include <SDL_mixer.h>
#else
#include <SDL/SDL_mixer.h>
#endif


MEM_HANDLE
_GetSoundBankData (FILE *fp, DWORD length)
{
#ifdef WIN32
	int omode;
#endif
	int snd_ct, n;
	DWORD opos;
	char CurrentLine[1024], filename[1024];
#define MAX_FX 256
	Mix_Chunk *sndfx[MAX_FX];
	STRING_TABLE Snd;
	
	opos = ftell (fp);

	{
		char *s1, *s2;
		extern char *_cur_resfile_name;

		if (_cur_resfile_name == 0
				|| (((s2 = 0), (s1 = strrchr (_cur_resfile_name, '/')) == 0)
						&& (s2 = strrchr (_cur_resfile_name, '\\')) == 0))
			n = 0;
		else
		{
			if (s2 > s1)
				s1 = s2;
			n = s1 - _cur_resfile_name + 1;
			strncpy (filename, _cur_resfile_name, n);
		}
	}

#ifdef WIN32
	omode = _setmode (fileno (fp), O_TEXT);
#endif
	snd_ct = 0;
	while (fgets (CurrentLine, sizeof (CurrentLine), fp) && snd_ct < MAX_FX)
	{
		if
		(
				sscanf(CurrentLine, "%s", &filename[n]) == 1 &&
				(sndfx[snd_ct] = Mix_LoadWAV (filename))
		)
		{
			++snd_ct;
		}
		else
		{
				fprintf (stderr, "_GetSoundBankData: Bad file!\n");
		}

		if (ftell (fp) - opos >= length)
			break;
	}
#ifdef WIN32
	_setmode (fileno (fp), omode);
#endif

	Snd = 0;
	if (snd_ct && (Snd = AllocStringTable (
					sizeof (STRING_TABLE_DESC)
							+ (sizeof (DWORD) * snd_ct)
							+ (sizeof (sndfx[0]) * snd_ct)
					)))
	{
		STRING_TABLEPTR fxTab;

		LockStringTable (Snd, &fxTab);
		if (fxTab == 0)
		{
			while (snd_ct--)
				Mix_FreeChunk (sndfx[snd_ct]);
			FreeStringTable (Snd);
			Snd = 0;
		}
		else
		{
			DWORD *offs, StringOffs;

			fxTab->StringCount = snd_ct;
			fxTab->flags = 0;
			offs = fxTab->StringOffsets;
			StringOffs = sizeof (STRING_TABLE_DESC) + (sizeof (DWORD) * snd_ct);
			memcpy ((BYTE *)fxTab + StringOffs, sndfx, sizeof (sndfx[0]) * snd_ct);
			do
			{
				*offs++ = StringOffs;
				StringOffs += sizeof (sndfx[0]);
			} while (snd_ct--);
			UnlockStringTable (Snd);
		}
	}

	return ((MEM_HANDLE)Snd);
}

BOOLEAN
_ReleaseSoundBankData (MEM_HANDLE Snd)
{
	STRING_TABLEPTR fxTab;

	LockStringTable (Snd, &fxTab);
	if (fxTab)
	{
		int snd_ct;
		Mix_Chunk **sptr;

		snd_ct = fxTab->StringCount;
		sptr = (Mix_Chunk **)((BYTE *)fxTab + fxTab->StringOffsets[0]);
		while (snd_ct--)
		{
			Mix_FreeChunk (*sptr);
			*sptr++ = 0;
		}
		UnlockStringTable (Snd);
		FreeStringTable (Snd);

		return (TRUE);
	}

	return (FALSE);
}

MEM_HANDLE
_GetMusicData (FILE *fp, DWORD length)
{
   	MEM_HANDLE h;
   	extern char *_cur_resfile_name;

   	h = 0;
   	if (_cur_resfile_name && (h = AllocMusicData (sizeof (void *))))
   	{
        char filename[1000];
        Mix_Music **pmus;

        LockMusicData (h, &pmus);    

        strcpy (filename, _cur_resfile_name);
        switch (optWhichMusic)
        {
            default:
            case MUSIC_3DO:
            {
                char threedoname[1000];

                strcpy (threedoname, filename);
                strcpy (&threedoname[strlen (threedoname)-3], "ogg");
                if (FileExists (threedoname))
                {
                    strcpy (filename, threedoname);
                }
                break;
            }
            case MUSIC_PC:
            {
                break;
            }
        }

        fprintf (stderr, "_GetMusicData(): loading %s\n", filename);
		if (pmus == 0 || (*pmus = Mix_LoadMUS (filename)) == 0)
		{
			UnlockMusicData (h);
			mem_release (h);
			h = 0;
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
		Mix_Music **pmus;

		LockMusicData (handle, &pmus);
		if (pmus == 0)
				return (FALSE);

		Mix_FreeMusic (*pmus);

		UnlockMusicData (handle);
		mem_release (handle);

		return (TRUE);
}

BOOLEAN
DestroySound(SOUND_REF target)
{
	return (_ReleaseSoundBankData (target));
}

#endif
