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

// auxiliary functions for use by io.c

#include <errno.h>
#include <fcntl.h>

#include "ioaux.h"
#include "iointrn.h"
#include "uioport.h"
#include "paths.h"

static int copyError(int error,
		uio_FileSystemHandler *fromHandler, uio_Handle *fromHandle,
		uio_FileSystemHandler *toHandler, uio_Handle *toHandle,
		uio_PDirHandle *toDir, const char *toName, char *buf);

/*
 * Follow a path starting from a specified physical dir as long as possible.
 * When you can get no further, 'endPDirHandle' will be filled in with a
 * reference to the dir where you ended up, and 'pathRest' will point into
 * the original path. to the beginning of the part that was not matched.
 * It is allowed to have endPDirHandle point to pDirHandle and/or restPath
 * point to path when calling this function. Just take care to keep a
 * reference to the original so you can decrement the ref counter.
 * returns: 0 if the complete path was matched
 *          ENOENT if some component (the next one) didn't exists
 *          ENODIR if a component (the next one) exists but isn't a dir
 */
int
uio_walkPhysicalPath(uio_PDirHandle *startPDirHandle, const char *path,
		size_t pathLen, uio_PDirHandle **endPDirHandle,
		const char **pathRest) {
	const char *pathEnd;
	const char *partStart, *partEnd;
	char *tempBuf;
	uio_PDirHandle *pDirHandle;
	uio_PDirEntryHandle *entry;
	int retVal;

	uio_PDirHandle_ref(startPDirHandle);
	pDirHandle = startPDirHandle;
	tempBuf = uio_alloca(strlen(path) + 1);
	pathEnd = path + pathLen;
	getFirstPathComponent(path, pathEnd, &partStart, &partEnd);
	while (1) {
		if (partStart == pathEnd) {
			retVal = 0;
			break;
		}
		memcpy(tempBuf, partStart, partEnd - partStart);
		tempBuf[partEnd - partStart] = '\0';

		entry = uio_getPDirEntryHandle(pDirHandle, tempBuf);
		if (entry == NULL) {
			retVal = ENOENT;
			break;
		}
		if (!uio_PDirEntryHandle_isDir(entry)) {
			uio_PDirEntryHandle_unref(entry);
			retVal = ENOTDIR;
			break;
		}
		uio_PDirHandle_unref(pDirHandle);
		pDirHandle = (uio_PDirHandle *) entry;
		getNextPathComponent(pathEnd, &partStart, &partEnd);
	}

	*pathRest = partStart;
	*endPDirHandle = pDirHandle;
	return retVal;
}

// Make a all directory components of a path, inside a physical directory.
uio_PDirHandle *
uio_makePath(uio_PDirHandle *pDirHandle, const char *path, size_t pathLen,
		mode_t mode) {
	const char *rest, *start, *end;
	uio_PDirHandle *(*mkdirFun)(uio_PDirHandle *, const char *, mode_t);
	char *buf;
	const char *pathEnd;
	uio_PDirHandle *newPDirHandle;
	
	mkdirFun = pDirHandle->pRoot->handler->mkdir;
	if (mkdirFun == NULL) {
		errno = ENOSYS;
		return NULL;
	}

	pathEnd = path + pathLen;
	
	buf = uio_alloca(pathLen + 1);
			// worst case length

	uio_walkPhysicalPath(pDirHandle, path, pathLen, &pDirHandle, &rest);
			// The reference to the original pDirHandle is still kept
			// by the calling function; uio_PDirHandle_unref should not
			// be called for it.
	// Rest now points into 'path' to the part from where no physical
	// dir exists.
	
	getFirstPathComponent(rest, pathEnd, &start, &end);
	while (start < pathEnd) {
		memcpy(buf, start, end - start);
		buf[end - start] = '\0';

		newPDirHandle = mkdirFun(pDirHandle, buf, mode);
		if (newPDirHandle == NULL) {
			int savedErrno = errno;
			uio_PDirHandle_unref(pDirHandle);
			errno = savedErrno;
			return NULL;
		}
		uio_PDirHandle_unref(pDirHandle);
		pDirHandle = newPDirHandle;
		getNextPathComponent(pathEnd, &start, &end);
	}
	return pDirHandle;
}

/*
 * permissions should already have been checked
 *
 * The new file will have the same permissions as the old.
 * If an error occurs during copying, an attempt will be made to
 * remove the copy.
 */
int
uio_copyFilePhysical(uio_PDirHandle *fromDir, const char *fromName,
		uio_PDirHandle *toDir, const char *toName) {
	uio_FileSystemHandler *fromHandler, *toHandler;
	uio_Handle *fromHandle;
	uio_Handle *toHandle;
#define BUFSIZE 0x10000
	struct stat statBuf;
	char *buf, *bufPtr;
	ssize_t numInBuf, numWritten;

	fromHandler = fromDir->pRoot->handler;
	toHandler = toDir->pRoot->handler;
	if (toHandler->write == NULL || fromHandler->fstat == NULL ||
			toHandler->unlink == NULL) {
		errno = ENOSYS;
		return -1;
	}

	fromHandle = fromHandler->open(fromDir, fromName, O_RDONLY, 0);
	if (fromHandle == NULL) {
		// errno is set
		return -1;
	}

	if (fromHandler->fstat(fromHandle, &statBuf) == -1)
		return copyError(errno, fromHandler, fromHandle,
				toHandler, NULL, NULL, NULL, NULL);

	toHandle = toHandler->open(toDir, toName, O_WRONLY | O_CREAT | O_EXCL,
			statBuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
	if (toHandle == NULL)
		return copyError(errno, fromHandler, fromHandle,
				toHandler, NULL, NULL, NULL, NULL);

	buf = uio_malloc(BUFSIZE);
			// not allocated on the stack, as this function may be called
			// from a thread with little stack space.
	while (1) {
		numInBuf = fromHandler->read(fromHandle, buf, BUFSIZE);
		if (numInBuf == -1)
		{
			if (errno == EINTR)
				continue;
			return copyError (errno, fromHandler, fromHandle,
					toHandler, toHandle, toDir, toName, buf);
		}
		if (numInBuf == 0)
			break;

		bufPtr = buf;
		do {
			numWritten = toHandler->write(toHandle, bufPtr, numInBuf);
			if (numWritten == -1)
			{
				if (errno == EINTR)
					continue;
				return copyError (errno, fromHandler, fromHandle,
					toHandler, toHandle, toDir, toName, buf);
			}
			numInBuf -= numWritten;
			bufPtr += numWritten;
		} while (numInBuf > 0);
	}

	uio_free(buf);
	toHandler->close(toHandle);
	fromHandler->close(fromHandle);
	return 0;
}

/*
 * Closes fromHandle if it's not -1.
 * Closes fromHandle if it's not -1.
 * Removes 'toName' from 'toDir' if it's not NULL.
 * Frees 'buf' if not NULL.
 * Always returns -1.
 */
static int
copyError(int error,
		uio_FileSystemHandler *fromHandler, uio_Handle *fromHandle,
		uio_FileSystemHandler *toHandler, uio_Handle *toHandle,
		uio_PDirHandle *toDir, const char *toName, char *buf) {
#ifdef DEBUG
	fprintf(stderr, "Error while copying: %s\n", strerror(error));
#endif

	if (fromHandle != NULL)
		fromHandler->close(fromHandle);
	
	if (toHandle != NULL)
		toHandler->close(toHandle);
	
	if (toName != NULL)
		toHandler->unlink(toDir, toName);
	
	if (buf != NULL)
		uio_free(buf);
	
	errno = error;
	return -1;
}

/*
 * Description: find PDirHandle and MountInfo structures for reading and
 *              writing for a path from a DirHandle.
 *              This can be used for opening a file and creating directories.
 * Arguments:   dirHandle - the directory to which the path is relative.
 *              path - the path to the component that is to be accessed.
 *              		The last component does not have to exist.
 *              flags - used to specify what kind of access is requested
 *              		Either O_RDONLY, O_RDWR, O_WRONLY. They may be
 *              		OR'd with other values accepted by open(). These
 *              		are ignored.
 *              extraFlags - either 0 or uio_GPA_NOWRITE
 *              		When 0, the path will be created if it doesn't
 *              		exist in the writing location, but does exist
 *              		in the reading location. With uio_GPA_NOWRITE, it
 *              		won't be created, and -1 will be returned and errno
 *              		will be set to ENOENT.
 *              mountInfoReadPtr - pointer to location where the pointer
 *              		to the MountInfo structure for the reading location
 *              		should be stored.
 *              readPDirHandlePtr - pointer to the location where the pointer
 *              		to the PDirHandle used for reading should be stored.
 *              readPRootPath - pointer to the location where the pointer
 *              		to the physical path to the reading location 
 *              		is to be stored.
 *              		The caller is responsible for freeing this.
 *              		Ignored if NULL.
 *              mountInfoWritePtr - pointer to location where the pointer
 *              		to the MountInfo structure for the writing location
 *              		should be stored.
 *              writePDirHandlePtr - pointer to the location where the pointer
 *              		to the PDirHandle used for writing should be stored.
 *              		NULL if O_RDONLY was specified.
 *              		If this is the same dir as the one refered
 *              		to by readPDirHandlePtr, the handles will be the
 *              		same too.
 *              writePRootPath - pointer to the location where the pointer
 *              		to the physical path to the writing location 
 *              		is to be stored.
 *              		The caller is responsible for freeing this.
 *              		Ignored if NULL.
 *              restPtr - pointer to the location where a newly created
 *              		string with as contents the last component of 'path'
 *              		is to be stored.
 *              		The caller is responsible for freeing this.
 *              		Ignored if NULL.
 * Returns:     0 - success
 *              -1 - failure (errno set)
 * NB:          This is the function that would most benefit from
 *              the introduction of LDirs.
 *              This is also the most messy function. It could
 *              use some more comments, or a cleanup (if possible).
 */
int
uio_getPhysicalAccess(uio_DirHandle *dirHandle, const char *path,
		int flags, int extraFlags,
		uio_MountInfo **mountInfoReadPtr, uio_PDirHandle **readPDirHandlePtr,
		char **readPRootPathPtr,
		uio_MountInfo **mountInfoWritePtr, uio_PDirHandle **writePDirHandlePtr,
		char **writePRootPathPtr,
		char **restPtr) {
	char *fullPath;  // path from dirHandle with 'path' added
	const char *pRootPath;  // path from the pRoot of a physical tree
	const char *readPRootPath, *writePRootPath;
	const char *rest, *readRest;
	uio_MountTree *tree;
	uio_MountTreeItem *item;
	uio_MountTreeItem *readItem, *writeItem;
	uio_PDirHandle *readPDirHandle, *writePDirHandle, *pDirHandle;
	int retVal;
	uio_bool entryExists;
			// Set if the entry pointed to by path exists (including
			// the last component of the path)

	// Determine the absolute path from 'path' which is relative to dirHandle.
	if (uio_resolvePath(dirHandle, path, strlen(path), &fullPath) == -1) {
		// errno is set
		return -1;
	}
	
	// get the MountTree effective for the path
	uio_findMountTree(dirHandle->repository->mountTree, fullPath,
			&tree, &rest);

	readItem = NULL;
	readPDirHandle = NULL;
	readPRootPath = NULL;
	writeItem = NULL;
	writePDirHandle = NULL;
	writePRootPath = NULL;
	readRest = NULL;
			// Satisfy compiler.
	entryExists = false;
	// try all the MountInfo structures in effect for this MountTree
	for (item = tree->pLocs; item != NULL; item = item->next) {
		pRootPath = uio_mountTreeItemRestPath(item, tree->lastComp, fullPath);
		retVal = uio_walkPhysicalPath(item->mountInfo->pDirHandle, pRootPath,
				strlen(pRootPath), &pDirHandle, &rest);
				// rest points inside fullPath
		if (retVal == 0) {
			// even the last component appeared to be a dir
			// As the last component did exist, we don't go on.
			uio_free(fullPath);
			uio_PDirHandle_unref(pDirHandle);
			if (readPDirHandle != NULL)
				uio_PDirHandle_unref(readPDirHandle);
			errno = EISDIR;
			return -1;
		}
		// check if this MountTreeItem is suitable for writing
		if (writeItem == NULL && !uio_mountInfoIsReadOnly(item->mountInfo))
			writeItem = item;
		if (strchr(rest, '/') == NULL) {
			// There's only dir component that was not matched.
			uio_PDirEntryHandle *entry;

			// This MountInfo will do for reading, if the file from the last
			// component is present in this dir.
			entry = uio_getPDirEntryHandle(pDirHandle, rest);
			if (entry != NULL) {
				// 'rest' exists, and it is not a dir, as otherwise
				// uio_getPDirEntryHandle wouldn't have stopped where it did.
				uio_PDirEntryHandle_unref(entry);
				readItem = item;
				if (readPDirHandle != NULL)
					uio_PDirHandle_unref(readPDirHandle);
				readPDirHandle = pDirHandle;
				readPRootPath = pRootPath;
				readRest = rest;
				entryExists = true;
				break;
			} else {
				// 'rest' doesn't exist
				// We're only interested in this dir if we want to write too.
				if (((flags & O_ACCMODE) != O_RDONLY) && readItem == NULL) {
					// Keep the first one.
					readItem = item;
					assert(readPDirHandle == NULL);
					readPDirHandle = pDirHandle;
					readPRootPath = pRootPath;
					readRest = rest;
					continue;
				}
			}
		} else {
			// There is more than one dir component that was not matched.
			// If the first component of the non-matched part is a file,
			// stop here and don't check lower dirs.
			if (retVal == ENOTDIR) {
				uio_free(fullPath);
				uio_PDirHandle_unref(pDirHandle);
				if (readPDirHandle != NULL)
					uio_PDirHandle_unref(readPDirHandle);
				errno = ENOTDIR;
				return -1;
			}
		}
		uio_PDirHandle_unref(pDirHandle);
	} // for
	// readItem is set to the first readItem for which the path completely
	// exists (including the final component). If there's no such readItem,
	// it is set to the first path for which the path exists, but without
	// the final component (entryExists is false in this case). If there's
	// no such path either, it's set to NULL.
	
	if (readItem == NULL) {
		uio_free(fullPath);
		errno = ENOENT;
		return -1;
	}
	if ((flags & O_ACCMODE) == O_RDONLY) {
		// write access is not needed
		*mountInfoReadPtr = readItem->mountInfo;
		*readPDirHandlePtr = readPDirHandle;
		if (readPRootPathPtr != NULL)
			*readPRootPathPtr = joinPathsAbsolute(
					readItem->mountInfo->dirName, readPRootPath);
		// Don't touch mountInfoWritePtr and writePDirHandlePtr.
		// they'd be NULL.
		*restPtr = uio_strdup(readRest);
		uio_free(fullPath);
		return 0;
	} else {
		if (!entryExists && !(flags & O_CREAT)) {
			// Though the path to the entry existed (readPDirHandle is set to
			// it), the entry itself doesn't, so we can't use it unless we
			// intend to create it.
			uio_PDirHandle_unref(readPDirHandle);
			uio_free(fullPath);
			errno = ENOENT;
			return -1;
		}
	}
	if (writeItem == readItem) {
		// The read directory is usable as write directory too.
		*mountInfoReadPtr = readItem->mountInfo;
		*readPDirHandlePtr = readPDirHandle;
		if (readPRootPathPtr != NULL)
			*readPRootPathPtr = joinPathsAbsolute(
					readItem->mountInfo->dirName, readPRootPath);
		*mountInfoWritePtr = writeItem->mountInfo;
				// writeItem == readItem
		uio_PDirHandle_ref(readPDirHandle);
		*writePDirHandlePtr = readPDirHandle;
				// No copy&paste error, the read PDirHandle is the write
				// pDirHandle too.
		if (writePRootPathPtr != NULL)
			*writePRootPathPtr = joinPathsAbsolute(
					writeItem->mountInfo->dirName, writePRootPath);
		if (restPtr != NULL)
			*restPtr = uio_strdup(readRest);
		uio_free(fullPath);
		return 0;
	}
	if (writeItem == NULL) {
		uio_free(fullPath);
		uio_PDirHandle_unref(readPDirHandle);
		errno = EPERM;
				// readItem is not NULL, so ENOENT would not be correct here.
		return -1;
	}

	// There exists no path for a write dir, so it will have to be created.
	// writeMountInfo indicates the physical tree where it should end up.

	if (extraFlags & uio_GPA_NOWRITE) {
		// The caller has specified that the path should not be created.
		uio_PDirHandle_unref(readPDirHandle);
		uio_free(fullPath);
		errno = ENOENT;
		return -1;
	}
	
	pRootPath = uio_mountTreeItemRestPath(writeItem, tree->lastComp,
			fullPath);

	rest = strrchr(pRootPath, '/');
			// rest points inside fullPath
	if (rest == NULL) {
		rest = pRootPath;
		uio_PDirHandle_ref(writeItem->mountInfo->pDirHandle);
		writePDirHandle = writeItem->mountInfo->pDirHandle;
	} else {
		writePDirHandle = uio_makePath(writeItem->mountInfo->pDirHandle,
				pRootPath, rest - pRootPath, 0777);
		if (writePDirHandle == NULL) {
			int savedErrno;
			if (errno == ENOSYS) {
				// mkdir not supported. We want to report that we failed
				// because of an error in the underlying layer.
				// EIO sounds like the best choice.
				errno = EIO;
			}
			savedErrno = errno;
			uio_PDirHandle_unref(readPDirHandle);
			uio_free(fullPath);
			errno = savedErrno;
			return -1;
		}
		rest++;  // skip the '/'
	}

	if (!entryExists) {
		// The path to the read dir exists, but the entry itself doesn't.
		// After we created the write dir, the same thing holds for
		// the write dir. As it occurs in an earlier MountItem, we'll use
		// the writeItem (and writePDirHandle) for reading too.
		readItem = writeItem;
		uio_PDirHandle_ref(writePDirHandle);
		uio_PDirHandle_unref(readPDirHandle);
		readPDirHandle = writePDirHandle;
	}
	
	*mountInfoReadPtr = readItem->mountInfo;
	*readPDirHandlePtr = readPDirHandle;
	if (readPRootPathPtr != NULL)
		*readPRootPathPtr = joinPathsAbsolute(
				readItem->mountInfo->dirName, readPRootPath);
	*mountInfoReadPtr = writeItem->mountInfo;
	*writePDirHandlePtr = writePDirHandle;
	if (writePRootPathPtr != NULL)
		*writePRootPathPtr = joinPathsAbsolute(
				writeItem->mountInfo->dirName, writePRootPath);
	if (restPtr != NULL)
		*restPtr = uio_strdup(rest);
	uio_free(fullPath);
	return 0;
}

// Get handles to the (existing) physical dirs that are effective in a
// path 'path' relative from 'dirHandle'
// returns the PDirHandles through '*pDirHandles'. It is NULL if none
// were found.
// If resItems != NULL, it returns the MountTreeItems belonging to those
// PDIrHandles through *resItems. It is NULL if none were found.
// numPDirHandles will contain the number of PDirHandles found.
// returns 0 if everything went ok.
// returns -1 in case of an error; errno is set.
int
uio_getPathPhysicalDirs(uio_DirHandle *dirHandle, const char *path,
		size_t pathLen, uio_PDirHandle ***resPDirHandles,
		int *resNumPDirHandles, uio_MountTreeItem ***resItems) {
	uio_PDirHandle **pDirHandles;
	char *fullPath;  // path from dirHandle with 'path' added
	uio_MountTree *tree;
	const char *rest;
	uio_MountTreeItem *item, **items;
	const char *pRootPath;  // path from the pRoot of a physical tree
	int numPDirHandles;
	int pDirI;  // PDirHandle iterator

	// Determine the absolute path from 'path', which is relative to dirHandle.
	if (uio_resolvePath(dirHandle, path, pathLen, &fullPath) == -1) {
		// errno is set
		return -1;
	}
	
	// get the MountTree effective for the path
	uio_findMountTree(dirHandle->repository->mountTree, fullPath,
			&tree, &rest);

	// fill pDirHandles with all the PDirHandles for the path
	numPDirHandles = uio_mountTreeCountPLocs(tree);
	pDirHandles = uio_malloc(numPDirHandles * sizeof (uio_PDirHandle *));
	if (resItems != NULL) {
		items = uio_malloc(numPDirHandles * sizeof (uio_MountTreeItem *));
	} else {
		items = NULL;  // satisfy compiler
	}
	pDirI = 0;
	for (item = tree->pLocs; item != NULL; item = item->next) {
		uio_PDirHandle *pDirHandle;
			
		pRootPath = uio_mountTreeItemRestPath(item, tree->lastComp, fullPath);
		switch (uio_walkPhysicalPath(item->mountInfo->pDirHandle, pRootPath,
					strlen(pRootPath), &pDirHandle, &rest)) {
			case 0:
				// complete path was matched
				pDirHandles[pDirI] = pDirHandle;
				if (resItems != NULL)
					items[pDirI] = item;
				pDirI++;
				continue;
			case ENOENT:
				// some component couldn't be matched
				uio_PDirHandle_unref(pDirHandle);
				continue;
			case ENOTDIR:
				// next component was not a dir
				// Don't look further at other mount Items.
				uio_PDirHandle_unref(pDirHandle);
				break;
			default:
				assert(false);
				uio_PDirHandle_unref(pDirHandle);
				continue;
		}
		break;
	}
	numPDirHandles = pDirI;

	uio_free(fullPath);

	*resPDirHandles = uio_realloc(pDirHandles,
			numPDirHandles * sizeof (uio_PDirHandle *));
	if (resItems != NULL)
		*resItems = uio_realloc(items,
				numPDirHandles * sizeof (uio_MountTreeItem *));
	*resNumPDirHandles = numPDirHandles;

	return 0;
}

// returns 0 if the path is valid and exists
// returns -1 if the path is not valid or does not exist.
// in this case errno will be set to:
// ENOENT if some component didn't exist
// ENOTDIR is some component exists, but is not a dir
// something else (like EPATHTOOLONG) for internal errors
// On success, 'resolvedPath' will be set to the absolute path as returned by
// uio_resolvePath.
int
uio_verifyPath(uio_DirHandle *dirHandle, const char *path,
		char **resolvedPath) {
	uio_MountTree *tree;
	uio_MountTreeItem *item;
	const char *rest;
	int retVal;

	// TODO: "////", "/somedir//", and "//somedir" are accepted as valid
	
	// Determine the absolute path from 'path' which is relative to dirHandle.
	if (uio_resolvePath(dirHandle, path, strlen(path), resolvedPath) == -1) {
		// errno is set
		return -1;
	}
	
	// get the MountTree effective for the path
	uio_findMountTree(dirHandle->repository->mountTree, *resolvedPath,
			&tree, &rest);

	if (rest[0] == '\0') {
		// Complete match. Even if there are no pLocs in effect here
		// (which can only happen in case the mount Tree is empty and
		// we're viewing '/').
		return 0;
	}
	
	// try all the MountInfo structures in effect for this MountTree
	for (item = tree->pLocs; item != NULL; item = item->next) {
		const char *pRootPath;
		uio_PDirHandle *pDirHandle;
			
		pRootPath = uio_mountTreeItemRestPath(item, tree->lastComp,
				*resolvedPath);
		retVal = uio_walkPhysicalPath(item->mountInfo->pDirHandle, pRootPath,
				strlen(pRootPath), &pDirHandle, &rest);
		uio_PDirHandle_unref(pDirHandle);
		switch (retVal) {
			case 0:
				// complete match. We're done.
				return 0;
			case ENOTDIR:
				// A component is matched, but not as a dir. Failed.
				uio_free(*resolvedPath);
				errno = ENOTDIR;
				return -1;
			case ENOENT:
				// no match; try next pLoc
				continue;
			default:
				// Unknown error. Let's bail out just to be safe.
#ifdef DEBUG
				fprintf(stderr, "Warning: Unknown error from "
						"uio_walkPhysicalPath: %s\n", strerror(retVal));
#endif
				uio_free(*resolvedPath);
				errno = retVal;
				return -1;
		}
	}
	
	// No match, exit with ENOENT.
	uio_free(*resolvedPath);
	errno = ENOENT;
	return -1;
}

// Get the absolute path pointed to by 'path' relative to 'dirHandle'
// The new path will be put in '*destPath', which will be newly allocated.
// It will be \0-terminated, and the length will be returned.
// On error, -1 will be returned, and errno will be set.
ssize_t
uio_resolvePath(uio_DirHandle *dirHandle, const char *path, size_t pathLen,
		char **destPath) {
	size_t len;
	const char *pathEnd, *start, *end;
	int numUp;  // number of ".." dirs still need to be matched.
	char *buffer;
	char *endBufPtr;
	uio_bool absolute;

	absolute = path[0] == '/';
	pathEnd = path + pathLen;
	
	// Pass 1: count the amount of space needed
	len = 0;
	numUp = 0;
	for (getLastPathComponent(path, pathEnd, &start, &end);
			end > path;
			getPreviousPathComponent(path, &start, &end)) {
		if (start[0] == '.') {
			if (start + 1 == end) {
				// "." matched
				continue;
			}
			if (start[1] == '.' && start + 2 == end) {
				// ".." matched
				numUp++;
				continue;
			}
		}
		if (numUp > 0) {
			// last 'numUp' components were ".."
			numUp--;
			continue;
		}
		len += (end - start) + 1;
				// the 1 is for the '/'
	}

	// The part from 'dirHandle->path' to 'dirHandle->rootEnd' is
	// always copied (for a valid path). The rest we'll have to count.
	// (Note the 'rootEnd' in the initialiser of the for loop.)
	len += (dirHandle->rootEnd - dirHandle->path);
	if (!absolute) {
		for (getLastPath0Component(dirHandle->rootEnd, &start, &end);
				end > dirHandle->rootEnd;
				getPreviousPath0Component(dirHandle->rootEnd, &start, &end)) {
			if (numUp > 0) {
				numUp--;
				continue;
			}
			len += (end - start) + 1;
					// the 1 is for the '/'
		}
	}
	if (numUp > 0) {
		// too many ".."
		errno = ENOENT;
		return -1;
	}

	if (len == 0) {
		*destPath = uio_malloc(1);
		(*destPath)[0] = '\0';
		return 0;
	}

	// len--;  // we don't want a '/' at the start
	// len++;  // for the terminating '\0'
	buffer = uio_malloc(len);
	
	// Pass 2: fill the buffer
	endBufPtr = buffer + len - 1;
	*endBufPtr = '\0';
	numUp = 0;
	for (getLastPathComponent(path, pathEnd, &start, &end);
			end > path;
			getPreviousPathComponent(path, &start, &end)) {
		if (start[0] == '.') {
			if (start + 1 == end) {
				// "." matched
				continue;
			}
			if (start[1] == '.' && start + 2 == end) {
				// ".." matched
				numUp++;
				continue;
			}
		}
		if (numUp > 0) {
			// last 'numUp' components were ".."
			numUp--;
			continue;
		}
		endBufPtr -= (end - start);
		memcpy(endBufPtr, start, end - start);
		if (endBufPtr != buffer) {
			// We want no '/' at the beginning
			endBufPtr--;
			*endBufPtr = '/';
		} else {
			// We're already done. Might as well take advantage of
			// the fact that we know that and exit immediatly:
			*destPath = buffer;
			return len;
		}
	}
	// copy the part from dirHandle->path to dirHandle->rootEnd
	endBufPtr -= (dirHandle->rootEnd - dirHandle->path);
	memcpy(endBufPtr, dirHandle->path, dirHandle->rootEnd - dirHandle->path);
	if (!absolute) {
		// copy (some of) the components from dirHandle->rootEnd on.
		for (getLastPath0Component(dirHandle->rootEnd, &start, &end);
				end > dirHandle->rootEnd;
				getPreviousPath0Component(dirHandle->rootEnd, &start, &end)) {
			if (numUp > 0) {
				numUp--;
				continue;
			}
			endBufPtr -= (end - start);
			memcpy(endBufPtr, start, end - start);
			if (endBufPtr != buffer) {
				// We want no '/' at the beginning
				endBufPtr--;
				*endBufPtr = '/';
			} else {
				// We're already done. Might as well take advantage of
				// the fact that we know that and exit immediatly:
				break;
			}
		}
	}

	*destPath = buffer;	
	return len;
}


