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

#ifndef _PATHS_H
#define PATHS_H

#include "types.h"
#include "uioport.h"

void getFirstPathComponent(const char *dir, const char *dirEnd,
		const char **startComp, const char **endComp);
void getFirstPath0Component(const char *dir, const char **startComp,
		const char **endComp);
void getNextPathComponent(const char *dirEnd,
		const char **startComp, const char **endComp);
void getNextPath0Component(const char **startComp, const char **endComp);
void getLastPathComponent(const char *dir, const char *dirEnd,
		const char **startComp, const char **endComp);
void getLastPath0Component(const char *dir, const char **startComp,
		const char **endComp);
void getPreviousPathComponent(const char *dir, const char **startComp,
		const char **endComp);
#define getPreviousPath0Component getPreviousPathComponent
char *joinPaths(const char *first, const char *second);

uio_bool validPathName(const char *path, size_t len);

#endif  /* PATHS_H */

