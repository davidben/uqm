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

#include "libs/graphics/sdl/sdl_common.h"
#include "libs/sound/sound_common.h"
#include "libs/sound/trackplayer.h"
#include <SDL/SDL_mixer.h>


#define VOICE_CHANNEL 0

extern int do_subtitles (UNICODE *pStr);

static int tct, tcur, no_voice;
#define MAX_CLIPS 50
static struct
{
	int text_spliced;
	UNICODE *text;
	Mix_Chunk *chunk;
} track_clip[MAX_CLIPS];

void
JumpTrack (int abort)
{
	Mix_ChannelFinished (0);

	if (track_clip[tcur].chunk)
		Mix_HaltChannel (VOICE_CHANNEL);

	no_voice = 1;
	do_subtitles ((void *)~0);

	if (abort)
		tcur = tct;
}

static void
advance_track (int channel_finished)
{
	if (channel_finished <= VOICE_CHANNEL)
	{
		if (channel_finished == VOICE_CHANNEL)
			++tcur;
		if (tcur < tct)
		{
			Mix_PlayChannel (VOICE_CHANNEL, track_clip[tcur].chunk, 0);
			do_subtitles (0);
		}
		else if (channel_finished == VOICE_CHANNEL)
		{
			--tcur;
			no_voice = 1;
		}
	}
}

void
ResumeTrack ()
{
	if (tcur < tct)
	{
		if (Mix_Playing /* Mix_Paused */ (VOICE_CHANNEL))
			Mix_Resume (VOICE_CHANNEL);
		else if (!no_voice)
		{
			if (tcur == 0)
				Mix_ChannelFinished (advance_track);
			advance_track (-1);
		}
	}
}

COUNT
PlayingTrack ()
{
	if (tcur < tct)
	{
		if (do_subtitles (track_clip[tcur].text)
				|| (no_voice && ++tcur < tct && do_subtitles (0)))
			return (tcur + 1);
		else if (track_clip[tcur].chunk)
			return (Mix_Playing (VOICE_CHANNEL) ? (COUNT)(tcur + 1) : 0);
	}

	return (0);
}

void
StopTrack ()
{
	Mix_ChannelFinished (0);
	Mix_HaltChannel (VOICE_CHANNEL);

	while (tct--)
	{
		if (track_clip[tct].text_spliced)
		{
			track_clip[tct].text_spliced = 0;
			HFree (track_clip[tct].text);
			track_clip[tct].text = 0;
		}
		if (track_clip[tct].chunk)
		{
			Mix_FreeChunk (track_clip[tct].chunk);
			track_clip[tct].chunk = 0;
		}
	}
	tct = tcur = 0;
	no_voice = 0;
	do_subtitles (0);
}

void
SpliceTrack (UNICODE *TrackName, UNICODE *TrackText)
{
	if (TrackText)
	{
		if (TrackName == 0)
		{
			if (tct)
			{
				int slen1, slen2;
				UNICODE *oTT;

				oTT = track_clip[tct - 1].text;
				slen1 = wstrlen (oTT);
				slen2 = wstrlen (TrackText);
				if (track_clip[tct - 1].text_spliced)
					track_clip[tct - 1].text = HRealloc (oTT, slen1 + slen2 + 1);
				else
				{
					track_clip[tct - 1].text = HMalloc (slen1 + slen2 + 1);
					wstrcpy (track_clip[tct - 1].text, oTT);
					track_clip[tct - 1].text_spliced = 1;
				}
				wstrcpy (&track_clip[tct - 1].text[slen1], TrackText);
			}
		}
		else if (tct < MAX_CLIPS)
		{			
			fprintf(stderr, "SpliceTrack(): loading %s\n", TrackName);

			track_clip[tct].chunk = Mix_LoadWAV (TrackName);		
			if (!track_clip[tct].chunk)
			{
				fprintf (stderr, "SpliceTrack(): Mix_LoadWAV failed for %s: %s\n",
						 TrackName, Mix_GetError());
			}

			track_clip[tct].text = TrackText;
			track_clip[tct].text_spliced = 0;
			++tct;
		}
	}
}

void
PauseTrack ()
{
	if (tcur < tct && track_clip[tcur].chunk)
		Mix_Pause (VOICE_CHANNEL);
}

void
FastReverse ()
{
	JumpTrack (0);
	no_voice = 0;
	tcur = 0;
	ResumeTrack ();
}

void
FastForward ()
{
	JumpTrack (0);
//    no_voice = 0;
}

#endif
