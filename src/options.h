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

/*
 * Eventually this should include all configuration stuff, 
 * for now there's few options which indicate 3do/pc flavors.
 * By Mika Kolehmainen.
 */

#ifndef OPTIONS_H
#define OPTIONS_H

#define UQM_MAJOR_VERSION 0
#define UQM_MINOR_VERSION 1
char UQM_VERSION_STRING[1000];

enum
{
	MUSIC_3DO,
	MUSIC_PC,
};
extern int optWhichMusic;

BOOLEAN FileExists (char *filename);

#endif
