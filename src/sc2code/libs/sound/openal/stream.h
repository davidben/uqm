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

/* By Mika Kolehmainen, 2002-10-23
 */

#ifndef STREAM_H
#define STREAM_H

void PlayStream (TFB_SoundSample *sample, ALuint source, ALboolean looping, 
		ALboolean scope, ALboolean rewind);
void StopStream (ALuint source);
void PauseStream (ALuint source);
void ResumeStream (ALuint source);
BOOLEAN PlayingStream (ALuint source);
int StreamDecoderTaskFunc (void *data);

extern BOOLEAN speech_advancetrack;
void advance_track (int channel_finished);

#endif
