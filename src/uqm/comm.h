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

#ifndef _COMM_H
#define _COMM_H

#include "globdata.h"
#include "libs/compiler.h"
#include "libs/gfxlib.h"

#ifdef COMM_INTERNAL

#define SLIDER_Y 107
#define SLIDER_HEIGHT 15

#include "commanim.h"

extern void DrawAlienFrame (FRAME aframe, SEQUENCE *pSeq);

extern LOCDATA CommData;
extern volatile BOOLEAN ClearSummary;

#endif

extern void init_communication (void);
extern void uninit_communication (void);
extern void AlienTalkSegue (COUNT wait_track);
BOOLEAN getLineWithinWidth(TEXT *pText, const unsigned char **startNext,
		SIZE maxWidth, COUNT maxChars);
extern void RedrawSubtitles (void);
extern BOOLEAN HaveSubtitlesChanged (void);

extern RECT CommWndRect; /* comm window rect */

#endif  /* _COMM_H */


