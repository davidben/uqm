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

#ifndef _FILEBLOCK_H
#define _FILEBLOCK_H

#include "io.h"
#include "uioport.h"

#include <sys/types.h>

typedef struct uio_FileBlock {
	uio_Handle *handle;
	int flags;
#define uio_FB_USE_MMAP 1
	off_t offset;
			// Offset to the start of the block in the file.
	size_t blockSize;
	char *buffer;
			// either allocated buffer, or buffer to mmap'ed area.
	size_t bufSize;
} uio_FileBlock;

uio_FileBlock *uio_openFileBlock(uio_Handle *handle);
uio_FileBlock *uio_openFileBlock2(uio_Handle *handle, off_t offset,
		size_t size);
ssize_t uio_accessFileBlock(uio_FileBlock *block, off_t offset, size_t length,
		char **buffer);
int uio_copyFileBlock(uio_FileBlock *block, off_t offset, char *buffer,
		size_t length);
int uio_closeFileBlock(uio_FileBlock *block);

#endif  /* _FILEBLOCK_H */


