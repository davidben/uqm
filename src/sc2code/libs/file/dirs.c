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

// Contains code handling directories

#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "port.h"
#include "config.h"
#include "filintrn.h"
#include "compiler.h"

#ifdef WIN32
#	include <direct.h>

// this would lead to conflicts
//#	include <shlobj.h>
typedef void *HWND;
typedef void *HANDLE;
typedef char *LPTSTR;
typedef long HRESULT;
#define SUCCEEDED(Status) ((HRESULT) (Status) >= 0)
HRESULT __stdcall SHGetFolderPathA (HWND hwndOwner, int nFolder,
		HANDLE hToken, DWORD dwFlags, LPTSTR pszPath);
#define SHGetFolderPath SHGetFolderPathA
#define CSIDL_APPDATA  0x001a  // <user name>\Application Data
#define CSIDL_FLAG_CREATE  0x8000
		// combine with CSIDL_ value to force folder creation in
		// SHGetFolderPath()
#endif

int
createDirectory(const char *dir, int mode)
{
#ifdef WIN32
	return mkdir (dir);
#else
	return mkdir (dir, (mode_t) mode);
#endif
}

// make all components of the path if they don't exist already
// returns 0 on success, -1 on failure.
// on failure, some parts may still have been created.
int
mkdirhier (const char *path)
{
	char *buf;              // buffer
	char *ptr;              // end of the string in buf
	const char *pathstart;  // start of a component of path
	const char *pathend;    // first char past the end of a component of path
	size_t len;
	struct stat statbuf;
	
	len = strlen (path);
	buf = alloca (len + 2);  // one extra for possibly added '/'

	ptr = buf;
	pathstart = path;

	if (*pathstart == '/') {
		*ptr = '/';
		ptr++;
		pathstart++;
	}

	if (*pathstart == '\0') {
		// path exists completely, nothing more to do
		return 0;
	}

	// walk through the path as long as the components exist
	while (1)
	{
		pathend = strchr (pathstart, '/');
		if (pathend == NULL)
			pathend = path + len;
		memcpy(ptr, pathstart, pathend - pathstart);
		ptr += pathend - pathstart;
		*ptr = '\0';
		
		if (stat (buf, &statbuf) == -1)
		{
			if (errno != ENOENT)
			{
				fprintf (stderr, "Can't stat %s: %s\n", buf,
						strerror (errno));
				return -1;
			}
			break;
		}
		
		if (*pathend == '\0')
			return 0;

		*ptr = '/';
		ptr++;
		pathstart = pathend + 1;
		while (*pathstart == '/')
			pathstart++;
		// pathstart is the next non-slash character

		if (*pathstart == '\0')
			return 0;
	}
	
	// create all components left
	while (1)
	{
		if (createDirectory (buf, 0777) == -1)
		{
			fprintf (stderr, "Error: Can't create %s: %s\n", buf,
					strerror (errno));
			return -1;
		}

		if (*pathend == '\0')
			break;

		*ptr = '/';
		ptr++;
		pathstart = pathend + 1;
		while (*pathstart == '/')
			pathstart++;
		// pathstart is the next non-slash character

		if (*pathstart == '\0')
			break;

		pathend = strchr (pathstart, '/');
		if (pathend == NULL)
			pathend = path + len;
		
		memcpy (ptr, pathstart, pathend - pathstart);
		ptr += pathend - pathstart;
		*ptr = '\0';
	}
	return 0;
}

// Get the user's home dir
// returns a pointer to a static buffer
const char *
getHomeDir (void)
{
	return getenv ("HOME");
}

// Expand ~/ in a path, and replaces \ by / on Windows.
// Environment variable parsing could be added here if we need it.
int
expandPath (char *dest, size_t len, const char *src)
{
	const char *destend;

#define CHECKLEN(n) \
		if (dest + (n) > destend) \
		{ \
			errno = ENAMETOOLONG; \
			return -1; \
		} \
		else \
			(void) 0

	destend = dest + len;
	if (src[0] == '~')
	{
		const char *home;
		size_t homelen;
		
		if (src[1] != '/')
		{
			errno = EINVAL;
			return -1;
		}

		home = getHomeDir ();
		if (home == NULL)
		{
			errno = ENOENT;
			return -1;
		}
		homelen = strlen (home);
		
		CHECKLEN (homelen);
		memcpy (dest, home, homelen);
		dest += homelen;
		src++;  /* skip the ~ */
	}

	while (*src != '\0')
	{
		switch (*src)
		{
#ifdef WIN32
			case '\\':
				/* Replaces backslashes in path by slashes in Windows. */
				CHECKLEN(1);
				*(dest++) = '/';
				src++;
				break;
#endif
#ifdef WIN32
			case '%':
			{
				/* Environment variable substitution in Windows */
				const char *end;     // end of env var name in src
				const char *envVar;
				char *envName;
				size_t envNameLen, envVarLen;
				
				src++;
				end = strchr (src, '%');
				if (end == NULL)
				{
					errno = EINVAL;
					return -1;
				}
				
				envNameLen = end - src;
				envName = malloc (envNameLen + 1);
				memcpy (envName, src, envNameLen + 1);
				envName[envNameLen] = '\0';
				envVar = getenv (envName);
				free (envName);

				if (envVar == NULL)
				{
#if 1
					if (strncmp (src, "APPDATA", envNameLen) == 0)
					{
						// fallback for when the APPDATA env var is not set
						if (getUserDataDir (dest, destend - dest) == -1)
							return -1;
						dest += strlen (dest);
						src = end + 1;
						break;
					}
#endif
					// Substitute an empty string
					src = end + 1;
					break;
				}

				envVarLen = strlen (envVar);
				CHECKLEN (envVarLen);
				strcpy (dest, envVar);
				dest += envVarLen;
				src = end + 1;
				break;
			}
#endif
#if 0
			case '$':
				/* Environment variable substitution for Unix */
				// not implemented
				break;
#endif
			default:
				CHECKLEN(1);
				*(dest++) = *(src++);
				break;
		}
	}
	*dest = '\0';
	return 0;
}

// returns 0 on success, and fills dir with the path to the dir where there
// user data is stored.
// return -1 on error, and sets errno.
int
getUserDataDir (char *dir, size_t len)
{
#ifdef WIN32
	char buf[PATH_MAX];
	
	if (!SUCCEEDED (SHGetFolderPath(NULL,
			CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, buf)))
	{
		errno = ENOENT;
		return -1;
	}
	
	if (strlen (buf) >= len)
	{
		errno = ENAMETOOLONG;
		return -1;
	}
	
	strcpy (dir, buf);
	return 0;
#else
	const char *homeDir;
	homeDir = getHomeDir();
	
	if (strlen (homeDir) >= len)
	{
		errno = ENAMETOOLONG;
		return -1;
	}
	
	strcpy (dir, homeDir);
	return 0;
#endif	
}

BOOLEAN
fileExists (const char *name)
{
	return access (name, F_OK) == 0;
}

