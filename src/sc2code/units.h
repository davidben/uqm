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

#ifndef _UNITS_H
#define _UNITS_H

#include "libs/gfxlib.h"

extern int ScreenWidth;
extern int ScreenHeight;

#define SCREEN_WIDTH ScreenWidth
#define SCREEN_HEIGHT ScreenHeight
#define SAFE_X 0
		/* Left and right screen margin to be left unused */
#define SAFE_Y 0
		/* Top and bottom screen margin to be left unused */
#define SIS_ORG_X (7 + SAFE_X)
#define SIS_ORG_Y (10 + SAFE_Y)
#define STATUS_WIDTH 64
		/* Width of the status "window" (the right part of the screen) */
#define STATUS_HEIGHT (SCREEN_HEIGHT - (SAFE_Y * 2))
		/* Height of the status "window" (the right part of the screen) */
#define SPACE_WIDTH (SCREEN_WIDTH - STATUS_WIDTH - (SAFE_X * 2))
		/* Width of the space "window" (the left part of the screen) */
#define SPACE_HEIGHT (SCREEN_HEIGHT - (SAFE_Y * 2))
		/* Height of the space "window" (the left part of the screen) */
#define SIS_SCREEN_WIDTH (SPACE_WIDTH - 14)
		/* Width of the usable part of the space "window" */
#define SIS_SCREEN_HEIGHT (SPACE_HEIGHT - 13)
		/* Height of the usable part of the space "window" */


#define MAX_REDUCTION 3
#define MAX_VIS_REDUCTION 2
#define REDUCTION_SHIFT 1
#define NUM_VIEWS (MAX_VIS_REDUCTION + 1)

#define ZOOM_SHIFT 8
#define MAX_ZOOM_OUT (1 << (ZOOM_SHIFT + MAX_REDUCTION - 1))

#define ONE_SHIFT 2
#define BACKGROUND_SHIFT 3
#define SCALED_ONE (1 << ONE_SHIFT)
#define DISPLAY_TO_WORLD(x) ((x)<<ONE_SHIFT)
#define WORLD_TO_DISPLAY(x) ((x)>>ONE_SHIFT)
#define DISPLAY_ALIGN(x) ((COORD)(x)&~(SCALED_ONE-1))
#define DISPLAY_ALIGN_X(x) ((COORD)((COUNT)(x)%LOG_SPACE_WIDTH)&~(SCALED_ONE-1))
#define DISPLAY_ALIGN_Y(y) ((COORD)((COUNT)(y)%LOG_SPACE_HEIGHT)&~(SCALED_ONE-1))

#define LOG_SPACE_WIDTH \
		(DISPLAY_TO_WORLD (SPACE_WIDTH) << MAX_REDUCTION)
#define LOG_SPACE_HEIGHT \
		(DISPLAY_TO_WORLD (SPACE_HEIGHT) << MAX_REDUCTION)
#define TRANSITION_WIDTH \
		(DISPLAY_TO_WORLD (SPACE_WIDTH) << MAX_VIS_REDUCTION)
#define TRANSITION_HEIGHT \
		(DISPLAY_TO_WORLD (SPACE_HEIGHT) << MAX_VIS_REDUCTION)
		
#define MAX_X_UNIVERSE 9999
#define MAX_Y_UNIVERSE 9999
#define MAX_X_LOGICAL \
		((UNIVERSE_TO_LOGX (MAX_X_UNIVERSE + 1) > UNIVERSE_TO_LOGX (-1) ? \
				UNIVERSE_TO_LOGX (MAX_X_UNIVERSE) : UNIVERSE_TO_LOGX (-1)) \
				- 1L)
#define MAX_Y_LOGICAL \
		((UNIVERSE_TO_LOGY (MAX_Y_UNIVERSE + 1) > UNIVERSE_TO_LOGY (-1) ? \
				UNIVERSE_TO_LOGY (MAX_Y_UNIVERSE) : UNIVERSE_TO_LOGY (-1)) \
				- 1L)
#define SECTOR_WIDTH 195
#define SECTOR_HEIGHT 25

#define SPHERE_RADIUS_INCREMENT 11

#define MAX_FLEET_STRENGTH (254 * SPHERE_RADIUS_INCREMENT)

#define UNIT_SCREEN_WIDTH 63
#define UNIT_SCREEN_HEIGHT 50

static inline COORD
logxToUniverse (SDWORD lx)
{
	return (COORD) ((lx * ((MAX_X_UNIVERSE + 1) >> 4)) * 10
			/ ((SDWORD) ((LOG_SPACE_WIDTH) >> 4) * SECTOR_WIDTH));
}
#define LOGX_TO_UNIVERSE(lx) \
		logxToUniverse (lx)
static inline COORD
logyToUniverse (SDWORD ly)
{
	return (COORD) (MAX_Y_UNIVERSE -
			(COORD)(((SDWORD) (ly) * ((MAX_Y_UNIVERSE + 1) >> 4))
			/ ((SDWORD) ((LOG_SPACE_HEIGHT) >> 4) * SECTOR_HEIGHT)));
}
#define LOGY_TO_UNIVERSE(ly) \
		logyToUniverse (ly)
static inline SDWORD
universeToLogx (COORD ux)
{
	return ((SDWORD) ux * ((SDWORD) ((LOG_SPACE_WIDTH) >> 4) * SECTOR_WIDTH)
			+ ((((MAX_X_UNIVERSE + 1) >> 4) * 10) >> 1))
			/ (((MAX_X_UNIVERSE + 1) >> 4) * 10);
}
#define UNIVERSE_TO_LOGX(ux) \
		universeToLogx (ux)
static inline SDWORD
universeToLogy (COORD uy)
{
	return ((SDWORD) (MAX_Y_UNIVERSE - uy)
			* ((SDWORD) ((LOG_SPACE_HEIGHT) >> 4) * SECTOR_HEIGHT)
			+ (((MAX_Y_UNIVERSE + 1) >> 4) >> 1))
			/ ((MAX_Y_UNIVERSE + 1) >> 4);
}
#define UNIVERSE_TO_LOGY(uy) \
		universeToLogy (uy)

#define CIRCLE_SHIFT 6
#define FULL_CIRCLE (1 << CIRCLE_SHIFT)
#define OCTANT_SHIFT (CIRCLE_SHIFT - 3) /* (1 << 3) == 8 */
#define HALF_CIRCLE (FULL_CIRCLE >> 1)
#define QUADRANT (FULL_CIRCLE >> 2)
#define OCTANT (FULL_CIRCLE >> 3)

#define FACING_SHIFT 4

#define ANGLE_TO_FACING(a) (((a)+(1<<(CIRCLE_SHIFT-FACING_SHIFT-1))) \
										>>(CIRCLE_SHIFT-FACING_SHIFT))
#define FACING_TO_ANGLE(f) ((f)<<(CIRCLE_SHIFT-FACING_SHIFT))

#define NORMALIZE_ANGLE(a) ((COUNT)(a)&(COUNT)(FULL_CIRCLE-1))
#define NORMALIZE_FACING(f) ((COUNT)(f)&((1 << FACING_SHIFT)-1))

#define DEGREES_TO_ANGLE(d) NORMALIZE_ANGLE((((d) % 360) * FULL_CIRCLE \
				+ HALF_CIRCLE) / 360)
#define ANGLE_TO_DEGREES(d) (NORMALIZE_ANGLE(d) * 360 / FULL_CIRCLE)

#define SIN_SHIFT 14
#define SIN_SCALE (1 << SIN_SHIFT)
#define INT_ADJUST(x) ((x)<<SIN_SHIFT)
#define FLT_ADJUST(x) (SIZE)((x)*SIN_SCALE)
#define UNADJUST(x) (SIZE)((x)>>SIN_SHIFT)
#define ROUND(x,y) ((x)+((x)>=0?((y)>>1):-((y)>>1)))

extern SIZE sinetab[];
#define SINVAL(a) sinetab[NORMALIZE_ANGLE(a)]
#define COSVAL(a) SINVAL((a)+QUADRANT)
#define SINE(a,m) ((SIZE)((((long)SINVAL(a))*(long)(m))>>SIN_SHIFT))
#define COSINE(a,m) SINE((a)+QUADRANT,m)
extern COUNT ARCTAN (SIZE delta_x, SIZE delta_y);

#define WRAP_VAL(v,w) ((COUNT)((v)<0?((v)+(w)):((v)>=(w)?((v)-(w)):(v))))
#define WRAP_X(x) WRAP_VAL(x,LOG_SPACE_WIDTH)
#define WRAP_Y(y) WRAP_VAL(y,LOG_SPACE_HEIGHT)
#define WRAP_DELTA_X(dx) ((dx)<0 ? \
				((-(dx)<=LOG_SPACE_WIDTH>>1)?(dx):(LOG_SPACE_WIDTH+(dx))) : \
				(((dx)<=LOG_SPACE_WIDTH>>1)?(dx):((dx)-LOG_SPACE_WIDTH)))
#define WRAP_DELTA_Y(dy) ((dy)<0 ? \
				((-(dy)<=LOG_SPACE_HEIGHT>>1)?(dy):(LOG_SPACE_HEIGHT+(dy))) : \
				(((dy)<=LOG_SPACE_HEIGHT>>1)?(dy):((dy)-LOG_SPACE_HEIGHT)))
#endif /* _UNITS_H */

