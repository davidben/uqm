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

#include "strintrn.h"

BOOLEAN
DestroyStringTable (STRING_TABLE StringTable)
{
	return (FreeStringTable (StringTable));
}

STRING
CaptureStringTable (STRING_TABLE StringTable)
{
	if (StringTable != 0)
	{
		COUNT StringTableIndex;
		STRING_TABLEPTR StringTablePtr;

		LockStringTable (StringTable, &StringTablePtr);
		StringTableIndex = GetStringTableIndex (StringTable);
		return (BUILD_STRING (StringTable, StringTableIndex));
	}

	return ((STRING)NULL_PTR);
}

STRING_TABLE
ReleaseStringTable (STRING String)
{
	STRING_TABLE StringTable;

	StringTable = GetStringTable (String);
	if (StringTable != 0)
		UnlockStringTable (StringTable);

	return (StringTable);
}

STRING_TABLE
GetStringTable (STRING String)
{
	return ((STRING_TABLE)LOWORD (String));
}

COUNT
GetStringTableCount (STRING String)
{
	COUNT StringCount;
	STRING_TABLE StringTable;

	StringTable = GetStringTable (String);
	if (StringTable == 0)
		StringCount = 0;
	else
	{
		STRING_TABLEPTR StringTablePtr;

		LockStringTable (StringTable, &StringTablePtr);
		StringCount = StringTablePtr->StringCount;
		UnlockStringTable (StringTable);
	}

	return (StringCount);
}

COUNT
GetStringTableIndex (STRING String)
{
	return (STRING_INDEX (String));
}

STRING
SetAbsStringTableIndex (STRING String, COUNT StringTableIndex)
{
	STRING_TABLE StringTable;

	StringTable = GetStringTable (String);
	if (StringTable == 0)
		String = 0;
	else
	{
		STRING_TABLEPTR StringTablePtr;

		LockStringTable (StringTable, &StringTablePtr);
		StringTableIndex = StringTableIndex % StringTablePtr->StringCount;
		UnlockStringTable (StringTable);

		String = BUILD_STRING (StringTable, StringTableIndex);
	}

	return (String);
}

STRING
SetRelStringTableIndex (STRING String, SIZE StringTableOffs)
{
	STRING_TABLE StringTable;

	StringTable = GetStringTable (String);
	if (StringTable == 0)
		String = 0;
	else
	{
		STRING_TABLEPTR StringTablePtr;
		COUNT StringTableIndex;

		LockStringTable (StringTable, &StringTablePtr);
		while (StringTableOffs < 0)
			StringTableOffs += StringTablePtr->StringCount;
		StringTableIndex = (STRING_INDEX (String) + StringTableOffs)
				% StringTablePtr->StringCount;
		UnlockStringTable (StringTable);

		String = BUILD_STRING (StringTable, StringTableIndex);
	}

	return (String);
}

COUNT
GetStringLength (STRING String)
{
	COUNT StringLength;
	STRING_TABLE StringTable;

	StringTable = GetStringTable (String);
	if (StringTable == 0)
		StringLength = 0;
	else
	{
		COUNT StringIndex;
		STRING_TABLEPTR StringTablePtr;

		StringIndex = STRING_INDEX (String);
		LockStringTable (StringTable, &StringTablePtr);

		{
			STRINGPTR start;
			STRINGPTR end;

			start = (STRINGPTR) ((BYTE *) StringTablePtr +
					StringTablePtr->StringOffsets[StringIndex]);
			end = (STRINGPTR) ((BYTE *) StringTablePtr +
					StringTablePtr->StringOffsets[StringIndex + 1]);
			StringLength = utf8StringCountN(start, end);
		}

#if 0
		StringLength = (COUNT)(
				StringTablePtr->StringOffsets[StringIndex + 1]
				- StringTablePtr->StringOffsets[StringIndex]);
#endif
		UnlockStringTable (StringTable);
	}

	return (StringLength);
}

COUNT
GetStringLengthBin (STRING String)
{
	COUNT StringLength;
	STRING_TABLE StringTable;

	StringTable = GetStringTable (String);
	if (StringTable == 0)
		StringLength = 0;
	else
	{
		COUNT StringIndex;
		STRING_TABLEPTR StringTablePtr;

		StringIndex = STRING_INDEX (String);
		LockStringTable (StringTable, &StringTablePtr);

		StringLength = (COUNT)(
				StringTablePtr->StringOffsets[StringIndex + 1]
				- StringTablePtr->StringOffsets[StringIndex]);
		UnlockStringTable (StringTable);
	}

	return (StringLength);
}

STRINGPTR
GetStringSoundClip (STRING String)
{
	STRINGPTR StringAddr;
	STRING_TABLE StringTable;

	StringTable = GetStringTable (String);
	if (StringTable == 0)
		StringAddr = 0;
	else
	{
		COUNT StringIndex;
		STRING_TABLEPTR StringTablePtr;

		StringIndex = STRING_INDEX (String);
		LockStringTable (StringTable, &StringTablePtr);
		if (!(StringTablePtr->flags & HAS_SOUND_CLIPS)
				|| ((StringIndex += StringTablePtr->StringCount + 1),
						StringTablePtr->StringOffsets[StringIndex + 1]
								== StringTablePtr->StringOffsets[StringIndex]))
			StringAddr = 0;
		else
		{
			StringAddr = (STRINGPTR) ((BYTE *) StringTablePtr +
					StringTablePtr->StringOffsets[StringIndex]);
		}
		UnlockStringTable (StringTable);
	}

	return (StringAddr);
}

STRINGPTR
GetStringTimeStamp (STRING String)
{
	STRINGPTR StringAddr;
	STRING_TABLE StringTable;

	StringTable = GetStringTable (String);
	if (StringTable == 0)
		StringAddr = 0;
	else
	{
		COUNT StringIndex;
		STRING_TABLEPTR StringTablePtr;
		int offset;

		StringIndex = STRING_INDEX (String);
		LockStringTable (StringTable, &StringTablePtr);
		offset = (StringTablePtr->flags & HAS_SOUND_CLIPS) ? 1 : 0;
		if (!(StringTablePtr->flags & HAS_TIMESTAMP)
				|| ((StringIndex += (StringTablePtr->StringCount + 1) << offset),
						StringTablePtr->StringOffsets[StringIndex + 1]
								== StringTablePtr->StringOffsets[StringIndex]))
			StringAddr = 0;
		else
		{
			StringAddr = (STRINGPTR) ((BYTE *) StringTablePtr +
					StringTablePtr->StringOffsets[StringIndex]);
		}
		UnlockStringTable (StringTable);
	}

	return (StringAddr);
}

STRINGPTR
GetStringAddress (STRING String)
{
	STRINGPTR StringAddr;
	STRING_TABLE StringTable;

	StringTable = GetStringTable (String);
	if (StringTable == 0)
		StringAddr = 0;
	else
	{
		COUNT StringIndex;
		STRING_TABLEPTR StringTablePtr;

		StringIndex = STRING_INDEX (String);
		LockStringTable (StringTable, &StringTablePtr);
		StringAddr = (STRINGPTR) ((BYTE *) StringTablePtr +
				StringTablePtr->StringOffsets[StringIndex]);
		UnlockStringTable (StringTable);
	}

	return (StringAddr);
}

BOOLEAN
GetStringContents (STRING String, STRINGPTR StringBuf, BOOLEAN
		AppendSpace)
{
	STRINGPTR StringAddr;
	COUNT StringLength;

	if ((StringAddr = GetStringAddress (String)) != 0 &&
			(StringLength = GetStringLengthBin (String)) != 0)
	{
		memcpy (StringBuf, StringAddr, StringLength);
		if (AppendSpace)
			StringBuf[StringLength++] = ' ';
		StringBuf[StringLength] = '\0';

		return (TRUE);
	}

	*StringBuf = '\0';
	return (FALSE);
}

