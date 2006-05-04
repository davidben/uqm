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

#include "options.h"
#include "port.h"
#include "resintrn.h"
#include "libs/misc.h"
#include "libs/log.h"


const char *_cur_resfile_name;
// When a file is being loaded, _cur_resfile_name is set to its name.
// At other times, it is NULL.

static ResourceDesc *
lookupResourceDesc (ResourceIndex *idx, RESOURCE res) {
	// Binary search through the resources.
	COUNT l, h;

	if (idx->numRes == 0)
		return NULL;
	
	l = 0;
	h = idx->numRes;

	while (l + 1 != h)
	{
		COUNT m = (l + h) / 2;
		if (idx->res[m]->res <= res)
			l = m;
		else
			h = m;
	}

	if (idx->res[l]->res == res)
		return idx->res[l];
	return NULL;
}

// Load every resource in a package.
static void
loadPackage (ResourceIndex *idx, RES_PACKAGE pkg) {
	COUNT i;

	for (i = 0; i < idx->numRes; i++) {
		if (GET_PACKAGE (idx->res[i]->res) != pkg)
			continue;
	
		if (idx->res[i]->handle != NULL_HANDLE)
			continue;  // Already loaded

		loadResourceDesc (idx, idx->res[i]);
	}
}

MEM_HANDLE
loadResourceDesc (ResourceIndex *idx, ResourceDesc *desc)
{
	RES_TYPE resType = GET_TYPE (desc->res);
	
	if (resType >= idx->typeInfo.numTypes ||
			idx->typeInfo.handlers[resType].loadFun == NULL)
	{
		log_add (log_Warning, "Warning: Unable to load '%s'; no handler "
				"for type %d defined.", desc->path, resType);
		return NULL_HANDLE;
	}

	desc->handle = loadResource (desc->path,
			idx->typeInfo.handlers[resType].loadFun);
	return desc->handle;
}

MEM_HANDLE
loadResource(const char *path, ResourceLoadFun *loadFun)
{
	uio_Stream *stream;
	long dataLen;
	MEM_HANDLE handle;

	stream = res_OpenResFile (contentDir, path, "rb");
	if (stream == NULL)
	{
		log_add (log_Warning, "Warning: Can't open '%s'", path);
		return NULL_HANDLE;
	}

	dataLen = LengthResFile (stream);
	log_add (log_Info, "\t'%s' -- %lu bytes", path, dataLen);
	
	if (dataLen == 0)
	{
		log_add (log_Warning, "Warning: Trying to load empty file '%s'.", path);
		goto err;
	}

	_cur_resfile_name = path;
	handle = (*loadFun) (stream, dataLen);
	_cur_resfile_name = NULL;
	res_CloseResFile (stream);

	return handle;

err:
	res_CloseResFile (stream);
	return NULL_HANDLE;
}

// Get a resource by its resource ID.
// For historical reasons, if the resource is not already loaded,
// the *entire* package will be (pre-)loaded.
// NB. It may be better to add an extra flag to res_GetResource() to
// indicate whether you really want this, or add a separate function
// for preloading the entire package.
MEM_HANDLE
res_GetResource (RESOURCE res)
{
	ResourceIndex *resourceIndex;
	ResourceDesc *desc;
	
	resourceIndex = _get_current_index_header ();

	desc = lookupResourceDesc (resourceIndex, res);
	if (desc == NULL)
	{
		log_add (log_Warning, "Trying to get undefined resource %08x",
				(DWORD) res);
		return NULL_HANDLE;
	}

	if (desc->handle != NULL_HANDLE)
		return desc->handle;

	// Load the entire package, for historical reasons.
	loadPackage (resourceIndex, GET_PACKAGE (res));

	return desc->handle;
			// May still be NULL_HANDLE, if the load failed.
}

// NB: this function appears to be never called!
void
res_FreeResource (RESOURCE res)
{
	ResourceDesc *desc;
	ResourceFreeFun *freeFun;
	ResourceIndex *idx;

	desc = lookupResourceDesc (_get_current_index_header(), res);
	if (desc == NULL)
	{
		log_add (log_Debug, "Warning: trying to free an unrecognised "
				"resource.");
		return;
	}
	
	if (desc->handle == NULL_HANDLE)
	{
		log_add (log_Debug, "Warning: trying to free not loaded "
				"resource.");
		return;
	}

	idx = _get_current_index_header ();
	freeFun = idx->typeInfo.handlers[GET_TYPE (res)].freeFun;
	(*freeFun) (desc->handle);
	desc->handle = NULL_HANDLE;
}

// By calling this function the caller will be responsible of unloading
// the resource. If res_GetResource() get called again for this
// resource, a NEW copy will be loaded, regardless of whether a detached
// copy still exists.
MEM_HANDLE
res_DetachResource (RESOURCE res)
{
	ResourceDesc *desc;
	MEM_HANDLE result;

	desc = lookupResourceDesc (_get_current_index_header(), res);
	if (desc == NULL)
	{
		log_add (log_Debug, "Warning: trying to detach from an unrecognised "
				"resource.");
		return NULL_HANDLE;
	}
	
	if (desc->handle == NULL_HANDLE)
	{
		log_add (log_Debug, "Warning: trying to detach from a not loaded "
				"resource.");
		return NULL_HANDLE;
	}

	result = desc->handle;
	desc->handle = NULL_HANDLE;

	return result;
}


