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
/*
 * This file contains definitions that might be included in one system, but
 * omited in another.
 *
 * Created by Serge van den Boom
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "port.h"

#ifndef HAVE_STRUPR
char *
strupr (char *str)
{
	char *ptr;
	
	ptr = str;
	while (*ptr)
	{
		*ptr = toupper (*ptr);
		ptr++;
	}
	return str;
}
#endif

#ifndef HAVE_SETENV
int
setenv (const char *name, const char *value, int overwrite)
{
	char *string, *ptr;
	size_t nameLen, valueLen;

	if (!overwrite)
	{
		char *old;

		old = getenv (name);
		if (old != NULL)
			return 0;
	}

	nameLen = strlen (name);	
	valueLen = strlen (value);

	string = malloc (nameLen + valueLen + 2);
			// "NAME=VALUE\0"
			// putenv() does NOT make a copy, but uses the string passed.

	ptr = string;

	strcpy (string, name);
	ptr += nameLen;

	*ptr = '=';
	ptr++;
	
	strcpy (ptr, value);
	
	return putenv (string);
}
#endif


