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

#ifndef _VARARGS_H
#define _VARARGS_H

typedef char *vararg_list;
typedef char *vararg_dcl;
#define vararg_start(list,alist) (list) = (vararg_list)&(alist)
#define vararg_end(list)
#define vararg_val(list,type) \
		(sizeof (type) >= sizeof (int) ? \
		(((type *)((list) += sizeof (type)))[-1]) : \
		((type)((int *)((list) += sizeof (int)))[-1]))
#define vararg_farval(list,type) \
		(sizeof (type) >= sizeof (int) ? \
		(((type *)((list) += sizeof (type)))[-1]) : \
		(((int *)((list) += sizeof (int)))[-1]))
#define begin_arg_stack(name) char name[0
#define add_to_arg_stack(type) + (sizeof (type) > sizeof (int) ? \
												  sizeof (type) : sizeof (int))
#define end_arg_stack        ];
#define arg_stack_push(list,type,val) \
		(sizeof (type) >= sizeof (int) ? \
		(((type *)((list) += sizeof (type)))[-1] = (val)) : \
		(((int *)((list) += sizeof (int)))[-1] = (int)(val)))


#endif /* _VARARGS_H */

