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

#include "uioport.h"
#include "iointrn.h"
#include "uiostream.h"

#include <errno.h>
#include <stdio.h>

#include "uioutils.h"
#ifdef uio_MEM_DEBUG
#	include "memdebug.h"
#endif

#define uio_Stream_BLOCK_SIZE 1024
		// The alignment of file reads as well as the buffer size.
#define uio_Stream_BLOCK_PAGE_START(offset) \
		((offset) & ~(uio_Stream_BLOCK_SIZE - 1))

static inline uio_Stream *uio_Stream_new(uio_Handle *handle, int openFlags);
static inline void uio_Stream_delete(uio_Stream *stream);
static inline uio_Stream *uio_Stream_alloc(void);
static inline void uio_Stream_free(uio_Stream *stream);
static int uio_Stream_fillReadBuffer(uio_Stream *stream);
static int uio_Stream_alignReadBuffer(uio_Stream *stream);
static int uio_Stream_flushWriteBuffer(uio_Stream *stream);


uio_Stream *
uio_fopen(uio_DirHandle *dir, const char *path, const char *mode) {
	int openFlags;
	uio_Handle *handle;
	uio_Stream *stream;
	
	switch (*mode) {
		case 'r':
			openFlags = O_RDONLY;
			break;
		case 'w':
			openFlags = O_WRONLY | O_CREAT | O_TRUNC;
			break;
		case 'a':
			openFlags = O_WRONLY| O_CREAT | O_APPEND;
		default:
			errno = EINVAL;
			fprintf(stderr, "Invalid mode string in call to uio_fopen().\n");
			return NULL;
	}
	mode++;

	// 'b' may either be the second or the third character.
	if (*mode == 'b') {
		mode++;
#ifdef WIN32
		openFlags |= O_BINARY;
#endif
		if (*mode == '+') {
			mode++;
			openFlags = (openFlags & ~O_ACCMODE) | O_RDWR;
		}
	} else
	if (*mode == '+') {
		mode++;
		openFlags = (openFlags & ~O_ACCMODE) | O_RDWR;
	}

	// Any characters in the mode string that might follow are ignored.
	
	handle = uio_open(dir, path, openFlags, S_IRUSR | S_IWUSR |
			S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (handle == NULL) {
		// errno is set
		return NULL;
	}

	stream = uio_Stream_new(handle, openFlags);
	return stream;
}

int
uio_fclose(uio_Stream *stream) {
	if (stream->writeStart != NULL)
		uio_Stream_flushWriteBuffer(stream);
	uio_close(stream->handle);
	uio_Stream_delete(stream);
	return 0;
}

// If less than nmembs could be read, or an error occurs, the file pointer
// is undefined. clearerr() followed by fseek() need to be called before
// attempting to read or write again.
// I don't have the C standard myself, but I suspect this is the
// official behaviour for fread() and fwrite().
size_t
uio_fread(void *buf, size_t size, size_t nmemb, uio_Stream *stream) {
	size_t bytesToRead;
	size_t bytesRead;

	bytesToRead = size * nmemb;
	bytesRead = 0;

	assert((stream->openFlags & O_ACCMODE) != O_WRONLY);
	if (stream->writeStart != NULL) {
		if (uio_Stream_flushWriteBuffer(stream) == -1) {
			// errno is set
			return -1;
		}
	}
	if (stream->readEnd - stream->bufPtr > 0) {
		// First use what's in the buffer.
		size_t numRead;

		numRead = minu(stream->readEnd - stream->bufPtr, bytesToRead);
		memcpy(buf, stream->bufPtr, numRead);
		(char *) buf += numRead;
		stream->bufPtr += numRead;
		bytesToRead -= numRead;
		bytesRead += numRead;
	}
	if (bytesToRead == 0) {
		// Done already
		return nmemb;
	}

	{
		ssize_t numRead;
		numRead = uio_read(stream->handle, buf, bytesToRead);
		if (numRead == -1) {
			stream->status = uio_Stream_STATUS_ERROR;
			goto out;
		}
		bytesRead += numRead;
		stream->seekLow += numRead;
		if ((size_t) numRead < bytesToRead) {
			// End of file
			stream->status = uio_Stream_STATUS_EOF;
			goto out;
		}
		uio_Stream_alignReadBuffer(stream);
	}
	
out:
	if (bytesToRead == 0)
		return nmemb;
	return bytesRead / size;
}

char *
uio_fgets(char *s, int size, uio_Stream *stream) {
	int orgSize;
	char *buf;

	assert((stream->openFlags & O_ACCMODE) != O_WRONLY);

	if (stream->writeStart != NULL) {
		if (uio_Stream_flushWriteBuffer(stream) == -1) {
			// errno is set
			return NULL;
		}
	}
	
	size--;
	orgSize = size;
	buf = s;
	while (size > 0) {
		size_t maxRead;
		const char *newLinePos;

		// Fill buffer if empty.
		if (stream->bufPtr >= stream->readEnd) {
			if (uio_Stream_fillReadBuffer(stream) == -1) {
				// errno is set
				stream->status = uio_Stream_STATUS_ERROR;
				return NULL;
			}
			if (stream->bufPtr == stream->readEnd) {
				// End-of-file
				stream->status = uio_Stream_STATUS_EOF;
				if (size == orgSize) {
					// Nothing was read.
					return NULL;
				}
				break;
			}
		}
		assert (stream->bufPtr < stream->readEnd);
		
		// Search in buffer
		maxRead = minu(stream->readEnd - stream->bufPtr, size);
		newLinePos = memchr(stream->bufPtr, '\n', maxRead);
		if (newLinePos != NULL) {
			// Newline found.
			maxRead = newLinePos + 1 - stream->bufPtr;
			memcpy(buf, stream->bufPtr, maxRead);
			stream->bufPtr += maxRead;
			buf[maxRead] = '\0';
			return buf;
		}
		// No newline present.
		memcpy(buf, stream->bufPtr, maxRead);
		stream->bufPtr += maxRead;
		buf += maxRead;
		size -= maxRead;
	}

	*buf = '\0';
	return s;	
}

int
uio_fgetc(uio_Stream *stream) {
	int result;

	assert((stream->openFlags & O_ACCMODE) != O_WRONLY);

	if (stream->writeStart != NULL) {
		if (uio_Stream_flushWriteBuffer(stream) == -1) {
			// errno is set
			return -1;
		}
	}

	if (stream->bufPtr >= stream->readEnd) {
		if (uio_Stream_fillReadBuffer(stream) == -1) {
			stream->status = uio_Stream_STATUS_ERROR;
			return (int) EOF;
		}
		if (stream->bufPtr == stream->readEnd) {
			// End-of-file
			stream->status = uio_Stream_STATUS_EOF;
			return (int) EOF;
		}
	}
	assert(stream->bufPtr < stream->readEnd);

	result = (int) *((unsigned char *) stream->bufPtr);
	stream->bufPtr++;
	return result;
}

// Only one character pushback is guaranteed, just like with stdio ungetc().
int
uio_ungetc(int c, uio_Stream *stream) {
	assert((stream->openFlags & O_ACCMODE) != O_WRONLY);
	assert(c >= 0 && c <= 255);

	return (int) EOF;
			// not implemented
//	return c;
}

int
uio_fputc(int c, uio_Stream *stream) {
	assert((stream->openFlags & O_ACCMODE) != O_RDONLY);
	assert(c >= 0 && c <= 255);

	if (stream->writeStart == NULL)
		stream->writeStart = stream->bufPtr;
	if (stream->bufPtr == stream->bufEnd) {
		// The buffer is full. Flush it out.
		if (uio_Stream_flushWriteBuffer(stream) == -1) {
			// errno is set
			return EOF;
		}
	}

	*(unsigned char *) stream->bufPtr = (unsigned char) c;
	stream->bufPtr++;
	return c;
}

int
uio_fputs(const char *s, uio_Stream *stream) {
	int result;
	
	result = uio_fwrite(s, strlen(s), 1, stream);
	if (result != 1)
		return EOF;
	return 0;
}

int
uio_fseek(uio_Stream *stream, long offset, int whence) {
	int newPos;

	if (stream->writeStart != NULL) {
		if (uio_Stream_flushWriteBuffer(stream) == -1) {
			// errno is set
			return -1;
		}
	}

	assert(whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END);
	switch(whence) {
		case SEEK_SET:
			break;
		case SEEK_CUR:
			offset += stream->seekLow - (stream->readEnd - stream->bufPtr);
			break;
		case SEEK_END: {
			struct stat statBuf;
			
			if (uio_fstat(stream->handle, &statBuf) == -1) {
				// errno is set
				return -1;
			}
			offset += statBuf.st_size;
			break;
		}
	}

	// TODO: when implementing pushback: throw away pushback buffer.
	
	// Maybe the new location is still inside the read buffer.
	// If not, we must throw the buffer away.
	// If the buffer was polluted from calls to ungetc(),
	// always throw away the buffer.
	if (offset >= stream->seekLow - (stream->readEnd - stream->buf) &&
			offset <= stream->seekLow) {
		// The buffer can be reused.
		stream->status = uio_Stream_STATUS_OK;
		stream->bufPtr = stream->buf +
				(stream->readEnd - stream->buf) +
				(offset - stream->seekLow);
		return 0;
	}

	// The read buffer is not reusable.
	newPos = uio_lseek(stream->handle, offset, SEEK_SET);
	if (newPos == -1) {
		// errno is set
		return -1;
	}
	stream->seekLow = newPos;
	stream->status = uio_Stream_STATUS_OK;
			// Clear error or end-of-file flag.
	stream->bufPtr = stream->buf;
	stream->readEnd = stream->buf;
	
	if ((stream->openFlags & O_ACCMODE) == O_RDONLY) {
		// We're done if there's no writing involved.
		return 0;
	}

	if (uio_Stream_alignReadBuffer(stream) == -1) {
		// Even when the buffer can't be filled, it might be
		// a suitable location for writing (for instance, at the end
		// of the file).
		return 0;
	}
	
	return 0;
}

long
uio_ftell(uio_Stream *stream) {
	return (long) stream->seekLow - (stream->readEnd - stream->bufPtr);
}

// If less that nmemb elements could be written, or an error occurs, the
// file pointer is undefined. clearerr() followed by fseek() need to be
// called before attempting to read or write again.
// I don't have the C standard myself, but I suspect this is the official
// behaviour for fread() and fwrite().
size_t
uio_fwrite(const void *buf, size_t size, size_t nmemb, uio_Stream *stream) {
	ssize_t bytesToWrite;
	ssize_t bytesWritten;

	assert((stream->openFlags & O_ACCMODE) != O_RDONLY);

	bytesToWrite = size * nmemb;
	if (stream->writeStart == NULL)
		stream->writeStart = stream->bufPtr;
	if (bytesToWrite < stream->bufEnd - stream->bufPtr) {
		// There's enough space in the write buffer to store everything.
		memcpy(stream->bufPtr, buf, bytesToWrite);
		stream->bufPtr += bytesToWrite;
		return nmemb;
	}

	// Not enough space in the write buffer to write everything.
	// Flush what's left in the write buffer, and then directly write 'buf'.
	if (uio_Stream_flushWriteBuffer(stream) == -1) {
		// errno is set
		return 0;
	}

	bytesWritten = uio_write(stream->handle, buf, bytesToWrite);
	if (bytesWritten != bytesToWrite) {
		stream->status = uio_Stream_STATUS_ERROR;
		if (bytesWritten == -1)
			return 0;
	}
	stream->seekLow += stream->readEnd - stream->bufPtr + bytesWritten;
	stream->readStart = stream->buf;
	stream->bufPtr = stream->buf;
	stream->readEnd = stream->buf;
	stream->writeStart = NULL;
	if (bytesWritten == bytesToWrite)
		return nmemb;
	return (size_t) bytesWritten / size;
}

// NB: stdio fflush() accepts NULL to flush all streams. uio_flush() does
// not.
int
uio_fflush(uio_Stream *stream) {
	assert(stream != NULL);
	assert((stream->openFlags & O_ACCMODE) != O_RDONLY);
	return uio_Stream_flushWriteBuffer(stream);
}

int
uio_feof(uio_Stream *stream) {
	return stream->status == uio_Stream_STATUS_EOF;
}

int
uio_ferror(uio_Stream *stream) {
	return stream->status == uio_Stream_STATUS_ERROR;
}

void
uio_clearerr(uio_Stream *stream) {
	stream->status = uio_Stream_STATUS_OK;
}

// Counterpart of fileno()
uio_Handle *
uio_streamHandle(uio_Stream *stream) {
	return stream->handle;	
}

static int
uio_Stream_flushWriteBuffer(uio_Stream *stream) {
	ssize_t bytesWritten;
	off_t newPos;

	newPos = uio_lseek(stream->handle,
			(off_t) (stream->seekLow - (stream->readEnd - stream->writeStart)),
			SEEK_SET);
	if (newPos == -1) {
		// errno is set
		return -1;
	}
	stream->seekLow = newPos;
	
	bytesWritten = uio_write(stream->handle, stream->writeStart,
			stream->bufPtr - stream->writeStart);
	if (bytesWritten != stream->bufPtr - stream->writeStart) {
		stream->status = uio_Stream_STATUS_ERROR;
		if (bytesWritten != -1)
			stream->seekLow += bytesWritten;
		return -1;
	}
	stream->seekLow += bytesWritten;
	stream->writeStart = NULL;

	if (stream->bufPtr > stream->readEnd) {
		stream->readEnd = stream->bufPtr;
	} else {
		newPos = uio_lseek(stream->handle,
				(off_t) (stream->seekLow + (stream->readEnd - stream->bufPtr)),
				SEEK_SET);
		if (newPos == -1) {
			// errno is set
			// At least keep the internal state consistent so that
			// a new uio_fseek() (after an uio_clearerr()) can succeed:
			stream->readEnd = stream->bufPtr;
			// SeekLow of the buffer is not aligned on a block.
			return -1;
		}
		stream->seekLow = newPos;
	}
	return 0;
}

static int
uio_Stream_fillReadBuffer(uio_Stream *stream) {
	ssize_t numRead;

	assert(stream->bufPtr == stream->readEnd);

	numRead = uio_read(stream->handle, stream->buf,
				uio_Stream_BLOCK_SIZE);
	if (numRead == -1)
		return -1;
	stream->bufPtr = stream->buf;
	stream->readEnd = stream->buf + numRead;
	stream->seekLow += numRead;
	return 0;	
}

static int
uio_Stream_alignReadBuffer(uio_Stream *stream) {
	off_t endAlign;
	ssize_t numRead;
	
	endAlign = uio_Stream_BLOCK_PAGE_START(stream->seekLow +
			uio_Stream_BLOCK_SIZE - 1);
	if (endAlign == stream->seekLow) {
		// Nothing to do.
		return 0;
	}

	numRead = uio_read(stream->handle, stream->buf,
				endAlign - stream->seekLow);
	if (numRead == -1)
		return -1;
	stream->bufPtr = stream->buf;
	stream->readEnd = stream->buf + numRead;
	stream->seekLow += numRead;
	return 0;
}

static inline uio_Stream *
uio_Stream_new(uio_Handle *handle, int openFlags) {
	uio_Stream *result;

	result = uio_Stream_alloc();
	result->handle = handle;
	result->openFlags = openFlags;
	result->status = uio_Stream_STATUS_OK;
	result->buf = uio_malloc(uio_Stream_BLOCK_SIZE);
	result->bufPtr = result->buf;
	result->readStart = result->buf;
	result->readEnd = result->buf;
	result->writeStart = NULL;
	result->bufEnd = result->buf + uio_Stream_BLOCK_SIZE;
	result->seekLow = 0;
	return result;
}

static inline uio_Stream *
uio_Stream_alloc(void) {
	uio_Stream *result = uio_malloc(sizeof (uio_Stream));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_Stream, (void *) result);
#endif
	return result;
}

static inline void
uio_Stream_delete(uio_Stream *stream) {
	uio_free(stream->buf);
	uio_Stream_free(stream);
}

static inline void
uio_Stream_free(uio_Stream *stream) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_Stream, (void *) stream);
#endif
	uio_free(stream);
}

