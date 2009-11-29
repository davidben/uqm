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
#include "libs/memlib.h"
#include "tfb_draw.h"
#include <math.h>

#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif

FRAME _CurFramePtr;

FRAME
SetContextFGFrame (FRAME Frame)
{
	FRAME LastFrame;

	if (Frame != (LastFrame = (FRAME)_CurFramePtr))
	{
		if (LastFrame)
			DeactivateDrawable ();

		_CurFramePtr = Frame;
		if (_CurFramePtr)
			ActivateDrawable ();

		if (ContextActive ())
		{
			SwitchContextFGFrame (Frame);
		}
	}

	return (LastFrame);
}

FRAME
GetContextFGFrame (void)
{
	return _CurFramePtr;
}

DRAWABLE
CreateDisplay (CREATE_FLAGS CreateFlags, SIZE *pwidth, SIZE *pheight)
{
	DRAWABLE Drawable;

	if (!DisplayActive ())
		return (0);

	Drawable = _request_drawable (1, SCREEN_DRAWABLE,
			(CreateFlags & (WANT_PIXMAP | (GetDisplayFlags () & WANT_MASK))),
			GetDisplayWidth (), GetDisplayHeight ());
	if (Drawable)
	{
		FRAME F;

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
	Drawable = (DRAWABLE) HCalloc(sizeof (DRAWABLE_DESC));
	if (Drawable)
	{
		int i;
		Drawable->Frame = (FRAME)HMalloc (sizeof (FRAME_DESC) * n);
		if (Drawable->Frame == NULL)
		{
			HFree (Drawable);
			return NULL;
		}

		/* Zero out the newly allocated frames, since HMalloc doesn't have
		 * MEM_ZEROINIT. */
		for (i = 0; i < n; i++) {
			FRAME F;
			F = &Drawable->Frame[i];
			F->parent = Drawable;
			F->Type = 0;
			F->Index = 0;
			F->image = 0;
			F->Bounds.width = 0;
			F->Bounds.height = 0;
			F->HotSpot.x = 0;
			F->HotSpot.y = 0;
		}
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

	Drawable = _request_drawable (num_frames, RAM_DRAWABLE,
			(CreateFlags & (WANT_MASK | WANT_PIXMAP
				| WANT_ALPHA | MAPPED_TO_DISPLAY)),
			width, height);
	if (Drawable)
	{
		FRAME F;

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
	if (_CurFramePtr && (Drawable == _CurFramePtr->parent))
		SetContextFGFrame ((FRAME)NULL);

	if (Drawable)
	{
		FreeDrawable (Drawable);

		return (TRUE);
	}

	return (FALSE);
}

BOOLEAN
GetFrameRect (FRAME FramePtr, RECT *pRect)
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
SetFrameHot (FRAME FramePtr, HOT_SPOT HotSpot)
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
GetFrameHot (FRAME FramePtr)
{
	if (FramePtr)
	{
		return FramePtr->HotSpot;
	}

	return (MAKE_HOT_SPOT (0, 0));
}

DRAWABLE
RotateFrame (FRAME Frame, int angle_deg)
{
	DRAWABLE Drawable;
	FRAME RotFramePtr;
	double dx, dy;
	double d;
	double angle = angle_deg * M_PI / 180;

	Drawable = _request_drawable (1, RAM_DRAWABLE, WANT_PIXMAP, 0, 0);
	if (!Drawable)
		return 0;
	RotFramePtr = CaptureDrawable (Drawable);
	if (!RotFramePtr)
	{
		FreeDrawable (Drawable);
		return 0;
	}

	RotFramePtr->image = TFB_DrawImage_New_Rotated (
			Frame->image, angle_deg);
	SetFrameBounds (RotFramePtr, RotFramePtr->image->extent.width,
			RotFramePtr->image->extent.height);

	/* now we need to rotate the hot-spot, eww */
	dx = Frame->HotSpot.x - (GetFrameWidth (Frame) / 2);
	dy = Frame->HotSpot.y - (GetFrameHeight (Frame) / 2);
	d = sqrt ((double)dx*dx + (double)dy*dy);
	if ((int)d != 0)
	{
		double organg = atan2 (-dy, dx);
		dx = cos (organg + angle) * d;
		dy = -sin (organg + angle) * d;
	}
	RotFramePtr->HotSpot.x = (GetFrameWidth (RotFramePtr) / 2) + (int)dx;
	RotFramePtr->HotSpot.y = (GetFrameHeight (RotFramePtr) / 2) + (int)dy;

	ReleaseDrawable (RotFramePtr);

	return Drawable;
}

// color.a is ignored
void
SetFrameTransparentColor (FRAME Frame, Color color)
{
	TFB_DrawCanvas_SetTransparentColor (Frame->image->NormalImg,
			color.r, color.g, color.b, FALSE);
}

