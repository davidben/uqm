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

#ifndef SOUND_COMMON_H
#define SOUND_COMMON_H

// driver for TFB_InitSound
enum
{
	TFB_SOUNDDRIVER_MIXSDL,
	TFB_SOUNDDRIVER_OPENAL
};
extern int SoundDriver;

// flags for TFB_InitSound
#define TFB_SOUNDFLAGS_HQAUDIO (1<<0) // high quality audio
#define TFB_SOUNDFLAGS_MQAUDIO (1<<1) // medium quality audio
#define TFB_SOUNDFLAGS_LQAUDIO (1<<2) // low quality audio

int TFB_InitSound (int driver, int flags);
void TFB_UninitSound (void);

extern int musicVolume;
extern float musicVolumeScale;
extern float sfxVolumeScale;
extern float speechVolumeScale;

#endif
