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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "physical.h"
#ifdef uio_MEM_DEBUG
#	include "memdebug.h"
#endif
#include "uioport.h"

static inline uio_PRoot *uio_PRoot_alloc(void);
static inline void uio_PRoot_free(uio_PRoot *pRoot);

// NB: ref counter is not incremented
uio_PDirHandle *
uio_PRoot_getRootDirHandle(uio_PRoot *pRoot) {
	return pRoot->rootDir;
}

void
uio_PRoot_deletePRootExtra(uio_PRoot *pRoot) {
	if (pRoot->extra == NULL)
		return;
	assert(pRoot->handler->deletePRootExtra != NULL);
	pRoot->handler->deletePRootExtra(pRoot->extra);
}

// note: sets refMount count to 1
//       set handlerRef count to 0
uio_PRoot *
uio_PRoot_new(uio_PDirHandle *topDirHandle,
		uio_FileSystemHandler *handler, uio_Handle *handle,
		uio_PRootExtra extra, int flags) {
	uio_PRoot *pRoot;
	
	pRoot = uio_PRoot_alloc();
	pRoot->mountRef = 1;
	pRoot->handleRef = 0;
	pRoot->rootDir = topDirHandle;
	pRoot->handler = handler;
	pRoot->handle = handle;
	pRoot->extra = extra;
	pRoot->flags = flags;

	return pRoot;
}

static inline void
uio_PRoot_delete(uio_PRoot *pRoot) {
	assert(pRoot->handler->umount != NULL);
	pRoot->handler->umount(pRoot);
	if (pRoot->handle)
		uio_Handle_unref(pRoot->handle);
	uio_PRoot_deletePRootExtra(pRoot);
	uio_PRoot_free(pRoot);
}

static inline uio_PRoot *
uio_PRoot_alloc(void) {
	uio_PRoot *result = uio_malloc(sizeof (uio_PRoot));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_PRoot, (void *) result);
#endif
	return result;
}

static inline void
uio_PRoot_free(uio_PRoot *pRoot) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_PRoot, (void *) pRoot);
#endif
	uio_free(pRoot);
}

void
uio_PRoot_refHandle(uio_PRoot *pRoot) {
	pRoot->handleRef++;
}

void
uio_PRoot_unrefHandle(uio_PRoot *pRoot) {
	assert(pRoot->handleRef > 0);
	pRoot->handleRef--;
	if (pRoot->handleRef == 0 && pRoot->mountRef == 0)
		uio_PRoot_delete(pRoot);
}

void
uio_PRoot_refMount(uio_PRoot *pRoot) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugRef(uio_PRoot, (void *) pRoot);
#endif
	pRoot->handleRef++;
}

void
uio_PRoot_unrefMount(uio_PRoot *pRoot) {
	assert(pRoot->mountRef > 0);
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugUnref(uio_PRoot, (void *) pRoot);
#endif
	pRoot->mountRef--;
	if (pRoot->mountRef == 0 && pRoot->handleRef == 0)
		uio_PRoot_delete(pRoot);
}


