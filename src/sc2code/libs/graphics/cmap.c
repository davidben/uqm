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
#include "libs/log.h"
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
} FadeControl;

typedef struct xform_control
{
	int CMapIndex; // -1 means unused
	COLORMAPPTR CMapPtr;
	SIZE Ticks;
	DWORD StartTime;
	DWORD EndTime;
	TFB_Palette OldCMap[NUMBER_OF_PLUTVALS];
} XFORM_CONTROL;

#define MAX_XFORMS 16
static struct
{
	XFORM_CONTROL TaskControl[MAX_XFORMS];
	volatile int Highest;
			// 'pending' is Highest >= 0
	Mutex Lock;
} XFormControl;

volatile int FadeAmount = FADE_NORMAL_INTENSITY;
static volatile int FadeEnd, XForming;

#define SPARE_COLORMAPS  20
#define MAP_POOL_SIZE    (MAX_COLORMAPS + SPARE_COLORMAPS)

static TFB_ColorMap mappool[MAP_POOL_SIZE];
static TFB_ColorMap *poolhead;

static TFB_ColorMap * colormaps[MAX_COLORMAPS];
static int mapcount;
Mutex maplock;


void
InitColorMaps (void)
{
	int i;

	// init colormaps
	maplock = CreateMutex ("Colormaps Lock", SYNC_CLASS_TOPLEVEL | SYNC_CLASS_VIDEO);
	// init static pool
	for (i = 0; i < MAP_POOL_SIZE - 1; ++i)
		mappool[i].next = mappool + i + 1;
	mappool[i].next = NULL;
	poolhead = mappool;

	// init xform control
	XFormControl.Highest = -1;
	XFormControl.Lock = CreateMutex ("Transform Lock", SYNC_CLASS_TOPLEVEL | SYNC_CLASS_VIDEO);
	for (i = 0; i < MAX_XFORMS; ++i)
		XFormControl.TaskControl[i].CMapIndex = -1;
}

void
UninitColorMaps (void)
{
	// uninit xform control
	DestroyMutex (XFormControl.Lock);
	
	// uninit colormaps
	DestroyMutex (maplock);
}

static inline TFB_ColorMap *
alloc_colormap (void)
		// returns an addrefed object
{
	TFB_ColorMap *map;

	if (!poolhead)
		return NULL;

	map = poolhead;
	poolhead = map->next;
	
	map->next = NULL;
	map->index = -1;
	map->refcount = 1;
	map->version = 0;

	return map;
}

static TFB_ColorMap *
clone_colormap (TFB_ColorMap *from, int index)
		// returns an addrefed object
{
	TFB_ColorMap *map;

	map = alloc_colormap ();
	if (!map)
	{
		static DWORD NextTime = 0;
		DWORD Now;

		if (!from)
		{
			log_add (log_Warning, "FATAL: clone_colormap(): "
					"no maps available");
			exit (EXIT_FAILURE);
		}
		
		Now = GetTimeCounter ();
		if (Now >= NextTime)
		{
			log_add (log_Warning, "clone_colormap(): static pool exhausted");
			NextTime = Now + ONE_SECOND;
		}	
		
		// just overwrite the current one -- better than aborting
		map = from;
		from->refcount++;
	}
	else
	{	// fresh new map
		map->index = index;
		if (from)
			map->version = from->version;
	}
	map->version++;
	
	return map;
}

static inline void
free_colormap (TFB_ColorMap *map)
{
	if (!map)
	{
		log_add (log_Warning, "free_colormap(): tried to free a NULL map");
		return;
	}

	map->next = poolhead;
	poolhead = map;
}

static inline TFB_ColorMap *
get_colormap (int index)
{
	TFB_ColorMap *map;

	map = colormaps[index];
	if (!map)
	{
		log_add (log_Fatal, "BUG: get_colormap(): map not present");
		exit (EXIT_FAILURE);
	}

	map->refcount++;
	return map;
}

static inline void
release_colormap (TFB_ColorMap *map)
{
	if (!map)
		return;

	if (map->refcount <= 0)
	{
		log_add (log_Warning, "BUG: release_colormap(): refcount not >0");
		return;
	}

	map->refcount--;
	if (map->refcount == 0)
		free_colormap (map);
}

void
TFB_ReturnColorMap (TFB_ColorMap *map)
{
	LockMutex (maplock);
	release_colormap (map);
	UnlockMutex (maplock);
}

TFB_ColorMap *
TFB_GetColorMap (int index)
{
	TFB_ColorMap *map;

	LockMutex (maplock);
	map = get_colormap (index);
	UnlockMutex (maplock);

	return map;
}

void
TFB_ColorMapToRGB (TFB_Palette *pal, int index)
{
	TFB_ColorMap *map = NULL;

	if (index < mapcount)
		map = colormaps[index];

	if (!map)
	{
		log_add (log_Warning, "TFB_ColorMapToRGB(): "
				"requested non-present colormap %d", index);
		return;
	}

	memcpy (pal, map->colors, sizeof (map->colors));
}

BOOLEAN
SetColorMap (COLORMAPPTR map)
{
	int start, end;
	int total_size;
	UBYTE *colors = (UBYTE*)map;
	TFB_ColorMap **mpp;
	
	if (!map)
		return TRUE;

	start = *colors++;
	end = *colors++;
	if (start > end)
	{
		log_add (log_Warning, "ERROR: SetColorMap(): "
				"starting map (%d) not less or eq ending (%d)",
				start, end);
		return FALSE;
	}
	if (start >= MAX_COLORMAPS)
	{
		log_add (log_Warning, "ERROR: SetColorMap(): "
				"starting map (%d) beyond range (0-%d)",
				start, (int)MAX_COLORMAPS - 1);
		return FALSE;
	}
	if (end >= MAX_COLORMAPS)
	{
		log_add (log_Warning, "SetColorMap(): "
				"ending map (%d) beyond range (0-%d)\n",
				end, (int)MAX_COLORMAPS - 1);
		end = MAX_COLORMAPS - 1;
	}

	total_size = end + 1;

	LockMutex (maplock);

	if (total_size > mapcount)
		mapcount = total_size;
	
	// parse the supplied PLUTs into our colormaps
	for (mpp = colormaps + start; start <= end; ++start, ++mpp)
	{
		int i;
		TFB_Palette *pal;
		TFB_ColorMap *newmap;
		TFB_ColorMap *oldmap;

		oldmap = *mpp;
		newmap = clone_colormap (oldmap, start);
		
		for (i = 0, pal = newmap->colors; i < NUMBER_OF_PLUTVALS; ++i, ++pal)
		{
			pal->r = *colors++;
			pal->g = *colors++;
			pal->b = *colors++;
		}

		*mpp = newmap;
		release_colormap (oldmap);
	}

	UnlockMutex (maplock);

	return TRUE;
}

/* Fade Transforms */

static int 
fade_xform_task (void *data)
{
	SIZE TDelta, TTotal;
	DWORD CurTime;
	Task task = (Task)data;

	XForming = TRUE;
	while (FadeControl.tc.XFormTask == 0
			&& (!Task_ReadState (task, TASK_EXIT)))
		TaskSwitch ();
	TTotal = FadeControl.Ticks;
	FadeControl.tc.XFormTask = 0;

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

			FadeAmount += (FadeEnd - FadeAmount) * TDelta / TTotal;
			//log_add (log_Debug, "fade_xform_task FadeAmount %d\n", FadeAmount);
		} while ((TTotal -= TDelta) && (!Task_ReadState (task, TASK_EXIT)));
	}

	XForming = FALSE;

	FinishTask (task);
	return 0;
}

static void
FlushFadeXForms (void)
{
	if (XForming)
	{
		XForming = FALSE;
		TaskSwitch ();
	}
}

static DWORD
XFormFade (COLORMAPPTR ColorMapPtr, SIZE TimeInterval)
{
	BYTE what;
	DWORD TimeOut;

	FlushFadeXForms ();

	what = *ColorMapPtr;
	switch (what)
	{
	case FadeAllToBlack:
	case FadeSomeToBlack:
		FadeEnd = FADE_NO_INTENSITY;
		break;
	case FadeAllToColor:
	case FadeSomeToColor:
		FadeEnd = FADE_NORMAL_INTENSITY;
		break;
	case FadeAllToWhite:
	case FadeSomeToWhite:
		FadeEnd = FADE_FULL_INTENSITY;
		break;
	default:
		return (GetTimeCounter ());
	}

	FadeControl.Ticks = TimeInterval;
	if (FadeControl.Ticks <= 0 ||
			(FadeControl.tc.XFormTask = AssignTask (fade_xform_task,
					1024, "fade transform")) == 0)
	{
		FadeAmount = FadeEnd;
		TimeOut = GetTimeCounter ();
	}
	else
	{
		do
			TaskSwitch ();
		while (FadeControl.tc.XFormTask);

		TimeOut = GetTimeCounter () + TimeInterval + 1;
	}

	return (TimeOut);
}

/* Colormap Transforms */

static void
finish_colormap_xform (int which)
{
	SetColorMap (XFormControl.TaskControl[which].CMapPtr);
	XFormControl.TaskControl[which].CMapIndex = -1;
	// check Highest ptr
	if (which == XFormControl.Highest)
	{
		do
			--which;
		while (which >= 0 && XFormControl.TaskControl[which].CMapIndex == -1);
		
		XFormControl.Highest = which;
	}
}


/* This gives the XFormColorMap task a timeslice to do its thing
 * Only one thread should ever be allowed to be calling this at any time
 */
BOOLEAN
XFormColorMap_step (void)
{
	BOOLEAN Changed = FALSE;
	int x;
	DWORD Now = GetTimeCounter ();

	LockMutex (XFormControl.Lock);

	for (x = 0; x <= XFormControl.Highest; ++x)
	{
		XFORM_CONTROL *control = &XFormControl.TaskControl[x];
		int index = control->CMapIndex;
		int TicksLeft = control->EndTime - Now;
		TFB_ColorMap *curmap;

		if (index < 0)
			continue; // unused slot

		LockMutex (maplock);

		curmap = colormaps[index];
		if (!curmap)
		{
			UnlockMutex (maplock);
			log_add (log_Error, "BUG: XFormColorMap_step(): no current map");
			finish_colormap_xform (x);
			continue;
		}

		if (TicksLeft > 0)
		{
#define XFORM_SCALE 0x10000
			TFB_ColorMap *newmap = NULL;
			UBYTE *pNewCMap;
			TFB_Palette *pCurCMap, *pOldCMap;
			int frac;
			int i;

			newmap = clone_colormap (curmap, index);

			pCurCMap = newmap->colors;
			pOldCMap = control->OldCMap;
			pNewCMap = (UBYTE*)control->CMapPtr + 2;

			frac = (int)(control->Ticks - TicksLeft) * XFORM_SCALE
					/ control->Ticks;

			for (i = 0; i < NUMBER_OF_PLUTVALS; i++, ++pCurCMap, ++pOldCMap)
			{
				
				pCurCMap->r = (UBYTE)(pOldCMap->r + ((int)*pNewCMap - pOldCMap->r)
							* frac / XFORM_SCALE);
				pNewCMap++;

				pCurCMap->g = (UBYTE)(pOldCMap->g + ((int)*pNewCMap - pOldCMap->g)
							* frac / XFORM_SCALE);
				pNewCMap++;
				
				pCurCMap->b = (UBYTE)(pOldCMap->b + ((int)*pNewCMap - pOldCMap->b)
							* frac / XFORM_SCALE);
				pNewCMap++;
			}

			colormaps[index] = newmap;
			release_colormap (curmap);
		}

		UnlockMutex (maplock);

		if (TicksLeft <= 0)
		{	// asked for immediate xform or already done
			finish_colormap_xform (x);
		}
		
		Changed = TRUE;
	}

	UnlockMutex (XFormControl.Lock);

	return Changed;
}

static void
FlushPLUTXForms (void)
{
	int i;

	LockMutex (XFormControl.Lock);

	for (i = 0; i <= XFormControl.Highest; ++i)
	{
		if (XFormControl.TaskControl[i].CMapIndex >= 0)
			finish_colormap_xform (i);
	}
	XFormControl.Highest = -1; // all gone

	UnlockMutex (XFormControl.Lock);
}

static DWORD
XFormPLUT (COLORMAPPTR ColorMapPtr, SIZE TimeInterval)
{
	TFB_ColorMap *map;
	XFORM_CONTROL *control;
	int index;
	int x;
	int first_avail = -1;
	DWORD EndTime;
	DWORD Now;

	Now = GetTimeCounter ();
	index = *ColorMapPtr;

	LockMutex (XFormControl.Lock);
	// Find an available slot, or reuse if required
	for (x = 0; x <= XFormControl.Highest
			&& index != XFormControl.TaskControl[x].CMapIndex;
			++x)
	{
		if (first_avail == -1 && XFormControl.TaskControl[x].CMapIndex == -1)
			first_avail = x;
	}

	if (index == XFormControl.TaskControl[x].CMapIndex)
	{	// already xforming this colormap -- cancel and reuse slot
		finish_colormap_xform (x);
	}
	else if (first_avail >= 0)
	{	// picked up a slot along the way
		x = first_avail;
	}
	else if (x >= MAX_XFORMS)
	{	// flush some xforms if the queue is full
		log_add (log_Debug, "WARNING: XFormPLUT(): no slots available");
		x = XFormControl.Highest;
		finish_colormap_xform (x);
	}
	// take next unused one
	control = &XFormControl.TaskControl[x];
	if (x > XFormControl.Highest)
		XFormControl.Highest = x;

	// make a copy of the current map
	LockMutex (maplock);
	map = colormaps[index];
	if (!map)
	{
		UnlockMutex (maplock);
		UnlockMutex (XFormControl.Lock);
		log_add (log_Warning, "BUG: XFormPLUT(): no current map");
		return (0);
	}
	memcpy (control->OldCMap, map->colors, sizeof (map->colors));
	UnlockMutex (maplock);

	control->CMapIndex = index;
	control->CMapPtr = ColorMapPtr;
	control->Ticks = TimeInterval;
	if (control->Ticks < 0)
		control->Ticks = 0; /* prevent negative fade */
	control->StartTime = Now;
	control->EndTime = EndTime = Now + control->Ticks;

	UnlockMutex (XFormControl.Lock);

	return (EndTime);
}


DWORD
XFormColorMap (COLORMAPPTR ColorMapPtr, SIZE TimeInterval)
{
	int what;

	if (!ColorMapPtr)
		return (0);

	what = *ColorMapPtr;
	if (what >= (int)FadeAllToWhite && what <= (int)FadeSomeToColor)
		return XFormFade (ColorMapPtr, TimeInterval);
	else
		return XFormPLUT (ColorMapPtr, TimeInterval);
}

void
FlushColorXForms (void)
{
	FlushFadeXForms ();
	FlushPLUTXForms ();
}
