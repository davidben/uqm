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

#ifndef _WEAPON_H
#define _WEAPON_H

#include "element.h"
#include "libs/gfxlib.h"

typedef struct
{
	COORD cx, cy, ex, ey;
	ELEMENT_FLAGS sender;
	SIZE pixoffs;
	COUNT face;
	COLOR color;
} LASER_BLOCK;
typedef LASER_BLOCK *PLASER_BLOCK;

typedef struct
{
	COORD cx, cy;
	ELEMENT_FLAGS sender;
	SIZE pixoffs, speed, hit_points, damage;
	COUNT face, index, life;
	PFRAME farray;
	void (*preprocess_func) (PELEMENT ElementPtr);
	SIZE blast_offs;
} MISSILE_BLOCK;
typedef MISSILE_BLOCK *PMISSILE_BLOCK;

extern HELEMENT initialize_laser (PLASER_BLOCK pLaserBlock);
extern HELEMENT initialize_missile (PMISSILE_BLOCK pMissileBlock);
extern HELEMENT weapon_collision (PELEMENT ElementPtr0, PPOINT pPt0,
		PELEMENT ElementPtr1, PPOINT pPt1);
extern SIZE TrackShip (ELEMENTPTR Tracker, PCOUNT pfacing);
extern void Untarget (ELEMENTPTR ElementPtr);

#define MODIFY_IMAGE (1 << 0)
#define MODIFY_SWAP (1 << 1)

extern FRAME ModifySilhouette (ELEMENTPTR ElementPtr, PSTAMP
		modify_stamp, BYTE modify_flags);

#endif /* _WEAPON_H */

