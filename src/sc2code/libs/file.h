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

// Contains file handling code

#ifndef _FILE_H

#include "port.h"

// for bool
#include "types.h"

// from temp.h
#include "libs/uio.h"
void initTempDir (void);
void unInitTempDir (void);
char *tempFilePath (const char *filename);
extern uio_DirHandle *tempDir;


// from dirs.h
int mkdirhier (const char *path);
const char *getHomeDir (void);
int createDirectory (const char *dir, int mode);

int expandPath (char *dest, size_t len, const char *src, int what);
// values for 'what':
#define EP_HOME      1
		// Expand '~' for home dirs.
#define EP_ABSOLUTE  2
		// Make paths absolute
#define EP_ENVVARS   4
		// Expand environment variables.
#define EP_DOTS      8
		// Process '..' and '.' (not implemented)
#define EP_SLASHES   16
		// Change (Windows style) backslashes to (Unix style) slashes.
#define EP_ALL (EP_HOME | EP_ENVVARS | EP_ABSOLUTE | EP_DOTS | EP_SLASHES)
		// Everything
// Everything except Windows style backslashes on Unix Systems:
#ifdef WIN32
#	define EP_ALL_SYSTEM (EP_HOME | EP_ENVVARS | EP_ABSOLUTE | EP_DOTS | \
		EP_SLASHES)
#else
#	define EP_ALL_SYSTEM (EP_HOME | EP_ENVVARS | EP_ABSOLUTE | EP_DOTS)
#endif

// from files.h
int copyFile (uio_DirHandle *srcDir, const char *srcName,
		uio_DirHandle *dstDir, const char *newName);
bool fileExists (const char *name);
bool fileExists2(uio_DirHandle *dir, const char *fileName);


#endif  /* _FILE_H */

