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

#include "resintrn.h"
#include "misc.h"
#include "options.h"
#include "types.h"
#include "libs/list.h"
#include "libs/log.h"
#include <ctype.h>
#include <stdlib.h>


static bool copyResTypeHandlers (ResourceIndex *dest,
		const ResourceIndex *src);
static bool setResTypeHandlers (ResourceIndex *idx, RES_TYPE resType,
		const ResourceHandlers *handlers);
static MEM_HANDLE allocResourceIndex(void);
static void freeResourceIndex (MEM_HANDLE h);


static List_List *indexList;


static void
initIndexList (void)
{
	indexList = List_newList ();
}

static void
uninitIndexList (void)
{
	List_deleteList (indexList);
	indexList = NULL;
}

void
forAllResourceIndices(
		void (*callback) (ResourceIndex *idx, void *extra),
		void *extra) {
	List_Link *link;

	for (link = indexList->first; link != NULL; link = link->next)
	{
		ResourceIndex *idx = (ResourceIndex *) link->entry;
		(*callback) (idx, extra);
	}
}


#if 0
static ResourceDesc *
lookupResourceIndex (const char *path)
{
	List_Link *link;

	for (link = indexList->first; link != NULL; link = link->next)
	{
		ResourceDesc *desc = (ResourceDesc *) link->entry;
		
		if (strcmp (desc->path, path) == 0)
			return link->entry;
	}

	return NULL;
}
#endif

static ResourceDesc *
newResourceDesc (RESOURCE res, char *path)
{
	ResourceDesc *result;

	result = HMalloc (sizeof (ResourceDesc));
	if (result == NULL)
		return NULL;

	result->res = res;
	result->path = path;
	result->handle = NULL_HANDLE;
	//result->ref = 0;
	return result;
}

static MEM_HANDLE
loadResourceIndex (uio_Stream *stream, const char *fileName) {
	size_t lineNum;
	ResourceDesc **descs = NULL;
	COUNT numRes;
	int numDescsAlloced = 0;
	MEM_HANDLE indexHandle = NULL_HANDLE;
	ResourceIndex *ndx = NULL;
#ifdef DEBUG
	RESOURCE lastResource = 0x00000000;
#endif
	
	// This loop parses lines of the format "<resnum> <path>".
	// Resnum is in hex. '#' can be used to add comments.
	lineNum = 0;
	numRes = 0;
	for (;;)
	{
		char lineBuf[PATH_MAX + 80];
		char *ptr;
		char *endPtr;
		char *path;
		char *newPath;
		RESOURCE res;
		ResourceDesc *desc;
		
		if (uio_fgets (lineBuf, sizeof lineBuf, stream) == NULL)
		{
			if (uio_ferror (stream))
				goto err;
			else
				break;  // EOF
		}

		lineNum++;

		// Skip comments.
		ptr = strchr (lineBuf, '#');
		if (ptr != NULL)
			*ptr = '\0';

		// Omit leading whitespace.
		ptr = lineBuf;
		while (isspace (*ptr))
			ptr++;
		
		// Omit trailing whitespace.
		endPtr = ptr + strlen(ptr);
		while (endPtr != ptr && isspace (endPtr[-1]))
		{
			endPtr--;
			*endPtr = '\0';
		}

		// If the line is empty (apart from comments and whitespace)
		// go to the next one.
		if (*ptr == 0)
			continue;

		{
			unsigned int resPackage;
			unsigned int resInstance;
			unsigned int resType;
			unsigned int numParsed;

			if (sscanf (ptr, "%i %i %i %n",
					&resPackage, &resInstance, &resType, &numParsed) != 3)
			{
				log_add (log_Warning, "Resource index '%s': Invalid line %d.",
						fileName, lineNum);
				continue;
			}
			path = ptr + numParsed;
		
			res = MAKE_RESOURCE (resPackage, resType, resInstance);
		}

		while (isspace (*path))
			path++;
		if (*path == '\0')
		{
			// No path supplied.
			log_add (log_Warning, "Resource index '%s': Invalid line %d.",
					fileName, lineNum);
			continue;
		}

#ifdef DEBUG
		// We need the list to be sorted, as we binary search through it.
		if (res <= lastResource)
		{
			log_add (log_Fatal, "Fatal: resource index '%s' is not sorted "
					"on the resource number, or contains a double entry. "
					"Problem encountered on line %d.", fileName, lineNum);
			explode ();
		}
		lastResource = res;
#endif

#define DESC_NUM_INCREMENT 80
		if (numDescsAlloced <= numRes)
		{
			// Need to enlarge the array of resource descs.
			ResourceDesc **newDescs = HRealloc (descs,
					sizeof (ResourceDesc *) *
					(numDescsAlloced + DESC_NUM_INCREMENT));
			if (newDescs == NULL)
				goto err;  // Original descs is untouched.
			descs = newDescs;
			numDescsAlloced += DESC_NUM_INCREMENT;
		}

		// Can't use strdup; it doesn't use HMalloc.
		newPath = HMalloc (endPtr - path + 1);
		if (newPath == NULL)
			goto err;
		strcpy (newPath, path);

		desc = newResourceDesc (res, newPath);
		if (desc == NULL)
			goto err;

		descs[numRes] = desc;	
		numRes++;
	}

	{
		ResourceDesc **newDescs = HRealloc (descs,
				sizeof (ResourceDesc *) * numRes);
		if (newDescs == NULL && numRes != 0)
			goto err;  // Original descs is untouched.
		descs = newDescs;
	}

	if (numRes == 0)
		log_add (log_Debug, "Warning: Resource index '%s' contains no valid "
				"entries.", fileName);

	indexHandle = allocResourceIndex ();
	if (indexHandle == NULL_HANDLE)
		goto err;
	
	ndx = lockResourceIndex (indexHandle);
	ndx->res = descs;
	ndx->numRes = numRes;
	ndx->typeInfo.numTypes = 0;
	ndx->typeInfo.handlers = NULL;
	List_add (indexList, (void *) ndx);
	unlockResourceIndex (indexHandle);

	return indexHandle;

err:
	if (indexHandle != NULL_HANDLE)
		freeResourceIndex (indexHandle);
	HFree (descs);
	return NULL_HANDLE;
}

// Get the data associated with a resource of type RES_INDEX
// (In other words, a ResourceIndex)
MEM_HANDLE
_GetResFileData (uio_Stream *res_fp, DWORD fileLength)
{
	MEM_HANDLE handle;
	ResourceIndex *ndx;
	ResourceIndex *parentNdx;

#if 0
	ResourceDesc *desc;

	desc = lookupResourceIndex (_cur_resfile_name);
	assert(desc != NULL);

	if (desc->handle != NULL_HANDLE)
		return desc->handle;
#endif

	handle = loadResourceIndex (res_fp, _cur_resfile_name);
	if (handle == NULL_HANDLE)
		return NULL_HANDLE;

	ndx = lockResourceIndex (handle);
	parentNdx = _get_current_index_header ();
	if (parentNdx != NULL)
		copyResTypeHandlers (ndx, parentNdx);
	
	unlockResourceIndex (handle);

	(void) fileLength;
	return handle;
}

BOOLEAN
_ReleaseResFileData (MEM_HANDLE handle)
{
	ResourceIndex *ndx;

	ndx = lockResourceIndex (handle);
	if (ndx->typeInfo.handlers != NULL)
		HFree (ndx->typeInfo.handlers);
	unlockResourceIndex (handle);
	mem_release (handle);
	return TRUE;
}

MEM_HANDLE
InitResourceSystem (const char *resfile, RES_TYPE resType, BOOLEAN
		(*FileErrorFun) (const char *filename))
{
	MEM_HANDLE handle;
	ResourceIndex *ndx;
	ResourceHandlers handlers;

	initIndexList ();
	
	handlers.loadFun = _GetResFileData;
	handlers.freeFun = _ReleaseResFileData;

	SetResourceIndex (NULL_HANDLE);
	handle = loadResource (resfile, handlers.loadFun);
	if (handle == NULL_HANDLE)
		return NULL_HANDLE;

	ndx = lockResourceIndex (handle);
	setResTypeHandlers (ndx, resType, &handlers);
	unlockResourceIndex (handle);

	SetResourceIndex (handle);

	(void) FileErrorFun;
	return handle;
}

void
UninitResourceSystem (void)
{
#if 0
	List_Link *link;

	for (link = indexList->first; link != NULL; link = link->next)
	{
		ResourceIndex *ndx = (ResourceIndex *) link->entry;
		
	}
#endif
	uninitIndexList ();
	
	SetResourceIndex (NULL_HANDLE);
}

MEM_HANDLE
OpenResourceIndexInstance (RESOURCE res)
{
	MEM_HANDLE hRH;

	hRH = res_GetResource (res);
	if (hRH)
		res_DetachResource (res);

	return (hRH);
}

// Sets the current global resource index.
// This is the index res_GetResource() calls will use.
MEM_HANDLE
SetResourceIndex (MEM_HANDLE newIndexHandle)
{
	static MEM_HANDLE currentIndexHandle;
			// NB: currentIndexHandle is locked.
	MEM_HANDLE oldIndexHandle;
	
	if (currentIndexHandle != NULL_HANDLE)
		unlockResourceIndex (currentIndexHandle);

	oldIndexHandle = currentIndexHandle;
	currentIndexHandle = newIndexHandle;

	if (currentIndexHandle != NULL_HANDLE) {
		ResourceIndex *ndx;

		ndx = lockResourceIndex (currentIndexHandle);
		_set_current_index_header (ndx);
	} else
		_set_current_index_header (NULL);
	
	return oldIndexHandle;
}

BOOLEAN
CloseResourceIndex (MEM_HANDLE handle)
{
	ResourceIndex *ndx;
	
	unlockResourceIndex (handle);

	ndx = lockResourceIndex (handle);
	if (ndx == _get_current_index_header ())
		SetResourceIndex (NULL_HANDLE);

	List_remove ((void *) indexList, ndx);
	
	unlockResourceIndex (handle);

	_ReleaseResFileData (handle);

	return TRUE;
}

BOOLEAN
InstallResTypeVectors (RES_TYPE resType, ResourceLoadFun *loadFun,
		ResourceFreeFun *freeFun)
{
	ResourceHandlers handlers;
	handlers.loadFun = loadFun;
	handlers.freeFun = freeFun;

	setResTypeHandlers(_get_current_index_header (), resType, &handlers);
	return TRUE;
}

static bool
setResTypeHandlers (ResourceIndex *idx, RES_TYPE resType,
		const ResourceHandlers *handlers)
{
	if (resType >= idx->typeInfo.numTypes) {
		// Have to enlarge the handler array.
		ResourceHandlers *newHandlers = HRealloc (idx->typeInfo.handlers,
				(resType + 1) * sizeof (ResourceHandlers));
		if (newHandlers == NULL)
			return false;  // idx->typeInfo.handlers is untouched

		// Clear the space for new entries. No need to init the last one;
		// it's going to be used immediately.
		memset (&newHandlers[idx->typeInfo.numTypes], 0,
				(resType - idx->typeInfo.numTypes /* + 1 - 1 */)
				* sizeof (ResourceHandlers));

		idx->typeInfo.handlers = newHandlers;
		idx->typeInfo.numTypes = resType + 1;
	}

	idx->typeInfo.handlers[resType] = *handlers;
	return true;
}

static bool
copyResTypeHandlers (ResourceIndex *dest, const ResourceIndex *src)
{
	assert (src != dest);

	if (dest->typeInfo.handlers != NULL)
		HFree (dest->typeInfo.handlers);

	dest->typeInfo.handlers = HMalloc (src->typeInfo.numTypes *
			sizeof (ResourceHandlers));
	if (dest->typeInfo.handlers == NULL)
		return false;
	
	memcpy (dest->typeInfo.handlers, src->typeInfo.handlers,
			src->typeInfo.numTypes * sizeof (ResourceHandlers));
	dest->typeInfo.numTypes = src->typeInfo.numTypes;
	
	return true;
}


static MEM_HANDLE
allocResourceIndex (void) {
	return mem_allocate (sizeof (ResourceIndex), MEM_PRIMARY,
			INDEX_HEADER_PRIORITY, MEM_SIMPLE);
}

static void
freeResourceIndex (MEM_HANDLE h) {
	mem_release (h);
}


