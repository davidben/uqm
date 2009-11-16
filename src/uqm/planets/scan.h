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

#ifndef _SCAN_H
#define _SCAN_H

#include "libs/gfxlib.h"
#include "../menustat.h"


typedef struct
{
	POINT start;
	COUNT start_dot;
	COUNT num_dots;
	COUNT dots_per_semi;
} SCAN_DESC;

typedef struct
{
	POINT *line_base;
	COUNT num_scans;
	COUNT num_same_scans;
	SCAN_DESC *scan_base;
} SCAN_BLOCK;

#define SAME_SCAN (1 << 16)
#define SCALED_ROOT_TWO 92682L /* root 2 * (1 << 16) */
#define SCALE_FACTOR 16

extern void RepairBackRect (RECT *pRect);
extern void GeneratePlanetSide (void);

extern void RedrawSurfaceScan (const POINT *newLoc);

#endif /* _SCAN_H */

