/*  stringbank.h, Copyright (c) 2005 Michael C. Martin */

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

#ifndef _STRINGBANK_H_
#define _STRINGBANK_H_

#ifdef DEBUG
#include <stdio.h>
#endif

/* Put str into the string bank. */
const char *StringBank_AddString (const char *str);

/* Put str into the string bank if it's not already there.  Much slower. */
const char *StringBank_AddOrFindString (const char *str);

#ifdef DEBUG
/* Print out a list of the contents of the string bank to the named stream. */
void StringBank_Dump (FILE *s);
#endif  /* DEBUG */

#endif  /* _STRINGBANK_H_ */
