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

#ifndef _RESINTRN_H
#define _RESINTRN_H

#include <string.h>
#include "reslib.h"
#include "index.h"

typedef struct
{
	DWORD flags_and_data_loc;
	COUNT num_valid_handles;
} RES_HANDLE_LIST;
typedef RES_HANDLE_LIST *PRES_HANDLE_LIST;
typedef PRES_HANDLE_LIST RES_HANDLE_LISTPTR;

typedef PDWORD ENCODEPTR;
typedef PBYTE DATAPTR;

#define RES_HANDLE_LIST_PRIORITY DEFAULT_MEM_PRIORITY

#define AllocResourceHandleList(nt,ni) \
	mem_allocate ((MEM_SIZE)(sizeof (RES_HANDLE_LIST) \
			+ (PACKMEM_LIST_SIZE * (nt)) \
			+ (INSTANCE_LIST_SIZE * (ni))), MEM_ZEROINIT | MEM_PRIMARY, \
			RES_HANDLE_LIST_PRIORITY, MEM_SIMPLE)
#define LockResourceHandleList(pRH,h,p,rp,tp,dp) \
do \
{ \
	*(rp) = (RES_HANDLE_LISTPTR)mem_lock ((MEM_HANDLE)h); \
	*(tp) = (ENCODEPTR)&(*(rp))[1]; \
	*(dp) = (DATAPTR)&(*(tp))[CountPackageTypes (pRH,p)]; \
} while (0)
#define UnlockResourceHandleList(h) mem_unlock((MEM_HANDLE)h)
#define FreeResourceHandleList(h) mem_release((MEM_HANDLE)h)

#define DoLoad(pRH, t,fp,len) (*(pRH)->TypeList[t-1].func_vectors.load_func)(fp,len)
#define DoFree(pRH, t,h) (*(pRH)->TypeList[t-1].func_vectors.free_func)(h)

extern MEM_HANDLE _GetResFileData (FILE *res_fp, DWORD flen);
#define IsIndexType(pRH, t) ((pRH)->TypeList[t-1].func_vectors.load_func \
										==_GetResFileData)

#define LastResPackage(pRH) ((pRH)->num_packages)
#define LastResType(pRH) ((pRH)->num_types)
#define GetInstanceCount(pRH,t) \
		((pRH)->TypeList[(t)-1].instance_count)

#define ValidResPackage(pRH,p) ((p)<=LastResPackage(pRH))
#define ValidResType(pRH,t) ((t)!=0&&(t)<=LastResType(pRH))

#define CountPackageTypes(pRH,p) \
		(COUNT)GET_TYPE ((pRH)->PackageList[(p)-1].packmem_info)
#define CountPackageInstances(pRH,p) \
		(COUNT)GET_INSTANCE ((pRH)->PackageList[(p)-1].packmem_info)

extern void _set_current_index_header (INDEX_HEADERPTR ResHeaderPtr);
extern INDEX_HEADERPTR _get_current_index_header (void);

extern char *_cur_resfile_name;

#endif /* _RESINTRN_H */

