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
#include "libs/log.h"
#include "libs/uio/charhashtable.h"
#include "stringbank.h"

/* The CharHashTable owns its keys, but not its values. We will keep the 
   values in a StringBank. */

static CharHashTable_HashTable *map = NULL;
static stringbank *bank = NULL;

static void
check_map_init (void)
{
	if (map == NULL) {
		map = CharHashTable_newHashTable (NULL, NULL, NULL, NULL, 0, 0.85, 0.9);
	}
	if (bank == NULL) {
		bank = StringBank_Create ();
	}
}

void
res_ClearTables (void)
{
	if (map != NULL) {
		CharHashTable_deleteHashTable (map);
		map = NULL;
	}
	if (bank != NULL) {
		StringBank_Free (bank);
		bank = NULL;
	}
}

BOOLEAN
res_Remove (const char *key)
{
	return CharHashTable_remove (map, key);
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
	return StringBank_AddOrFindString (bank, buf);
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
	return CharHashTable_find (map, key);
}

void
res_PutString (const char *key, const char *value)
{
	check_map_init ();
	
	value = StringBank_AddOrFindString (bank, value);
	if (!CharHashTable_add (map, key, value))
	{
		CharHashTable_remove (map, key);
		CharHashTable_add (map, key, value);
	}
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

static void
load_from_string (char *d)
{
	int len, i;

	check_map_init ();
	if (!map) return;

	len = strlen(d);
	i = 0;
	while (i < len) {
		int key_start, key_end, value_start, value_end;
		/* Starting a line: search for non-whitespace */
		while ((i < len) && isspace (d[i])) i++;
		if (i >= len) break;  /* Done parsing! */
		/* If it was a comment, skip to end of comment/file */
		if (d[i] == '#') {
			while ((i < len) && (d[i] != '\n')) i++;
			if (i >= len) break;
			continue; /* Back to keyword search */
		}
		key_start = i;
		/* Find the = on this line */
		while ((i < len) && (d[i] != '=') &&
		       (d[i] != '\n') && (d[i] != '#')) i++;
		if (i >= len) {  /* Bare key at EOF */
			log_add (log_Warning, "Warning: Bare keyword at EOF");
			break;
		}
		/* Comments here mean incomplete line too */
		if (d[i] != '=') {
			log_add (log_Warning, "Warning: Key without value");
			while ((i < len) && (d[i] != '\n')) i++;
			if (i >= len) break;
			continue; /* Back to keyword search */
		}
		/* Key ends at first whitespace before = , or at key_start*/
		key_end = i;
		while ((key_end > key_start) && isspace (d[key_end-1]))
			key_end--;
		
		/* Consume the = */
		i++;
		/* Value starts at first non-whitespace after = on line... */
		while ((i < len) && (d[i] != '#') && (d[i] != '\n') &&
		       isspace (d[i]))
			i++;
		value_start = i;
		/* Until first non-whitespace before terminator */
		while ((i < len) && (d[i] != '#') && (d[i] != '\n'))
			i++;
		value_end = i;
		while ((value_end > value_start) && isspace (d[value_end-1]))
			value_end--;
		/* Skip past EOL or EOF */
		while ((i < len) && (d[i] != '\n'))
			i++;
		i++;

		/* We now have start and end values for key and value.
		   We terminate the strings for both by writing \0s, then
		   make a new map entry. */
		d[key_end] = '\0';
		d[value_end] = '\0';
		res_PutString (d+key_start, d+value_start);
	}
}

static void
load_from_file (uio_Stream *f)
{
	long flen;
	char *data;

	flen = LengthResFile (f);

	data = malloc (flen + 1);
	if (!data) {
		return;
	}

	flen = ReadResFile (data, 1, flen, f);
	data[flen] = '\0';

	load_from_string (data);
	free (data);
}

static void
load_from_filename (uio_DirHandle *path, const char *fname)
{
	uio_Stream *f = res_OpenResFile (path, fname, "rt");
	if (!f) {
		return;
	}
	load_from_file (f);
	res_CloseResFile(f);
}

void
res_LoadFile (uio_Stream *s)
{
	check_map_init ();
	
	load_from_file (s);
}

void
res_LoadFilename (uio_DirHandle *path, const char *fname)
{
	check_map_init ();
	
	load_from_filename (path, fname);
}

void
res_SaveFile (uio_Stream *f, const char *prefix)
{
	CharHashTable_Iterator *i;
	int prefix_len = 0;
	
	if (prefix)
		prefix_len = strlen (prefix);
	
	check_map_init ();

	i = CharHashTable_getIterator (map);
	while (!CharHashTable_iteratorDone (i))
	{
		const char *key = CharHashTable_iteratorKey (i);
		const char *value = (const char *)CharHashTable_iteratorValue (i);
		if (!prefix || !strncmp (prefix, key, prefix_len)) {
			WriteResFile (key, 1, strlen (key), f);
			PutResFileChar(' ', f);
			PutResFileChar('=', f);
			PutResFileChar(' ', f);
			WriteResFile (value, 1, strlen (value), f);
			PutResFileNewline(f);
		}
		i = CharHashTable_iteratorNext (i);
	}
	CharHashTable_freeIterator (i);
}

void
res_SaveFilename (uio_DirHandle *path, const char *fname, const char *prefix)
{
	uio_Stream *f;
	
	check_map_init ();
	f = res_OpenResFile (path, fname, "wb");
	if (f) {
		res_SaveFile (f, prefix);
		res_CloseResFile (f);
	}
}
