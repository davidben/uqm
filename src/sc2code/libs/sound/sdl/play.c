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

#include "libs/sound/sndintrn.h"
#include "libs/graphics/sdl/sdl_common.h"
#include "libs/sound/sound_common.h"

#include <SDL_mixer.h>

static MUSIC_REF curMusicRef;

void
PLRPlaySong (MUSIC_REF MusicRef, BOOLEAN Continuous, BYTE Priority)
{
	Mix_Music **pmus;

	LockMusicData (MusicRef, &pmus);
	if (pmus)
	{
		PLRStop (curMusicRef);

		curMusicRef = MusicRef;
		Mix_PlayMusic (*pmus, Continuous ? -1 : 1);
	}
	(void) Priority;  /* Satisfy compiler because of unused variable */
}

void
PLRStop (MUSIC_REF MusicRef)
{
	if (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0)
	{
		Mix_HaltMusic ();

		UnlockMusicData (curMusicRef);
		curMusicRef = 0;
	}
}

BOOLEAN
PLRPlaying (MUSIC_REF MusicRef)
{
	if (curMusicRef && (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0))
	{
		if (Mix_PlayingMusic ())
			return (TRUE);
		PLRStop (curMusicRef);
	}

	return (FALSE);
}

void
PLRPause (MUSIC_REF MusicRef)
{
	if (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0)
	{
// Mix_FadeOutMusic (1 * 1000);
		Mix_PauseMusic ();
	}
}

void
PLRResume (MUSIC_REF MusicRef)
{
	if (MusicRef == curMusicRef || MusicRef == (MUSIC_REF)~0)
	{
// Mix_FadeInMusic (1 * 1000);
		Mix_ResumeMusic ();
	}
}

BOOLEAN
DestroyMusic (MUSIC_REF MusicRef)
{
	return (FreeMusicData (MusicRef));
}

#endif
