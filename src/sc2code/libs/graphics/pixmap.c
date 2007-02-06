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
		return &DrawablePtr->Frame[FrameIndex];
	}

	return (0);
}

DRAWABLE
ReleaseDrawable (FRAME FramePtr)
{
	if (FramePtr != 0)
	{
		DRAWABLE Drawable;
		DRAWABLEPTR DrawablePtr;

		DrawablePtr = GetFrameParentDrawable (FramePtr);
		Drawable = BUILD_DRAWABLE (DrawablePtr->hDrawable, FramePtr->Index);
		UnlockDrawable (Drawable);

		return (Drawable);
	}

	return (0);
}

MEM_HANDLE
GetFrameHandle (FRAME FramePtr)
{
	DRAWABLEPTR DrawablePtr;

	if (FramePtr == 0)
		return (0);

	DrawablePtr = GetFrameParentDrawable (FramePtr);
	return (DrawablePtr->hDrawable);
}

COUNT
GetFrameCount (FRAME FramePtr)
{
	DRAWABLEPTR DrawablePtr;

	if (FramePtr == 0)
		return (0);

	DrawablePtr = GetFrameParentDrawable (FramePtr);
	return DrawablePtr->MaxIndex + 1;
}

COUNT
GetFrameIndex (FRAME FramePtr)
{
	if (FramePtr == 0)
		return (0);

	return FramePtr->Index;
}

FRAME
SetAbsFrameIndex (FRAME FramePtr, COUNT FrameIndex)
{
	if (FramePtr != 0)
	{
		DRAWABLEPTR DrawablePtr;

		DrawablePtr = GetFrameParentDrawable (FramePtr);

		FrameIndex = FrameIndex	% (DrawablePtr->MaxIndex + 1);
		FramePtr = &DrawablePtr->Frame[FrameIndex];
	}

	return FramePtr;
}

FRAME
SetRelFrameIndex (FRAME FramePtr, SIZE FrameOffs)
{
	if (FramePtr != 0)
	{
		COUNT num_frames;
		DRAWABLEPTR DrawablePtr;

		DrawablePtr = GetFrameParentDrawable (FramePtr);
		num_frames = DrawablePtr->MaxIndex + 1;
		if (FrameOffs < 0)
		{
			while ((FrameOffs += num_frames) < 0)
				;
		}

		FrameOffs = ((SWORD)FramePtr->Index + FrameOffs) % num_frames;
		FramePtr = &DrawablePtr->Frame[FrameOffs];
	}

	return FramePtr;
}

FRAME
SetEquFrameIndex (FRAME DstFramePtr, FRAME SrcFramePtr)
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
IncFrameIndex (FRAME FramePtr)
{
	DRAWABLEPTR DrawablePtr;

	if (FramePtr == 0)
		return (0);

	DrawablePtr = GetFrameParentDrawable (FramePtr);
	if (FramePtr->Index < DrawablePtr->MaxIndex)
		return ++FramePtr;
	else
		return DrawablePtr->Frame;
}

FRAME
DecFrameIndex (FRAME FramePtr)
{
	if (FramePtr == 0)
		return (0);

	if (FramePtr->Index > 0)
		return --FramePtr;
	else
	{
		DRAWABLEPTR DrawablePtr;

		DrawablePtr = GetFrameParentDrawable (FramePtr);
		return &DrawablePtr->Frame[DrawablePtr->MaxIndex];
	}
}
