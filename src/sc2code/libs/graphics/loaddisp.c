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


/*
		LoadDisplay---Drawable

		Allocate a chunk of memory and read from the display into that
		chunk of memory.


		LoadDisplayPixmapDrawable()
		LoadDisplayMaskDrawable()
		LoadDisplayPixmapMaskDrawable()

*/


DRAWABLE
LoadDisplayPixmap (PRECT area, FRAME frame)
{
	DRAWABLE buffer;

	buffer = BUILD_DRAWABLE (
			GetFrameHandle (frame),
			GetFrameIndex (frame)
			);
	if (buffer || (buffer = CreateDrawable (
			WANT_PIXMAP | MAPPED_TO_DISPLAY,
			area->extent.width,
			area->extent.height,
			1))
		)
	{
		frame = CaptureDrawable (buffer);
		ReadDisplay (area, (FRAMEPTR)frame);
		ReleaseDrawable (frame);
	}

	return (buffer);
}


