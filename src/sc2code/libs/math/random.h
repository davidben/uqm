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
 
/****************************************************************************
* FILE: random.h
* DESC: definitions and externs for random number generators
*
* HISTORY: Created 6/ 6/1989
* LAST CHANGED:
*
* Copyright (c) 1989, Robert Leyland and Scott Anderson
****************************************************************************/

/* ----------------------------DEFINES------------------------------------ */

#ifndef SLOW_N_STUPID
#define TABLE_SIZE 1117 /* a "nice" prime number */
#define _FAST_ fast_random()
#else /* FAST_N_UGLY */
#define TABLE_SIZE ( (1 << 10) - 1 )
#define _FAST_ ( random_table[ fast_index++ & TABLE_SIZE ] )
#endif


#define FASTRAND(n) ( (int) ( (unsigned int)_FAST_ % (n) ) )
#define SFASTRAND(n) ( (int)_FAST_ % (n) )
#define AND_FASTRAND(n) ( (int)_FAST_ & (n) )

#define RAND(n) ( (int) ( (unsigned int)TFB_Random() % (n) ) )
#define SRAND(n) ( (int)TFB_Random() % (n) )
#define AND_RAND(n) ( (int)TFB_Random() & (n) )

#define INDEXED_RANDOM(x) (random_table[x])

/* ----------------------------GLOBALS/EXTERNS---------------------------- */

extern DWORD random_table[TABLE_SIZE];
extern COUNT fast_index; /* fast random cycling index */

