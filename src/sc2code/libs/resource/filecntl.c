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

#ifdef WIN32
#include <dos.h>
#include <io.h>
#endif
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "resintrn.h"

FILE *
OpenResFile (PVOID filename, const char *mode)
{
	FILE *fp;
	struct stat sb;

#ifdef WIN32
	if (stat (filename, &sb) == 0 && (sb.st_mode & _S_IFDIR))
#else
	if (stat (filename, &sb) == 0 && S_ISDIR(sb.st_mode))
#endif
		return ((FILE *) ~0);

	fp = fopen (filename, mode);

	return (fp);
}

BOOLEAN
CloseResFile (FILE *fp)
{
	if (fp)
	{
		if (fp != (FILE *)~0)
			fclose (fp);
		return (TRUE);
	}

	return (FALSE);
}

BOOLEAN
DeleteResFile (PVOID filename)
{
	return (remove (filename) == 0);
}

int
ReadResFile (PVOID lpBuf, COUNT size, COUNT count, FILE *fp)
{
	int retval;

	retval = fread (lpBuf, size, count, fp);

	return (retval);
}

int
WriteResFile (PVOID lpBuf, COUNT size, COUNT count, FILE *fp)
{
	int retval;

	retval = fwrite (lpBuf, size, count, fp);

	return (retval);
}

int
GetResFileChar (FILE *fp)
{
	int retval;

	retval = getc (fp);

	return (retval);
}

int
PutResFileChar (char ch, FILE *fp)
{
	int retval;

	retval = putc (ch, fp);

	return (retval);
}

long
SeekResFile (FILE *fp, long offset, int whence)
{
	long retval;

	retval = fseek (fp, offset, whence);

	return (retval);
}

long
TellResFile (FILE *fp)
{
	long retval;

	retval = ftell (fp);

	return (retval);
}

long
LengthResFile (FILE *fp)
{
if (fp == (FILE *)~0)
	return (1);
#ifdef WIN32
	return filelength (fileno (fp));
#else
	{
		struct stat sb;
		if (fstat(fileno(fp), &sb) == -1)
			return 1;
		return sb.st_size;
	}
#endif
}

