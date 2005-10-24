/*  stringbank.c, Copyright (c) 2005 Michael C. Martin */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stringbank.h"

#define CHUNK_SIZE (1024 - sizeof (void *) - sizeof (int))

typedef struct _stringbank_chunk {
	char data[CHUNK_SIZE];
	int len;
	struct _stringbank_chunk *next;
} chunk;

static chunk *bank = NULL;

static void 
add_chunk (void)
{
	chunk *n = malloc (sizeof (chunk));
	n->len = 0;
	n->next = bank;
	bank = n;
}

const char *
StringBank_AddString (const char *str)
{
	int len = strlen (str);
	chunk *x = bank;
	if (len > CHUNK_SIZE)
		return NULL;
	while (x) {
		int remaining = CHUNK_SIZE - x->len;
		if (len < remaining) {
			char *result = x->data + x->len;
			strcpy (result, str);
			x->len += len + 1;
			return result;
		}
		x = x->next;
	}
	/* No room in any currently existing chunk */
	add_chunk ();
	strcpy (bank->data, str);
	bank->len += len + 1;
	return bank->data;
}

const char *
StringBank_AddOrFindString (const char *str)
{
	int len = strlen (str);
	chunk *x = bank;
	if (len > CHUNK_SIZE)
		return NULL;
	while (x) {
		int i = 0;
		while (i < x->len) {
			if (!strcmp (x->data + i, str))
				return x->data + i;
			while (x->data[i]) i++;
			i++;
		}
		x = x->next;
	}
	/* We didn't find it, so add it */
	return StringBank_AddString (str);
}

#ifdef DEBUG

void
StringBank_Dump (FILE *s)
{
	chunk *x = bank;
	while (x) {
		int i = 0;
		while (i < x->len) {
			fprintf (s, "\"%s\"\n", x->data + i);
			while (x->data[i]) i++;
			i++;
		}
		x = x->next;
	}
}

#endif
