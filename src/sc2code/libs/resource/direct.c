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

/* This file contains two versions of LoadDirEntryTable; one for Windows, and
 * one for other systems (POSIX).
 * It might be better to split this up in seperate files eventually.
 * - SvdB
 */
#ifdef WIN32
#include <io.h>
#include <ctype.h>
#include "libs/strings/strintrn.h"

DIRENTRY_REF
LoadDirEntryTable (PSTR pattern, PCOUNT pnum_entries)
{
	COUNT num_entries, length;
	STRING_TABLE StringTable;
	STRING_TABLEPTR lpST;
	PDWORD lpLastOffs;

	num_entries = 0;
	StringTable = 0;
	do
	{
		COUNT slen;
		long handle;
		struct _finddata_t f;

		if (num_entries == 0)
			length = 0;
		else
		{
			slen = sizeof (STRING_TABLE_DESC)
					+ (num_entries * sizeof (DWORD));
			StringTable = AllocResourceData (slen + length, 0);
			LockStringTable (StringTable, &lpST);
			if (lpST == 0)
			{
				FreeStringTable (StringTable);
				StringTable = 0;

				break;
			}

			lpST->StringCount = num_entries;
			lpLastOffs = &lpST->StringOffsets[0];
			*lpLastOffs = slen;

			num_entries = 0;
			length = slen;
		}

		handle = _findfirst (pattern, &f);
		if (handle != -1)
		{
			do
			{
				if (f.attrib & (_A_HIDDEN | _A_SUBDIR | _A_SYSTEM))
					continue;

				slen = strlen (f.name) + 1;
				length += slen;

				if ((MEM_HANDLE)StringTable)
				{
					PSTR lpStr;
					PDWORD lpLo, lpHi;

					lpLo = &lpST->StringOffsets[0];
					lpHi = lpLastOffs - 1;
					while (lpLo <= lpHi)
					{
						char c1, c2;
						PSTR pStr;
						COUNT LocLen;
						PDWORD lpMid;

						lpMid = lpLo + ((lpHi - lpLo) >> 1);

						LocLen = lpMid[1] - lpMid[0];
						if (LocLen > slen)
							LocLen = slen;

						lpStr = (PSTR)lpST + lpMid[0];
						pStr = f.name;
						while (LocLen--
								&& (c1 = toupper (*lpStr++))
										== (c2 = toupper (*pStr++)))
							;

						if (c1 <= c2)
							lpLo = lpMid + 1;
						else
							lpHi = lpMid - 1;
					}

					lpStr = (PSTR)lpST + lpLo[0];
					memmove (lpStr + slen, lpStr, lpLastOffs[0] - lpLo[0]);
					strcpy (lpStr, f.name);

					for (lpHi = lpLastOffs++; lpHi >= lpLo; --lpHi)
						lpHi[1] = lpHi[0] + slen;
				}

				++num_entries;
			} while (_findnext (handle, &f) == 0);
			_findclose (handle);
		}
	} while (num_entries && (MEM_HANDLE)StringTable == 0);

	if ((MEM_HANDLE)StringTable == 0)
		*pnum_entries = 0;
	else
	{
		*pnum_entries = num_entries;

		UnlockStringTable (StringTable);
	}

	return ((DIRENTRY_REF)StringTable);
}

#else  /* ! defined(WIN32) */

#include "libs/strings/strintrn.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#include <ctype.h>

DIRENTRY_REF
LoadDirEntryTable (PSTR pattern, PCOUNT pnum_entries)
{
	COUNT num_entries, length;
	STRING_TABLE StringTable;
	STRING_TABLEPTR lpST;
	PDWORD lpLastOffs;
	PSTR slash;     // Pointer inside pattern to the last /
	SBYTE path[PATH_MAX + 1];  // buffer for a filename with path
	PSTR file;       // Pointer inside path to the filename
	size_t pathlen;  // length of path, excluding last / and filename

	slash = (PSTR) strrchr ((const char *) pattern, '/');
	if (slash == NULL)
	{
		pathlen = 1;
		path[0] = '.';
	}
	else
	{
		pathlen = slash - pattern;
		memcpy (path, pattern, pathlen);
		pattern = slash + 1;
	}
	file = path + pathlen + 1;
	
	num_entries = 0;
	StringTable = 0;
	do
	{
		COUNT slen;
		DIR *handle;

		if (num_entries == 0)
			length = 0;
		else
		{
			slen = sizeof (STRING_TABLE_DESC)
					+ (num_entries * sizeof (DWORD));
			StringTable = AllocResourceData (slen + length, 0);
			LockStringTable (StringTable, &lpST);
			if (lpST == 0)
			{
				FreeStringTable (StringTable);
				StringTable = 0;

				break;
			}

			lpST->StringCount = num_entries;
			lpLastOffs = &lpST->StringOffsets[0];
			*lpLastOffs = slen;

			num_entries = 0;
			length = slen;
		}

		
		path[pathlen] = '\0';  // strip any file part
		handle = opendir((const char *) path);
		if (handle != NULL)
		{
			path[pathlen] = '/';
					// change the 0 char to a slash; a filename will be
					// attached here.
			while (1)
			{
				struct dirent *de;
				struct stat sb;

				de = readdir(handle);
				if (de == NULL)
					break;
				if (de->d_name[0] == '.')
					continue;
				strcpy (file, de->d_name);
						// attach the filename to path
				if (stat(path, &sb) == -1)
					continue;
				if (!S_ISREG(sb.st_mode))
					continue;
				if (fnmatch(pattern, de->d_name, 0) != 0)
					continue;

				slen = strlen (de->d_name) + 1;
				length += slen;

				if ((MEM_HANDLE)StringTable)
				{
					PSTR lpStr;
					PDWORD lpLo, lpHi;

					lpLo = &lpST->StringOffsets[0];
					lpHi = lpLastOffs - 1;
					while (lpLo <= lpHi)
					{
						char c1, c2;
						PSTR pStr;
						COUNT LocLen;
						PDWORD lpMid;

						lpMid = lpLo + ((lpHi - lpLo) >> 1);

						LocLen = lpMid[1] - lpMid[0];
						if (LocLen > slen)
							LocLen = slen;

						lpStr = (PSTR)lpST + lpMid[0];
						pStr = de->d_name;
						while (LocLen--
								&& (c1 = toupper (*lpStr++))
										== (c2 = toupper (*pStr++)))
							;

						if (c1 <= c2)
							lpLo = lpMid + 1;
						else
							lpHi = lpMid - 1;
					}

					lpStr = (PSTR)lpST + lpLo[0];
					memmove (lpStr + slen, lpStr, lpLastOffs[0] - lpLo[0]);
					strcpy (lpStr, de->d_name);

					for (lpHi = lpLastOffs++; lpHi >= lpLo; --lpHi)
						lpHi[1] = lpHi[0] + slen;
				}

				++num_entries;
			}
			closedir(handle);
		}
	} while (num_entries && (MEM_HANDLE)StringTable == 0);

	if ((MEM_HANDLE)StringTable == 0)
		*pnum_entries = 0;
	else
	{
		*pnum_entries = num_entries;

		UnlockStringTable (StringTable);
	}

	return ((DIRENTRY_REF)StringTable);
}

#endif
