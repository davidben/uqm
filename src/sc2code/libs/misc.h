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

// This file includes some misc things, previously in SDL_wrapper.h 
// before modularization. -Mika

#ifndef MISC_H
#define MISC_H

#define Stream FILE
#define MEMTYPE_ANY  1

#define SqrtF16 sqrt
#define Atan2F16 atan2
#define CosF16 cos
#define SinF16 sin

int CD_get_drive(void);  // Returns 0
unsigned int CD_get_volsize(void);  // Returns 0

void OpenMathFolio(void);

extern void *HMalloc (int size);
extern void HFree (void *p);
extern void *HCalloc (int size);
extern void *HRealloc (void *p, int size);

extern int TFB_DEBUG_HALT;

#endif
