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

#ifndef _ERRLIB_H
#define _ERRLIB_H

#include "compiler.h"
#include "vargs.h"

typedef SWORD ERR_ID;
typedef DWORD ERR_NO;

typedef BOOLEAN ERR_BOOL;
#define ERR_FAILURE FALSE
#define ERR_SUCCESS TRUE

typedef enum
{
	ERR_IN_MEMORY,
	ERR_IN_FILE
} ERR_LOC;

extern ERR_ID err_register (PSTR module_name, ERR_LOC err_loc, PSTR
		err_string_path, COUNT num_errors);
extern ERR_BOOL err_unregister (ERR_ID err_id);
extern ERR_BOOL err_push (ERR_ID err_id, PSTR func_name, ERR_NO err_no,
		vararg_dcl alist, ...);
extern ERR_BOOL err_clear (void);
extern ERR_BOOL err_report (ERR_BOOL (*err_func) (COUNT top_stack, PSTR
		mname, PSTR fname, PSTR pStr));

#endif /* _ERRLIB_H */

