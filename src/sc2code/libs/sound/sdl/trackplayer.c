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
#include "SDL_mixer.h"
#include "SDL_sound.h"


#define VOICE_CHANNEL 0

extern int do_subtitles (UNICODE *pStr);

static int tct, tcur, no_voice;
#define MAX_CLIPS 50
static struct
{
	int text_spliced;
	UNICODE *text;
	Mix_Chunk *chunk;
	Uint8 *speechdata;
	Uint32 speechlen;

} track_clip[MAX_CLIPS];



Uint8* Sound_Own_DecodeAll (Sound_Sample *sample, Uint32 bufsize, Uint32 *decoded_bytes)
{
    Uint8 *buf = NULL;
	
	*decoded_bytes = 0;
	buf = (Uint8*) malloc (bufsize);
	if (!buf) 
	{
		fprintf (stderr, "Sound_Own_DecodeAll(): cannot allocate %d bytes\n", bufsize);
		return NULL;
	}

    while ( ((sample->flags & SOUND_SAMPLEFLAG_EOF) == 0) &&
            ((sample->flags & SOUND_SAMPLEFLAG_ERROR) == 0) )
	{
		Uint32 br = Sound_Decode (sample);
		
		if (*decoded_bytes + br >= bufsize)
		{
			Uint8 *newbuf;
			
			bufsize *= 2;
			newbuf = (Uint8*) realloc (buf, bufsize);
			if (!newbuf)
			{
				fprintf (stderr, "Sound_Own_DecodeAll(): cannot realloc %d bytes\n", bufsize);
				return buf;
			}
			buf = newbuf;
		}

		memcpy (&buf[*decoded_bytes], sample->buffer, br);
		*decoded_bytes += br;
	}
    
	//fprintf(stderr, "Sound_Own_DecodeAll(): decoded %d bytes, allocated memory size %d\n", *decoded_bytes, bufsize);
	return buf;	
}

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
		if (track_clip[tct].speechdata)
		{
			//fprintf (stderr, "StopTrack(): freeing atleast %d bytes\n", track_clip[tct].speechlen);
			free (track_clip[tct].speechdata);
			track_clip[tct].speechdata = 0;
			track_clip[tct].speechlen = 0;
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
			Sound_AudioInfo sound_desired;
			Sound_Sample *sample;
			Uint16 format;
			int rate, channels;
			
			//fprintf(stderr, "SpliceTrack(): trackname %s\n", TrackName);

			// this is needed because sample must be converted to mixer
			// format or it doesn't play right
			Mix_QuerySpec (&rate, &format, &channels);
			sound_desired.rate = rate;
			sound_desired.format = format;
			sound_desired.channels = channels;
			
			sample = Sound_NewSampleFromFile (TrackName, &sound_desired, 16384);
			
			if (!sample)
			{
				fprintf (stderr, "SpliceTrack(): couldn't load %s: %s\n",
						TrackName, Sound_GetError());
				track_clip[tct].chunk = NULL;
				track_clip[tct].speechdata = NULL;
				track_clip[tct].speechlen = 0;
			}
			else
			{				
				track_clip[tct].speechdata = Sound_Own_DecodeAll (sample, 1048576, 
					&track_clip[tct].speechlen);
				
				if (track_clip[tct].speechdata)
				{
					track_clip[tct].chunk = Mix_QuickLoad_RAW (track_clip[tct].speechdata, 
						track_clip[tct].speechlen);
					
					if (!track_clip[tct].chunk)
					{
						fprintf (stderr, "SpliceTrack(): Mix_QuickLoad_RAW failed for %s: %s\n", 
							TrackName, Mix_GetError());

						free (track_clip[tct].speechdata);
						track_clip[tct].speechdata = NULL;
						track_clip[tct].speechlen = 0;
					}
				}
				else
				{
					fprintf (stderr, "SpliceTrack(): Sound_Own_DecodeAll failed for %s\n", TrackName);
					track_clip[tct].chunk = NULL;
					track_clip[tct].speechdata = NULL;
					track_clip[tct].speechlen = 0;
				}

				Sound_FreeSample (sample);
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
