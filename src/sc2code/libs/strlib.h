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

#ifndef _STRLIB_H
#define _STRLIB_H

#include "compiler.h"
#include "port.h"
#include "libs/uio.h"

#include <stddef.h>

typedef DWORD STRING_TABLE;
typedef DWORD STRING;
typedef PBYTE STRINGPTR;

extern BOOLEAN InstallStringTableResType (COUNT string_type);
extern STRING_TABLE LoadStringTableInstance (DWORD res);
extern STRING_TABLE LoadStringTableFile (uio_DirHandle *dir,
		const char *fileName);
extern BOOLEAN DestroyStringTable (STRING_TABLE StringTable);
extern STRING CaptureStringTable (STRING_TABLE StringTable);
extern STRING_TABLE ReleaseStringTable (STRING String);
extern STRING_TABLE GetStringTable (STRING String);
extern COUNT GetStringTableCount (STRING String);
extern COUNT GetStringTableIndex (STRING String);
extern STRING SetAbsStringTableIndex (STRING String, COUNT
		StringTableIndex);
extern STRING SetRelStringTableIndex (STRING String, SIZE
		StringTableOffs);
extern COUNT GetStringLength (STRING String);
extern COUNT GetStringLengthBin (STRING String);
extern STRINGPTR GetStringAddress (STRING String);
extern STRINGPTR GetStringSoundClip (STRING String);
extern STRINGPTR GetStringTimeStamp (STRING String);
extern BOOLEAN GetStringContents (STRING String, STRINGPTR StringBuf,
		BOOLEAN AppendSpace);

wchar_t getCharFromString(const unsigned char **ptr);
wchar_t getCharFromStringN(const unsigned char **ptr,
		const unsigned char *end);
unsigned char *getLineFromString(const unsigned char *start,
		const unsigned char **end, const unsigned char **startNext);
size_t utf8StringCount(const unsigned char *start);
size_t utf8StringCountN(const unsigned char *start,
		const unsigned char *end);
int utf8StringPos (const unsigned char *pStr, wchar_t ch);
unsigned char *utf8StringCopy (unsigned char *dst, size_t size,
		const unsigned char *src);
int utf8StringCompare (const unsigned char *str1, const unsigned char *str2);
unsigned char *skipUTF8Chars(const unsigned char *ptr, size_t num);
size_t getWideFromString(wchar_t *wstr, size_t maxcount,
		const unsigned char *start);
size_t getWideFromStringN(wchar_t *wstr, size_t maxcount,
		const unsigned char *start, const unsigned char *end);
int getStringFromChar(unsigned char *ptr, size_t size, wchar_t ch);
size_t getStringFromWideN(unsigned char *ptr, size_t size,
		const wchar_t *wstr, size_t count);
size_t getStringFromWide(unsigned char *ptr, size_t size,
		const wchar_t *wstr);
int isWideGraphChar(wchar_t ch);
int isWidePrintChar(wchar_t ch);
wchar_t toWideUpper(wchar_t ch);
wchar_t toWideLower(wchar_t ch);

#define WCHAR_DEGREE_SIGN   0x00b0
#define STR_DEGREE_SIGN     "\xC2\xB0"
#define WCHAR_INFINITY_SIGN 0x221e
#define STR_INFINITY_SIGN   "\xE2\x88\x9E"
#define WCHAR_EARTH_SIGN    0x2641
#define STR_EARTH_SIGN      "\xE2\x99\x81"
#define WCHAR_MIDDLE_DOT    0x00b7
#define STR_MIDDLE_DOT      "\xC2\xB7"

#endif /* _STRLIB_H */

