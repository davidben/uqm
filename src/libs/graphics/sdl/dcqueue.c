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

#include "port.h"
#include "sdl_common.h"
#include "libs/threadlib.h"
#include SDL_INCLUDE(SDL_thread.h)
#include "libs/graphics/drawcmd.h"
#include "libs/graphics/sdl/dcqueue.h"
#include "libs/log.h"


static RecursiveMutex DCQ_Mutex;

CondVar RenderingCond;

TFB_DrawCommand DCQ[DCQ_MAX];

TFB_DrawCommandQueue DrawCommandQueue;

// Wait for the queue to be emptied.
static void
TFB_WaitForSpace (int requested_slots)
{
	int old_depth, i;
	log_add (log_Debug, "DCQ overload (Size = %d, FullSize = %d, "
			"Requested = %d).  Sleeping until renderer is done.",
			DrawCommandQueue.Size, DrawCommandQueue.FullSize,
			requested_slots);
	// Restore the DCQ locking level.  I *think* this is
	// always 1, but...
	TFB_BatchReset ();
	old_depth = GetRecursiveMutexDepth (DCQ_Mutex);
	for (i = 0; i < old_depth; i++)
		UnlockRecursiveMutex (DCQ_Mutex);
	WaitCondVar (RenderingCond);
	for (i = 0; i < old_depth; i++)
		LockRecursiveMutex (DCQ_Mutex);
	log_add (log_Debug, "DCQ clear (Size = %d, FullSize = %d).  Continuing.",
			DrawCommandQueue.Size, DrawCommandQueue.FullSize);
}

void
Lock_DCQ (int slots)
{
	LockRecursiveMutex (DCQ_Mutex);
	while (DrawCommandQueue.FullSize >= DCQ_MAX - slots)
	{
		TFB_WaitForSpace (slots);
	}
}

void
Unlock_DCQ (void)
{
	UnlockRecursiveMutex (DCQ_Mutex);
}

// Always have the DCQ locked when calling this.
static void
Synchronize_DCQ (void)
{
	if (!DrawCommandQueue.Batching)
	{
		int front = DrawCommandQueue.Front;
		int back  = DrawCommandQueue.InsertionPoint;
		DrawCommandQueue.Back = DrawCommandQueue.InsertionPoint;
		if (front <= back)
		{
			DrawCommandQueue.Size = (back - front);
		}
		else
		{
			DrawCommandQueue.Size = (back + DCQ_MAX - front);
		}
		DrawCommandQueue.FullSize = DrawCommandQueue.Size;
	}
}

void
TFB_BatchGraphics (void)
{
	LockRecursiveMutex (DCQ_Mutex);
	DrawCommandQueue.Batching++;
	UnlockRecursiveMutex (DCQ_Mutex);
}

void
TFB_UnbatchGraphics (void)
{	
	LockRecursiveMutex (DCQ_Mutex);
	if (DrawCommandQueue.Batching)
	{
		DrawCommandQueue.Batching--;
	}
	Synchronize_DCQ ();
	UnlockRecursiveMutex (DCQ_Mutex);
}

// Cancel all pending batch operations, making them unbatched.  This will
// cause a small amount of flicker when invoked, but prevents 
// batching problems from freezing the game.
void
TFB_BatchReset (void)
{
	LockRecursiveMutex (DCQ_Mutex);
	DrawCommandQueue.Batching = 0;
	Synchronize_DCQ ();
	UnlockRecursiveMutex (DCQ_Mutex);
}


// Draw Command Queue Stuff

void
Init_DrawCommandQueue (void)
{
	DrawCommandQueue.Back = 0;
	DrawCommandQueue.Front = 0;
	DrawCommandQueue.InsertionPoint = 0;
	DrawCommandQueue.Batching = 0;
	DrawCommandQueue.FullSize = 0;
	DrawCommandQueue.Size = 0;

	DCQ_Mutex = CreateRecursiveMutex ("DCQ", SYNC_CLASS_TOPLEVEL | SYNC_CLASS_VIDEO);
}

void
Uninit_DrawCommandQueue (void)
{
	DestroyRecursiveMutex (DCQ_Mutex);
}

void
TFB_DrawCommandQueue_Push (TFB_DrawCommand* Command)
{
	Lock_DCQ (1);
	DCQ[DrawCommandQueue.InsertionPoint] = *Command;
	DrawCommandQueue.InsertionPoint = (DrawCommandQueue.InsertionPoint + 1)
			% DCQ_MAX;
	DrawCommandQueue.FullSize++;
	Synchronize_DCQ ();
	Unlock_DCQ ();
}

int
TFB_DrawCommandQueue_Pop (TFB_DrawCommand *target)
{
	LockRecursiveMutex (DCQ_Mutex);

	if (DrawCommandQueue.Size == 0)
	{
		Unlock_DCQ ();
		return (0);
	}

	if (DrawCommandQueue.Front == DrawCommandQueue.Back &&
			DrawCommandQueue.Size != DCQ_MAX)
	{
		log_add (log_Debug, "Augh!  Assertion failure in DCQ!  "
				"Front == Back, Size != DCQ_MAX");
		DrawCommandQueue.Size = 0;
		Unlock_DCQ ();
		return (0);
	}

	*target = DCQ[DrawCommandQueue.Front];
	DrawCommandQueue.Front = (DrawCommandQueue.Front + 1) % DCQ_MAX;

	DrawCommandQueue.Size--;
	DrawCommandQueue.FullSize--;
	UnlockRecursiveMutex (DCQ_Mutex);

	return 1;
}

void
TFB_DrawCommandQueue_Clear ()
{
	LockRecursiveMutex (DCQ_Mutex);
	DrawCommandQueue.Size = 0;
	DrawCommandQueue.Front = 0;
	DrawCommandQueue.Back = 0;
	DrawCommandQueue.Batching = 0;
	DrawCommandQueue.FullSize = 0;
	DrawCommandQueue.InsertionPoint = 0;
	UnlockRecursiveMutex (DCQ_Mutex);
}

void
TFB_EnqueueDrawCommand (TFB_DrawCommand* DrawCommand)
{
	if (TFB_DEBUG_HALT)
	{
		return;
	}

	if (DrawCommand->Type <= TFB_DRAWCOMMANDTYPE_COPYTOIMAGE
			&& _CurFramePtr->Type == SCREEN_DRAWABLE)
	{
		static RECT scissor_rect;

		// Set the clipping region.
		if (scissor_rect.corner.x != _pCurContext->ClipRect.corner.x
				|| scissor_rect.corner.y != _pCurContext->ClipRect.corner.y
				|| scissor_rect.extent.width !=
				_pCurContext->ClipRect.extent.width
				|| scissor_rect.extent.height !=
				_pCurContext->ClipRect.extent.height)
		{
			// Enqueue command to set the glScissor spec
			TFB_DrawCommand DC;

			scissor_rect = _pCurContext->ClipRect;

			if (scissor_rect.extent.width)
			{
				DC.Type = TFB_DRAWCOMMANDTYPE_SCISSORENABLE;
				DC.data.scissor.x = scissor_rect.corner.x;
				DC.data.scissor.y = scissor_rect.corner.y;
				DC.data.scissor.w = scissor_rect.extent.width;
				DC.data.scissor.h = scissor_rect.extent.height;
			}
			else
			{
				DC.Type = TFB_DRAWCOMMANDTYPE_SCISSORDISABLE;
			}
				
			TFB_EnqueueDrawCommand(&DC);
		}
	}

	TFB_DrawCommandQueue_Push (DrawCommand);
}

#endif


