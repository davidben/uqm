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
#include "SDL_wrapper.h"

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
			SetContextGraphicsFunctions ();
		}
	}

	return (LastFrame);
}

FRAME
SetContextBGFrame (FRAME Frame)
{
	FRAME LastFrame;

	LastFrame = BGFrame;
	BGFrame = Frame;
	if (ContextActive ())
		SwitchContextBGFrame (Frame);

	return (LastFrame);
}

DRAWABLE
_request_drawable (COUNT NumFrames, DRAWABLE_TYPE DrawableType,
		CREATE_FLAGS flags, SIZE width, SIZE height)
{
	DRAWABLE Drawable;

	Drawable = AllocDrawableImage (
			NumFrames, DrawableType, flags, width, height
			);
	if (Drawable)
	{
		DRAWABLEPTR DrawablePtr;

		if ((DrawablePtr = LockDrawable (Drawable)) == 0)
		{
			FreeDrawable (Drawable);
			Drawable = 0;
		}
		else
		{
			int imgw, imgh;
			FRAMEPTR FramePtr;

			DrawablePtr->hDrawable = GetDrawableHandle (Drawable);
			TYPE_SET (DrawablePtr->FlagsAndIndex, flags << FTYPE_SHIFT);
			INDEX_SET (DrawablePtr->FlagsAndIndex, NumFrames - 1);

			imgw = (flags & MAPPED_TO_DISPLAY) ? width * ScreenWidthActual / ScreenWidth : width;
			imgh = (flags & MAPPED_TO_DISPLAY) ? height * ScreenHeightActual / ScreenHeight : height;
			FramePtr = &DrawablePtr->Frame[NumFrames - 1];
			while (NumFrames--)
			{
				TFB_Image *Image;


				if (DrawableType == RAM_DRAWABLE
						&& (Image = TFB_LoadImage (SDL_CreateRGBSurface (
										SDL_HWSURFACE,
										imgw,
										imgh,
										24,
										0x00FF0000,
										0x0000FF00,
										0x000000FF,
										0x00000000
										))))
					FramePtr->DataOffs = (BYTE *)Image - (BYTE *)FramePtr;

				TYPE_SET (FramePtr->TypeIndexAndFlags, DrawableType);
				INDEX_SET (FramePtr->TypeIndexAndFlags, NumFrames);
				SetFrameBounds (FramePtr, width, height);
				--FramePtr;
			}
			UnlockDrawable (Drawable);
		}
	}

	return (Drawable);
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

DRAWABLE
CopyDrawable (DRAWABLE Drawable)
{
	DRAWABLEPTR DrawablePtr;

	DrawablePtr = LockDrawable (Drawable);
	if (DrawablePtr)
	{
		DRAWABLE CopyDrawable;
		DWORD size;

		if (TYPE_GET (DrawablePtr->Frame[0].TypeIndexAndFlags) == SCREEN_DRAWABLE)
			CopyDrawable = 0;
		else if ((CopyDrawable = AllocDrawable (1,
				(size = mem_get_size ((MEM_HANDLE)Drawable))
				- sizeof (DRAWABLE_DESC))))
		{
			DRAWABLEPTR CopyDrawablePtr;

			if ((CopyDrawablePtr = LockDrawable (CopyDrawable)) == 0)
			{
				FreeDrawable (CopyDrawable);
				CopyDrawable = 0;
			}
			else
			{
				PBYTE lpDst, lpSrc;

				lpDst = (PBYTE)CopyDrawablePtr;
				lpSrc = (PBYTE)DrawablePtr;
				do
				{
					COUNT num_bytes;

					num_bytes = size >= 0x7FFF ? 0x7FFF : (COUNT)size;
					memcpy (lpDst, lpSrc, num_bytes);
					FAR_PTR_ADD (&lpDst, num_bytes);
					FAR_PTR_ADD (&lpSrc, num_bytes);
					size -= num_bytes;
				} while (size);
				CopyDrawablePtr->hDrawable = (MEM_HANDLE)CopyDrawable;
				UnlockDrawable (CopyDrawable);
			}
		}
		UnlockDrawable (Drawable);

		return (CopyDrawable);
	}

	return (0);
}

BOOLEAN
DestroyDrawable (DRAWABLE Drawable)
{
	DRAWABLEPTR DrawablePtr;

	if (LOWORD (Drawable) == GetFrameHandle (_CurFramePtr))
		SetContextFGFrame ((FRAME)0);
	if (LOWORD (Drawable) == GetFrameHandle (BGFrame))
		SetContextBGFrame ((FRAME)0);

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
		pRect->corner.x = -GetFrameHotX (FramePtr);
		pRect->corner.y = -GetFrameHotY (FramePtr);
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

		OldHot = GetFrameHotSpot (FramePtr);
			SetFrameHotSpot (FramePtr, HotSpot);

			return (OldHot);
	}

	return (MAKE_HOT_SPOT (0, 0));
}

HOT_SPOT
GetFrameHot (FRAMEPTR FramePtr)
{
	if (FramePtr)
	{
		return (GetFrameHotSpot (FramePtr));
	}

	return (MAKE_HOT_SPOT (0, 0));
}

