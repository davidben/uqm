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

#include "mthintrn.h"
#include "random.h" /* get the externs for error checking */

/* ----------------------------GLOBAL DATA-------------------------------- */

DWORD random_table[TABLE_SIZE];
COUNT fast_index = 0; /* fast random cycling index */

/* ----------------------------PROTOTYPES--------------------------------- */

DWORD indexed_random_table(COUNT index);
void reseed_fast_random();
DWORD fast_random();
DWORD seed_fast_random(DWORD seed);

/*****************************************************************************
* FUNC: DWORD seed_fast_random(DWORD seed)
*
* DESC:generate a small table of random numbers for later use by fast_random()
*
* NOTES: the table has a prime number of entries
*
* HISTORY: Created By Robert Leyland
*
*****************************************************************************/

DWORD
seed_fast_random(DWORD seed)
{
		DWORD retval;
		COUNT index;

		retval = TFB_SeedRandom(seed);

		for( index = 0; index < TABLE_SIZE; index++ )
				random_table[index] = TFB_Random();

		fast_index = 0; /* must be set to the start of the table for
														repeatability... */

		return (retval);
}


/*****************************************************************************
* FUNC: DWORD fast_random()
*
* DESC:return the next random number from the random number table
*
* NOTES:
*
* HISTORY: Created By Robert Leyland
*
*****************************************************************************/

DWORD
fast_random()
{
		fast_index++;
		if( fast_index == TABLE_SIZE )
				fast_index = 0;

		return( random_table[fast_index] );
}

/*****************************************************************************
* FUNC: void reseed_fast_random()
*
* DESC:shifts the table pointer by calling the "real" random
*
* NOTES:
*
* HISTORY: Created By Robert Leyland
*
*****************************************************************************/

void
reseed_fast_random()
{
		fast_index = (COUNT)(TFB_Random() % TABLE_SIZE);
		
}


/*****************************************************************************
* FUNC: DWORD indexed_random_table(COUNT index)
*
* DESC:return a "random" number from the random number table, as indexed
* for repeatability
*
* NOTES:used by texturing functions
*
* HISTORY: Created By Robert Leyland
*
*****************************************************************************/

DWORD
indexed_random_table(COUNT index)
{
		if( index > TABLE_SIZE ) /* only do % if really needed */
				index = index % TABLE_SIZE;

		return(random_table[index]);
}



