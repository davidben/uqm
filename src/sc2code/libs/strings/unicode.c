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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "port.h"

#include <stddef.h>
#include <stdio.h>

// Resynchronise (skip everything starting with 0x10xxxxxx):
static inline void
resyncUTF8(const unsigned char **ptr) {
	while ((**ptr & 0xc0) == 0x80)
		*ptr++;
}

// Get one character from a UTF-8 encoded string.
// *ptr will point to the start of the next character.
// Returns 0 if the encoding is bad. This can be distinguished from the
// '\0' character by checking whether **ptr == '\0' before calling this
// function.
wchar_t
getCharFromString(const unsigned char **ptr) {
	wchar_t result;

	if (**ptr < 0x80) {
		// 0xxxxxxx, regular ASCII
		result = **ptr;
		(*ptr)++;

		return result;
	}

	if ((**ptr & 0xe0) == 0xc0) {
		// 110xxxxx; 10xxxxxx must follow
		// Value between 0x00000080 and 0x000007ff (inclusive)
		result = **ptr & 0x1f;
		(*ptr)++;
		
		if ((**ptr & 0xc0) != 0x80)
			goto err;
		result = (result << 6) | ((**ptr) & 0x3f);
		(*ptr)++;
		
		if (result < 0x00000080) {
			// invalid encoding - must reject
			goto err;
		}
		return result;
	}

	if ((**ptr & 0xf0) == 0xe0) {
		// 1110xxxx; 10xxxxxx 10xxxxxx must follow
		// Value between 0x00000800 and 0x0000ffff (inclusive)
		result = **ptr & 0x0f;
		(*ptr)++;
		
		if ((**ptr & 0xc0) != 0x80)
			goto err;
		result = (result << 6) | ((**ptr) & 0x3f);
		(*ptr)++;
		
		if ((**ptr & 0xc0) != 0x80)
			goto err;
		result = (result << 6) | ((**ptr) & 0x3f);
		(*ptr)++;
		
		if (result < 0x00000800) {
			// invalid encoding - must reject
			goto err;
		}
		return result;
	}

	if ((**ptr & 0xf8) == 0xf0) {
		// 11110xxx; 10xxxxxx 10xxxxxx 10xxxxxx must follow
		// Value between 0x00010000 and 0x0010ffff (inclusive)
		result = **ptr & 0x07;
		(*ptr)++;
		
		if ((**ptr & 0xc0) != 0x80)
			goto err;
		result = (result << 6) | ((**ptr) & 0x3f);
		(*ptr)++;
		
		if ((**ptr & 0xc0) != 0x80)
			goto err;
		result = (result << 6) | ((**ptr) & 0x3f);
		(*ptr)++;
		
		if ((**ptr & 0xc0) != 0x80)
			goto err;
		result = (result << 6) | ((**ptr) & 0x3f);
		(*ptr)++;
		
		if (result < 0x00010000) {
			// invalid encoding - must reject
			goto err;
		}
		return result;
	}
	
err:
	fprintf(stderr, "Warning: Invalid UTF8 sequence.\n");
	
	// Resynchronise (skip everything starting with 0x10xxxxxx):
	resyncUTF8(ptr);
	
	return 0;
}

wchar_t
getCharFromStringN(const unsigned char **ptr, const unsigned char *end) {
	int numBytes;

	if (**ptr < 0x80) {
		numBytes = 1;
	} else if ((**ptr & 0xe0) == 0xc0) {
		numBytes = 2;
	} else if ((**ptr & 0xf0) == 0xe0) {
		numBytes = 3;
	} else if ((**ptr & 0xf8) == 0xf0) {
		numBytes = 4;
	} else
		goto err;

	if (*ptr + numBytes > end)
		goto err;

	return getCharFromString(ptr);

err:
	*ptr = end;
	return 0;
}

// Get one line from a string.
// A line is terminated with either CRLF (DOS/Windows),
// LF (Unix, MacOS X), or CR (old MacOS).
// The end of the string is reached when **startNext == '\0'.
// NULL is returned if the string is not valid UTF8. In this case
// *end points to the first invalid character (or the character before if
// it was a LF), and *startNext to the start of the next (possibly invalid
// too) character.
unsigned char *
getLineFromString(const unsigned char *start, const unsigned char **end,
		const unsigned char **startNext) {
	const unsigned char *ptr = start;
	const unsigned char *lastPtr;
	wchar_t ch;

	// Search for the first newline.
	for (;;) {
		if (*ptr == '\0') {
			*end = ptr;
			*startNext = ptr;
			return (unsigned char *) start;
		}
		lastPtr = ptr;
		ch = getCharFromString(&ptr);
		if (ch == '\0') {
			// Bad string
			*end = lastPtr;
			*startNext = ptr;
			return NULL;
		}
		if (ch == '\n') {
			*end = lastPtr;
			if (*ptr == '\0'){
				// LF at the end of the string.
				*startNext = ptr;
				return (unsigned char *) start;
			}
			ch = getCharFromString(&ptr);
			if (ch == '\0') {
				// Bad string
				return NULL;
			}
			if (ch == '\r') {
				// LFCR
				*startNext = ptr;
			} else {
				// LF
				*startNext = *end;
			}
			return (unsigned char *) start;
		} else if (ch == '\r') {
			*end = lastPtr;
			*startNext = ptr;
			return (unsigned char *) start;
		} // else: a normal character
	}
}

size_t
utf8StringCount(const unsigned char *start) {
	size_t count = 0;
	wchar_t ch;

	for (;;) {
		ch = getCharFromString(&start);
		if (ch == '\0')
			return count;
		count++;
	}
}

size_t
utf8StringCountN(const unsigned char *start, const unsigned char *end) {
	size_t count = 0;
	wchar_t ch;

	for (;;) {
		ch = getCharFromStringN(&start, end);
		if (ch == '\0')
			return count;
		count++;
	}
}

unsigned char *
skipUTF8Chars(const unsigned char *ptr, size_t num) {
	wchar_t ch;
	const unsigned char *oldPtr;

	while (num--) {
		oldPtr = ptr;
		ch = getCharFromString(&ptr);
		if (ch == '\0')
			return (unsigned char *) oldPtr;
	}
	return (unsigned char *) ptr;
}


