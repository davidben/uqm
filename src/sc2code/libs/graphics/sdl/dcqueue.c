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

SDL_mutex *DCQ_mutex;

#define DCQ_MAX 4096
TFB_DrawCommand DCQ[DCQ_MAX];

TFB_DrawCommandQueue *DrawCommandQueue;


// Draw Command Queue Stuff

TFB_DrawCommandQueue*
TFB_DrawCommandQueue_Create()
{
		TFB_DrawCommandQueue* myQueue;
		
		myQueue = (TFB_DrawCommandQueue*) HMalloc(
				sizeof(TFB_DrawCommandQueue));

		myQueue->Back = 0;
		myQueue->Front = 0;
		myQueue->Size = 0;

		DCQ_mutex = SDL_CreateMutex();

		return (myQueue);
}

void
TFB_DrawCommandQueue_Push (TFB_DrawCommandQueue* myQueue,
		TFB_DrawCommand* Command)
{
	SDL_mutexP(DCQ_mutex);
	if (myQueue->Size < DCQ_MAX)
	{
		DCQ[myQueue->Back] = *Command;
		myQueue->Back = (myQueue->Back + 1) % DCQ_MAX;
		myQueue->Size++;
	}
	else
	{
		TFB_DeallocateDrawCommand(Command);
	}

	SDL_mutexV(DCQ_mutex);
}

int
TFB_DrawCommandQueue_Pop (TFB_DrawCommandQueue *myQueue, TFB_DrawCommand *target)
{
	SDL_mutexP(DCQ_mutex);

	if (myQueue->Size == 0)
	{
		SDL_mutexV(DCQ_mutex);
		return (0);
	}

	if (myQueue->Front == myQueue->Back && myQueue->Size != DCQ_MAX)
	{
		printf("Augh!  Assertion failure in DCQ!  Front == Back, Size != DCQ_MAX\n");
		myQueue->Size = 0;
		SDL_mutexV(DCQ_mutex);
		return (0);
	}

	*target = DCQ[myQueue->Front];
	myQueue->Front = (myQueue->Front + 1) % DCQ_MAX;

	myQueue->Size--;
	SDL_mutexV(DCQ_mutex);

	return 1;
}

void
TFB_DeallocateDrawCommand (TFB_DrawCommand* Command)
{
	//HFree(Command);
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
			TFB_DrawCommand DC_auto;
			TFB_DrawCommand* DC = &DC_auto;

			scissor_rect = _pCurContext->ClipRect;

			DC->Type = scissor_rect.extent.width
					? (DC->x = scissor_rect.corner.x,
					DC->y=scissor_rect.corner.y,
					DC->w=scissor_rect.extent.width,
					DC->h=scissor_rect.extent.height),
					TFB_DRAWCOMMANDTYPE_SCISSORENABLE
					: TFB_DRAWCOMMANDTYPE_SCISSORDISABLE;

			DC->image = 0;
			
			TFB_EnqueueDrawCommand(DC);
		}
	}

	TFB_DrawCommandQueue_Push (DrawCommandQueue, DrawCommand);
}

#endif
