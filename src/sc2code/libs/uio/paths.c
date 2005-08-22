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

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "paths.h"
#include "uioport.h"
#include "mem.h"

// gets the first dir component of a path
// sets '*start' to the start of the first component
// sets '*end' to the end of the first component
// if *start >= dirEnd, then the end has been reached.
void
getFirstPathComponent(const char *dir, const char *dirEnd,
		const char **startComp, const char **endComp) {
	assert(dir <= dirEnd);
	*startComp = dir;
	if (*startComp == dirEnd) {
		*endComp = *startComp;
		return;
	}
	*endComp = memchr(*startComp, '/', dirEnd - *startComp);
	if (*endComp == NULL)
		*endComp = dirEnd;
}

// gets the first dir component of a path
// sets '*start' to the start of the first component
// sets '*end' to the end of the first component
// if **start == '\0', then the end has been reached.
void
getFirstPath0Component(const char *dir,
		const char **startComp, const char **endComp) {
	*startComp = dir;
	if (**startComp == '\0') {
		*endComp = *startComp;
		return;
	}
	*endComp = strchr(*startComp, '/');
	if (*endComp == NULL)
		*endComp = *startComp + strlen(*startComp);
}

// gets the next component of a path
// '*start' should be set to the start of the last component
// '*end' should be set to the end of the last component
// '*start' will be set to the start of the next component
// '*end' will be set to the end of the next component
// if *start >= dirEnd, then the end has been reached.
void
getNextPathComponent(const char *dirEnd,
		const char **startComp, const char **endComp) {
	assert(*endComp <= dirEnd);
	if (*endComp == dirEnd) {
		*startComp = *endComp;
		return;
	}
	assert(**endComp == '/');
	*startComp = *endComp + 1;
	*endComp = memchr(*startComp, '/', dirEnd - *startComp);
	if (*endComp == NULL)
		*endComp = dirEnd;
}

// gets the next component of a path
// '*start' should be set to the start of the last component
// '*end' should be set to the end of the last component
// '*start' will be set to the start of the next component
// '*end' will be set to the end of the next component
// if **start == '\0', then the end has been reached.
void
getNextPath0Component(const char **startComp, const char **endComp) {
	if (**endComp == '\0') {
		*startComp = *endComp;
		return;
	}
	assert(**endComp == '/');
	*startComp = *endComp + 1;
	*endComp = strchr(*startComp, '/');
	if (*endComp == NULL)
		*endComp = *startComp + strlen(*startComp);
}

// if *end == dir, the beginning has been reached
void
getLastPathComponent(const char *dir, const char *endDir,
		const char **startComp, const char **endComp) {
	*endComp = endDir;
//	if (*(*endComp - 1) == '/')
//		(*endComp)--;
	*startComp = *endComp;
	// TODO: use memrchr where available
	while ((*startComp) - 1 >= dir && *(*startComp - 1) != '/')
		(*startComp)--;
}

// if *end == dir, the beginning has been reached
// pre: dir is \0-terminated
void
getLastPath0Component(const char *dir,
		const char **startComp, const char **endComp) {
	*endComp = dir + strlen(dir);
//	if (*(*endComp - 1) == '/')
//		(*endComp)--;
	*startComp = *endComp;
	// TODO: use memrchr where available
	while ((*startComp) - 1 >= dir && *(*startComp - 1) != '/')
		(*startComp)--;
}

// if *end == dir, the beginning has been reached
void
getPreviousPathComponent(const char *dir,
		const char **startComp, const char **endComp) {
	if (*startComp == dir) {
		*endComp = *startComp;
		return;
	}
	*endComp = *startComp - 1;
	*startComp = *endComp;
	while ((*startComp) - 1 >= dir && *(*startComp - 1) != '/')
		(*startComp)--;
}

// Combine two parts of a paths into a new path.
// The new path starts with a '/' only when the first part does.
// The first path may (but doesn't have to) end on a '/', or may be empty.
// Pre: the second path doesn't start with a '/'
char *
joinPaths(const char *first, const char *second) {
	char *result, *resPtr;
	size_t firstLen, secondLen;

	if (first[0] == '\0')
		return uio_strdup(second);
	
	firstLen = strlen(first);
	if (first[firstLen - 1] == '/')
		firstLen--;
	secondLen = strlen(second);
	result = uio_malloc(firstLen + secondLen + 2);
	resPtr = result;

	memcpy(resPtr, first, firstLen);
	resPtr += firstLen;

	*resPtr = '/';
	resPtr++;

	memcpy(resPtr, second, secondLen);
	resPtr += secondLen;

	*resPtr = '\0';
	return result;
}

// Combine two parts of a paths into a new path, 
// The new path will always start with a '/'.
// The first path may (but doesn't have to) end on a '/', or may be empty.
// Pre: the second path doesn't start with a '/'
char *
joinPathsAbsolute(const char *first, const char *second) {
	char *result, *resPtr;
	size_t firstLen, secondLen;

	if (first[0] == '\0') {
		secondLen = strlen(second);
		result = uio_malloc(secondLen + 2);
		result[0] = '/';
		memcpy(&result[1], second, secondLen);
		result[secondLen + 1] = '\0';
		return result;
	}

	firstLen = strlen(first);
	if (first[firstLen - 1] == '/')
		firstLen--;
	secondLen = strlen(second);
	result = uio_malloc(firstLen + secondLen + 3);
	resPtr = result;

	*resPtr = '/';
	resPtr++;

	memcpy(resPtr, first, firstLen);
	resPtr += firstLen;

	*resPtr = '/';
	resPtr++;

	memcpy(resPtr, second, secondLen);
	resPtr += secondLen;

	*resPtr = '\0';
	return result;
}

uio_bool
validPathName(const char *path, size_t len) {
	const char *pathEnd;
	const char *start, *end;

	pathEnd = path + len;
	getFirstPathComponent(path, pathEnd, &start, &end);
	while (start < pathEnd) {
		if (end == start || (end - start == 1 && start[0] == '.') ||
				(end - start == 2 && start[0] == '.' && start[1] == '.'))
			return false;
		getNextPathComponent(pathEnd, &start, &end);
	}
	return true;
}


