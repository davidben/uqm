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

#include "options.h"
#include "sound.h"
#include "libs/reslib.h"
#include "libs/log.h"
#include <math.h>


static void CheckFinishedChannels (void);


void
PlayChannel (COUNT channel, PVOID sample, SoundPosition pos,
		void *positional_object, unsigned char priority)
{
	TFB_SoundSample *tfb_sample = *(TFB_SoundSample**) sample;

	StopSource (channel);
	// all finished (stopped) channels can be cleaned up at this point
	// since this is the only func that can initiate an sfx sound
	CheckFinishedChannels ();
	
	soundSource[channel].sample = tfb_sample;
	soundSource[channel].positional_object = positional_object;
	
	if (optStereoSFX)
		UpdateSoundPosition (channel, pos);

	audio_Sourcei (soundSource[channel].handle, audio_BUFFER,
			tfb_sample->buffer[0]);
	audio_SourcePlay (soundSource[channel].handle);
	(void) priority;
}

void
StopChannel (COUNT channel, unsigned char Priority)
{
	StopSource (channel);
	(void)Priority; // ignored
}

static void
CheckFinishedChannels (void)
{
	int i;

	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		audio_IntVal state;

		audio_GetSourcei (soundSource[i].handle, audio_SOURCE_STATE,
				&state);
		if (state == audio_STOPPED)
		{
			CleanSource (i);
			// and if it failed... we still dont care
			audio_GetError();
		}
	}
}

BOOLEAN
ChannelPlaying (COUNT WhichChannel)
{
	audio_IntVal state;
	
	audio_GetSourcei (soundSource[WhichChannel].handle,
			audio_SOURCE_STATE, &state);
	if (state == audio_PLAYING)
		return TRUE;
	return FALSE;
}

void *
GetPositionalObject (COUNT channel)
{
	return soundSource[channel].positional_object;
}

void
SetPositionalObject (COUNT channel, void *positional_object)
{
	soundSource[channel].positional_object = positional_object;
}

void
UpdateSoundPosition (COUNT channel, SoundPosition pos)
{
	const float ATTENUATION = 160.0f;
	const float MIN_DISTANCE = 0.5f;
	float fpos[3];

	if (pos.positional)
	{
		float dist;

		fpos[0] = pos.x / ATTENUATION;
		fpos[1] = 0.0f;
		fpos[2] = pos.y / ATTENUATION;
		dist = (float) sqrt (fpos[0] * fpos[0] + fpos[2] * fpos[2]);
		if (dist < MIN_DISTANCE)
		{	// object is too close to listener
			// move it away along the same vector
			float scale = MIN_DISTANCE / dist;
			fpos[0] *= scale;
			fpos[2] *= scale;
		}

		audio_Sourcefv (soundSource[channel].handle, audio_POSITION, fpos);
		//log_add (log_Debug, "UpdateSoundPosition(): channel %d, pos %d %d, posobj %x",
		//		channel, pos.x, pos.y, (unsigned int)soundSource[channel].positional_object);
	}
	else
	{
		fpos[0] = fpos[1] = 0.0f;
		fpos[2] = -1.0f;
		audio_Sourcefv (soundSource[channel].handle, audio_POSITION, fpos);
	}
}

void
SetChannelVolume (COUNT channel, COUNT volume, BYTE priority)
		// I wonder what this whole priority business is...
		// I can probably ignore it.
{
	audio_Sourcef (soundSource[channel].handle, audio_GAIN, 
		(volume / (float)MAX_VOLUME) * sfxVolumeScale);
	(void)priority; // ignored
}

// Status: Ignored
PBYTE
GetSampleAddress (SOUND sound)
		// I might be prototyping this wrong, type-wise.
{
	return ((PBYTE)GetSoundAddress (sound));
}

// Status: Ignored
COUNT
GetSampleLength (SOUND sound)
{
	(void)sound;
	return 0;
}

// Status: Ignored
void
SetChannelRate (COUNT channel, DWORD rate_hz, unsigned char priority)
{
	(void) channel;
	(void) rate_hz;
	(void) priority;
}

// Status: Ignored
COUNT
GetSampleRate (SOUND sound)
{
	(void) sound;
	return 0;
}

MEM_HANDLE
_GetSoundBankData (uio_Stream *fp, DWORD length)
{
	int snd_ct, n;
	DWORD opos;
	char CurrentLine[1024], filename[1024];
#define MAX_FX 256
	TFB_SoundSample *sndfx[MAX_FX];
	STRING_TABLE Snd;

	(void) length;  // ignored
	opos = uio_ftell (fp);

	{
		char *s1, *s2;

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

	snd_ct = 0;
	while (uio_fgets (CurrentLine, sizeof (CurrentLine), fp) &&
			snd_ct < MAX_FX)
	{
		if (sscanf(CurrentLine, "%s", &filename[n]) == 1)
		{
			log_add (log_Info, "_GetSoundBankData(): loading %s", filename);

			sndfx[snd_ct] = (TFB_SoundSample *) HCalloc (sizeof (TFB_SoundSample));

			sndfx[snd_ct]->decoder = SoundDecoder_Load (contentDir,
					filename, 4096, 0, 0);
			if (!sndfx[snd_ct]->decoder)
			{
				log_add (log_Warning, "_GetSoundBankData(): couldn't load %s", filename);
				HFree (sndfx[snd_ct]);
			}
			else
			{
				uint32 decoded_bytes;

				decoded_bytes = SoundDecoder_DecodeAll (sndfx[snd_ct]->decoder);
				log_add (log_Info, "_GetSoundBankData(): decoded_bytes %d", decoded_bytes);
				
				sndfx[snd_ct]->num_buffers = 1;
				sndfx[snd_ct]->buffer = (audio_Object *) HMalloc (
						sizeof (audio_Object) * sndfx[snd_ct]->num_buffers);
				audio_GenBuffers (sndfx[snd_ct]->num_buffers, sndfx[snd_ct]->buffer);
				audio_BufferData (sndfx[snd_ct]->buffer[0], sndfx[snd_ct]->decoder->format,
					sndfx[snd_ct]->decoder->buffer, decoded_bytes,
					sndfx[snd_ct]->decoder->frequency);

				SoundDecoder_Free (sndfx[snd_ct]->decoder);
				sndfx[snd_ct]->decoder = NULL;
				
				++snd_ct;
			}
		}
		else
		{
			log_add (log_Warning, "_GetSoundBankData: Bad file!");
		}

		// pkunk insult fix 2002/11/12 (ftell shouldn't be needed for loop to terminate)
		/*if (uio_ftell (fp) - opos >= length)
			break;*/
	}

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
			{
				if (sndfx[snd_ct]->decoder)
					SoundDecoder_Free (sndfx[snd_ct]->decoder);
				HFree (sndfx[snd_ct]);
			}

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
		TFB_SoundSample **sptr;

		snd_ct = fxTab->StringCount;
		sptr = (TFB_SoundSample **)((BYTE *)fxTab + fxTab->StringOffsets[0]);
		while (snd_ct--)
		{
			int i;
			
			for (i = 0; i < NUM_SOUNDSOURCES; ++i)
			{
				if (soundSource[i].sample == (*sptr))
				{
					StopSource (i);
					soundSource[i].sample = NULL;
				}
			}

            if ((*sptr)->decoder)
			    SoundDecoder_Free ((*sptr)->decoder);
			audio_DeleteBuffers ((*sptr)->num_buffers, (*sptr)->buffer);
			HFree ((*sptr)->buffer);
			if ((*sptr)->buffer_tag)
				HFree ((*sptr)->buffer_tag);
			HFree (*sptr);
			*sptr++ = 0;
		}
		UnlockStringTable (Snd);
		FreeStringTable (Snd);

		return (TRUE);
	}

	return (FALSE);
}

BOOLEAN
DestroySound(SOUND_REF target)
{
	return _ReleaseSoundBankData ((MEM_HANDLE) target);
}
