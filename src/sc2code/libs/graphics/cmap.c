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

#include "gfx_common.h"
#include "libs/tasklib.h"
#include <string.h>


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

UBYTE* _varPLUTs;
static unsigned int varPLUTsize = 0;


void TFB_ColorMapToRGB (TFB_Palette *pal, int colormap_index)
{
	int i;
	UBYTE *colors;

	colors = GET_VAR_PLUT (colormap_index);

	for (i = 0; i < NUMBER_OF_PLUTVALS; i++)
	{
		pal[i].r = *colors++;
		pal[i].g = *colors++;
		pal[i].b = *colors++;
	}
}

BOOLEAN
SetColorMap (COLORMAPPTR map)
{
	int start, end;
	DWORD size, total_size;
	UBYTE *colors = (UBYTE*)map;
	UBYTE *vp;
	
	if (!map)
		return TRUE;

	start = *colors++;
	end = *colors++;
	if (start > end)
		return TRUE;
	
	size = (end - start + 1) * PLUT_BYTE_SIZE;
	total_size = (end + 1) * PLUT_BYTE_SIZE;

	if (!_varPLUTs)
	{
		_varPLUTs = (UBYTE *) HMalloc (total_size);
		varPLUTsize = total_size;
	}
	else if (total_size > varPLUTsize)
	{
		_varPLUTs = (UBYTE *) HRealloc (_varPLUTs, total_size);
		varPLUTsize = total_size;
	}
	if (!_varPLUTs)
	{
		varPLUTsize = 0;
		return TRUE;
	}
	
	vp = GET_VAR_PLUT (start);
	memcpy (vp, colors, size);

	//fprintf (stderr, "SetColorMap(): vp %x map %x bytes %d, start %d end %d\n", vp, map, bytes, start, end);
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
			SleepThreadUntil (CurTime + ONE_SECOND / 60);
			CurTime = GetTimeCounter ();
			if (!XForming || (TDelta = (SIZE)(CurTime - StartTime)) > TTotal)
				TDelta = TTotal;

			FadeAmount += (end - FadeAmount) * TDelta / TTotal;
			//fprintf (stderr, "xform_clut_task FadeAmount %d\n", FadeAmount);
		} while ((TTotal -= TDelta) && (!Task_ReadState (task, TASK_EXIT)));
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
