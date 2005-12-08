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

#ifndef _FONT_H
#define _FONT_H

#define MAX_DELTAS 100

typedef struct FontPage
{
	struct FontPage *next;
	wchar_t pageStart;
#define CHARACTER_PAGE_MASK 0xff800
	wchar_t firstChar;
	size_t numChars;
	TFB_Char *charDesc;
} FONT_PAGE;
typedef FONT_PAGE *PFONT_PAGE;

static inline FONT_PAGE *
AllocFontPage (int numChars)
{
	FONT_PAGE *result = HMalloc (sizeof (FONT_PAGE));
	result->charDesc = HCalloc (numChars * sizeof *result->charDesc);
	return result;
}

static inline void
FreeFontPage (FONT_PAGE *page)
{
	HFree (page->charDesc);
	HFree (page);
}

typedef struct
{
	FONT_REF FontRef;

	UWORD Leading;
	UWORD LeadingWidth;
	FONT_PAGE *fontPages;
} FONT_DESC;
typedef FONT_DESC *PFONT_DESC;

#define FONTPTR PFONT_DESC
#define CHAR_DESCPTR PCHAR_DESC

#define FONT_PRIORITY DEFAULT_MEM_PRIORITY

#define AllocFont(size) \
	(FONT_REF)mem_allocate ((MEM_SIZE)(sizeof (FONT_DESC) + (size)), \
			MEM_ZEROINIT | MEM_GRAPHICS, FONT_PRIORITY, MEM_SIMPLE)
#define LockFont(h) (FONTPTR)mem_lock (h)
#define UnlockFont(h) mem_unlock (h)
#define FreeFont _ReleaseFontData

#define NULL_FONT (FONTPTR)NULL_PTR

extern FONTPTR _CurFontPtr;

extern MEM_HANDLE _GetFontData (uio_Stream *fp, DWORD length);
extern BOOLEAN _ReleaseFontData (MEM_HANDLE handle);

#endif /* _FONT_H */

