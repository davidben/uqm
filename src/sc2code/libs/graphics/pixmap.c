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

#include "gfxintrn.h"

FRAME
CaptureDrawable (DRAWABLE Drawable)
{
	DRAWABLEPTR DrawablePtr;

	DrawablePtr = LockDrawable (Drawable);
	if (DrawablePtr)
	{
		COUNT FrameIndex;

		FrameIndex = GetDrawableIndex (Drawable);
		return ((FRAME)&DrawablePtr->Frame[FrameIndex]);
	}

	return (0);
}

DRAWABLE
ReleaseDrawable (FRAMEPTR FramePtr)
{
	if (FramePtr != 0)
	{
		DRAWABLE Drawable;
		DRAWABLEPTR DrawablePtr;

		DrawablePtr = GetFrameParentDrawable (FramePtr);
		Drawable = BUILD_DRAWABLE (
				DrawablePtr->hDrawable, INDEX_GET (FramePtr->TypeIndexAndFlags)
				);
		UnlockDrawable (Drawable);

		return (Drawable);
	}

	return (0);
}

MEM_HANDLE
GetFrameHandle (FRAMEPTR FramePtr)
{
	DRAWABLEPTR DrawablePtr;

	if (FramePtr == 0)
		return (0);

	DrawablePtr = GetFrameParentDrawable (FramePtr);
	return (DrawablePtr->hDrawable);
}

COUNT
GetFrameCount (FRAMEPTR FramePtr)
{
	DRAWABLEPTR DrawablePtr;

	if (FramePtr == 0)
		return (0);

	DrawablePtr = GetFrameParentDrawable (FramePtr);
	return (INDEX_GET (DrawablePtr->FlagsAndIndex) + 1);
}

COUNT
GetFrameIndex (FRAMEPTR FramePtr)
{
	if (FramePtr == 0)
		return (0);

	return (INDEX_GET (FramePtr->TypeIndexAndFlags));
}

FRAME
SetAbsFrameIndex (FRAMEPTR FramePtr, COUNT FrameIndex)
{
	if (FramePtr != 0)
	{
		DRAWABLEPTR DrawablePtr;

		DrawablePtr = GetFrameParentDrawable (FramePtr);

		FrameIndex = FrameIndex
				% (INDEX_GET (DrawablePtr->FlagsAndIndex) + 1);
		FramePtr = (FRAMEPTR)&DrawablePtr->Frame[FrameIndex];
	}

	return ((FRAME)FramePtr);
}

FRAME
SetRelFrameIndex (FRAMEPTR FramePtr, SIZE FrameOffs)
{
	if (FramePtr != 0)
	{
		COUNT num_frames;
		DRAWABLEPTR DrawablePtr;

		DrawablePtr = GetFrameParentDrawable (FramePtr);
		num_frames = INDEX_GET (DrawablePtr->FlagsAndIndex) + 1;
		if (FrameOffs < 0)
		{
			while ((FrameOffs += num_frames) < 0)
				;
		}

		FrameOffs = (INDEX_GET (FramePtr->TypeIndexAndFlags) + FrameOffs) % num_frames;
		FramePtr = (FRAMEPTR)&DrawablePtr->Frame[FrameOffs];
	}

	return ((FRAME)FramePtr);
}

FRAME
SetEquFrameIndex (FRAMEPTR DstFramePtr, FRAMEPTR SrcFramePtr)
{
	if (DstFramePtr && SrcFramePtr)
		return ((FRAME)(
				(PBYTE)((GetFrameParentDrawable (DstFramePtr))->Frame)
				+ ((PBYTE)SrcFramePtr -
				(PBYTE)((GetFrameParentDrawable (SrcFramePtr))->Frame))
				));

	return (0);
}

FRAME
IncFrameIndex (FRAMEPTR FramePtr)
{
	DRAWABLEPTR DrawablePtr;

	if (FramePtr == 0)
		return (0);

	DrawablePtr = GetFrameParentDrawable (FramePtr);
	if (INDEX_GET (FramePtr->TypeIndexAndFlags) < INDEX_GET (DrawablePtr->FlagsAndIndex))
		return ((FRAME)++FramePtr);
	else
		return ((FRAME)DrawablePtr->Frame);
}

FRAME
DecFrameIndex (FRAMEPTR FramePtr)
{
	if (FramePtr == 0)
		return (0);

	if (INDEX_GET (FramePtr->TypeIndexAndFlags))
		return ((FRAME)--FramePtr);
	else
	{
		DRAWABLEPTR DrawablePtr;

		DrawablePtr = GetFrameParentDrawable (FramePtr);
		return ((FRAME)&DrawablePtr->Frame[INDEX_GET (DrawablePtr->FlagsAndIndex)]);
	}
}
