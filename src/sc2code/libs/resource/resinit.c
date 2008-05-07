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
	ndx->typeInfo.numTypes = 0;
	ndx->typeInfo.handlers = NULL;
	ndx->map = CharHashTable_newHashTable (NULL, NULL, NULL, NULL, 0, 0.85, 0.9);
	return ndx;
}

static void
freeResourceIndex (RESOURCE_INDEX h) {
	if (h != NULL)
	{
		/* TODO: This leaks the contents of h->map */
		CharHashTable_deleteHashTable (h->map);
		if (h->typeInfo.handlers != NULL)
			HFree (h->typeInfo.handlers);
		HFree (h);
	}
}

#define TYPESIZ 32
/* These MUST COME in the SAME ORDER as the enums in reslib.h!!!
   They also must be no more than TYPESIZ-1 characters long, but that's
   unlikely to be a problem. */
static const char *res_type_strings[] = {
	"UNKNOWNRES",
	"GFXRES",
	"FONTRES",
	"STRTAB",
	"SNDRES",
	"MUSICRES",
	"CODE",
	NULL
};	

static ResourceDesc *
newResourceDesc (const char *res_id, const char *resval)
{
	char *path;
	int pathlen;
	RES_TYPE resType;
	ResourceDesc *result;
	RESOURCE_INDEX idx = _get_current_index_header ();

	path = strchr (resval, ':');
	if (path == NULL)
	{
		log_add (log_Warning, "Could not find type information for resource '%s'", res_id);
		return NULL;
	}
	else
	{
		char typestr[TYPESIZ];
		int n = path - resval;

		if (n >= TYPESIZ)
		{
			n = TYPESIZ - 1;
		}

		strncpy (typestr, resval, n);
		typestr[n] = '\0';

		path++;
		pathlen = strlen (path);

		resType = UNKNOWNRES;
		while (res_type_strings[resType])
		{
			if (!strcmp (typestr, res_type_strings[resType]))
			{
				break;
			}
			resType++;
		}
		if (!res_type_strings[resType])
		{
			log_add (log_Warning, "Illegal type '%s' for resource '%s'", typestr, res_id);
			return NULL;
		}
	}

	if (resType >= idx->typeInfo.numTypes)
	{
		log_add (log_Warning, "Warning: Unable to load '%s'; no handler "
				"for type %d defined.", res_id, resType);
		return NULL;
	}
	
	if (idx->typeInfo.handlers[resType].loadFun == NULL)
	{
		log_add (log_Warning, "Warning: Unable to load '%s'; no handler "
				"for type %s defined.", res_id, res_type_strings[resType]);
		return NULL;
	}

	result = HMalloc (sizeof (ResourceDesc));
	if (result == NULL)
		return NULL;

	result->fname = HMalloc (pathlen + 1);
	strncpy (result->fname, path, pathlen);
	result->fname[pathlen] = '\0';
	result->restype = resType;
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

RESOURCE_INDEX
InitResourceSystem (void)
{
	RESOURCE_INDEX ndx = allocResourceIndex ();
	
	_set_current_index_header (ndx);

	INIT_INSTANCES ();

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
InstallResTypeVectors (RES_TYPE resType, ResourceLoadFun *loadFun,
		ResourceFreeFun *freeFun)
{
	ResourceHandlers handlers;
	RESOURCE_INDEX idx = _get_current_index_header ();
	handlers.loadFun = loadFun;
	handlers.freeFun = freeFun;

	if (resType >= idx->typeInfo.numTypes) {
		// Have to enlarge the handler array.
		ResourceHandlers *newHandlers = HRealloc (idx->typeInfo.handlers,
				(resType + 1) * sizeof (ResourceHandlers));
		if (newHandlers == NULL)
			return FALSE;  // idx->typeInfo.handlers is untouched

		// Clear the space for new entries. No need to init the last one;
		// it's going to be used immediately.
		memset (&newHandlers[idx->typeInfo.numTypes], 0,
				(resType - idx->typeInfo.numTypes /* + 1 - 1 */)
				* sizeof (ResourceHandlers));

		idx->typeInfo.handlers = newHandlers;
		idx->typeInfo.numTypes = resType + 1;
	}

	idx->typeInfo.handlers[resType] = handlers;
	return TRUE;
}
