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

static struct
{
    COLORMAPPTR CMapPtr;
    SIZE Ticks;
    union
    {
		COUNT NumCycles;
		TASK XFormTask;
    } tc;
} TaskControl;

UBYTE _batch_flags;
DWORD* _varPLUTs;
static unsigned int	varPLUTsize = VARPLUTS_SIZE;

static BOOLEAN has_colormap = FALSE;
static TFB_Palette cmap_rgb[256];
static int cmap_type = 0;


BOOLEAN TFB_HasColorMap ()
{
	return has_colormap;
}

int TFB_GetColorMapType ()
{
	return cmap_type;
}

void TFB_ReleaseColorMap ()
{
	//printf("TFB_ReleaseColorMap()\n");
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

// Status: Implemented
BOOLEAN
BatchColorMap (COLORMAPPTR ColorMapPtr)
{
	return (SetColorMap (ColorMapPtr, TFB_COLORMAP_NONE));
}

// Status: Implemented
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
		if (!(_varPLUTs = (unsigned int*) HMalloc (bytes)))
		    return TRUE;
		varPLUTsize = bytes;
    }
	
    vp = (UBYTE*)_varPLUTs + (start * PLUT_BYTE_SIZE);
    
	memcpy (vp, colors, bytes);
	cmap_type = type;
	TFB_ColorMapToRGB (vp);
	has_colormap = TRUE;

	//printf("SetColorMap(): vp %x map %x bytes %d, start %d end %d\n",vp, map, bytes, start, end);
	return TRUE;
}

void
_threedo_set_colors (UBYTE *colors, unsigned int indices)
{
    int i, start, end;
    //unsigned int *ce;

    start = (int)LOBYTE (indices);
    end = (int)HIBYTE (indices);

    //ce = colorEntries;
    for (i = start; i <= end; i++)
    {
		UBYTE	r, g, b;

		r = (*colors << 2) | (*colors >> 4);
		++colors;
		g = (*colors << 2) | (*colors >> 4);
		++colors;
		b = (*colors << 2) | (*colors >> 4);
		++colors;
	
		//*ce++ = MakeCLUTColorEntry (i, r, g, b);
    }
    
    //ceCt = end - start + 1;

    //_threedo_add_task (TASK_SET_CLUT);
    //_batch_flags |= CYCLE_PENDING;
}

static int
color_cycle (void *foo)
{
	UWORD	color_beg, color_len, color_indices;
	COUNT	Cycles, CycleCount;
	DWORD	TimeIn, SleepTicks;
	COLORMAPPTR	BegMapPtr, CurMapPtr;

	Cycles = CycleCount = TaskControl.tc.NumCycles;
	BegMapPtr = TaskControl.CMapPtr;
	SleepTicks = TaskControl.Ticks;

	color_indices = MAKE_WORD (BegMapPtr[0], BegMapPtr[1]);
	color_beg = *BegMapPtr++ * 3;
	color_len = (*BegMapPtr++ + 1) * 3 - color_beg;
	CurMapPtr = BegMapPtr;

	TaskControl.CMapPtr = 0;
	TimeIn = GetTimeCounter ();
	for (;;)
	{
		//_threedo_set_colors (CurMapPtr, color_indices);

		if (--Cycles)
			CurMapPtr += color_len + sizeof (BYTE) * 2;
		else
		{
			Cycles = CycleCount;
			CurMapPtr = BegMapPtr;
		}

		TimeIn = SleepTask (TimeIn + SleepTicks);

		if (TaskControl.CMapPtr)
		{
			TaskControl.CMapPtr = (COLORMAPPTR)0;
			for (;;)
				TaskSwitch ();
		}
	}
}

CYCLE_REF 
CycleColorMap (COLORMAPPTR ColorMapPtr, COUNT Cycles, SIZE TimeInterval)
{
	if (ColorMapPtr && Cycles && ColorMapPtr[0] <= ColorMapPtr[1])
	{
		TASK	T;

		while (TaskControl.CMapPtr)
		{
			TaskSwitch ();
		}

		TaskControl.CMapPtr = ColorMapPtr;
		TaskControl.tc.NumCycles = Cycles;
		if ((TaskControl.Ticks = TimeInterval) <= 0)
			TaskControl.Ticks = 1;

		if (T = AddTask (color_cycle, 0))
		{
			_batch_flags |= ENABLE_CYCLE;
			do
			TaskSwitch ();
			while (TaskControl.CMapPtr);
		}
		TaskControl.CMapPtr = 0;

		return ((CYCLE_REF)T);
	}

	return (0);
}

void 
StopCycleColorMap (CYCLE_REF CycleRef)
{
	TASK	T;

	if (T = (TASK)CycleRef)
	{
		TaskControl.CMapPtr = (COLORMAPPTR)1;
		while (TaskControl.CMapPtr)
			TaskSwitch ();

		_batch_flags &= ~ENABLE_CYCLE;
		DeleteTask (T);
	}
}

#define NO_INTENSITY		0x00
#define NORMAL_INTENSITY	0xff
#define FULL_INTENSITY		(0xff * 32)

static int cur = NORMAL_INTENSITY, end, XForming;

static
int xform_clut_task (void *foo)
{
	TASK	T;
	SIZE	TDelta, TTotal;
	DWORD	CurTime;

	XForming = TRUE;
	while ((T = TaskControl.tc.XFormTask) == 0)
		TaskSwitch ();
	TTotal = TaskControl.Ticks;
	TaskControl.tc.XFormTask = 0;

	{
		CurTime = GetTimeCounter ();
		do
		{
			DWORD	StartTime;

			StartTime = CurTime;
			CurTime = SleepTask (CurTime + 2);
			if (!XForming || (TDelta = (SIZE)(CurTime - StartTime)) > TTotal)
				TDelta = TTotal;

			cur += (end - cur) * TDelta / TTotal;
			//_threedo_change_clut (cur);
		} while (TTotal -= TDelta);
	}

	XForming = FALSE;
	DeleteTask (T);

	return 0;
}

//Status: Partially implemented
DWORD
XFormColorMap (COLORMAPPTR ColorMapPtr, SIZE TimeInterval)
{
	BYTE	what;
	DWORD	TimeOut;

	//printf ("Partially implemented function activated: XFormColorMap()\n");

	FlushColorXForms ();

	if (ColorMapPtr == (COLORMAPPTR)0)
		return (0);

	switch (what = *ColorMapPtr)
	{
	case FadeAllToBlack:
	case FadeSomeToBlack:
		end = NO_INTENSITY;
		break;
	case FadeAllToColor:
	case FadeSomeToColor:
		end = NORMAL_INTENSITY;
		break;
	case FadeAllToWhite:
	case FadeSomeToWhite:
		end = FULL_INTENSITY;
		break;
	default:
		return (GetTimeCounter ());
	}

	//if (what == FadeAllToBlack || what == FadeAllToWhite || what == FadeAllToColor)
	//_threedo_enable_fade ();

	if ((TaskControl.Ticks = TimeInterval) <= 0
		|| (TaskControl.tc.XFormTask = AddTask (xform_clut_task, 1024)) == 0)
	{
		//_threedo_change_clut (end);
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

//Status: Implemented
void
FlushColorXForms()
{
	//printf ("Partially implemented function activated: FlushColorXForms()\n");

	if (XForming)
	{
		XForming = FALSE;
		TaskSwitch ();
	}
}
