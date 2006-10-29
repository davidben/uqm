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

#ifndef _COMMANIM_H
#define _COMMANIM_H

#include "libs/compiler.h"
#include "libs/gfxlib.h"
#include "libs/tasklib.h"

#ifdef COMM_INTERNAL
typedef enum
{
	UP_DIR,
	DOWN_DIR,
	NO_DIR
} ANIM_DIR;

typedef enum
{
	PICTURE_ANIM,
	COLOR_ANIM
} ANIM_TYPE;

struct SEQUENCE
{
	COUNT Alarm;
	ANIM_DIR Direction;
	BYTE FramesLeft;
	ANIM_TYPE AnimType;
	union
	{
		FRAME CurFrame;
		COLORMAP CurCMap;
	} AnimObj;
};
#endif

typedef struct SEQUENCE SEQUENCE;
typedef SEQUENCE *PSEQUENCE;

void UpdateSpeechGraphics (BOOLEAN Initialize);
Task StartCommAnimTask(void);

extern volatile BOOLEAN PauseAnimTask;

#endif  /* _COMMANIM_H */


