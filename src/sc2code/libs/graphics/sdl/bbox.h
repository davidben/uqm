#include "gfxlib.h"
#include "graphics/tfb_draw.h"
#include "graphics/gfx_common.h"

#ifndef _BBOX_H_
#define _BBOX_H_

/* Bounding Box operations.  These operations are NOT synchronized.
 * However, they should only be accessed by TFB_FlushGraphics and
 * TFB_SwapBuffers, or the routines that they exclusively call -- all
 * of which are only callable by the thread that is permitted to touch
 * the screen.  No explicit locks should therefore be required. */

typedef struct {
	int valid;   // If zero, the next point registered becomes the region
	RECT region; // The actual modified rectangle
	RECT clip;   // Points outside of this rectangle are pushed to
		     // the closest border point
} TFB_BoundingBox;

extern TFB_BoundingBox TFB_BBox;

void TFB_BBox_RegisterPoint (int x, int y);
void TFB_BBox_RegisterRect (PRECT r);
void TFB_BBox_RegisterCanvas (TFB_Canvas c, int x, int y);

void TFB_BBox_Reset (void);
void TFB_BBox_GetClipRect (TFB_Canvas c);

#endif
