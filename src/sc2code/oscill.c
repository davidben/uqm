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

#include "libs/graphics/gfx_common.h"
#include "libs/sound/trackplayer.h"

static STAMP sliderStamp;
static STAMP buttonStamp;
int sliderSpace;  // slider width - button width


/*
 * Initialise the communication progress bar
 * x - x location of slider
 * y - y location of slider
 * width - width of slider
 * height - height of slider
 * bwidth - width of button indicating current progress
 * bheight - height of button indicating progress
 * f - image for the slider
 */                        

void
InitSlider (int x, int y, int width, int height,
		int bwidth, int bheight, FRAME f)
{
	sliderStamp.origin.x = x;
	sliderStamp.origin.y = y;
	sliderStamp.frame = f;
	buttonStamp.origin.x = x;
	buttonStamp.origin.y = y - ((bheight - height) >> 1);
	sliderSpace = width - bwidth;
}

void
SetSliderImage (FRAME f)
{
	buttonStamp.frame = f;
}

void
Slider (void)
{
	int len, offs;
	
	if (GetSoundInfo (&len, &offs))
	{
		if (offs > len)
			offs = len;
		if (len == 0)
			len = 1;
		buttonStamp.origin.x = sliderStamp.origin.x + sliderSpace * offs / len;
		BatchGraphics ();
		DrawStamp (&sliderStamp);
		DrawStamp (&buttonStamp);
		UnbatchGraphics ();
	}
}

