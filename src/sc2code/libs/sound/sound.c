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

#include "libs/sound/sound_common.h"
#include "sound.h"

TFB_SoundSource soundSource[NUM_SOUNDSOURCES];


void
StopSound (void)
{
	int i;

	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		TFBSound_SourceRewind (soundSource[i].handle);
	}
}

BOOLEAN
SoundPlaying (void)
{
	int i;

	for (i = 0; i < NUM_SOUNDSOURCES; ++i)
	{
		TFB_SoundSample *sample;
		sample = soundSource[i].sample;
		if (sample && sample->decoder)
		{
			BOOLEAN result;
			LockMutex (soundSource[i].stream_mutex);
			result = PlayingStream (i);
			UnlockMutex (soundSource[i].stream_mutex);
			if (result)
				return TRUE;
		}
		else
		{
			TFBSound_IntVal state;
			TFBSound_GetSourcei (soundSource[i].handle, TFBSOUND_SOURCE_STATE, &state);
			if (state == TFBSOUND_PLAYING)
				return TRUE;
		}
	}

	return FALSE;
}

TFB_SoundChain *
create_soundchain (TFB_SoundDecoder *decoder, float startTime)
{
	TFB_SoundChain *chain;
	chain = (TFB_SoundChain *)HMalloc (sizeof (TFB_SoundChain));
	chain->decoder = decoder;
	chain->next =NULL;
	chain->start_time = startTime;
	chain->tag.buf_name = 0;
	chain->tag.type = 0;
	chain->tag.data = 0;
	return (chain);
}

void
destroy_soundchain (TFB_SoundChain *chain)
{
	while (chain->next)
	{
		TFB_SoundChain *tmp_chain = chain->next;
		chain->next = chain->next->next;
		if (tmp_chain->decoder)
			SoundDecoder_Free (tmp_chain->decoder);
		if (tmp_chain->tag.data)
			HFree (tmp_chain->tag.data);
		HFree (tmp_chain);
	}
	SoundDecoder_Free (chain->decoder);
	HFree (chain);
}

TFB_SoundChain *
get_previous_chain (TFB_SoundChain *first_chain, TFB_SoundChain *current_chain)
{
	TFB_SoundChain *prev_chain;
	prev_chain = first_chain;
	if (prev_chain == current_chain)
		return (prev_chain);
	while (prev_chain->next)
	{
		if (prev_chain->next == current_chain)
			return (prev_chain);
		prev_chain = prev_chain->next;
	}
	return (first_chain);
}

// Status: Ignored
BOOLEAN
InitSound (int argc, char* argv[])
{
	return TRUE;
}

// Status: Ignored
void
UninitSound (void)
{
}

void SetSFXVolume (float volume)
{
	int i;
	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		TFBSound_Sourcef (soundSource[i].handle, TFBSOUND_GAIN, volume);
	}	
}

void SetSpeechVolume (float volume)
{
	TFBSound_Sourcef (soundSource[SPEECH_SOURCE].handle, TFBSOUND_GAIN, volume);
}

