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

#include "libs/strings/strintrn.h"
#include "port.h"
#include "libs/uio.h"
#include <sys/stat.h>

DIRENTRY_REF
LoadDirEntryTable (uio_DirHandle *dirHandle, const char *path,
		const char *pattern, match_MatchType matchType, PCOUNT pnum_entries)
{
	uio_DirList *dirList;
	COUNT num_entries, length;
	COUNT i;
	COUNT slen;
	uio_DirHandle *dir;
	STRING_TABLE StringTable;
	STRING_TABLEPTR lpST;
	PSTR lpStr;
	PDWORD lpLastOffs;

	dir = uio_openDirRelative (dirHandle, path, 0);
	assert(dir != NULL);
	dirList = uio_getDirList (dir, "", pattern, matchType);
	assert(dirList != NULL);
	num_entries = 0;
	length = 0;

	// First, count the amount of space needed
	for (i = 0; i < dirList->numNames; i++)
	{
		struct stat sb;

		if (dirList->names[i][0] == '.')
		{
			dirList->names[i] = NULL;
			continue;
		}
		if (uio_stat (dir, dirList->names[i], &sb) == -1)
		{
			dirList->names[i] = NULL;
			continue;
		}
		if (!S_ISREG (sb.st_mode))
		{
			dirList->names[i] = NULL;
			continue;
		}
		length += strlen (dirList->names[i]) + 1;
		num_entries++;
	}
	uio_closeDir (dir);

	if (num_entries == 0) {
		uio_DirList_free(dirList);
		*pnum_entries = 0;
		return ((DIRENTRY_REF) 0);
	}

	slen = sizeof (STRING_TABLE_DESC)
			+ (num_entries * sizeof (DWORD));
	StringTable = AllocResourceData (slen + length, 0);
	LockStringTable (StringTable, &lpST);
	if (lpST == 0)
	{
		FreeStringTable (StringTable);
		uio_DirList_free(dirList);
		*pnum_entries = 0;
		return ((DIRENTRY_REF) 0);
	}
	lpST->StringCount = num_entries;
	lpLastOffs = &lpST->StringOffsets[0];
	*lpLastOffs = slen;
	lpStr = (PSTR)lpST + slen;

	for (i = 0; i < dirList->numNames; i++)
	{
		int size;
		if (dirList->names[i] == NULL)
			continue;
		size = strlen (dirList->names[i]) + 1;
		memcpy (lpStr, dirList->names[i], size);
		lpLastOffs[1] = lpLastOffs[0] + size;
		lpLastOffs++;
		lpStr += size;
	}
	
	uio_DirList_free(dirList);
	*pnum_entries = num_entries;
	UnlockStringTable (StringTable);
	return ((DIRENTRY_REF) StringTable);
}	


