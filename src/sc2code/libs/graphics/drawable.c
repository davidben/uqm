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
#include "misc.h"

FRAMEPTR _CurFramePtr;

FRAME
SetContextFGFrame (FRAME Frame)
{
	FRAME LastFrame;

	if (Frame != (LastFrame = (FRAME)_CurFramePtr))
	{
		if (LastFrame)
			DeactivateDrawable ();

		_CurFramePtr = (FRAMEPTR)Frame;
		if (_CurFramePtr)
			ActivateDrawable ();

		if (ContextActive ())
		{
			SwitchContextFGFrame (Frame);
		}
	}

	return (LastFrame);
}

DRAWABLE
CreateDisplay (CREATE_FLAGS CreateFlags, PSIZE pwidth, PSIZE pheight)
{
	DRAWABLE Drawable;

	if (!DisplayActive ())
		return (0);

	Drawable = _request_drawable (
			(COUNT)1, (DRAWABLE_TYPE)SCREEN_DRAWABLE,
			(CREATE_FLAGS)(CreateFlags & (WANT_PIXMAP | (GetDisplayFlags () & WANT_MASK))),
			(SIZE)GetDisplayWidth (),
			(SIZE)GetDisplayHeight ()
			);
	if (Drawable)
	{
		FRAMEPTR F;

		if ((F = CaptureDrawable (Drawable)) == 0)
			DestroyDrawable (Drawable);
		else
		{
			*pwidth = GetFrameWidth (F);
			*pheight = GetFrameHeight (F);

			ReleaseDrawable (F);
			return (Drawable);
		}
	}

	*pwidth = *pheight = 0;
	return (0);
}

DRAWABLE
AllocDrawable (COUNT n) 
{
	DRAWABLE Drawable;
	Drawable = (DRAWABLE)mem_allocate ((MEM_SIZE)(sizeof (DRAWABLE_DESC)), 
			MEM_ZEROINIT | MEM_GRAPHICS,
			DRAWABLE_PRIORITY, MEM_SIMPLE);
	if (Drawable)
	{
		DRAWABLEPTR DrawablePtr;
		int i;
		DrawablePtr = LockDrawable (Drawable);
		DrawablePtr->Frame = (FRAMEPTR)HMalloc ((MEM_SIZE)(sizeof (FRAME_DESC) * n));

		/* Zero out the newly allocated frames, since HMalloc doesn't have MEM_ZEROINIT. */
		for (i = 0; i < n; i++) {
			FRAMEPTR F;
			F = &DrawablePtr->Frame[i];
			F->parent = DrawablePtr;
			F->TypeIndexAndFlags = 0;
			F->image = 0;
			F->Bounds = 0;
			F->HotSpot.x = F->HotSpot.y = 0;
		}
		
		UnlockDrawable (Drawable);
	}
	return Drawable;
}

DRAWABLE
CreateDrawable (CREATE_FLAGS CreateFlags, SIZE width, SIZE height, COUNT
		num_frames)
{
	DRAWABLE Drawable;

	if (!DisplayActive ())
		return (0);

	Drawable = _request_drawable (
			(COUNT)num_frames, (DRAWABLE_TYPE)RAM_DRAWABLE,
			(CREATE_FLAGS)(CreateFlags & (WANT_MASK | WANT_PIXMAP | MAPPED_TO_DISPLAY)),
			(SIZE)width, (SIZE)height
			);
	if (Drawable)
	{
		FRAMEPTR F;

		F = CaptureDrawable (Drawable);
		if (F)
		{
			ReleaseDrawable (F);

			return (Drawable);
		}
	}

	return (0);
}

BOOLEAN
DestroyDrawable (DRAWABLE Drawable)
{
	DRAWABLEPTR DrawablePtr;

	if (LOWORD (Drawable) == GetFrameHandle (_CurFramePtr))
		SetContextFGFrame ((FRAME)0);

	DrawablePtr = LockDrawable (Drawable);
	if (DrawablePtr)
	{
		UnlockDrawable (Drawable);
		FreeDrawable (Drawable);

		return (TRUE);
	}

	return (FALSE);
}

BOOLEAN
GetFrameRect (FRAMEPTR FramePtr, PRECT pRect)
{
	if (FramePtr)
	{
		pRect->corner.x = -FramePtr->HotSpot.x;
		pRect->corner.y = -FramePtr->HotSpot.y;
		pRect->extent.width = GetFrameWidth (FramePtr);
		pRect->extent.height = GetFrameHeight (FramePtr);

		return (TRUE);
	}

	return (FALSE);
}

HOT_SPOT
SetFrameHot (FRAMEPTR FramePtr, HOT_SPOT HotSpot)
{
	if (FramePtr)
	{
		HOT_SPOT OldHot;

		OldHot = FramePtr->HotSpot;
		FramePtr->HotSpot = HotSpot;

		return (OldHot);
	}

	return (MAKE_HOT_SPOT (0, 0));
}

HOT_SPOT
GetFrameHot (FRAMEPTR FramePtr)
{
	if (FramePtr)
	{
		return FramePtr->HotSpot;
	}

	return (MAKE_HOT_SPOT (0, 0));
}

