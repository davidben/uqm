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

#ifndef UNICODE_H
#define UNICODE_H

#include "port.h"
#include <sys/types.h>
		// For size_t

typedef uint32 UniChar;

UniChar getCharFromString(const unsigned char **ptr);
UniChar getCharFromStringN(const unsigned char **ptr,
		const unsigned char *end);
unsigned char *getLineFromString(const unsigned char *start,
		const unsigned char **end, const unsigned char **startNext);
size_t utf8StringCount(const unsigned char *start);
size_t utf8StringCountN(const unsigned char *start,
		const unsigned char *end);
int utf8StringPos (const unsigned char *pStr, UniChar ch);
unsigned char *utf8StringCopy (unsigned char *dst, size_t size,
		const unsigned char *src);
int utf8StringCompare (const unsigned char *str1, const unsigned char *str2);
unsigned char *skipUTF8Chars(const unsigned char *ptr, size_t num);
size_t getUniCharFromString(UniChar *wstr, size_t maxcount,
		const unsigned char *start);
size_t getUniCharFromStringN(UniChar *wstr, size_t maxcount,
		const unsigned char *start, const unsigned char *end);
int getStringFromChar(unsigned char *ptr, size_t size, UniChar ch);
size_t getStringFromWideN(unsigned char *ptr, size_t size,
		const UniChar *wstr, size_t count);
size_t getStringFromWide(unsigned char *ptr, size_t size,
		const UniChar *wstr);
int UniChar_isGraph(UniChar ch);
int UniChar_isPrint(UniChar ch);
UniChar UniChar_toUpper(UniChar ch);
UniChar UniChar_toLower(UniChar ch);


#endif  /* UNICODE_H */

