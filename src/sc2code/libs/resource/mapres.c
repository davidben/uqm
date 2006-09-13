/*  mapres.h, Copyright (c) 2005 Michael C. Martin */

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope thta it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  Se the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libs/reslib.h"
#include "alist.h"
#include "stringbank.h"

static alist *map = NULL;

static void
check_map_init (void)
{
	if (map == NULL) {
		map = Alist_New ();
	}
}

void
res_ClearTables (void)
{
	if (map != NULL) {
		Alist_Free (map);
		map = NULL;
	}
}

/* Type conversion routines. */
static const char *
bool2str (BOOLEAN b)
{
	return b ? "yes" : "no";
}

static const char *
int2str (int i) {
	char buf[20];
	sprintf (buf, "%d", i);
	return StringBank_AddOrFindString (map->bank, buf);
}

static int
str2int (const char *s) {
	return atoi(s);
}

static BOOLEAN
str2bool (const char *s) {
	if (!stricmp (s, "yes") ||
	    !stricmp (s, "true") ||
	    !stricmp (s, "1") )
		return TRUE;
	return FALSE;
}

void
res_LoadFile (uio_Stream *s)
{
	alist *d;
	check_map_init ();
	
	d = Alist_New_FromFile (s);
	if (d) {
		Alist_PutAll (map, d);
		Alist_Free (d);
	}
}

void
res_LoadFilename (uio_DirHandle *path, const char *fname)
{
	alist *d;
	check_map_init ();
	
	d = Alist_New_FromFilename (path, fname);
	if (d) {
		Alist_PutAll (map, d);
		Alist_Free (d);
	}
}

void
res_SaveFile (uio_Stream *f, const char *root)
{
	check_map_init ();
	Alist_Dump (map, f, root);
}

void
res_SaveFilename (uio_DirHandle *path, const char *fname, const char *root)
{
	uio_Stream *f;
	
	check_map_init ();
	f = res_OpenResFile (path, fname, "wb");
	if (f) {
		res_SaveFile (f, root);
		res_CloseResFile (f);
	}
}

BOOLEAN
res_IsBoolean (const char *key)
{
	const char *d;
	check_map_init ();

	d = res_GetString (key);
	if (!d) return 0;
		
	return !stricmp (d, "yes") ||
	       !stricmp (d, "no") ||
	       !stricmp (d, "true") ||
	       !stricmp (d, "false") ||
	       !stricmp (d, "0") ||
	       !stricmp (d, "1") ||
	       !stricmp (d, "");
}

BOOLEAN
res_IsInteger (const char *key)
{
	const char *d;
	check_map_init ();

	d = res_GetString (key);
	while (*d) {
		if (!isdigit (*d))
			return 0;
		d++;
	}
	return 1;
}

const char *
res_GetString (const char *key)
{
	check_map_init ();
	return Alist_GetString (map, key);
}

void
res_PutString (const char *key, const char *value)
{
	check_map_init ();
	Alist_PutString (map, key, value);
}

int
res_GetInteger (const char *key)
{
	check_map_init ();
	return str2int (res_GetString (key));
}

void
res_PutInteger (const char *key, int value)
{
	check_map_init ();
	res_PutString (key, int2str(value));
}

BOOLEAN
res_GetBoolean (const char *key)
{
	check_map_init ();
	return str2bool (res_GetString (key));
}

void
res_PutBoolean (const char *key, BOOLEAN value)
{
	check_map_init ();
	res_PutString (key, bool2str(value));
}

BOOLEAN
res_HasKey (const char *key)
{
	check_map_init ();
	return (res_GetString (key) != NULL);
}
