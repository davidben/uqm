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
static RESOURCE_INDEX allocResourceIndex(void);
static void freeResourceIndex (RESOURCE_INDEX h);


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
newResourceDesc (RESOURCE res, char *res_id)
{
	ResourceDesc *result;

	result = HMalloc (sizeof (ResourceDesc));
	if (result == NULL)
		return NULL;

	// log_add (log_Info, "    '%s' -> '%s'.", res_id, path);

	result->res = res;
	result->res_id = res_id;
	result->resdata = NULL;
	//result->ref = 0;
	return result;
}

static RESOURCE_INDEX
loadResourceIndex (uio_Stream *stream, const char *fileName) {
	size_t lineNum;
	ResourceDesc **descs = NULL;
	COUNT numRes;
	int numDescsAlloced = 0;
	RESOURCE_INDEX ndx = NULL;
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

	ndx = allocResourceIndex ();
	if (ndx == NULL)
		goto err;
	
	ndx->res = descs;
	ndx->numRes = numRes;
	ndx->typeInfo.numTypes = 0;
	ndx->typeInfo.handlers = NULL;
	List_add (indexList, (void *) ndx);

	return ndx;

err:
	if (ndx != NULL)
		freeResourceIndex (ndx);
	HFree (descs);
	return NULL;
}

// Get the data associated with a resource of type RES_INDEX
// (In other words, a ResourceIndex)
void *
_GetResFileData (uio_Stream *res_fp, DWORD fileLength)
{
	RESOURCE_INDEX ndx, parentNdx;

#if 0
	ResourceDesc *desc;

	desc = lookupResourceIndex (_cur_resfile_name);
	assert(desc != NULL);

	if (desc->handle != NULL_HANDLE)
		return desc->handle;
#endif

	ndx = loadResourceIndex (res_fp, _cur_resfile_name);
	parentNdx = _get_current_index_header ();
	if (parentNdx != NULL)
		copyResTypeHandlers (ndx, parentNdx);
	
	(void) fileLength;
	return ndx;
}

BOOLEAN
_ReleaseResFileData (RESOURCE_INDEX ndx)
{
	if (ndx->typeInfo.handlers != NULL)
		HFree (ndx->typeInfo.handlers);
	HFree (ndx);
	return TRUE;
}

RESOURCE_INDEX
InitResourceSystem (const char *mapfile, const char *resfile, RES_TYPE resType, 
		BOOLEAN (*FileErrorFun) (const char *filename))
{
	ResourceIndex *ndx;
	ResourceHandlers handlers;

	initIndexList ();
	
	handlers.loadFun = _GetResFileData;
	handlers.freeFun = _ReleaseResFileData;

	res_LoadFilename (contentDir, mapfile);

	SetResourceIndex (NULL);
	ndx = loadResource (resfile, handlers.loadFun);
	if (ndx == NULL)
		return NULL;

	setResTypeHandlers (ndx, resType, &handlers);

	SetResourceIndex (ndx);

	(void) FileErrorFun;
	(void) mapfile;
	return ndx;
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
	
	SetResourceIndex (NULL);
}

RESOURCE_INDEX
OpenResourceIndexInstance (RESOURCE res)
{
	RESOURCE_INDEX hRH;

	hRH = (RESOURCE_INDEX)res_GetResource (res);
	if (hRH)
		res_DetachResource (res);

	return (hRH);
}

// Sets the current global resource index.
// This is the index res_GetResource() calls will use.
RESOURCE_INDEX
SetResourceIndex (RESOURCE_INDEX newIndex)
{
	static RESOURCE_INDEX currentIndex;
			// NB: currentIndexHandle is locked.
	RESOURCE_INDEX oldIndex;
	
	oldIndex = currentIndex;
	currentIndex = newIndex;

	if (currentIndex != NULL) {
		RESOURCE_INDEX ndx;

		ndx = currentIndex;
		_set_current_index_header (ndx);
	} else
		_set_current_index_header (NULL);
	
	return oldIndex;
}

BOOLEAN
CloseResourceIndex (RESOURCE_INDEX ndx)
{
	if (ndx == _get_current_index_header ())
		SetResourceIndex (NULL);

	List_remove ((void *) indexList, ndx);
	
	_ReleaseResFileData (ndx);

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


static RESOURCE_INDEX
allocResourceIndex (void) {
	return HMalloc (sizeof (struct resource_index));
}

static void
freeResourceIndex (RESOURCE_INDEX h) {
	HFree (h);
}


