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

#ifndef _MENUSTAT_H
#define _MENUSTAT_H

#include "libs/gfxlib.h"
#include "libs/sndlib.h"
#include "libs/tasklib.h"


typedef struct menu_state
{
	BOOLEAN (*InputFunc) (struct menu_state *pMS);
	COUNT MenuRepeatDelay;

	SIZE Initialized;

	BYTE CurState;
	FRAME CurFrame;
	STRING CurString;
	POINT first_item;
	SIZE delta_item;

	FRAME ModuleFrame;
	Task flash_task;
	RECT flash_rect0, flash_rect1;
	FRAME flash_frame0, flash_frame1;

	MUSIC_REF hMusic;

	void *Extra;
} MENU_STATE;

typedef MENU_STATE *PMENU_STATE;

extern PMENU_STATE pMenuState;

#endif /* _MENUSTAT_H */


