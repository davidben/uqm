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

#ifdef GFXMODULE_SDL

#include "sdl_common.h"
#include "pure.h"
#include "opengl.h"
#include "primitives.h"
#include "libs/graphics/drawcmd.h"
#include "libs/graphics/tfb_draw.h"


//Status: Not entirely unimplemented!
BOOLEAN
InitGraphics (int argc, char* argv[], COUNT KbytesRequired)
		// Kbytes should only matter if we wanted to port Starcon2 to the
		// hand-helds...
{
	BOOLEAN ret;

	LoadDisplay (&_pCurDisplay);
	ActivateDisplay ();

	(void) argc;             /* lint */
	(void) argv;             /* lint */
	(void) KbytesRequired;   /* lint */

	ret = TRUE;
	return (ret);
}

// Status: Unimplemented
void
UninitGraphics ()  // Also probably empty
{
	// HFree (DrawCommandQueue);  This is static now!

	//mem_uninit ();
}

BOOLEAN
_image_intersect (IMAGE_BOX *box1, IMAGE_BOX *box2, RECT *rect)
{
	BOOLEAN ret;
	SDL_Surface *img1, *img2;
	int img1w, img1xpos, img1ypos, img2w, img2xpos, img2ypos;
	int x,y;
	Uint32 img1key, img2key;
	Uint32 img1mask, img2mask;
	GetPixelFn getpixel1, getpixel2;

	img1 = (SDL_Surface *)box1->FramePtr->image->NormalImg;
	img2 = (SDL_Surface *)box2->FramePtr->image->NormalImg;
	
	getpixel1 = getpixel_for(img1);
	getpixel2 = getpixel_for(img2);

	img1w = img1->w;
	img1xpos = box1->Box.corner.x;
	img1ypos = box1->Box.corner.y;
	if (img1->format->Amask)
	{	// use alpha transparency info
		img1mask = img1->format->Amask;
		// consider any not fully transparent pixel collidable
		img1key = 0;
	}
	else
	{	// colorkey transparency
		img1mask = ~img1->format->Amask;
		img1key = img1->format->colorkey & img1mask;
	}

	img2w = img2->w;
	img2xpos = box2->Box.corner.x;
	img2ypos = box2->Box.corner.y;
	if (img2->format->Amask)
	{	// use alpha transparency info
		img2mask = img2->format->Amask;
		// consider any not fully transparent pixel collidable
		img2key = 0;
	}
	else
	{	// colorkey transparency
		img2mask = ~img2->format->Amask;
		img2key = img2->format->colorkey & img2mask;
	}

	for (y = rect->corner.y; y < rect->corner.y + rect->extent.height; ++y)
	{
		for (x = rect->corner.x; x < rect->corner.x + rect->extent.width; ++x)
		{
			Uint32 p1 = getpixel1(img1, x - img1xpos, y - img1ypos)
						& img1mask;
			Uint32 p2 = getpixel2(img2, x - img2xpos, y - img2ypos)
						& img2mask;
			
			if ((p1 != img1key) && (p2 != img2key))
			{
				return TRUE;
			}
		}
	}
	ret = FALSE;

	return (ret);
}


#endif
