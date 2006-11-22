/*  alist.h, Copyright (c) 2005 Michael C. Martin */

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

#ifndef _ALIST_H_
#define _ALIST_H_

#include "libs/uio.h"
#include "stringbank.h"

/* Associative List types. */

typedef struct _alist_entry {
	const char *key, *value;
	struct _alist_entry *next;
} alist_entry;

typedef struct _alist_map {
	alist_entry *first;
	stringbank *bank;
} alist;

/* ***** alist_entry operations ***** */

/* Constructor and destructor */
alist_entry *AlistEntry_New (alist *m, const char *key, const char *value);
void AlistEntry_Free (alist_entry *e);

/* ***** alist operations ***** */

/* Standard constructor and destructor */
alist *Alist_New (void);
void Alist_Free (alist *m);

/* Specialized constructors: Parse an alist out of a stream, file, or
   string.  It expects a bunch of key=value statements, one per
   line. */
alist *Alist_New_FromFile (uio_Stream *f);
alist *Alist_New_FromFilename (uio_DirHandle *path, const char *fname);
alist *Alist_New_FromString (char *d);

/* Getting and Setting operations */
alist_entry *Alist_GetEntry (alist *m, const char *key);
const char *Alist_GetString (alist *m, const char *key);
void Alist_PutString (alist *m, const char *key, const char *value);
void Alist_PutAll (alist *dest, alist *src);

/* Dump the alist to the specified stream in a form that could be
   read later by the Alist_New_From* routines.  Dump only keys
   that begin with the prefix; NULL means all keys. */
void Alist_Dump (alist *m, uio_Stream *s, const char *prefix);

#endif   /* _ALIST_H_ */
