/*
 * Copyright (C) 2003  Serge van den Boom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * Nota bene: later versions of the GNU General Public License do not apply
 * to this program.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _uio_TYPES_H
#define _uio_TYPES_H

typedef enum
{
	false = 0,
	true
} uio_bool;

typedef unsigned char  uio_uint8;
typedef   signed char  uio_sint8;
typedef unsigned short uio_uint16;
typedef   signed short uio_sint16;
typedef unsigned int   uio_uint32;
typedef   signed int   uio_sint32;

typedef unsigned long  uio_uintptr;
		// Needs to be adapted for 64 bits systems

#endif  /* _uio_TYPES_H */


