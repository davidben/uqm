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

// Contains code handling directories

#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <direct.h>
#endif
#include "port.h"
#include "config.h"
#include "filintrn.h"

int
createDirectory(const char *dir, int mode) {
#ifdef WIN32
	return mkdir (dir);
#else
	return mkdir (dir, (mode_t) mode);
#endif
}

// make all components of the path if they don't exist already
// returns 0 on success, -1 on failure.
// on failure, some parts may still have been created.
int
mkdirhier (const char *path) {
	char *buf;              // buffer
	char *ptr;              // end of the string in buf
	const char *pathstart;  // start of a component of path
	const char *pathend;    // first char past the end of a component of path
	size_t len;
	struct stat statbuf;
	
	len = strlen (path);
	buf = alloca (len + 2);  // one extra for possibly added '/'

	ptr = buf;
	pathstart = path;

	if (*pathstart == '/') {
		*ptr = '/';
		ptr++;
		pathstart++;
	}

	if (*pathstart == '\0') {
		// path exists completely, nothing more to do
		return 0;
	}

	// walk through the path as long as the components exist
	while (1) {
		pathend = strchr (pathstart, '/');
		if (pathend == NULL)
			pathend = path + len;
		memcpy(ptr, pathstart, pathend - pathstart);
		ptr += pathend - pathstart;
		*ptr = '\0';
		
		if (stat (buf, &statbuf) == -1) {
			if (errno != ENOENT) {
				fprintf (stderr, "Can't stat %s: %s\n", buf,
						strerror (errno));
				return -1;
			}
			break;
		}
		
		if (*pathend == '\0')
			return 0;

		*ptr = '/';
		ptr++;
		pathstart = pathend + 1;
		while (*pathstart == '/')
			pathstart++;
		// pathstart is the next non-slash character

		if (*pathstart == '\0')
			return 0;
	}
	
	// create all components left
	while (1) {
		if (createDirectory (buf, 0777) == -1) {
			fprintf (stderr, "Error: Can't create %s: %s\n", buf,
					strerror (errno));
			return -1;
		}

		if (*pathend == '\0')
			break;

		*ptr = '/';
		ptr++;
		pathstart = pathend + 1;
		while (*pathstart == '/')
			pathstart++;
		// pathstart is the next non-slash character

		if (*pathstart == '\0')
			break;

		pathend = strchr (pathstart, '/');
		if (pathend == NULL)
			pathend = path + len;
		
		memcpy (ptr, pathstart, pathend - pathstart);
		ptr += pathend - pathstart;
		*ptr = '\0';
	}
	return 0;
}

// Get the user's home dir
// returns a pointer to a static buffer
const char *
getHomeDir(void) {
	return getenv ("HOME");
}

int
expandPath(char *dest, size_t len, const char *src) {
	size_t srclen;

	srclen = strlen (src);
	if (src[0] == '~')
	{
		const char *home;
		size_t homelen;
		
		if (src[1] != '/')
		{
			errno = EINVAL;
			return -1;
		}

		home = getHomeDir ();
		if (home == NULL)
		{
			errno = ENOENT;
			return -1;
		}
		homelen = strlen (home);
		
		if (homelen + srclen > len)
		{
			errno = ENAMETOOLONG;
			return -1;
		}
		memcpy(dest, home, homelen);
		dest += homelen;
		src++;  /* skip the ~ */
	} else {
		if (strlen (src) >= len)
		{
			errno = ENAMETOOLONG;
			return -1;
		}
	}

#ifdef WIN32
	/* Replaces backslashes in path by slashes. */
	while (*src != '\0')
	{
		if (*src == '\\')
		{
			*(dest++) = '/';
			src++;
		}
		else
			*(dest++) = *(src++);
	}
	*dest = '\0';
#else
	strcpy(dest, src);
#endif
	return 0;
}


