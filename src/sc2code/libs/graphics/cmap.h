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

#ifndef CMAP_H
#define CMAP_H

#define NUMBER_OF_VARPLUTS      256
#define NUMBER_OF_PLUTVALS      32
#define NUMBER_OF_PLUT_UINT32s  (NUMBER_OF_PLUTVALS >> 1)
#define PLUT_BYTE_SIZE          (sizeof (unsigned int) * \
		NUMBER_OF_PLUT_UINT32s)
#define NUMBER_OF_CLUTVALS      32
#define VARPLUTS_SIZE           (NUMBER_OF_VARPLUTS * \
		NUMBER_OF_PLUT_UINT32s * sizeof (unsigned int))
#define GET_VAR_PLUT(i)         (_varPLUTs + (i) * NUMBER_OF_PLUT_UINT32s)

#define BUILD_FRAME    (1 << 0)
#define FIND_PAGE      (1 << 1)
#define FIRST_BATCH    (1 << 2)
#define GRAB_OTHER     (1 << 3)
#define COLOR_CYCLE    (1 << 4)
#define CYCLE_PENDING  (1 << 5)
#define ENABLE_CYCLE   (1 << 6)

#define FADE_NO_INTENSITY      0
#define FADE_NORMAL_INTENSITY  255
#define FADE_FULL_INTENSITY    510

enum
{
	TFB_COLORMAP_NONE,
	TFB_COLORMAP_HYPERSPACE,
	TFB_COLORMAP_QUASISPACE,
	TFB_COLORMAP_PLANET,
	TFB_COLORMAP_COMM
};

extern DWORD* _varPLUTs;
extern volatile int FadeAmount;

BOOLEAN TFB_HasColorMap (void);
int TFB_GetColorMapType (void);
void TFB_ReleaseColorMap (void);
BOOLEAN TFB_CopyRGBColorMap (TFB_Palette *dst);

#endif
