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

// Contains code handling temporary files and dirs

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#ifdef WIN32
#	include <io.h>
#endif
#include <string.h>
#include <time.h>
#include "filintrn.h"
#include "timelib.h"
#include "port.h"
#include "libs/compiler.h"
#include "misc.h"

static char *tempDir;

static void
removeTempDir (void)
{
	rmdir (tempDir);
}

// Try if the null-terminated path 'dir' to a directory is valid
// as temp path.
// On success, 'buf' will be filled with the path, with a trailing /,
// null-terminated, and 0 is returned.
// On failure, EINVAL, ENAMETOOLONG, or one of the errors access() can return
// is returned, and the contents of buf is unspecified.
static int
tryTempDir (char *buf, size_t buflen, const char *dir)
{
	size_t len;
	int haveSlash;
	
	if (dir == NULL)
		return EINVAL;
		
	len = strlen (dir);
	haveSlash = (dir[len - 1] == '/'
#ifdef WIN32
			|| dir[len - 1] == '\\'
#endif
			);
	if ((haveSlash ? len : len + 1) >= buflen)
		return ENAMETOOLONG;
	
	strcpy (buf, dir);
	if (!haveSlash)
	{
		buf[len] = '/';
		len++;
		buf[len] = '\0';
	}
	if (access (buf, R_OK | W_OK) == -1)
		return errno;
	return 0;
}

static void
getTempDir (char *buf, size_t buflen) {
	if (tryTempDir (buf, buflen, getenv("TMP")) &&
			tryTempDir (buf, buflen, getenv("TEMP")) &&
			tryTempDir (buf, buflen, "/tmp/") &&
			tryTempDir (buf, buflen, "./"))
	{
		fprintf (stderr, "Fatal Error: Cannot find a suitable location "
				"to store temporary files.\n");
		exit(EXIT_FAILURE);
	}
}

#define NUM_TEMP_RETRIES 16
		// Number of files to try to open before giving up.
void
initTempDir (void) {
	size_t len;
	DWORD num;
	int i;
	char *tempPtr;
			// Pointer to the location in the tempDir string where the
			// path to the temp dir ends and the dir starts.

	tempDir = HMalloc (PATH_MAX);
	getTempDir (tempDir, PATH_MAX - 21);
			// reserve 8 chars for dirname, 1 for slash, and 12 for filename
	len = strlen(tempDir);

	num = ((DWORD) time (NULL));
//	num = GetTimeCounter () % 0xffffffff;
	tempPtr = tempDir + len;
	i = NUM_TEMP_RETRIES;
	while (i--)
	{
		sprintf (tempPtr, "%08lx", num + i);
		if (createDirectory (tempDir, 0700) == -1)
			continue;
		
		// Success, we've got a temp dir.
		tempDir = HRealloc (tempDir, len + 9);
		atexit (removeTempDir);
		return;
	}
	
	// Failure, could not make a temporary directory.
	fprintf(stderr, "Fatal error: Cannot get a name for a temporary "
			"directory.\n");
	exit(EXIT_FAILURE);
}

void
unInitTempDir (void) {
	// nothing at the moment.
	// the removing of the dir is handled via atexit
}

// return the path to a file in the temp dir with the specified filename.
// returns a pointer to a static buffer.
char *
tempFilePath (const char *filename) {
	static char file[PATH_MAX];
	
	if (snprintf(file, PATH_MAX, "%s/%s", tempDir, filename) == -1) {
		fprintf(stderr, "Path to temp file too long.\n");
		exit(EXIT_FAILURE);
	}
	return file;
}

