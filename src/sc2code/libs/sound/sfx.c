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

#ifdef WIN32
#include <io.h>
#endif
#include <fcntl.h>

#include "sound.h"

static void CheckFinishedChannels ();


void
PlayChannel (COUNT channel, PVOID sample, COUNT sample_length, COUNT loop_begin, 
			 COUNT loop_length, unsigned char priority)
{
	TFB_SoundSample *tfb_sample = *(TFB_SoundSample**) sample;	
	StopSource (channel);
	// all finished (stopped) channels can be cleaned up at this point
	// since this is the only func that can initiate an sfx sound
	CheckFinishedChannels ();
	soundSource[channel].sample = tfb_sample;
	TFBSound_Sourcei (soundSource[channel].handle, TFBSOUND_BUFFER,
			tfb_sample->buffer[0]);
	TFBSound_SourcePlay (soundSource[channel].handle);
}

void
StopChannel (COUNT channel, unsigned char Priority)
{
	StopSource (channel);
}

static void
CheckFinishedChannels ()
{
	int i;

	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		TFBSound_IntVal state;

		TFBSound_GetSourcei (soundSource[i].handle, TFBSOUND_SOURCE_STATE,
				&state);
		if (state == TFBSOUND_STOPPED)
		{
			CleanSource (i);
			// and if it failed... we still dont care
			TFBSound_GetError();
		}
	}
}

BOOLEAN
ChannelPlaying (COUNT WhichChannel)
{
	TFBSound_IntVal state;
	
	TFBSound_GetSourcei (soundSource[WhichChannel].handle,
			TFBSOUND_SOURCE_STATE, &state);
	if (state == TFBSOUND_PLAYING)
		return TRUE;
	return FALSE;
}

void
SetChannelVolume (COUNT channel, COUNT volume, BYTE priority)
		// I wonder what this whole priority business is...
		// I can probably ignore it.
{
	TFBSound_Sourcef (soundSource[channel].handle, TFBSOUND_GAIN, 
		(volume / (float)MAX_VOLUME) * sfxVolumeScale);
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
	return 0;
}

// Status: Ignored
void
SetChannelRate (COUNT channel, DWORD rate_hz, unsigned char priority)
{
}

// Status: Ignored
COUNT
GetSampleRate (SOUND sound)
{
	return 0;
}

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
	TFB_SoundSample *sndfx[MAX_FX];
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
		if (sscanf(CurrentLine, "%s", &filename[n]) == 1)
		{
			fprintf (stderr, "_GetSoundBankData(): loading %s\n", filename);

			sndfx[snd_ct] = (TFB_SoundSample *) HMalloc (sizeof (TFB_SoundSample));
			sndfx[snd_ct]->buffer_tag = 0;
			sndfx[snd_ct]->read_chain_ptr = NULL;

			sndfx[snd_ct]->decoder = SoundDecoder_Load (filename, 4096, 0, 0);
			if (!sndfx[snd_ct]->decoder)
			{
				fprintf (stderr, "_GetSoundBankData(): couldn't load %s\n", filename);
				HFree (sndfx[snd_ct]);
			}
			else
			{
				uint32 decoded_bytes;

				decoded_bytes = SoundDecoder_DecodeAll (sndfx[snd_ct]->decoder);
				//fprintf (stderr, "_GetSoundBankData(): decoded_bytes %d\n", decoded_bytes);
				
				sndfx[snd_ct]->num_buffers = 1;
				sndfx[snd_ct]->buffer = (TFBSound_Object *) HMalloc (
						sizeof (TFBSound_Object) * sndfx[snd_ct]->num_buffers);
				TFBSound_GenBuffers (sndfx[snd_ct]->num_buffers, sndfx[snd_ct]->buffer);
				TFBSound_BufferData (sndfx[snd_ct]->buffer[0], sndfx[snd_ct]->decoder->format,
					sndfx[snd_ct]->decoder->buffer, decoded_bytes,
					sndfx[snd_ct]->decoder->frequency);

				SoundDecoder_Free (sndfx[snd_ct]->decoder);
				sndfx[snd_ct]->decoder = NULL;
				
				++snd_ct;
			}
		}
		else
		{
			fprintf (stderr, "_GetSoundBankData: Bad file!\n");
		}

		// pkunk insult fix 2002/11/12 (ftell shouldn't be needed for loop to terminate)
		/*if (ftell (fp) - opos >= length)
			break;*/
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
			TFBSound_DeleteBuffers ((*sptr)->num_buffers, (*sptr)->buffer);
			HFree ((*sptr)->buffer);
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
