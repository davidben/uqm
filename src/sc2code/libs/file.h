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

/* Make sure we have PATH_MAX */
#ifdef WIN32
#	include <stdlib.h>
#	define PATH_MAX  _MAX_PATH
#else
	/* PATH_MAX is per POSIX defined in <limits.h> */
#	include <limits.h>
#endif
#ifdef _MSC_VER
#	include <io.h>
#	define access _access
#	define F_OK 0
#	define W_OK 2
#	define R_OK 4
#	include <direct.h>
#	define open _open
#	define mkdir _mkdir
#	define read _read
#	define rmdir _rmdir
#	define ssize_t int
#	define fstat _fstat
#	define S_IRWXU (S_IREAD | S_IWRITE | S_IEXEC)
#	define S_IRWXG 0
#	define S_IRWXO 0
#	define write _write
#	define stat _stat
#	define unlink _unlink
#else
#	include <unistd.h>
#endif

// for BOOLEAN
#include "compiler.h"

// from temp.h
void initTempDir (void);
void unInitTempDir (void);
char *tempFilePath (const char *filename);


// from dirs.h
int mkdirhier (const char *path);
const char *getHomeDir (void);
int createDirectory (const char *dir, int mode);
int expandPath (char *dest, size_t len, const char *src);

// from files.h
int copyFile (const char *srcName, const char *newName);
BOOLEAN fileExists (const char *name);


#endif  /* _FILE_H */

