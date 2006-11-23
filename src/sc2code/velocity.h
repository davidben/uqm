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

#ifndef _VELOCITY_H
#define _VELOCITY_H

#include <string.h> /* for memset */
#include "libs/gfxlib.h"

typedef struct velocity_desc
{
	COUNT TravelAngle;
	EXTENT vector;
	EXTENT fract;
	EXTENT error;
	EXTENT incr;
} VELOCITY_DESC;
typedef VELOCITY_DESC *PVELOCITY;
typedef PVELOCITY VELOCITYPTR;

#define ZeroVelocityComponents(pv) memset(pv,0,sizeof (*(pv)))
#define GetVelocityTravelAngle(pv) (pv)->TravelAngle

extern void GetCurrentVelocityComponents (VELOCITYPTR velocityptr, PSIZE
		pdx, PSIZE pdy);
extern void GetNextVelocityComponents (VELOCITYPTR velocityptr, PSIZE
		pdx, PSIZE pdy, COUNT num_frames);
extern void SetVelocityVector (VELOCITYPTR velocityptr, SIZE magnitude,
		COUNT facing);
extern void SetVelocityComponents (VELOCITYPTR velocityptr, SIZE dx, SIZE
		dy);
extern void DeltaVelocityComponents (VELOCITYPTR velocityptr, SIZE dx,
		SIZE dy);

static inline DWORD
VelocitySquared (SIZE dx, SIZE dy)
{
	return (DWORD)((long)dx * dx + (long)dy * dy);
}

#define VELOCITY_SHIFT 5
#define VELOCITY_SCALE (1<<VELOCITY_SHIFT)

#define VELOCITY_TO_WORLD(v) ((v)>>VELOCITY_SHIFT)
#define WORLD_TO_VELOCITY(l) ((l)<<VELOCITY_SHIFT)

#endif /* _VELOCITY_H */

