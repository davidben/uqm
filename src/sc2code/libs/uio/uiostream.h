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

#ifndef _STREAM_H
#define _STREAM_H


typedef struct uio_Stream uio_Stream;

#include "io.h"


uio_Stream *uio_fopen(uio_DirHandle *dir, const char *path, const char *mode);
int uio_fclose(uio_Stream *stream);
size_t uio_fread(void *buf, size_t size, size_t nmemb, uio_Stream *stream);
char *uio_fgets(char *buf, int size, uio_Stream *stream);
int uio_fgetc(uio_Stream *stream);
#define uio_getc uio_fgetc
int uio_ungetc(int c, uio_Stream *stream);
int uio_fputc(int c, uio_Stream *stream);
#define uio_putc uio_fputc
int uio_fputs(const char *s, uio_Stream *stream);
int uio_fseek(uio_Stream *stream, long offset, int whence);
long uio_ftell(uio_Stream *stream);
size_t uio_fwrite(const void *buf, size_t size, size_t nmemb,
		uio_Stream *stream);
int uio_fflush(uio_Stream *stream);
int uio_feof(uio_Stream *stream);
int uio_ferror(uio_Stream *stream);
void uio_clearerr(uio_Stream *stream);
uio_Handle *uio_streamHandle(uio_Stream *stream);


/* *** Internal definitions follow *** */
#ifdef uio_INTERNAL

#include <sys/types.h>
#include <fcntl.h>
#include "iointrn.h"

/*
 * Layout of buf:
 *
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | | | | | | | |D|D|D|D|D|D|D|D| | | | | | | |X|X|X|X|X|X|X|X|X|X|X|X|X|X|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * ^             ^               ^             ^                           ^
 * +-readStart   +-writeStart    +-bufPtr      +-readEnd            bufEnd-+
 *
 * buf - start of the buffer
 * readStart - start of the segment that was read
 * writeStart - start of the part which is written, or NULL if nothing
 *              written.
 * bufPtr - file pointer; here new reads or writes are done.
 * readEnd - end of the part of the file that is pre-read.
 * bufEnd - end of memory allocated for the buffer.
 * D - dirty, needs to be written to disk
 * X - invalid; reserved but not used memory
 *
 * Invariants:
 * buf <= bufPtr <= bufEnd
 * If writeStart != NULL: buf <= writeStart <= bufPtr <= bufEnd
 * buf <= readStart <= readEnd <= bufEnd
 * Note that writeStart and bufPtr may point past readEnd.
 *
 */
struct uio_Stream {
	char *buf;
	char *bufPtr;
	char *readStart;
			// Not really used atm.
	char *writeStart;
	char *readEnd;
	char *bufEnd;
	off_t seekLow;
			// Low(er) level file pointer. Always points to the position just
			// past the part that's in the buffer as [readStart..readEnd].
	uio_Handle *handle;
	int status;
#define uio_Stream_STATUS_OK 0
#define uio_Stream_STATUS_EOF 1
#define uio_Stream_STATUS_ERROR 2
	int openFlags;
			// Flags used for opening the file.
};


#endif  /* uio_INTERNAL */

#endif  /* _STREAM_H */


