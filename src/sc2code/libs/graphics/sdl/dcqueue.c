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

Semaphore DCQ_sem;

// variables for making the DCQ lock re-entrant
static int DCQ_locking_depth = 0;
static Uint32 DCQ_locking_thread = 0;

// Maximum size of the DCQ.  The larger the DCQ, the larger frameskips
// become tolerable before initiating livelock deterrence and game
// slowdown.  Other constants for controlling the frameskip/slowdown
// balance may be found in sdl_common.c near TFB_FlushGraphics.
#define DCQ_MAX 16384
TFB_DrawCommand DCQ[DCQ_MAX];

TFB_DrawCommandQueue *DrawCommandQueue;

// DCQ Synchronization: SDL-specific implementation of re-entrant
// locks to protect the Draw Command Queue.  Lock is re-entrant to
// allow livelock deterrence to be written much more cleanly.

void
Lock_DCQ (void)
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
	if (!DrawCommandQueue->Batching)
	{
		int front = DrawCommandQueue->Front;
		int back  = DrawCommandQueue->InsertionPoint;
		DrawCommandQueue->Back = DrawCommandQueue->InsertionPoint;
		if (front <= back)
		{
			DrawCommandQueue->Size = (back - front);
		}
		else
		{
			DrawCommandQueue->Size = (back + DCQ_MAX - front);
		}
	}
}

void
TFB_BatchGraphics (void)
{
	Lock_DCQ ();
	DrawCommandQueue->Batching++;
	Unlock_DCQ ();
}

void
TFB_UnbatchGraphics (void)
{
	Lock_DCQ ();
	if (DrawCommandQueue->Batching)
	{
		DrawCommandQueue->Batching--;
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
	Lock_DCQ ();
	DrawCommandQueue->Batching = 0;
	Synchronize_DCQ ();
	Unlock_DCQ ();
}

// Draw Command Queue Stuff
// TODO: Make this be statically allocated, too.  We only ever have one DCQ, after all.

TFB_DrawCommandQueue*
TFB_DrawCommandQueue_Create()
{
		TFB_DrawCommandQueue* myQueue;
		
		myQueue = (TFB_DrawCommandQueue*) HMalloc(
				sizeof(TFB_DrawCommandQueue));

		myQueue->Back = 0;
		myQueue->Front = 0;
		myQueue->InsertionPoint = 0;
		myQueue->Batching = 0;
		myQueue->FullSize = 0;
		myQueue->Size = 0;

		DCQ_sem = CreateSemaphore(1);

		return (myQueue);
}

void
TFB_DrawCommandQueue_Push (TFB_DrawCommandQueue* myQueue,
		TFB_DrawCommand* Command)
{
	Lock_DCQ ();
	if (myQueue->Size < DCQ_MAX - 1)
	{
		DCQ[myQueue->InsertionPoint] = *Command;
		myQueue->InsertionPoint = (myQueue->InsertionPoint + 1) % DCQ_MAX;
		myQueue->FullSize++;
		Synchronize_DCQ ();
	}
	else
	{
		fprintf (stderr, "DCQ overload.  Adjust your livelock deterrence constants!\n");
	}

	Unlock_DCQ ();
}

int
TFB_DrawCommandQueue_Pop (TFB_DrawCommandQueue *myQueue, TFB_DrawCommand *target)
{
	Lock_DCQ ();

	if (myQueue->Size == 0)
	{
		Unlock_DCQ ();
		return (0);
	}

	if (myQueue->Front == myQueue->Back && myQueue->Size != DCQ_MAX)
	{
		fprintf (stderr, "Augh!  Assertion failure in DCQ!  Front == Back, Size != DCQ_MAX\n");
		myQueue->Size = 0;
		Unlock_DCQ ();
		return (0);
	}

	*target = DCQ[myQueue->Front];
	myQueue->Front = (myQueue->Front + 1) % DCQ_MAX;

	myQueue->Size--;
	myQueue->FullSize--;
	Unlock_DCQ ();

	return 1;
}

void
TFB_DrawCommandQueue_Clear (TFB_DrawCommandQueue *myQueue)
{
	Lock_DCQ ();
	myQueue->Size = 0;
	myQueue->Front = 0;
	myQueue->Back = 0;
	myQueue->Batching = 0;
	myQueue->FullSize = 0;
	myQueue->InsertionPoint = 0;
	Unlock_DCQ ();
}

void
TFB_EnqueueDrawCommand (TFB_DrawCommand* DrawCommand)
{
	if (TFB_DEBUG_HALT)
	{
		return;
	}
		
	if (DrawCommand->Type <= TFB_DRAWCOMMANDTYPE_COPYFROMOTHERBUFFER
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

			DC.Type = scissor_rect.extent.width
					? (DC.x = scissor_rect.corner.x,
					DC.y=scissor_rect.corner.y,
					DC.w=scissor_rect.extent.width,
					DC.h=scissor_rect.extent.height),
					TFB_DRAWCOMMANDTYPE_SCISSORENABLE
					: TFB_DRAWCOMMANDTYPE_SCISSORDISABLE;

			DC.image = 0;
			DC.UsePalette = FALSE;
			
			DC.BlendNumerator = BlendNumerator;
			DC.BlendDenominator = BlendDenominator;

			TFB_EnqueueDrawCommand(&DC);
		}
	}

	TFB_DrawCommandQueue_Push (DrawCommandQueue, DrawCommand);
}

#endif
