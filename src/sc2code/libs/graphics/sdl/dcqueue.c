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
#include "libs/threadlib.h"
#include "SDL_thread.h"
#include "libs/graphics/drawcmd.h"
#include "libs/graphics/sdl/dcqueue.h"

Semaphore DCQ_sem;

// variables for making the DCQ lock re-entrant
static int DCQ_locking_depth = 0;
static Uint32 DCQ_locking_thread = 0;

TFB_DrawCommand DCQ[DCQ_MAX];

TFB_DrawCommandQueue DrawCommandQueue;

// DCQ Synchronization: SDL-specific implementation of re-entrant
// locks to protect the Draw Command Queue.  Lock is re-entrant to
// allow livelock deterrence to be written much more cleanly.

static void
_lock (void)
{
	Uint32 current_thread = SDL_ThreadID ();
	if (DCQ_locking_thread != current_thread)
	{
		SetSemaphore (DCQ_sem);
		DCQ_locking_thread = current_thread;
	}
	++DCQ_locking_depth;
	// fprintf (stderr, "DCQ_sem locking depth: %i\n", DCQ_locking_depth);
}

// Wait for the queue to be emptied.
static void
TFB_WaitForSpace (int requested_slots)
{
	int old_depth, i;
	fprintf (stderr, "DCQ overload (Size = %d, FullSize = %d, Requested = %d).  Sleeping until renderer is done.\n", DrawCommandQueue.Size, DrawCommandQueue.FullSize, requested_slots);
	// Restore the DCQ locking level.  I *think* this is
	// always 1, but...
	TFB_BatchReset ();
	old_depth = DCQ_locking_depth;
	for (i = 0; i < old_depth; i++)
		Unlock_DCQ ();
	WaitCondVar (RenderingCond);
	for (i = 0; i < old_depth; i++)
		_lock ();
	fprintf (stderr, "DCQ clear (Size = %d, FullSize = %d).  Continuing.\n", DrawCommandQueue.Size, DrawCommandQueue.FullSize);
}

void
Lock_DCQ (int slots)
{
	_lock ();
	while (DrawCommandQueue.FullSize >= DCQ_MAX - slots)
	{
		TFB_WaitForSpace (slots);
	}
}

void
Unlock_DCQ (void)
{
	Uint32 current_thread = SDL_ThreadID ();
	if (DCQ_locking_thread != current_thread)
	{
		fprintf (stderr, "%8x attempted to unlock the DCQ when it didn't hold it!\n", current_thread);
	}
	else
	{
		--DCQ_locking_depth;
		// fprintf (stderr, "DCQ_sem locking depth: %i\n", DCQ_locking_depth);
		if (!DCQ_locking_depth)
		{
			DCQ_locking_thread = 0;
			ClearSemaphore (DCQ_sem);
		}
	}
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
	_lock ();
	DrawCommandQueue.Batching++;
	Unlock_DCQ ();
}

void
TFB_UnbatchGraphics (void)
{
	_lock ();
	if (DrawCommandQueue.Batching)
	{
		DrawCommandQueue.Batching--;
	}
	Synchronize_DCQ ();
	Unlock_DCQ ();
}

// Cancel all pending batch operations, making them unbatched.  This will
// cause a small amount of flicker when invoked, but prevents 
// batching problems from freezing the game.
void
TFB_BatchReset (void)
{
	_lock ();
	DrawCommandQueue.Batching = 0;
	Synchronize_DCQ ();
	Unlock_DCQ ();
}


// Draw Command Queue Stuff

void
TFB_DrawCommandQueue_Create()
{
		DrawCommandQueue.Back = 0;
		DrawCommandQueue.Front = 0;
		DrawCommandQueue.InsertionPoint = 0;
		DrawCommandQueue.Batching = 0;
		DrawCommandQueue.FullSize = 0;
		DrawCommandQueue.Size = 0;
		DCQ_locking_depth = 0;
		DCQ_locking_thread = 0;

		DCQ_sem = CreateSemaphore(1, "DCQ");
}

void
TFB_DrawCommandQueue_Push (TFB_DrawCommand* Command)
{
	Lock_DCQ (1);
	DCQ[DrawCommandQueue.InsertionPoint] = *Command;
	DrawCommandQueue.InsertionPoint = (DrawCommandQueue.InsertionPoint + 1) % DCQ_MAX;
	DrawCommandQueue.FullSize++;
	Synchronize_DCQ ();
	Unlock_DCQ ();
}

int
TFB_DrawCommandQueue_Pop (TFB_DrawCommand *target)
{
	_lock ();

	if (DrawCommandQueue.Size == 0)
	{
		Unlock_DCQ ();
		return (0);
	}

	if (DrawCommandQueue.Front == DrawCommandQueue.Back && DrawCommandQueue.Size != DCQ_MAX)
	{
		fprintf (stderr, "Augh!  Assertion failure in DCQ!  Front == Back, Size != DCQ_MAX\n");
		DrawCommandQueue.Size = 0;
		Unlock_DCQ ();
		return (0);
	}

	*target = DCQ[DrawCommandQueue.Front];
	DrawCommandQueue.Front = (DrawCommandQueue.Front + 1) % DCQ_MAX;

	DrawCommandQueue.Size--;
	DrawCommandQueue.FullSize--;
	Unlock_DCQ ();

	return 1;
}

void
TFB_DrawCommandQueue_Clear ()
{
	_lock ();
	DrawCommandQueue.Size = 0;
	DrawCommandQueue.Front = 0;
	DrawCommandQueue.Back = 0;
	DrawCommandQueue.Batching = 0;
	DrawCommandQueue.FullSize = 0;
	DrawCommandQueue.InsertionPoint = 0;
	Unlock_DCQ ();
}

void
TFB_EnqueueDrawCommand (TFB_DrawCommand* DrawCommand)
{
	if (TFB_DEBUG_HALT)
	{
		return;
	}

	if (DrawCommand->Type == TFB_DRAWCOMMANDTYPE_SENDSIGNAL)
		DrawCommand->data.sendsignal.thread = CurrentThreadID ();

	if (DrawCommand->Type <= TFB_DRAWCOMMANDTYPE_COPYTOIMAGE
			&& TYPE_GET (_CurFramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
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
