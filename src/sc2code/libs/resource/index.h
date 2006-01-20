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

#ifndef _INDEX_H
#define _INDEX_H

#include <stdio.h>

typedef struct
{
	RESOURCE res;
	char *path;
	MEM_HANDLE handle;
	//COUNT ref;
} ResourceDesc;

typedef struct
{
	ResourceLoadFun *loadFun;
	ResourceFreeFun *freeFun;
} ResourceHandlers;

typedef struct
{
	RES_TYPE numTypes;
			/* Number of types in the handlers array (whether NULL or not).
			 * == the highest stored handler number + 1.
			 */
	ResourceHandlers *handlers;
} ResourceTypeInfo;

typedef struct
{
	ResourceDesc **res;
	ResourceTypeInfo typeInfo;
	size_t numRes;
} ResourceIndex;


void
forAllResourceIndices(
		void (*callback) (ResourceIndex *idx, void *extra), void *extra);


#define INDEX_HEADER_PRIORITY DEFAULT_MEM_PRIORITY

static inline ResourceIndex *
lockResourceIndex (MEM_HANDLE h) {
	return (ResourceIndex *) mem_lock (h);
}
static inline void
unlockResourceIndex (MEM_HANDLE h) {
	mem_unlock (h);
}


#endif /* _INDEX_H */

