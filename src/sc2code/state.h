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

#ifndef _STATE_H
#define _STATE_H

#include "libs/compiler.h"

extern void InitPlanetInfo (void);
extern void UninitPlanetInfo (void);
extern void GetPlanetInfo (void);
extern void PutPlanetInfo (void);

extern void InitGroupInfo (BOOLEAN FirstTime);
extern void UninitGroupInfo (void);
extern BOOLEAN GetGroupInfo (DWORD offset, BYTE which_group);
extern DWORD PutGroupInfo (DWORD offset, BYTE which_group);
extern void BuildGroups (void);

#define MAX_DESC_CHARS 60
typedef UNICODE GAME_DESC[MAX_DESC_CHARS + 1];

#define STARINFO_FILE "starinfo.dat"
#define STAR_BUFSIZE (NUM_SOLAR_SYSTEMS * sizeof (DWORD) \
		+ 3800 * (3 * sizeof (DWORD)))
#define RANDGRPINFO_FILE "randgrp.dat"
#define RAND_BUFSIZE (4 * 1024)
#define DEFGRPINFO_FILE "defgrp.dat"
#define DEF_BUFSIZE (10 * 1024)

typedef enum
{
	STARINFO,
	RANDGRPINFO,
	DEFGRPINFO
} INFO_TYPE;


#endif /* _STATE_H */

