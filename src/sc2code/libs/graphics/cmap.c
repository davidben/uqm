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

#include "libs/graphics/sdl/sdl_common.h"
#include "gfx_common.h"
#include "libs/tasklib.h"

static struct
{
	COLORMAPPTR CMapPtr;
	SIZE Ticks;
	union
	{
		COUNT NumCycles;
		Task XFormTask;
	} tc;
} TaskControl;

volatile int FadeAmount = FADE_NORMAL_INTENSITY;
static volatile int end, XForming;

DWORD* _varPLUTs;
static unsigned int varPLUTsize = VARPLUTS_SIZE;

static BOOLEAN has_colormap = FALSE;
static TFB_Palette cmap_rgb[256];
static int cmap_type = 0;


BOOLEAN TFB_HasColorMap (void)
{
	return has_colormap;
}

int TFB_GetColorMapType (void)
{
	return cmap_type;
}

void TFB_ReleaseColorMap (void)
{
	//fprintf (stderr, "TFB_ReleaseColorMap()\n");
	has_colormap = FALSE;
}

void TFB_ColorMapToRGB (UBYTE *colors)
{
	DWORD i, k, cval;
	UBYTE j, r, g, b;

	for (i = 0; i < 32;)
	{
		cval = MAKE_DWORD (MAKE_WORD (colors[3], colors[2]), MAKE_WORD (colors[1], colors[0]));
		colors += 4;

		r = (UBYTE)((cval >> 26) & 0x1f);
		g = (UBYTE)((cval >> 21) & 0x1f);
		b = (UBYTE)((cval >> 16) & 0x1f);
			
		for (j = 0; j < 8; ++j)
		{
			k = ((j << 5) + i);

			cmap_rgb[k].r = r * (j + 1);
			cmap_rgb[k].g = g * (j + 1);
			cmap_rgb[k].b = b * (j + 1);
		}

		++i;

		r = (UBYTE)((cval >> 10) & 0x1f);
		g = (UBYTE)((cval >> 5) & 0x1f);
		b = (UBYTE)(cval & 0x1f);

		for (j = 0; j < 8; ++j)
		{
			k = ((j << 5) + i);
			cmap_rgb[k].r = r * (j + 1);
			cmap_rgb[k].g = g * (j + 1);
			cmap_rgb[k].b = b * (j + 1);
		}

		++i;
	}
}

BOOLEAN TFB_CopyRGBColorMap (TFB_Palette *dst)
{
	if (cmap_type == TFB_COLORMAP_HYPERSPACE ||
		cmap_type == TFB_COLORMAP_QUASISPACE)
	{
		// TODO: some special colormaps still not implemented
		return FALSE;

	}
	else if (cmap_type == TFB_COLORMAP_PLANET)
	{
		int i;

		for (i = 0; i < 32; ++i)
		{
			dst[251-i].r = cmap_rgb[255-i].r;
			dst[251-i].g = cmap_rgb[255-i].g;
			dst[251-i].b = cmap_rgb[255-i].b;
		}
	}
	else 
	{
		memcpy(dst, cmap_rgb, sizeof(TFB_Palette) * 256);
	}

	return TRUE;
}

BOOLEAN
BatchColorMap (COLORMAPPTR ColorMapPtr)
{
	return (SetColorMap (ColorMapPtr, TFB_COLORMAP_NONE));
}

BOOLEAN
SetColorMap (COLORMAPPTR map, int type)
{
	int start, end, bytes;
	UBYTE *colors = (UBYTE*)map;
	UBYTE *vp;
	
	if (!map)
		return TRUE;

	start = *colors++;
	end = *colors++;
	if (start > end)
		return TRUE;
	
	bytes = (end - start + 1) * PLUT_BYTE_SIZE;

	if (!_varPLUTs)
	{
		if (!(_varPLUTs = (DWORD *) HMalloc (bytes)))
			return TRUE;
		varPLUTsize = bytes;
	}
	
	vp = (UBYTE*)_varPLUTs + (start * PLUT_BYTE_SIZE);
	
	memcpy (vp, colors, bytes);
	cmap_type = type;
	TFB_ColorMapToRGB (vp);
	has_colormap = TRUE;

	//fprintf (stderr, "SetColorMap(): vp %x map %x bytes %d, start %d end %d\n",vp, map, bytes, start, end);
	return TRUE;
}

static int 
xform_clut_task (void *data)
{
	SIZE TDelta, TTotal;
	DWORD CurTime;
	Task task = (Task)data;

	XForming = TRUE;
	while (TaskControl.tc.XFormTask == 0 && (!Task_ReadState (task, TASK_EXIT)))
		TaskSwitch ();
	TTotal = TaskControl.Ticks;
	TaskControl.tc.XFormTask = 0;

	{
		CurTime = GetTimeCounter ();
		do
		{
			DWORD StartTime;

			StartTime = CurTime;
			SleepThreadUntil (CurTime + 2);
			CurTime = GetTimeCounter ();
			if (!XForming || (TDelta = (SIZE)(CurTime - StartTime)) > TTotal)
				TDelta = TTotal;

			FadeAmount += (end - FadeAmount) * TDelta / TTotal;
			//fprintf (stderr, "xform_clut_task FadeAmount %d\n", FadeAmount);
		} while (TTotal -= TDelta && (!Task_ReadState (task, TASK_EXIT)));
	}

	XForming = FALSE;

	FinishTask (task);
	return 0;
}

DWORD
XFormColorMap (COLORMAPPTR ColorMapPtr, SIZE TimeInterval)
{
	BYTE what;
	DWORD TimeOut;

	FlushColorXForms ();

	if (ColorMapPtr == (COLORMAPPTR)0)
		return (0);

	what = *ColorMapPtr;
	switch (what)
	{
	case FadeAllToBlack:
	case FadeSomeToBlack:
		end = FADE_NO_INTENSITY;
		break;
	case FadeAllToColor:
	case FadeSomeToColor:
		end = FADE_NORMAL_INTENSITY;
		break;
	case FadeAllToWhite:
	case FadeSomeToWhite:
		end = FADE_FULL_INTENSITY;
		break;
	default:
		return (GetTimeCounter ());
	}

	TaskControl.Ticks = TimeInterval;
	if (TaskControl.Ticks <= 0 ||
			(TaskControl.tc.XFormTask = AssignTask (xform_clut_task,
					1024, "transform colormap")) == 0)
	{
		FadeAmount = end;
		TimeOut = GetTimeCounter ();
	}
	else
	{
		do
			TaskSwitch ();
		while (TaskControl.tc.XFormTask);

		TimeOut = GetTimeCounter () + TimeInterval + 1;
	}

	return (TimeOut);
}

void
FlushColorXForms()
{
	if (XForming)
	{
		XForming = FALSE;
		TaskSwitch ();
	}
}
