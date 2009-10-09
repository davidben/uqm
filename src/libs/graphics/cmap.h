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

#include "tfb_draw.h"

#ifndef CMAP_H
#define CMAP_H

#define MAX_COLORMAPS           250
#define PLUTVAL_BYTE_SIZE       3
#define NUMBER_OF_PLUTVALS      256
#define PLUT_BYTE_SIZE          (PLUTVAL_BYTE_SIZE * NUMBER_OF_PLUTVALS)

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

typedef struct tfb_colormap
{
	TFB_Palette colors[NUMBER_OF_PLUTVALS];
	int index;
	int version;
	int refcount;
	struct tfb_colormap *next;
} TFB_ColorMap;

extern volatile int FadeAmount;

extern void InitColorMaps (void);
extern void UninitColorMaps (void);

extern void TFB_ColorMapToRGB (TFB_Palette *pal, int colormap_index);
extern TFB_ColorMap * TFB_GetColorMap (int index);
extern void TFB_ReturnColorMap (TFB_ColorMap *map);

extern BOOLEAN XFormColorMap_step (void);


#endif
