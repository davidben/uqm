// Copyright Michael Martin, 2004.

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

#ifndef _SETUPMENU_H
#define _SETUPMENU_H

typedef enum {
	OPTVAL_DISABLED,
	OPTVAL_ENABLED
} OPT_ENABLABLE;

typedef enum {
	OPTVAL_PC,
	OPTVAL_3DO
} OPT_CONSOLETYPE;

typedef enum {
	OPTVAL_NO_SCALE,
	OPTVAL_BILINEAR_SCALE,
	OPTVAL_BIADAPT_SCALE,
	OPTVAL_BIADV_SCALE,
	OPTVAL_TRISCAN_SCALE
} OPT_SCALETYPE;

typedef struct globalopts_struct {
	OPT_SCALETYPE scaler;
	OPT_ENABLABLE subtitles, scanlines;
	OPT_CONSOLETYPE music, menu, text, cscan, scroll;
} GLOBALOPTS;

void SetupMenu (void);

void GetGlobalOptions (GLOBALOPTS *opts);
void SetGlobalOptions (GLOBALOPTS *opts);

#endif // _SETUPMENU_H
