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

#ifndef TRACKPLAYER_H
#define TRACKPLAYER_H

#include "compiler.h"


typedef void (*TFB_TrackCB) (void);

#define ACCEL_SCROLL_SPEED 300

void ResumeTrack(void);
void PauseTrack(void);
COUNT PlayingTrack(void);
void JumpTrack(void);
void FastForward_Smooth(void);
int FastForward_Page(void);
void FastReverse_Smooth(void);
void FastReverse_Page(void);
void StopTrack(void);
void SpliceTrack(UNICODE *filespec, UNICODE *textspec, UNICODE *TimeStamp, TFB_TrackCB cb);
void SpliceMultiTrack (UNICODE *TrackNames[], UNICODE *TrackText);
int GetSoundData (void *data);
int GetSoundInfo (int max_len);

#endif
