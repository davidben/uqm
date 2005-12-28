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

#include "oscill.h"

#include "sis.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/sound/trackplayer.h"


static FRAMEPTR scope_frame;
static int scope_init = 0;
static TFB_Image *scope_bg = NULL;
static TFB_Image *scope_surf = NULL;
static UBYTE scope_data[RADAR_WIDTH - 2];
BOOLEAN oscillDisabled = FALSE;

void
InitOscilloscope (DWORD x, DWORD y, DWORD width, DWORD height, FRAME f)
{
	scope_frame = f;
	if (!scope_init)
	{
		TFB_Canvas scope_bg_canvas, scope_surf_canvas;
		if (TFB_DrawCanvas_IsPaletted (scope_frame->image->NormalImg))
		{
			scope_bg_canvas = TFB_DrawCanvas_New_Paletted (width, height,
					scope_frame->image->Palette, -1);
			scope_surf_canvas = TFB_DrawCanvas_New_Paletted (width, height,
					scope_frame->image->Palette, -1);
		}
		else
		{
			scope_bg_canvas = TFB_DrawCanvas_New_ForScreen (width, height,
					FALSE);
			scope_surf_canvas = TFB_DrawCanvas_New_ForScreen (width, height,
					FALSE);
		}
		scope_bg = TFB_DrawImage_New (scope_bg_canvas);
		scope_surf = TFB_DrawImage_New (scope_surf_canvas);
		TFB_DrawImage_Image (scope_frame->image, 0, 0, 0, NULL, scope_bg);
		scope_init = 1;
	}
	/* remove compiler warnings */
	(void) x;
	(void) y;
}

void
UninitOscilloscope (void)
{
	// XXX: Is never called (BUG?)
	if (scope_bg)
	{
		TFB_DrawImage_Delete (scope_bg);
		scope_bg = NULL;
	}
	if (scope_surf)
	{
		TFB_DrawImage_Delete (scope_surf);
		scope_surf = NULL;
	}
	scope_init = 0;
}

// draws the oscilloscope
void
Oscilloscope (DWORD grab_data)
{
	STAMP s;

	if (oscillDisabled)
		return;

	if (!grab_data)
		return;

	TFB_DrawImage_Image (scope_bg, 0, 0, 0, NULL, scope_surf);
	if (GetSoundData (scope_data)) 
	{
		int i, r, g, b;		
		TFB_DrawCanvas_GetPixel (scope_bg->NormalImg,
				scope_bg->extent.width / 2, scope_bg->extent.height / 2,
				&r, &g, &b);
		for (i = 0; i < RADAR_WIDTH - 3; ++i)
			TFB_DrawImage_Line (i + 1, scope_data[i], i + 2,
					scope_data[i + 1], r, g, b, scope_surf);
	}
	TFB_DrawImage_Image (scope_surf, 0, 0, 0, NULL, scope_frame->image);

	s.frame = scope_frame;
	s.origin.x = s.origin.y = 0;
	DrawStamp (&s);
}


static STAMP sliderStamp;
static STAMP buttonStamp;
static BOOLEAN sliderChanged = FALSE;
int sliderSpace;  // slider width - button width
BOOLEAN sliderDisabled = FALSE;

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
	sliderChanged = TRUE;
	buttonStamp.frame = f;
}

void
Slider (void)
{
	int offs;
	static int last_offs = -1;

	if (sliderDisabled)
		return;
	
	offs = GetSoundInfo (sliderSpace);
	if (offs != last_offs || sliderChanged)
	{
		sliderChanged = FALSE;
		last_offs = offs;
		buttonStamp.origin.x = sliderStamp.origin.x + offs;
		BatchGraphics ();
		DrawStamp (&sliderStamp);
		DrawStamp (&buttonStamp);
		UnbatchGraphics ();
	}
}

