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
#include "nameref.h"
#include "types.h"
#include "libs/log.h"
#include "propfile.h"
#include <ctype.h>
#include <stdlib.h>

static RESOURCE_INDEX
allocResourceIndex (void) {
	RESOURCE_INDEX ndx = HMalloc (sizeof (RESOURCE_INDEX_DESC));
	ndx->map = CharHashTable_newHashTable (NULL, NULL, NULL, NULL, 0, 0.85, 0.9);
	return ndx;
}

static void
freeResourceIndex (RESOURCE_INDEX h) {
	if (h != NULL)
	{
		/* TODO: This leaks the contents of h->map */
		CharHashTable_deleteHashTable (h->map);
		HFree (h);
	}
}

#define TYPESIZ 32

static ResourceDesc *
newResourceDesc (const char *res_id, const char *resval)
{
	const char *path;
	int pathlen;
	ResourceHandlers *vtable;
	ResourceDesc *result, *handlerdesc;
	RESOURCE_INDEX idx = _get_current_index_header ();
	char typestr[TYPESIZ];

	path = strchr (resval, ':');
	if (path == NULL)
	{
		log_add (log_Warning, "Could not find type information for resource '%s'", res_id);
		strncpy(typestr, "sys.UNKNOWNRES", TYPESIZ);
		path = resval;
	}
	else
	{
		int n = path - resval;

		if (n >= TYPESIZ - 4)
		{
			n = TYPESIZ - 5;
		}
		strncpy (typestr, "sys.", TYPESIZ);
		strncat (typestr+1, resval, n);
		typestr[n+4] = '\0';
		path++;
	}
	pathlen = strlen (path);

	handlerdesc = lookupResourceDesc(idx, typestr);
	if (handlerdesc == NULL) {
		path = resval;
		log_add (log_Warning, "Illegal type '%s' for resource '%s'; treating as UNKNOWNRES", typestr, res_id);
		handlerdesc = lookupResourceDesc(idx, "sys.UNKNOWNRES");
	}

	vtable = (ResourceHandlers *)handlerdesc->resdata;

	if (vtable->loadFun == NULL)
	{
		log_add (log_Warning, "Warning: Unable to load '%s'; no handler "
				"for type %s defined.", res_id, typestr);
		return NULL;
	}

	result = HMalloc (sizeof (ResourceDesc));
	if (result == NULL)
		return NULL;

	result->fname = HMalloc (pathlen + 1);
	strncpy (result->fname, path, pathlen);
	result->fname[pathlen] = '\0';
	result->vtable = vtable;
	result->resdata = NULL;
	return result;
}

static void
process_resource_desc (const char *key, const char *value)
{
	CharHashTable_HashTable *map = _get_current_index_header ()->map;
	ResourceDesc *newDesc = newResourceDesc (key, value);
	if (newDesc != NULL)
	{
		if (!CharHashTable_add (map, key, newDesc))
		{
			ResourceDesc *oldDesc = (ResourceDesc *)CharHashTable_find (map, key);
			if (oldDesc != NULL)
			{
				if (newDesc->resdata != NULL)
				{
					/* XXX: It might be nice to actually clean it up properly */
					log_add (log_Warning, "LEAK WARNING: Replaced '%s' while it was live", key);
				}
				HFree (oldDesc->fname);
				HFree (oldDesc);
			}
			CharHashTable_remove (map, key);
			CharHashTable_add (map, key, newDesc);
		}
	}
}

void *
UseDescriptorAsRes (const char *descriptor)
{
	return (void *)descriptor;
}

void *
DescriptorToInt (const char *descriptor)
{
	intptr_t value = atoi(descriptor);
	return (void *) value;
}

void *
DescriptorToBoolean (const char *descriptor)
{
	if (!stricmp(descriptor, "true"))
	{
		return (void *)(1);
	}
	else
	{
		return (void *)(0);
	}
}
	    

BOOLEAN
NullFreeRes (void *data)
{
	(void)data;
	return TRUE;
}

RESOURCE_INDEX
InitResourceSystem (void)
{
	RESOURCE_INDEX ndx = allocResourceIndex ();
	
	_set_current_index_header (ndx);

	InstallResTypeVectors ("UNKNOWNRES", UseDescriptorAsRes, NullFreeRes);
	InstallResTypeVectors ("STRING", UseDescriptorAsRes, NullFreeRes);
	InstallResTypeVectors ("INT32", DescriptorToInt, NullFreeRes);
	InstallResTypeVectors ("BOOLEAN", DescriptorToBoolean, NullFreeRes);
	InstallGraphicResTypes ();
	InstallStringTableResType ();
	InstallAudioResTypes ();
	InstallCodeResType ();

	return ndx;
}

void
LoadResourceIndex (uio_DirHandle *dir, const char *rmpfile)
{
	PropFile_from_filename (dir, rmpfile, process_resource_desc);
}

void
UninitResourceSystem (void)
{
	freeResourceIndex (_get_current_index_header ());
	_set_current_index_header (NULL);
}

BOOLEAN
InstallResTypeVectors (const char *resType, ResourceLoadFun *loadFun,
		ResourceFreeFun *freeFun)
{
	ResourceHandlers *handlers;
	ResourceDesc *result;
	char key[TYPESIZ];
	int typelen;
	
	snprintf(key, TYPESIZ, "sys.%s", resType);
	key[TYPESIZ-1] = '\0';
	typelen = strlen(resType);
	
	handlers = HMalloc (sizeof (ResourceHandlers));
	if (handlers == NULL)
	{
		return FALSE;
	}
	handlers->loadFun = loadFun;
	handlers->freeFun = freeFun;
	handlers->resType = resType;
	
	result = HMalloc (sizeof (ResourceDesc));
	if (result == NULL)
		return FALSE;

	result->fname = HMalloc (strlen(resType) + 1);
	strncpy (result->fname, resType, typelen);
	result->fname[typelen] = '\0';
	result->vtable = NULL;
	result->resdata = handlers;

	CharHashTable_HashTable *map = _get_current_index_header ()->map;
	return CharHashTable_add (map, key, result) != 0;
}
