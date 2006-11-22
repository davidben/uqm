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

#include "colors.h"
#include "controls.h"
#include "encount.h"
#include "gameopt.h"
#include "gamestr.h"
#include "globdata.h"
#include "options.h"
#include "scan.h"
#include "shipcont.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "state.h"
#include "uqmdebug.h"
#include "libs/inplib.h"
#include "libs/strlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"



#define UNIVERSE_TO_DISPX(ux) \
		(COORD)(((((long)(ux) - pMenuState->flash_rect1.corner.x) \
		<< LOBYTE (pMenuState->delta_item)) \
		* SIS_SCREEN_WIDTH / (MAX_X_UNIVERSE + 1)) + ((SIS_SCREEN_WIDTH - 1) >> 1))
#define UNIVERSE_TO_DISPY(uy) \
		(COORD)(((((long)pMenuState->flash_rect1.corner.y - (uy)) \
		<< LOBYTE (pMenuState->delta_item)) \
		* SIS_SCREEN_HEIGHT / (MAX_Y_UNIVERSE + 1)) + ((SIS_SCREEN_HEIGHT - 1) >> 1))
#define DISP_TO_UNIVERSEX(dx) \
		((COORD)((((long)((dx) - ((SIS_SCREEN_WIDTH - 1) >> 1)) \
		* (MAX_X_UNIVERSE + 1)) >> LOBYTE (pMenuState->delta_item)) \
		/ SIS_SCREEN_WIDTH) + pMenuState->flash_rect1.corner.x)
#define DISP_TO_UNIVERSEY(dy) \
		((COORD)((((long)(((SIS_SCREEN_HEIGHT - 1) >> 1) - (dy)) \
		* (MAX_Y_UNIVERSE + 1)) >> LOBYTE (pMenuState->delta_item)) \
		/ SIS_SCREEN_HEIGHT) + pMenuState->flash_rect1.corner.y)

static BOOLEAN transition_pending;

static int
flash_cursor_func (void *data)
{
	BYTE c, val;
	POINT universe;
	STAMP s;
	Task task = (Task) data;

	if (LOBYTE (GLOBAL (CurrentActivity)) != IN_HYPERSPACE)
		universe = CurStarDescPtr->star_pt;
	else
	{
		universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	}
	s.frame = IncFrameIndex (pMenuState->CurFrame);

	c = 0x00;
	val = -0x02;
	while (!Task_ReadState(task, TASK_EXIT))
	{
		DWORD TimeIn;
		COLOR OldColor;
		CONTEXT OldContext;

		TimeIn = GetTimeCounter ();
		LockMutex (GraphicsLock);
		OldContext = SetContext (SpaceContext);

		if (c == 0x00 || c == 0x1A)
			val = -val;
		c += val;
		OldColor = SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (c, c, c), c));
		s.origin.x = UNIVERSE_TO_DISPX (universe.x);
		s.origin.y = UNIVERSE_TO_DISPY (universe.y);
		DrawFilledStamp (&s);
		SetContextForeGroundColor (OldColor);

		SetContext (OldContext);
		UnlockMutex (GraphicsLock);
		SleepThreadUntil (TimeIn + (ONE_SECOND >> 4));
	}
	FinishTask (task);
	return (0);
}

static void
DrawCursor (COORD curs_x, COORD curs_y)
{
	STAMP s;

	s.origin.x = curs_x;
	s.origin.y = curs_y;
	s.frame = pMenuState->CurFrame;

	DrawStamp (&s);
}

static void
DrawAutoPilot (PPOINT pDstPt)
{
	SIZE dx, dy,
				xincr, yincr,
				xerror, yerror,
				cycle, delta;
	POINT pt;

	if (LOBYTE (GLOBAL (CurrentActivity)) != IN_HYPERSPACE)
		pt = CurStarDescPtr->star_pt;
	else
	{
		pt.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		pt.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	}
	pt.x = UNIVERSE_TO_DISPX (pt.x);
	pt.y = UNIVERSE_TO_DISPY (pt.y);

	dx = UNIVERSE_TO_DISPX (pDstPt->x) - pt.x;
	if (dx >= 0)
		xincr = 1;
	else
	{
		xincr = -1;
		dx = -dx;
	}
	dx <<= 1;

	dy = UNIVERSE_TO_DISPY (pDstPt->y) - pt.y;
	if (dy >= 0)
		yincr = 1;
	else
	{
		yincr = -1;
		dy = -dy;
	}
	dy <<= 1;

	if (dx >= dy)
		cycle = dx;
	else
		cycle = dy;
	delta = xerror = yerror = cycle >> 1;

	SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x04, 0x04, 0x1F), 0x01));

	delta &= ~1;
	while (delta--)
	{
		if (!(delta & 1))
			DrawPoint (&pt);

		if ((xerror -= dx) <= 0)
		{
			pt.x += xincr;
			xerror += cycle;
		}
		if ((yerror -= dy) <= 0)
		{
			pt.y += yincr;
			yerror += cycle;
		}
	}
}

static void
GetSphereRect (EXTENDED_SHIP_FRAGMENTPTR StarShipPtr, PRECT pRect, PRECT
		pRepairRect)
{
	long diameter;

	diameter = (long)(
			StarShipPtr->ShipInfo.known_strength
			* SPHERE_RADIUS_INCREMENT
			);
	pRect->extent.width = UNIVERSE_TO_DISPX (diameter)
			- UNIVERSE_TO_DISPX (0);
	if (pRect->extent.width < 0)
		pRect->extent.width = -pRect->extent.width;
	else if (pRect->extent.width == 0)
		pRect->extent.width = 1;
	pRect->extent.height = UNIVERSE_TO_DISPY (diameter)
			- UNIVERSE_TO_DISPY (0);
	if (pRect->extent.height < 0)
		pRect->extent.height = -pRect->extent.height;
	else if (pRect->extent.height == 0)
		pRect->extent.height = 1;

	pRect->corner.x = UNIVERSE_TO_DISPX (
			StarShipPtr->ShipInfo.known_loc.x
			);
	pRect->corner.y = UNIVERSE_TO_DISPY (
			StarShipPtr->ShipInfo.known_loc.y
			);
	pRect->corner.x -= pRect->extent.width >> 1;
	pRect->corner.y -= pRect->extent.height >> 1;

	{
		TEXT t;
		STRING locString;

		SetContextFont (TinyFont);

		t.baseline.x = pRect->corner.x + (pRect->extent.width >> 1);
		t.baseline.y = pRect->corner.y + (pRect->extent.height >> 1) - 1;
		t.align = ALIGN_CENTER;
		locString = SetAbsStringTableIndex (
				StarShipPtr->ShipInfo.race_strings, 1
				);
		t.CharCount = GetStringLength (locString);
		t.pStr = (UNICODE *)GetStringAddress (locString);
		TextRect (&t, pRepairRect, NULL_PTR);
		
		if (pRepairRect->corner.x <= 0)
			pRepairRect->corner.x = 1;
		else if (pRepairRect->corner.x + pRepairRect->extent.width >= SIS_SCREEN_WIDTH)
			pRepairRect->corner.x = SIS_SCREEN_WIDTH - pRepairRect->extent.width - 1;
		if (pRepairRect->corner.y <= 0)
			pRepairRect->corner.y = 1;
		else if (pRepairRect->corner.y + pRepairRect->extent.height >= SIS_SCREEN_HEIGHT)
			pRepairRect->corner.y = SIS_SCREEN_HEIGHT - pRepairRect->extent.height - 1;

		BoxUnion (pRepairRect, pRect, pRepairRect);
		pRepairRect->extent.width++;
		pRepairRect->extent.height++;
	}
}

static void
DrawStarMap (COUNT race_update, PRECT pClipRect)
{
#define GRID_DELTA 500
	SIZE i;
	COUNT which_space;
	long diameter;
	RECT r, old_r;
	STAMP s;
	FRAME star_frame;
	STAR_DESCPTR SDPtr;
	BOOLEAN draw_cursor;

	if (pClipRect == (PRECT)-1)
	{
		pClipRect = 0;
		draw_cursor = FALSE;
	}
	else
	{
		LockMutex (GraphicsLock);
		draw_cursor = TRUE;
	}

	SetContext (SpaceContext);
	if (pClipRect)
	{
		GetContextClipRect (&old_r);
		pClipRect->corner.x += old_r.corner.x;
		pClipRect->corner.y += old_r.corner.y;
		SetContextClipRect (pClipRect);
		pClipRect->corner.x -= old_r.corner.x;
		pClipRect->corner.y -= old_r.corner.y;
		SetFrameHot (Screen,
				MAKE_HOT_SPOT (pClipRect->corner.x, pClipRect->corner.y));
	}

	if (transition_pending)
	{
		SetTransitionSource (NULL);
	}
	BatchGraphics ();
	
	which_space = GET_GAME_STATE (ARILOU_SPACE_SIDE);

	if (which_space <= 1)
	{
		SDPtr = &star_array[0];
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x07), 0x57));
		SetContextBackGroundColor (BLACK_COLOR);
	}
	else
	{
		SDPtr = &star_array[NUM_SOLAR_SYSTEMS + 1];
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x0B, 0x00), 0x6D));
		SetContextBackGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x08, 0x00), 0x6E));
	}
	ClearDrawable ();

	if (race_update == 0
			&& which_space < 2
			&& (diameter = (long)GLOBAL_SIS (FuelOnBoard) << 1))
	{
		COLOR OldColor;

		if (LOBYTE (GLOBAL (CurrentActivity)) != IN_HYPERSPACE)
			r.corner = CurStarDescPtr->star_pt;
		else
		{
			r.corner.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
			r.corner.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
		}

		r.extent.width = UNIVERSE_TO_DISPX (diameter)
				- UNIVERSE_TO_DISPX (0);
		if (r.extent.width < 0)
			r.extent.width = -r.extent.width;
		r.extent.height = UNIVERSE_TO_DISPY (diameter)
				- UNIVERSE_TO_DISPY (0);
		if (r.extent.height < 0)
			r.extent.height = -r.extent.height;

		r.corner.x = UNIVERSE_TO_DISPX (r.corner.x)
				- (r.extent.width >> 1);
		r.corner.y = UNIVERSE_TO_DISPY (r.corner.y)
				- (r.extent.height >> 1);

		OldColor = SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x03, 0x03, 0x03), 0x22));
		DrawFilledOval (&r);
		SetContextForeGroundColor (OldColor);
	}

	for (i = MAX_Y_UNIVERSE + 1; i >= 0; i -= GRID_DELTA)
	{
		SIZE j;

		r.corner.x = UNIVERSE_TO_DISPX (0);
		r.corner.y = UNIVERSE_TO_DISPY (i);
		r.extent.width = SIS_SCREEN_WIDTH << LOBYTE (pMenuState->delta_item);
		r.extent.height = 1;
		DrawFilledRectangle (&r);

		r.corner.y = UNIVERSE_TO_DISPY (MAX_Y_UNIVERSE + 1);
		r.extent.width = 1;
		r.extent.height = SIS_SCREEN_HEIGHT << LOBYTE (pMenuState->delta_item);
		for (j = MAX_X_UNIVERSE + 1; j >= 0; j -= GRID_DELTA)
		{
			r.corner.x = UNIVERSE_TO_DISPX (j);
			DrawFilledRectangle (&r);
		}
	}

	star_frame = SetRelFrameIndex (pMenuState->CurFrame, 2);
	if (which_space <= 1)
	{
		COUNT index;
		HSTARSHIP hStarShip, hNextShip;
		static const COLOR race_colors[] =
		{
			RACE_COLORS
		};

		for (index = 0,
				hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
				hStarShip != 0; ++index, hStarShip = hNextShip)
		{
			EXTENDED_SHIP_FRAGMENTPTR StarShipPtr;

			StarShipPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
					&GLOBAL (avail_race_q),
					hStarShip
					);
			hNextShip = _GetSuccLink (StarShipPtr);

			if (StarShipPtr->ShipInfo.known_strength)
			{
				RECT repair_r;

				GetSphereRect (StarShipPtr, &r, &repair_r);
				if (r.corner.x < SIS_SCREEN_WIDTH
						&& r.corner.y < SIS_SCREEN_HEIGHT
						&& r.corner.x + r.extent.width > 0
						&& r.corner.y + r.extent.height > 0
						&& (pClipRect == 0
						|| (repair_r.corner.x < pClipRect->corner.x + pClipRect->extent.width
						&& repair_r.corner.y < pClipRect->corner.y + pClipRect->extent.height
						&& repair_r.corner.x + repair_r.extent.width > pClipRect->corner.x
						&& repair_r.corner.y + repair_r.extent.height > pClipRect->corner.y)))
				{
					COLOR c;
					TEXT t;
					STRING locString;

					c = race_colors[index];
					if (index + 1 == race_update)
						SetContextForeGroundColor (WHITE_COLOR);
					else
						SetContextForeGroundColor (c);
					DrawOval (&r, 0);

					SetContextFont (TinyFont);

					t.baseline.x = r.corner.x + (r.extent.width >> 1);
					t.baseline.y = r.corner.y + (r.extent.height >> 1) - 1;
					t.align = ALIGN_CENTER;
					locString = SetAbsStringTableIndex (
							StarShipPtr->ShipInfo.race_strings, 1
							);
					t.CharCount = GetStringLength (locString);
					t.pStr = (UNICODE *)GetStringAddress (locString);
					TextRect (&t, &r, NULL_PTR);

					if (r.corner.x <= 0)
						t.baseline.x -= r.corner.x - 1;
					else if (r.corner.x + r.extent.width >= SIS_SCREEN_WIDTH)
						t.baseline.x -= (r.corner.x + r.extent.width)
								- SIS_SCREEN_WIDTH + 1;
					if (r.corner.y <= 0)
						t.baseline.y -= r.corner.y - 1;
					else if (r.corner.y + r.extent.height >= SIS_SCREEN_HEIGHT)
						t.baseline.y -= (r.corner.y + r.extent.height)
								- SIS_SCREEN_HEIGHT + 1;

					{
						BYTE r, g, b;
						COLOR c32k;

						c32k = COLOR_32k (c);
						r = (BYTE)((c32k >> (5 * 2)) & 0x1F);
						if ((r += 0x03) > 0x1F) r = 0x1F;
						g = (BYTE)((c32k >> (5 * 1)) & 0x1F);
						if ((g += 0x03) > 0x1F) g = 0x1F;
						b = (BYTE)((c32k >> (5 * 0)) & 0x1F);
						if ((b += 0x03) > 0x1F) b = 0x1F;

						SetContextForeGroundColor (
								BUILD_COLOR (MAKE_RGB15 (r, g, b), COLOR_256 (c) - 1)
								);
					}
					font_DrawText (&t);
				}
			}

			UnlockStarShip (
					&GLOBAL (avail_race_q), hStarShip
					);
		}
	}

	do
	{
		BYTE star_type;

		star_type = SDPtr->Type;

#ifdef NOTYET
{
UNICODE buf[256];

GetClusterName (SDPtr, buf);
printf ("%s\n", buf);
}
#endif /* NOTYET */
		s.origin.x = UNIVERSE_TO_DISPX (SDPtr->star_pt.x);
		s.origin.y = UNIVERSE_TO_DISPY (SDPtr->star_pt.y);
		if (which_space <= 1)
			s.frame = SetRelFrameIndex (star_frame,
					STAR_TYPE (star_type)
					* NUM_STAR_COLORS
					+ STAR_COLOR (star_type));
		else if (SDPtr->star_pt.x == ARILOU_HOME_X
				&& SDPtr->star_pt.y == ARILOU_HOME_Y)
			s.frame = SetRelFrameIndex (star_frame,
					SUPER_GIANT_STAR * NUM_STAR_COLORS + GREEN_BODY);
		else
			s.frame = SetRelFrameIndex (star_frame,
					GIANT_STAR * NUM_STAR_COLORS + GREEN_BODY);
		DrawStamp (&s);

		++SDPtr;
	} while (SDPtr->star_pt.x <= MAX_X_UNIVERSE
			&& SDPtr->star_pt.y <= MAX_Y_UNIVERSE);

	if (GET_GAME_STATE (ARILOU_SPACE))
	{
		if (which_space <= 1)
		{
			s.origin.x = UNIVERSE_TO_DISPX (ARILOU_SPACE_X);
			s.origin.y = UNIVERSE_TO_DISPY (ARILOU_SPACE_Y);
		}
		else
		{
			s.origin.x = UNIVERSE_TO_DISPX (QUASI_SPACE_X);
			s.origin.y = UNIVERSE_TO_DISPY (QUASI_SPACE_Y);
		}
		s.frame = SetRelFrameIndex (star_frame,
				GIANT_STAR * NUM_STAR_COLORS + GREEN_BODY);
		DrawStamp (&s);
	}

	if (race_update == 0
			&& GLOBAL (autopilot.x) != ~0
			&& GLOBAL (autopilot.y) != ~0)
		DrawAutoPilot (&GLOBAL (autopilot));

	if (transition_pending)
	{
		GetContextClipRect (&r);
		ScreenTransition (3, &r);
		transition_pending = FALSE;
	}
	
	UnbatchGraphics ();

	if (pClipRect)
	{
		SetContextClipRect (&old_r);
		SetFrameHot (Screen, MAKE_HOT_SPOT (0, 0));
	}

	if (race_update == 0)
	{
		if (draw_cursor)
		{
			GetContextClipRect (&r);
			LoadIntoExtraScreen (&r);
			DrawCursor (
					UNIVERSE_TO_DISPX (pMenuState->first_item.x),
					UNIVERSE_TO_DISPY (pMenuState->first_item.y)
					);
		}
	}

	if (draw_cursor)
		UnlockMutex (GraphicsLock);
}

static void
EraseCursor (COORD curs_x, COORD curs_y)
{
	RECT r;

	GetFrameRect (pMenuState->CurFrame, &r);

	if ((r.corner.x += curs_x) < 0)
	{
		r.extent.width += r.corner.x;
		r.corner.x = 0;
	}
	else if (r.corner.x + r.extent.width >= SIS_SCREEN_WIDTH)
		r.extent.width = SIS_SCREEN_WIDTH - r.corner.x;
	if ((r.corner.y += curs_y) < 0)
	{
		r.extent.height += r.corner.y;
		r.corner.y = 0;
	}
	else if (r.corner.y + r.extent.height >= SIS_SCREEN_HEIGHT)
		r.extent.height = SIS_SCREEN_HEIGHT - r.corner.y;

#ifndef OLD
	RepairBackRect (&r);
#else /* NEW */
	r.extent.height += r.corner.y & 1;
	r.corner.y &= ~1;
	UnlockMutex (GraphicsLock);
	DrawStarMap (0, &r);
	LockMutex (GraphicsLock);
#endif /* OLD */
}

static void
ZoomStarMap (SIZE dir)
{
#define MAX_ZOOM_SHIFT 4
	if (dir > 0)
	{
		if (LOBYTE (pMenuState->delta_item) < MAX_ZOOM_SHIFT)
		{
			++pMenuState->delta_item;
			pMenuState->flash_rect1.corner = pMenuState->first_item;

			DrawStarMap (0, NULL_PTR);
			SleepThread (ONE_SECOND >> 3);
		}
	}
	else if (dir < 0)
	{
		if (LOBYTE (pMenuState->delta_item))
		{
			if (LOBYTE (pMenuState->delta_item) > 1)
				pMenuState->flash_rect1.corner = pMenuState->first_item;
			else
			{
				pMenuState->flash_rect1.corner.x = MAX_X_UNIVERSE >> 1;
				pMenuState->flash_rect1.corner.y = MAX_Y_UNIVERSE >> 1;
			}
			--pMenuState->delta_item;

			DrawStarMap (0, NULL_PTR);
			SleepThread (ONE_SECOND >> 3);
		}
	}
}

static void
UpdateCursorLocation (PMENU_STATE pMS, int sx, int sy, const POINT *newpt)
{
	STAMP s;
	POINT pt;

	pt.x = UNIVERSE_TO_DISPX (pMS->first_item.x);
	pt.y = UNIVERSE_TO_DISPY (pMS->first_item.y);

	if (newpt)
	{	// absolute move
		sx = sy = 0;
		s.origin.x = UNIVERSE_TO_DISPX (newpt->x);
		s.origin.y = UNIVERSE_TO_DISPY (newpt->y);
		pMS->first_item = *newpt;
	}
	else
	{	// incremental move
		s.origin.x = pt.x + sx;
		s.origin.y = pt.y + sy;
	}

	if (sx)
	{
		pMS->first_item.x =
				DISP_TO_UNIVERSEX (s.origin.x) - sx;
		while (UNIVERSE_TO_DISPX (pMS->first_item.x) == pt.x)
		{
			pMS->first_item.x += sx;
			if (pMS->first_item.x < 0)
			{
				pMS->first_item.x = 0;
				break;
			}
			else if (pMS->first_item.x > MAX_X_UNIVERSE)
			{
				pMS->first_item.x = MAX_X_UNIVERSE;
				break;
			}
		}
		s.origin.x = UNIVERSE_TO_DISPX (pMS->first_item.x);
	}

	if (sy)
	{
		pMS->first_item.y =
				DISP_TO_UNIVERSEY (s.origin.y) + sy;
		while (UNIVERSE_TO_DISPY (pMS->first_item.y) == pt.y)
		{
			pMS->first_item.y -= sy;
			if (pMS->first_item.y < 0)
			{
				pMS->first_item.y = 0;
				break;
			}
			else if (pMS->first_item.y > MAX_Y_UNIVERSE)
			{
				pMS->first_item.y = MAX_Y_UNIVERSE;
				break;
			}
		}
		s.origin.y = UNIVERSE_TO_DISPY (pMS->first_item.y);
	}

	if (s.origin.x < 0 || s.origin.y < 0
			|| s.origin.x >= SIS_SCREEN_WIDTH
			|| s.origin.y >= SIS_SCREEN_HEIGHT)
	{
		pMS->flash_rect1.corner = pMS->first_item;
		DrawStarMap (0, NULL_PTR);

		s.origin.x = UNIVERSE_TO_DISPX (pMS->first_item.x);
		s.origin.y = UNIVERSE_TO_DISPY (pMS->first_item.y);
	}
	else
	{
		LockMutex (GraphicsLock);
		EraseCursor (pt.x, pt.y);
		// ClearDrawable ();
		DrawCursor (s.origin.x, s.origin.y);
		UnlockMutex (GraphicsLock);
	}
}

#define CURSOR_INFO_BUFSIZE 256

static void
UpdateCursorInfo (PMENU_STATE pMS, UNICODE *prevbuf)
{
	UNICODE buf[CURSOR_INFO_BUFSIZE] = "";
	POINT pt;
	STAR_DESCPTR SDPtr, BestSDPtr;

	pt.x = UNIVERSE_TO_DISPX (pMS->first_item.x);
	pt.y = UNIVERSE_TO_DISPY (pMS->first_item.y);

	SDPtr = BestSDPtr = 0;
	while ((SDPtr = FindStar (SDPtr, &pMS->first_item, 75, 75)))
	{
		if (UNIVERSE_TO_DISPX (SDPtr->star_pt.x) == pt.x
				&& UNIVERSE_TO_DISPY (SDPtr->star_pt.y) == pt.y
				&& (BestSDPtr == 0
				|| STAR_TYPE (SDPtr->Type) >= STAR_TYPE (BestSDPtr->Type)))
			BestSDPtr = SDPtr;
	}

	if (BestSDPtr)
	{
		pMS->first_item = BestSDPtr->star_pt;
		GetClusterName (BestSDPtr, buf);
	}

	if (GET_GAME_STATE (ARILOU_SPACE))
	{
		POINT ari_pt;

		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		{
			ari_pt.x = ARILOU_SPACE_X;
			ari_pt.y = ARILOU_SPACE_Y;
		}
		else
		{
			ari_pt.x = QUASI_SPACE_X;
			ari_pt.y = QUASI_SPACE_Y;
		}

		if (UNIVERSE_TO_DISPX (ari_pt.x) == pt.x
				&& UNIVERSE_TO_DISPY (ari_pt.y) == pt.y)
		{
			pMS->first_item = ari_pt;
			utf8StringCopy (buf, sizeof (buf),
					GAME_STRING (STAR_STRING_BASE + 132));
		}
	}

	LockMutex (GraphicsLock);
	DrawHyperCoords (pMS->first_item);
	if (strcmp (buf, prevbuf) != 0)
	{
		strcpy (prevbuf, buf);
		DrawSISMessage (buf);
	}
	UnlockMutex (GraphicsLock);
}

static void
UpdateFuelRequirement (PMENU_STATE pMS)
{
	UNICODE buf[80];
	COUNT fuel_required;
	DWORD f;
	POINT pt;

	if (LOBYTE (GLOBAL (CurrentActivity)) != IN_HYPERSPACE)
		pt = CurStarDescPtr->star_pt;
	else
	{
		pt.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		pt.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	}
	pt.x -= pMS->first_item.x;
	pt.y -= pMS->first_item.y;

	f = (DWORD)((long)pt.x * pt.x + (long)pt.y * pt.y);
	if (f == 0 || GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1)
		fuel_required = 0;
	else
		fuel_required = square_root (f) + (FUEL_TANK_SCALE / 20);

	sprintf (buf, "%s %u.%u",
			GAME_STRING (NAVIGATION_STRING_BASE + 4),
			fuel_required / FUEL_TANK_SCALE,
			(fuel_required % FUEL_TANK_SCALE) / 10);

	LockMutex (GraphicsLock);
	DrawStatusMessage (buf);
	UnlockMutex (GraphicsLock);
}

#define STAR_SEARCH_BUFSIZE 256

typedef struct starsearch_state
{
	PMENU_STATE pMS;
	UNICODE Text[STAR_SEARCH_BUFSIZE];
	UNICODE LastText[STAR_SEARCH_BUFSIZE];
	DWORD LastChangeTime;
	int FirstIndex;
	int CurIndex;
	int LastIndex;
	BOOLEAN SingleClust;
	BOOLEAN SingleMatch;
	UNICODE Buffer[STAR_SEARCH_BUFSIZE];
	const UNICODE *Prefix;
	const UNICODE *Cluster;
	int PrefixLen;
	int ClusterLen;
	int ClusterPos;
	int SortedStars[NUM_SOLAR_SYSTEMS];
} STAR_SEARCH_STATE;

static int
compStarName (const void *ptr1, const void *ptr2)
{
	int index1;
	int index2;

	index1 = *(const int *) ptr1;
	index2 = *(const int *) ptr2;
	if (star_array[index1].Postfix != star_array[index2].Postfix)
	{
		return utf8StringCompare (GAME_STRING (star_array[index1].Postfix),
				GAME_STRING (star_array[index2].Postfix));
	}

	if (star_array[index1].Prefix < star_array[index2].Prefix)
		return -1;
	
	if (star_array[index1].Prefix > star_array[index2].Prefix)
		return 1;

	return 0;
}

static void
SortStarsOnName (STAR_SEARCH_STATE *pSS)
{
	int i;
	int *sorted = pSS->SortedStars;

	for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
		sorted[i] = i;

	qsort (sorted, NUM_SOLAR_SYSTEMS, sizeof (int), compStarName);
}

static void
SplitStarName (STAR_SEARCH_STATE *pSS)
{
	UNICODE *buf = pSS->Buffer;
	UNICODE *next;
	UNICODE *sep;

	pSS->Prefix = 0;
	pSS->PrefixLen = 0;
	pSS->Cluster = 0;
	pSS->ClusterLen = 0;
	pSS->ClusterPos = 0;

	// skip leading space
	for (next = buf; *next != '\0' &&
			getCharFromString ((const UNICODE **)&next) == ' ';
			buf = next)
		;
	if (*buf == '\0')
	{	// no text
		return;
	}

	pSS->Prefix = buf;

	// See if player gave a prefix
	for (buf = next; *next != '\0' &&
			getCharFromString ((const UNICODE **)&next) != ' ';
			buf = next)
		;
	if (*buf != '\0')
	{	// found possibly separating ' '
		sep = buf;
		// skip separating space
		for (buf = next; *next != '\0' &&
				getCharFromString ((const UNICODE **)&next) == ' ';
				buf = next)
			;
	}

	if (*buf == '\0')
	{	// reached the end -- cluster only
		pSS->Cluster = pSS->Prefix;
		pSS->ClusterLen = utf8StringCount (pSS->Cluster);
		pSS->ClusterPos = utf8StringCountN (pSS->Buffer, pSS->Cluster);
		pSS->Prefix = 0;
		return;
	}

	// consider the rest cluster name (whatever there is)
	pSS->Cluster = buf;
	pSS->ClusterLen = utf8StringCount (pSS->Cluster);
	pSS->ClusterPos = utf8StringCountN (pSS->Buffer, pSS->Cluster);
	*sep = '\0'; // split
	pSS->PrefixLen = utf8StringCount (pSS->Prefix);
}

static inline int
SkipStarCluster (int *sortedStars, int istar)
{
	int Postfix = star_array[sortedStars[istar]].Postfix;

	for (++istar; istar < NUM_SOLAR_SYSTEMS &&
			star_array[sortedStars[istar]].Postfix == Postfix;
			++istar)
		;
	return istar;
}

static int
FindNextStarIndex (STAR_SEARCH_STATE *pSS, int from, BOOLEAN WithinClust)
{
	int i;

	if (!pSS->Cluster)
		return -1; // nothing to search for

	for (i = from; i < NUM_SOLAR_SYSTEMS; ++i)
	{
		STAR_DESCPTR SDPtr = &star_array[pSS->SortedStars[i]];
		UNICODE FullName[STAR_SEARCH_BUFSIZE];
		UNICODE *ClusterName = GAME_STRING (SDPtr->Postfix);
		const UNICODE *sptr;
		const UNICODE *dptr;
		int dlen;
		int c;
		
		dlen = utf8StringCount (ClusterName);
		if (pSS->ClusterLen > dlen)
		{	// no match, skip the rest of cluster
			i = SkipStarCluster (pSS->SortedStars, i) - 1;
			continue;
		}

		for (c = 0, sptr = pSS->Cluster, dptr = ClusterName;
				c < pSS->ClusterLen; ++c)
		{
			wchar_t sc = getCharFromString (&sptr);
			wchar_t dc = getCharFromString (&dptr);

			if (toWideUpper (sc) != toWideUpper (dc))
				break;
		}

		if (c < pSS->ClusterLen)
		{	// no match here, skip the rest of cluster
			i = SkipStarCluster (pSS->SortedStars, i) - 1;
			continue;
		}

		if (!SDPtr->Prefix || WithinClust)
			// singular star - prefix not checked
			// or searching within clusters
			break;

		if (!pSS->Prefix)
		{	// searching for cluster name only
			// return only the first stars in a cluster
			if (i == 0 || SDPtr->Postfix !=
					star_array[pSS->SortedStars[i - 1]].Postfix)
			{	// found one
				break;
			}
			else
			{	// another star in the same cluster, skip cluster
				i = SkipStarCluster (pSS->SortedStars, i) - 1;
				continue;
			}
		}

		// check prefix
		GetClusterName (SDPtr, FullName);
		dlen = utf8StringCount (FullName);
		if (pSS->PrefixLen > dlen)
			continue;

		for (c = 0, sptr = pSS->Prefix, dptr = FullName;
				c < pSS->PrefixLen; ++c)
		{
			wchar_t sc = getCharFromString (&sptr);
			wchar_t dc = getCharFromString (&dptr);

			if (toWideUpper (sc) != toWideUpper (dc))
				break;
		}

		if (c >= pSS->PrefixLen)
			break; // found one
	}

	return (i < NUM_SOLAR_SYSTEMS) ? i : -1;
}

static void
DrawMatchedStarName (PTEXTENTRY_STATE pTES)
{
	STAR_SEARCH_STATE *pSS = (STAR_SEARCH_STATE *) pTES->CbParam;
	PMENU_STATE pMS = pSS->pMS;
	UNICODE buf[STAR_SEARCH_BUFSIZE] = "";
	SIZE ExPos = 0;
	SIZE CurPos = -1;
	STAR_DESCPTR SDPtr = &star_array[pSS->SortedStars[pSS->CurIndex]];
	COUNT flags;

	if (pSS->SingleClust || pSS->SingleMatch)
	{	// draw full star name
		GetClusterName (SDPtr, buf);
		ExPos = -1;
		flags = DSME_SETFR;
	}
	else
	{	// draw substring match
		UNICODE *pstr = buf;

		strcpy (pstr, pSS->Text);
		ExPos = pSS->ClusterPos;
		pstr = skipUTF8Chars (pstr, pSS->ClusterPos);

		strcpy (pstr, GAME_STRING (SDPtr->Postfix));
		ExPos += pSS->ClusterLen;
		CurPos = pTES->CursorPos;

		flags = DSME_CLEARFR;
		if (pTES->JoystickMode)
			flags |= DSME_BLOCKCUR;
	}
	
	LockMutex (GraphicsLock);
	DrawSISMessageEx (buf, CurPos, ExPos, flags);
	DrawHyperCoords (pMS->first_item);
	UnlockMutex (GraphicsLock);
}

static void
MatchNextStar (STAR_SEARCH_STATE *pSS, BOOLEAN Reset)
{
	if (Reset)
		pSS->FirstIndex = -1; // reset cache
	
	if (pSS->FirstIndex < 0)
	{	// first time after changes
		pSS->CurIndex = -1;
		pSS->LastIndex = -1;
		pSS->SingleClust = FALSE;
		pSS->SingleMatch = FALSE;
		strcpy (pSS->Buffer, pSS->Text);
		SplitStarName (pSS);
	}

	pSS->CurIndex = FindNextStarIndex (pSS, pSS->CurIndex + 1,
			pSS->SingleClust);
	if (pSS->FirstIndex < 0) // first search
		pSS->FirstIndex = pSS->CurIndex;
	
	if (pSS->CurIndex >= 0)
	{	// remember as last (searching forward-only)
		pSS->LastIndex = pSS->CurIndex;
	}
	else
	{	// wrap around
		pSS->CurIndex = pSS->FirstIndex;

		if (pSS->FirstIndex == pSS->LastIndex && pSS->FirstIndex != -1)
		{
			if (!pSS->Prefix)
			{	// only one cluster matching
				pSS->SingleClust = TRUE;
			}
			else
			{	// exact match
				pSS->SingleMatch = TRUE;
			}
		}
	}
}

static BOOLEAN
OnStarNameChange (PTEXTENTRY_STATE pTES)
{
	STAR_SEARCH_STATE *pSS = (STAR_SEARCH_STATE *) pTES->CbParam;
	PMENU_STATE pMS = pSS->pMS;
	COUNT flags;
	BOOLEAN ret = TRUE;

	if (strcmp (pSS->Text, pSS->LastText) != 0)
	{	// string changed
		pSS->LastChangeTime = GetTimeCounter ();
		strcpy (pSS->LastText, pSS->Text);
		
		// reset the search
		MatchNextStar (pSS, TRUE);
	}

	if (pSS->CurIndex < 0)
	{	// nothing found
		if (pSS->Text[0] == '\0')
			flags = DSME_SETFR;
		else
			flags = DSME_CLEARFR;
		if (pTES->JoystickMode)
			flags |= DSME_BLOCKCUR;

		LockMutex (GraphicsLock);
		ret = DrawSISMessageEx (pSS->Text, pTES->CursorPos, -1, flags);
		UnlockMutex (GraphicsLock);
	}
	else
	{
		STAR_DESCPTR SDPtr;

		// move the cursor to the found star
		SDPtr = &star_array[pSS->SortedStars[pSS->CurIndex]];
		UpdateCursorLocation (pMS, 0, 0, &SDPtr->star_pt);

		DrawMatchedStarName (pTES);
		UpdateFuelRequirement (pMS);
	}

	return ret;
}

static BOOLEAN
OnStarNameFrame (PTEXTENTRY_STATE pTES)
{
	STAR_SEARCH_STATE *pSS = (STAR_SEARCH_STATE *) pTES->CbParam;
	PMENU_STATE pMS = pSS->pMS;

	if (PulsedInputState.menu[KEY_MENU_NEXT])
	{	// search for next match
		STAR_DESCPTR SDPtr;

		MatchNextStar (pSS, FALSE);

		if (pSS->CurIndex < 0)
		{	// nothing found
			if (PulsedInputState.menu[KEY_MENU_NEXT])
				PlayMenuSound (MENU_SOUND_FAILURE);
			return TRUE;
		}

		// move the cursor to the found star
		SDPtr = &star_array[pSS->SortedStars[pSS->CurIndex]];
		UpdateCursorLocation (pMS, 0, 0, &SDPtr->star_pt);

		DrawMatchedStarName (pTES);
		UpdateFuelRequirement (pMS);
	}
	
	return TRUE;
}

static BOOLEAN
DoStarSearch (PMENU_STATE pMS)
{
	TEXTENTRY_STATE tes;
	STAR_SEARCH_STATE *pss;
	BOOLEAN success;

	pss = HMalloc (sizeof (*pss));
	if (!pss)
		return FALSE;

	LockMutex (GraphicsLock);
	DrawSISMessageEx ("", 0, 0, DSME_SETFR);
	UnlockMutex (GraphicsLock);

	pss->pMS = pMS;
	pss->LastChangeTime = 0;
	pss->Text[0] = '\0';
	pss->LastText[0] = '\0';
	pss->FirstIndex = -1;
	SortStarsOnName (pss);

	// text entry setup
	tes.Initialized = FALSE;
	tes.MenuRepeatDelay = 0;
	tes.BaseStr = pss->Text;
	tes.MaxSize = sizeof (pss->Text);
	tes.CursorPos = 0;
	tes.CbParam = pss;
	tes.ChangeCallback = OnStarNameChange;
	tes.FrameCallback = OnStarNameFrame;

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	success = DoTextEntry (&tes);

	LockMutex (GraphicsLock);
	DrawSISMessageEx (pss->Text, -1, -1, DSME_CLEARFR);
	UnlockMutex (GraphicsLock);

	HFree (pss);

	return success;
} 

static BOOLEAN
DoMoveCursor (PMENU_STATE pMS)
{
#define MIN_ACCEL_DELAY (ONE_SECOND / 60)
#define MAX_ACCEL_DELAY (ONE_SECOND / 8)
#define STEP_ACCEL_DELAY (ONE_SECOND / 120)
	static UNICODE last_buf[CURSOR_INFO_BUFSIZE];

	pMS->MenuRepeatDelay = (COUNT)pMS->CurState;
	if (!pMS->Initialized)
	{
		pMS->Initialized = TRUE;
		pMS->InputFunc = DoMoveCursor;

		SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
		SetMenuRepeatDelay (MIN_ACCEL_DELAY, MAX_ACCEL_DELAY, STEP_ACCEL_DELAY, TRUE);

		pMS->flash_task = AssignTask (flash_cursor_func, 2048,
				"flash location on star map");
		
		last_buf[0] = '\0';
		UpdateCursorInfo (pMS, last_buf);
		UpdateFuelRequirement (pMS);

		return TRUE;
	}
	else if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		if (pMS->flash_task)
		{
			ConcludeTask (pMS->flash_task);
			pMS->flash_task = 0;
		}

		return (FALSE);
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		GLOBAL (autopilot) = pMS->first_item;
#ifdef DEBUG
		if (instantMove)
		{
			PlayMenuSound (MENU_SOUND_INVOKED);
			if (pMS->flash_task)
			{
				ConcludeTask (pMS->flash_task);
				pMS->flash_task = 0;
			}

			if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE)
			{
				// Move to the new location immediately.
				doInstantMove ();
			}
			else if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY)
			{
				// We're in a solar system; exit it.
				GLOBAL (CurrentActivity) |= END_INTERPLANETARY;
			
				// Set a hook to move to the new location:
				debugHook = doInstantMove;
			}

			return (FALSE);
		}
#endif
		DrawStarMap (0, NULL_PTR);
	}
	else if (PulsedInputState.menu[KEY_MENU_SEARCH])
	{
		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		{	// HyperSpace search
			POINT oldpt = pMS->first_item;

			if (!DoStarSearch (pMS))
			{	// search failed or canceled - return cursor
				UpdateCursorLocation (pMS, 0, 0, &oldpt);
			}
			// make sure cmp fails
			strcpy (last_buf, "  <random garbage>  ");
			UpdateCursorInfo (pMS, last_buf);
			UpdateFuelRequirement (pMS);

			SetMenuRepeatDelay (MIN_ACCEL_DELAY, MAX_ACCEL_DELAY,
					STEP_ACCEL_DELAY, TRUE);
			SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
		}
		else
		{	// no search in QuasiSpace
			PlayMenuSound (MENU_SOUND_FAILURE);
		}
	}
	else
	{
		SBYTE sx, sy;
		SIZE ZoomIn, ZoomOut;

		ZoomIn = ZoomOut = 0;
		if (PulsedInputState.menu[KEY_MENU_ZOOM_IN])
			ZoomIn = 1;
		else if (PulsedInputState.menu[KEY_MENU_ZOOM_OUT])
			ZoomOut = 1;

		ZoomStarMap (ZoomIn - ZoomOut);

		sx = sy = 0;
		if (PulsedInputState.menu[KEY_MENU_LEFT])    sx =   -1;
		if (PulsedInputState.menu[KEY_MENU_RIGHT])   sx =    1;
		if (PulsedInputState.menu[KEY_MENU_UP])      sy =   -1;
		if (PulsedInputState.menu[KEY_MENU_DOWN])    sy =    1;

		if (sx != 0 || sy != 0)
		{
			UpdateCursorLocation (pMS, sx, sy, NULL_PTR);
			UpdateCursorInfo (pMS, last_buf);
			UpdateFuelRequirement (pMS);
		}
	}

	{
		BOOLEAN result = !(GLOBAL (CurrentActivity & CHECK_ABORT));
		if (!result)
		{
			if (pMS->flash_task)
			{
				ConcludeTask (pMS->flash_task);
				pMS->flash_task = 0;
			}
		}
		return result;
	}
}

static void
RepairMap (COUNT update_race, PRECT pLastRect, PRECT pNextRect)
{
	RECT r;

	/* make a rect big enough for text */
	r.extent.width = 50;
	r.corner.x = (pNextRect->corner.x + (pNextRect->extent.width >> 1))
			- (r.extent.width >> 1);
	if (r.corner.x < 0)
		r.corner.x = 0;
	else if (r.corner.x + r.extent.width >= SIS_SCREEN_WIDTH)
		r.corner.x = SIS_SCREEN_WIDTH - r.extent.width;
	r.extent.height = 9;
	r.corner.y = (pNextRect->corner.y + (pNextRect->extent.height >> 1))
			- r.extent.height;
	if (r.corner.y < 0)
		r.corner.y = 0;
	else if (r.corner.y + r.extent.height >= SIS_SCREEN_HEIGHT)
		r.corner.y = SIS_SCREEN_HEIGHT - r.extent.height;
	BoxUnion (pLastRect, &r, &r);
	BoxUnion (pNextRect, &r, &r);
	*pLastRect = *pNextRect;

	if (r.corner.x < 0)
	{
		r.extent.width += r.corner.x;
		r.corner.x = 0;
	}
	if (r.corner.x + r.extent.width > SIS_SCREEN_WIDTH)
		r.extent.width = SIS_SCREEN_WIDTH - r.corner.x;
	if (r.corner.y < 0)
	{
		r.extent.height += r.corner.y;
		r.corner.y = 0;
	}
	if (r.corner.y + r.extent.height > SIS_SCREEN_HEIGHT)
		r.extent.height = SIS_SCREEN_HEIGHT - r.corner.y;

	r.extent.height += r.corner.y & 1;
	r.corner.y &= ~1;
	
	DrawStarMap (update_race, &r);
}

static void
UpdateMap (void)
{
	BYTE ButtonState, VisibleChange;
	BOOLEAN MapDrawn, Interrupted;
	COUNT index;
	HSTARSHIP hStarShip, hNextShip;

	FlushInput ();
	ButtonState = 1; /* assume a button down */

	MapDrawn = Interrupted = FALSE;
	for (index = 1,
			hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip; ++index, hStarShip = hNextShip)
	{
		EXTENDED_SHIP_FRAGMENTPTR StarShipPtr;

		StarShipPtr = (EXTENDED_SHIP_FRAGMENTPTR)LockStarShip (
				&GLOBAL (avail_race_q),
				hStarShip
				);
		hNextShip = _GetSuccLink (StarShipPtr);

		if (ButtonState)
		{
			if (!AnyButtonPress (TRUE))
				ButtonState = 0;
		}
		else if ((Interrupted = (BOOLEAN)(
				Interrupted || AnyButtonPress (TRUE)
				)))
			MapDrawn = TRUE;

		if (StarShipPtr->ShipInfo.known_strength)
		{
			SIZE dx, dy, delta;
			RECT r, last_r, temp_r0, temp_r1;

			dx = StarShipPtr->ShipInfo.loc.x
					- StarShipPtr->ShipInfo.known_loc.x;
			dy = StarShipPtr->ShipInfo.loc.y
					- StarShipPtr->ShipInfo.known_loc.y;
			if (dx || dy)
			{
				SIZE xincr, yincr,
						xerror, yerror,
						cycle;

				if (dx >= 0)
					xincr = 1;
				else
				{
					xincr = -1;
					dx = -dx;
				}
				dx <<= 1;

				if (dy >= 0)
					yincr = 1;
				else
				{
					yincr = -1;
					dy = -dy;
				}
				dy <<= 1;

				if (dx >= dy)
					cycle = dx;
				else
					cycle = dy;
				delta = xerror = yerror = cycle >> 1;

				if (!MapDrawn)
				{
					DrawStarMap ((COUNT)~0, NULL_PTR);
					MapDrawn = TRUE;
				}

				GetSphereRect (StarShipPtr, &temp_r0, &last_r);
				++last_r.extent.width;
				++last_r.extent.height;
				VisibleChange = FALSE;
				do
				{
					do
					{
						if ((xerror -= dx) <= 0)
						{
							StarShipPtr->ShipInfo.known_loc.x += xincr;
							xerror += cycle;
						}
						if ((yerror -= dy) <= 0)
						{
							StarShipPtr->ShipInfo.known_loc.y += yincr;
							yerror += cycle;
						}
						GetSphereRect (StarShipPtr, &temp_r1, &r);
					} while (delta--
							&& ((delta & 0x1F)
							|| (temp_r0.corner.x == temp_r1.corner.x
							&& temp_r0.corner.y == temp_r1.corner.y)));

					if (ButtonState)
					{
						if (!AnyButtonPress (TRUE))
							ButtonState = 0;
					}
					else if ((Interrupted = (BOOLEAN)(
								Interrupted || AnyButtonPress (TRUE)
								)))
					{
						MapDrawn = TRUE;
						goto DoneSphereMove;
					}

					++r.extent.width;
					++r.extent.height;
					if (temp_r0.corner.x != temp_r1.corner.x
							|| temp_r0.corner.y != temp_r1.corner.y)
					{
						VisibleChange = TRUE;
						RepairMap (index, &last_r, &r);
					}
				} while (delta >= 0);
				if (VisibleChange)
					RepairMap ((COUNT)~0, &last_r, &r);

DoneSphereMove:
				StarShipPtr->ShipInfo.known_loc = StarShipPtr->ShipInfo.loc;
			}

			delta = StarShipPtr->ShipInfo.actual_strength
					- StarShipPtr->ShipInfo.known_strength;
			if (delta)
			{
				if (!MapDrawn)
				{
					DrawStarMap ((COUNT)~0, NULL_PTR);
					MapDrawn = TRUE;
				}

				if (delta > 0)
					dx = 1;
				else
				{
					delta = -delta;
					dx = -1;
				}
				--delta;

				GetSphereRect (StarShipPtr, &temp_r0, &last_r);
				++last_r.extent.width;
				++last_r.extent.height;
				VisibleChange = FALSE;
				do
				{
					do
					{
						StarShipPtr->ShipInfo.known_strength += dx;
						GetSphereRect (StarShipPtr, &temp_r1, &r);
					} while (delta--
							&& ((delta & 0xF)
							|| temp_r0.extent.height == temp_r1.extent.height));

					if (ButtonState)
					{
						if (!AnyButtonPress (TRUE))
							ButtonState = 0;
					}
					else if ((Interrupted = (BOOLEAN)(
								Interrupted || AnyButtonPress (TRUE)
								)))
					{
						MapDrawn = TRUE;
						goto DoneSphereGrowth;
					}
					++r.extent.width;
					++r.extent.height;
					if (temp_r0.extent.height != temp_r1.extent.height)
					{
						VisibleChange = TRUE;
						RepairMap (index, &last_r, &r);
					}
				} while (delta >= 0);
				if (VisibleChange || temp_r0.extent.width != temp_r1.extent.width)
					RepairMap ((COUNT)~0, &last_r, &r);

DoneSphereGrowth:
				StarShipPtr->ShipInfo.known_strength =
						StarShipPtr->ShipInfo.actual_strength;
			}
		}

		UnlockStarShip (
				&GLOBAL (avail_race_q), hStarShip
				);
	}
}

static BOOLEAN
DoStarMap (void)
{
	MENU_STATE MenuState;
	POINT universe;
	//FRAME OldFrame;
	RECT clip_r;
	CONTEXT OldContext;

	pMenuState = &MenuState;
	memset (pMenuState, 0, sizeof (*pMenuState));

	MenuState.flash_rect1.corner.x = MAX_X_UNIVERSE >> 1;
	MenuState.flash_rect1.corner.y = MAX_Y_UNIVERSE >> 1;
	MenuState.CurFrame = SetAbsFrameIndex (MiscDataFrame, 48);
	MenuState.delta_item = 0;

	if (LOBYTE (GLOBAL (CurrentActivity)) != IN_HYPERSPACE)
		universe = CurStarDescPtr->star_pt;
	else
	{
		universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	}

	MenuState.first_item = GLOBAL (autopilot);
	if (MenuState.first_item.x == ~0 && MenuState.first_item.y == ~0)
		MenuState.first_item = universe;

	UnlockMutex (GraphicsLock);
	TaskSwitch ();

	MenuState.InputFunc = DoMoveCursor;
	MenuState.Initialized = FALSE;

	transition_pending = TRUE;
	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		UpdateMap ();

	LockMutex (GraphicsLock);
	
	DrawStarMap (0, (PRECT)-1);
	transition_pending = FALSE;
	
	BatchGraphics ();
	OldContext = SetContext (SpaceContext);
	GetContextClipRect (&clip_r);
	SetContext (OldContext);
	LoadIntoExtraScreen (&clip_r);
	DrawCursor (
			UNIVERSE_TO_DISPX (MenuState.first_item.x),
			UNIVERSE_TO_DISPY (MenuState.first_item.y)
			);
	UnbatchGraphics ();
	UnlockMutex (GraphicsLock);

	DoInput ((PVOID)&MenuState, FALSE);
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	pMenuState = 0;

	LockMutex (GraphicsLock);

	DrawHyperCoords (universe);
	DrawSISMessage (NULL_PTR);
	DrawStatusMessage (NULL_PTR);

	if (GLOBAL (autopilot.x) == universe.x
			&& GLOBAL (autopilot.y) == universe.y)
		GLOBAL (autopilot.x) = GLOBAL (autopilot.y) = ~0;

	return (GLOBAL (autopilot.x) != ~0
			&& GLOBAL (autopilot.y) != ~0);
}

BOOLEAN
DoFlagshipCommands (PMENU_STATE pMS)
{
	/* TODO: Make this carried cleanly by MENU_STATE structure */
	// static DWORD NextTime;
	if (!(pMS->Initialized & 1))
	{
		/* This has some dependency on the IPtask_func */
		ChangeSolarSys ();
		// NextTime = GetTimeCounter ();
	}
	else
	{
		BOOLEAN select = PulsedInputState.menu[KEY_MENU_SELECT];
		LockMutex (GraphicsLock);
		if (*(volatile BYTE *)&pMS->CurState == 0
				&& (*(volatile SIZE *)&pMS->Initialized & 1)
				&& !(GLOBAL (CurrentActivity)
				& (START_ENCOUNTER | END_INTERPLANETARY
				| CHECK_ABORT | CHECK_LOAD))
				&& GLOBAL_SIS (CrewEnlisted) != (COUNT)~0)
		{
			UnlockMutex (GraphicsLock);
			IP_frame ();
			return TRUE;
		}
		UnlockMutex (GraphicsLock);

		if (pMS->CurState)
		{
			BOOLEAN DoMenu;
			BYTE NewState;

			/* If pMS->CurState == 0, then we're flying
			 * around in interplanetary.  This needs to be
			 * corrected for the MenuChooser, which thinks
			 * that "0" is the first menu option */
			pMS->CurState --;
			DoMenu = DoMenuChooser (pMS, 
				(BYTE)(pMS->Initialized <= 1 ? PM_STARMAP : PM_SCAN));
			pMS->CurState ++;

			if (!DoMenu) {

				NewState = pMS->CurState;
				if (LastActivity == CHECK_LOAD)
					select = TRUE;
				if (select)
				{
					if (NewState != SCAN + 1 && NewState != (GAME_MENU) + 1)
					{
						LockMutex (GraphicsLock);
						SetFlashRect (NULL_PTR, (FRAME)0);
						UnlockMutex (GraphicsLock);
					}

					switch (NewState - 1)
					{
						case SCAN:
							ScanSystem ();
							break;
						case EQUIP_DEVICE:
						{
							pMenuState = pMS;
							if (!Devices (pMS))
								select = FALSE;
							pMenuState = 0;
							if (GET_GAME_STATE (PORTAL_COUNTER)) {
								// A player-induced portal to QuasiSpace is
								// opening.
								return (FALSE);
							}
							break;
						}
						case CARGO:
						{
							Cargo (pMS);
							break;
						}
						case ROSTER:
						{
							if (!Roster ())
								select = FALSE;
							break;
						}
						case GAME_MENU:
							if (GameOptions () == 0)
								return (FALSE);						
							break;
						case STARMAP:
						{
							BOOLEAN AutoPilotSet;

							LockMutex (GraphicsLock);
							if (++pMS->Initialized > 3) {
								pSolarSysState->PauseRotate = 1;
								RepairSISBorder ();
							}

							AutoPilotSet = DoStarMap ();
							SetDefaultMenuRepeatDelay ();

							if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE
									|| (GLOBAL (CurrentActivity) & CHECK_ABORT))
							{
								UnlockMutex (GraphicsLock);
								return (FALSE);
							}
							else if (pMS->Initialized <= 3)
							{
								ZoomSystem ();
								--pMS->Initialized;
							}
							UnlockMutex (GraphicsLock);

							if (!AutoPilotSet && pMS->Initialized >= 3)
							{
								LoadPlanet (NULL);
								--pMS->Initialized;
								pSolarSysState->PauseRotate = 0;
								LockMutex (GraphicsLock);
								SetFlashRect (SFR_MENU_3DO, (FRAME)0);
								UnlockMutex (GraphicsLock);
								break;
							}
						}
						case NAVIGATION:
							if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE)
								return (FALSE);

							if (pMS->Initialized <= 1)
							{
								pMS->Initialized = 1;
								ResumeGameClock ();
							}
							else if (pMS->flash_task)
							{
								FreePlanet ();
								LockMutex (GraphicsLock);
								LoadSolarSys ();
								ValidateOrbits ();
								ZoomSystem ();
								UnlockMutex (GraphicsLock);
							}

							LockMutex (GraphicsLock);
							pMS->CurState = 0;
							FlushInput ();
							UnlockMutex (GraphicsLock);
							break;
					}
				
					if (GLOBAL (CurrentActivity) & CHECK_ABORT)
						;
					else if (pMS->CurState)
					{
						LockMutex (GraphicsLock);
						SetFlashRect (SFR_MENU_3DO, (FRAME)0);
						UnlockMutex (GraphicsLock);
						if (select)
						{
							if (optWhichMenu != OPT_PC)
								pMS->CurState = (NAVIGATION) + 1;
							DrawMenuStateStrings ((BYTE)(pMS->Initialized <= 1 ? PM_STARMAP : PM_SCAN),
									pMS->CurState - 1);
						}
					}
					else
					{
						LockMutex (GraphicsLock);
						SetFlashRect (NULL_PTR, (FRAME)0);
						UnlockMutex (GraphicsLock);
						DrawMenuStateStrings (PM_STARMAP, -NAVIGATION);
					}
				}
			}
		}
	}

	return (!(GLOBAL (CurrentActivity)
			& (START_ENCOUNTER | END_INTERPLANETARY
			| CHECK_ABORT | CHECK_LOAD))
			&& GLOBAL_SIS (CrewEnlisted) != (COUNT)~0);
}

