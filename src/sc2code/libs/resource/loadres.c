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

#include "resintrn.h"
#include "declib.h"

MEM_HANDLE
GetResourceData (uio_Stream *fp, DWORD length, MEM_FLAGS mem_flags)
{
	MEM_HANDLE RData;
	DECODE_REF fh;

	if (length == ~(DWORD)0)
		length = LengthResFile (fp), fh = 0;
	else if ((fh = copen (fp, FILE_STREAM, STREAM_READ)))
		cfilelength (fh, &length);
	else
		length -= sizeof (DWORD);

	RData = AllocResourceData (length, mem_flags);
	if (RData)
	{
		RESOURCE_DATAPTR RDPtr;

		LockResourceData (RData, &RDPtr);
		if (RDPtr == NULL_PTR)
		{
			FreeResourceData (RData);
			RData = 0;
		}
		else
		{
			COUNT num_read;

			do
			{
#define READ_LENGTH 0x00007FFFL
				num_read = length >= READ_LENGTH ?
						(COUNT)READ_LENGTH : (COUNT)length;
				if (fh)
				{
					if (cread (RDPtr, 1, num_read, fh) != num_read)
						break;
				}
				else
				{
					if ((int)(ReadResFile (RDPtr, 1, num_read, fp)) != (int)num_read)
						break;
				}
				RDPtr = (RESOURCE_DATAPTR) ((BYTE *) RDPtr + num_read);
			} while (length -= num_read);

			UnlockResourceData (RData);
			if (length > 0)
			{
				FreeResourceData (RData);
				RData = 0;
			}
		}
	}

	cclose (fh);

	return (RData);
}


