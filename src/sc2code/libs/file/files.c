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

// Contains code handling files

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "port.h"
#include "config.h"
#include "types.h"
#include "filintrn.h"

static int copyError(int srcFd, int dstFd, const char *unlinkPath);

BOOLEAN
fileExists (const char *name)
{
	return access (name, F_OK) == 0;
}

/*
 * Copy a file with path srcName to a file with name newName.
 * If the destination already exists, the operation fails.
 * Links are followed.
 * Special files (fifos, char devices, block devices, etc) will be
 * read as long as there is data available and the destination will be
 * a regular file with that data.
 * The new file will have the same permissions as the old.
 * If an error occurs during copying, an attempt will be made to
 * remove the copy.
 */
int
copyFile (const char *srcName, const char *newName)
{
	int src, dst;
	struct stat sb;
#define BUFSIZE 65536
	uint8 buf[BUFSIZE], *bufPtr;
	ssize_t numInBuf, numWritten;
	
	src = open (srcName, O_RDONLY);
	if (src == -1)
		return -1;
	
	if (fstat (src, &sb) == -1)
		return copyError (src, -1, NULL);
	
	dst = open (newName, O_WRONLY | O_CREAT | O_EXCL,
			sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
	if (dst == -1)
		return copyError (src, -1, NULL);
	
	while (1) {
		numInBuf = read (src, buf, BUFSIZE);
		if (numInBuf == -1)
		{
			if (errno == EINTR)
				continue;
			return copyError (src, dst, newName);
		}
		if (numInBuf == 0)
			break;
		
		bufPtr = buf;
		do {
			numWritten = write (dst, bufPtr, numInBuf);
			if (numWritten == -1)
			{
				if (errno == EINTR)
					continue;
				return copyError (src, dst, newName);
			}
			numInBuf -= numWritten;
			bufPtr += numWritten;
		} while (numInBuf > 0);
	}
	
	close(src);
	close(dst);
	errno = 0;
	return 0;
}

/*
 * Closes srcFd if it's not -1.
 * Closes dstFd if it's not -1.
 * Removes unlinkpath if it's not NULL.
 * Always returns -1.
 * errno is what was before the call.
 */
static int
copyError(int srcFd, int dstFd, const char *unlinkPath)
{
	int savedErrno;

	savedErrno = errno;

#ifdef DEBUG
	fprintf (stderr, "Error while copying: %s\n", strerror (errno))
#endif

	if (srcFd >= 0)
		close (srcFd);
	
	if (dstFd >= 0)
		close (dstFd);
	
	if (unlinkPath != NULL)
		unlink (unlinkPath);
	
	errno = savedErrno;
	return -1;
}

