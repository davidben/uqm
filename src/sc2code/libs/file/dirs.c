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
#include <assert.h>
#include "port.h"
#include "config.h"
#include "filintrn.h"
#include "compiler.h"
#include "misc.h"

#ifdef WIN32
#	include <direct.h>
#	include <ctype.h>
#else
#	include <pwd.h>
#endif

/* Try to find a suitable value for %APPDATA% if it isn't defined on
 * Windows.
 */
#define APPDATA_FALLBACK


static char *expandPathAbsolute (char *dest, size_t destLen,
		const char *src, int what);


int
createDirectory(const char *dir, int mode)
{
	return MKDIR(dir, mode);
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

#ifdef WIN32
	// driveletter + semicolon on Windows.
	if (isalpha(pathstart[0]) && pathstart[1] == ':')
	{
		*(ptr++) = *(pathstart++);
		*(ptr++) = *(pathstart++);
	}
#endif

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
// returns a pointer to a static buffer from either getenv() or getpwuid().
const char *
getHomeDir (void)
{
#ifdef WIN32
	return getenv ("HOME");
#else
	const char *home;
	struct passwd *pw;

	home = getenv ("HOME");
	if (home != NULL)
		return home;

	pw = getpwuid (getuid ());
	if (pw == NULL)
		return NULL;
	// NB: pw points to a static buffer.

	return pw->pw_name;
#endif
}

// Expand ~/ in a path, and replaces \ by / on Windows.
// Environment variable parsing could be added here if we need it.
// Returns 0 on success.
// Returns -1 on failure, setting errno.
int
expandPath (char *dest, size_t len, const char *src, int what)
{
	char *destptr, *destend;
	char *buf, *bufptr, *bufend;
	const char *srcend;

#define CHECKLEN(bufname, n) \
		if (bufname##ptr + (n) >= bufname##end) \
		{ \
			errno = ENAMETOOLONG; \
			return -1; \
		} \
		else \
			(void) 0

	destptr = dest;
	destend = dest + len;

	if (what & EP_ENVVARS)
	{
		buf = alloca (len);
		bufptr = buf;
		bufend = buf + len;
		while (*src != '\0')
		{
			switch (*src)
			{
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
					envName = HMalloc (envNameLen + 1);
					memcpy (envName, src, envNameLen + 1);
					envName[envNameLen] = '\0';
					envVar = getenv (envName);
					HFree (envName);

					if (envVar == NULL)
					{
#ifdef APPDATA_FALLBACK
						if (strncmp (src, "APPDATA", envNameLen) != 0)
						{
							// Substitute an empty string
							src = end + 1;
							break;
						}

						// fallback for when the APPDATA env var is not set
						// Using SHGetFolderPath or SHGetSpecialFolderPath
						// is problematic (not everywhere available).
						fprintf(stderr, "Warning: %%APPDATA%% is not set. "
								"Falling back to \"%%USERPROFILE%%\\Application "
								"Data\"\n");
						envVar = getenv ("USERPROFILE");
						if (envVar != NULL)
						{
#define APPDATA_STRING "\\Application Data"
							envVarLen = strlen (envVar);
							CHECKLEN (buf,
									envVarLen + sizeof (APPDATA_STRING) - 1);
							strcpy (bufptr, envVar);
							bufptr += envVarLen;
							strcpy (bufptr, APPDATA_STRING);
							bufptr += sizeof (APPDATA_STRING) - 1;
							src = end + 1;
							break;
						}
						
						// fallback to "../userdata"
						fprintf(stderr, "Warning: %%USERPROFILE%% is not set. "
								"Falling back to \"..\\userdata\" for %%APPDATA%%"
								"\n");
#define APPDATA_FALLBACK_STRING "..\\userdata"
						CHECKLEN (buf, sizeof (APPDATA_FALLBACK_STRING) - 1);
						strcpy (bufptr, APPDATA_FALLBACK_STRING);
						bufptr += sizeof (APPDATA_FALLBACK_STRING) - 1;
						src = end + 1;
						break;

#else  /* !defined (APPDATA_FALLBACK) */
						// Substitute an empty string
						src = end + 1;
						break;
#endif  /* APPDATA_FALLBACK */
					}

					envVarLen = strlen (envVar);
					CHECKLEN (buf, envVarLen);
					strcpy (bufptr, envVar);
					bufptr += envVarLen;
					src = end + 1;
					break;
				}
#endif
#if 0
				case '$':
					/* Environment variable substitution for Unix */
					// not implemented
					// Should accept both $BLA_123 and ${BLA}
					break;
#endif
				default:
					CHECKLEN(buf, 1);
					*(bufptr++) = *(src++);
					break;
			}  // switch
		}  // while
		*bufptr = '\0';
		src = buf;
		srcend = bufptr;
	}  // if (what & EP_ENVVARS)
	else
		srcend = src + strlen (src);

	
	if (what & EP_HOME)
	{
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
		
			if (what & EP_ABSOLUTE) {
				destptr = expandPathAbsolute (dest, destend - dest,
						home, what);
				if (destptr == NULL)
				{
					// errno is set
					return -1;
				}
				what &= ~EP_ABSOLUTE;
						// The part after the '~' should not be seen
						// as absolute.
			}

			CHECKLEN (dest, homelen);
			memcpy (destptr, home, homelen);
			destptr += homelen;
			src++;  /* skip the ~ */
		}
	}
	
	if (what & EP_ABSOLUTE)
	{
		destptr = expandPathAbsolute (destptr, destend - destptr, src,
				what);
		if (destptr == NULL)
		{
			// errno is set
			return -1;
		}
	}

	CHECKLEN (dest, srcend - src);
	memcpy (destptr, src, srcend - src + 1);
			// The +1 is for the '\0'. It is already taken into account by
			// CHECKLEN.
	
	if (what & EP_SLASHES)
	{
		/* Replacing backslashes in path by slashes. */
		destptr = dest;
		while (*destptr != '\0')
		{
			if (*destptr == '\\')
				*destptr = '/';
			destptr++;
		}
	}
	return 0;
}

// helper for expandPath, expanding an absolute path
// returns a pointer to the end of the filled in part of dest.
static char *
expandPathAbsolute (char *dest, size_t destLen, const char *src, int what)
{
	if (src[0] == '/' || ((what & EP_SLASHES) && src[0] == '\\')
#ifdef WIN32
			|| (isalpha(src[0]) && (src[1] == ':'))
#endif
			) {
		// Path is already absolute; nothing to do
		return dest;
	}

	// Path is not already absolute; we've got work to do.
	if (getcwd (dest, destLen) == NULL)
	{
		// errno is set
		return NULL;
	}

	{
		size_t tempLen;
		tempLen = strlen (dest);
		assert (tempLen > 0);
		dest += tempLen;
		destLen -= tempLen;
	}
	if (dest[-1] != '/'
#ifdef WIN32
			&& dest[-1] != '\\'
#endif
			)
	{
		// Need to add a slash.
		// There's always space, as we overwrite the '\0' that getcwd()
		// always returns.
		dest[0] = '/';
		dest++;
		destLen--;
	}
	return dest;
}


