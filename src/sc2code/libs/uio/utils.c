/*
 * Copyright (C) 2003  Serge van den Boom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * Nota bene: later versions of the GNU General Public License do not apply
 * to this program.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <errno.h>
#include <time.h>
#include <stdio.h>

#include "iointrn.h"
#include "ioaux.h"
#include "utils.h"

static int uio_copyError(uio_Handle *srcHandle, uio_Handle *dstHandle,
		uio_DirHandle *unlinkHandle, const char *unlinkPath, uio_uint8 *buf);

struct uio_StdioAccessHandle {
	uio_DirHandle *tempRoot;
	char *tempDirName;
	uio_DirHandle *tempDir;
	char *fileName;
	char *stdioPath;
};

static inline uio_StdioAccessHandle *uio_StdioAccessHandle_new(
		uio_DirHandle *tempRoot, char *tempDirName,
		uio_DirHandle *tempDir, char *fileName,
		char *stdioPath);
static inline void uio_StdioAccessHandle_delete(
		uio_StdioAccessHandle *handle);
static inline uio_StdioAccessHandle *uio_StdioAccessHandle_alloc(void);
static inline void uio_StdioAccessHandle_free(uio_StdioAccessHandle *handle);

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
uio_copyFile(uio_DirHandle *srcDir, const char *srcName,
		uio_DirHandle *dstDir, const char *newName) {
	uio_Handle *src, *dst;
	struct stat sb;
#define BUFSIZE 65536
	uio_uint8 *buf, *bufPtr;
	ssize_t numInBuf, numWritten;
	
	src = uio_open(srcDir, srcName, O_RDONLY
#ifdef WIN32
			| O_BINARY
#endif
			, 0);
	if (src == NULL)
		return -1;
	
	if (uio_fstat(src, &sb) == -1)
		return uio_copyError(src, NULL, NULL, NULL, NULL);
	
	dst = uio_open(dstDir, newName, O_WRONLY | O_CREAT | O_EXCL
#ifdef WIN32
			| O_BINARY
#endif
			, sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
	if (dst == NULL)
		return uio_copyError(src, NULL, NULL, NULL, NULL);
	
	buf = uio_malloc(BUFSIZE);
			// This was originally a statically allocated buffer,
			// but as this function might be run from a thread with
			// a small Stack, this is better.
	while (1) {
		numInBuf = uio_read(src, buf, BUFSIZE);
		if (numInBuf == -1) {
			if (errno == EINTR)
				continue;
			return uio_copyError(src, dst, dstDir, newName, buf);
		}
		if (numInBuf == 0)
			break;
		
		bufPtr = buf;
		do
		{
			numWritten = uio_write(dst, bufPtr, numInBuf);
			if (numWritten == -1) {
				if (errno == EINTR)
					continue;
				return uio_copyError(src, dst, dstDir, newName, buf);
			}
			numInBuf -= numWritten;
			bufPtr += numWritten;
		} while (numInBuf > 0);
	}
	
	uio_free(buf);
	uio_close(src);
	uio_close(dst);
	errno = 0;
	return 0;
}

/*
 * Closes srcHandle if it's not -1.
 * Closes dstHandle if it's not -1.
 * Removes unlinkpath from the unlinkHandle dir if it's not NULL.
 * Frees 'buf' if not NULL.
 * Always returns -1.
 * errno is what was before the call.
 */
static int
uio_copyError(uio_Handle *srcHandle, uio_Handle *dstHandle,
		uio_DirHandle *unlinkHandle, const char *unlinkPath, uio_uint8 *buf) {
	int savedErrno;

	savedErrno = errno;

#ifdef DEBUG
	fprintf(stderr, "Error while copying: %s\n", strerror(errno));
#endif

	if (srcHandle != NULL)
		uio_close(srcHandle);
	
	if (dstHandle != NULL)
		uio_close(dstHandle);
	
	if (unlinkPath != NULL)
		uio_unlink(unlinkHandle, unlinkPath);
	
	if (buf != NULL)
		uio_free(buf);
	
	errno = savedErrno;
	return -1;
}

#define NUM_TEMP_RETRIES 16
		// Retry this many times to create a temporary dir, before giving
		// up. If undefined, keep trying indefinately.

uio_StdioAccessHandle *
uio_getStdioAccess(uio_DirHandle *dir, const char *path, int flags,
		uio_DirHandle *tempDir) {
	int res;
	uio_MountHandle *mountHandle;
	const char *name;
	char *newPath;
	char *tempDirName;
	uio_DirHandle *newDir;
	uio_FileSystemID fsID;

	res = uio_getFileLocation(dir, path, flags, &mountHandle, &newPath);
	if (res == -1) {
		// errno is set
		return NULL;
	}
	
	fsID = uio_getMountFileSystemType(mountHandle);
	if (fsID == uio_FSTYPE_STDIO) {
		// Current location is usable.
		return uio_StdioAccessHandle_new(NULL, NULL, NULL, NULL, newPath);
	}
	uio_free(newPath);

	{
		uio_uint32 dirNum;
		int i;

		// Current location is not usable. Create a directory with a
		// generated name, as a temporary location to store a copy of
		// the file.
		dirNum = (uio_uint32) time(NULL);
		tempDirName = uio_malloc(sizeof "01234567");
		for (i = 0; ; i++) {
#ifdef NUM_TEMP_RETRIES
			if (i >= NUM_TEMP_RETRIES) {
				// Using ENOSPC to report that we couldn't create a
				// temporary dir, getting EEXIST.
				uio_free(tempDirName);
				errno = ENOSPC;
				return NULL;
			}
#endif
			
			sprintf(tempDirName, "%08lx", (unsigned long) dirNum + i);
			
			res = uio_mkdir(tempDir, tempDirName, 0700);
			if (res == -1) {
				int savedErrno;
				if (errno == EEXIST)
					continue;
				savedErrno = errno;
#ifdef DEBUG
				fprintf(stderr, "Error: Could not create temporary dir: %s\n",
						strerror(errno));
#endif
				uio_free(tempDirName);
				errno = savedErrno;
				return NULL;
			}
			break;
		}

		newDir = uio_openDirRelative(tempDir, tempDirName, 0);
		if (newDir == NULL) {
#ifdef DEBUG
			fprintf(stderr, "Error: Could not open temporary dir: %s\n",
					strerror(errno));
#endif
			res = uio_rmdir(tempDir, tempDirName);
#ifdef DEBUG
			if (res == -1)
				fprintf(stderr, "Warning: Could not remove temporary dir: "
						"%s.\n", strerror(errno));
#endif
			uio_free(tempDirName);
			errno = EIO;
			return NULL;
		}

		// Get the last component of path. This should be the file to
		// access.
		name = strrchr(path, '/');
		if (name == NULL)
			name = path;

		// Copy the file
		res = uio_copyFile(dir, path, newDir, name);
		if (res == -1) {
			int savedErrno = errno;
#ifdef DEBUG
			fprintf(stderr, "Error: Could not copy file to temporary dir: "
					"%s\n", strerror(errno));
#endif
			uio_closeDir(newDir);
			uio_free(tempDirName);
			errno = savedErrno;
			return NULL;
		}
	}

	res = uio_getFileLocation(newDir, name, flags, &mountHandle, &newPath);
	if (res == -1) {
		int savedErrno = errno;
		fprintf(stderr, "Error: uio_getStdioAccess: Could not get location "
				"of temporary dir: %s.\n", strerror(errno));
		uio_closeDir(newDir);
		uio_free(tempDirName);
		errno = savedErrno;
		return NULL;
	}
	
	fsID = uio_getMountFileSystemType(mountHandle);
	if (fsID != uio_FSTYPE_STDIO) {
		// Temp dir isn't on a stdio fs either.
		fprintf(stderr, "Error: uio_getStdioAccess: Temporary file location "
				"isn't on a stdio filesystem.\n");
		uio_closeDir(newDir);
		uio_free(tempDirName);
		uio_free(newPath);
//		errno = EXDEV;
		errno = EINVAL;
		return NULL;
	}

	uio_DirHandle_ref(tempDir);
	return uio_StdioAccessHandle_new(tempDir, tempDirName, newDir, 
			uio_strdup(name), newPath);
}

void
uio_releaseStdioAccess(uio_StdioAccessHandle *handle) {
	if (handle->tempDir != NULL) {
		if (uio_unlink(handle->tempDir, handle->fileName) == -1) {
#ifdef DEBUG
			fprintf(stderr, "Error: Could not remove temporary file: "
					"%s\n", strerror(errno));
#endif
		}

		// Need to free this handle in advance. There should be no handles
		// to a dir left when removing it.
		uio_DirHandle_unref(handle->tempDir);
		handle->tempDir = NULL;

		if (uio_rmdir(handle->tempRoot, handle->tempDirName) == -1) {
#ifdef DEBUG
			fprintf(stderr, "Error: Could not remove temporary directory: "
					"%s\n", strerror(errno));
#endif
		}
	}

	uio_StdioAccessHandle_delete(handle);
}

const char *
uio_StdioAccessHandle_getPath(uio_StdioAccessHandle *handle) {
	return (const char *) handle->stdioPath;
}

// references to tempRoot and tempDir are not increased.
// no copies of arguments are made.
// By calling this function control of the values is transfered to
// the handle.
static inline uio_StdioAccessHandle *
uio_StdioAccessHandle_new(
		uio_DirHandle *tempRoot, char *tempDirName,
		uio_DirHandle *tempDir, char *fileName, char *stdioPath) {
	uio_StdioAccessHandle *result;

	result = uio_StdioAccessHandle_alloc();
	result->tempRoot = tempRoot;
	result->tempDirName = tempDirName;
	result->tempDir = tempDir;
	result->fileName = fileName;
	result->stdioPath = stdioPath;

	return result;
}

static inline void
uio_StdioAccessHandle_delete(uio_StdioAccessHandle *handle) {
	if (handle->tempDir != NULL)
		uio_DirHandle_unref(handle->tempDir);
	if (handle->fileName != NULL)
		uio_free(handle->fileName);
	if (handle->tempRoot != NULL)
		uio_DirHandle_unref(handle->tempRoot);
	if (handle->tempDirName != NULL)
		uio_free(handle->tempDirName);
	uio_free(handle->stdioPath);
	uio_StdioAccessHandle_free(handle);
}

static inline uio_StdioAccessHandle *
uio_StdioAccessHandle_alloc(void) {
	return uio_malloc(sizeof (uio_StdioAccessHandle));
}

static inline void
uio_StdioAccessHandle_free(uio_StdioAccessHandle *handle) {
	uio_free(handle);
}

