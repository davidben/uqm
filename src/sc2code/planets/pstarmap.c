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

#include "starcon.h"
#include "libs/graphics/gfx_common.h"

//Added by Chris

void RepairBackRect (PRECT pRect);

//End Added by Chris

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

int
flash_cursor_func(void *data)
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
		SetSemaphore (GraphicsSem);
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
		ClearSemaphore (GraphicsSem);
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
	FRAME OldFrame;
	BOOLEAN draw_cursor;

	if (pClipRect == (PRECT)-1)
	{
		pClipRect = 0;
		draw_cursor = FALSE;
	}
	else
	{
		SetSemaphore (GraphicsSem);
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
	OldFrame = SetContextBGFrame ((FRAME)0);
	BatchGraphics ();
	
	SetContextDrawState (DEST_PIXMAP | DRAW_REPLACE);

	which_space = GET_GAME_STATE (ARILOU_SPACE_SIDE);

	if (which_space <= 1)
	{
		SDPtr = &star_array[0];
		SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x7), 0x57));
		SetContextBackGroundColor (BLACK_COLOR);
	}
	else
	{
		SDPtr = &star_array[NUM_SOLAR_SYSTEMS + 1];
		SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0xB, 0x00), 0x6D));
		SetContextBackGroundColor (BUILD_COLOR (MAKE_RGB15 (0x00, 0x8, 0x00), 0x6E));
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

		OldColor = SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x3, 0x3, 0x3), 0x22));
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
		COLOR race_colors[] =
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
						r = (c32k >> (5 * 2)) & 0x1F;
						if ((r += 0x03) > 0x1F) r = 0x1F;
						g = (c32k >> (5 * 1)) & 0x1F;
						if ((g += 0x03) > 0x1F) g = 0x1F;
						b = (c32k >> (5 * 0)) & 0x1F;
						if ((b += 0x03) > 0x1F) b = 0x1F;

						SetContextForeGroundColor (
								BUILD_COLOR (MAKE_RGB15 (r, g, b), COLOR_256 (c) - 1)
								);
					}
					DrawText (&t);
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
UNICODE buf[40];

GetClusterName (SDPtr, buf);
wprintf ("%s\n", buf);
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

	SetContextBGFrame (OldFrame);

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
		ClearSemaphore (GraphicsSem);
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
	ClearSemaphore (GraphicsSem);
	DrawStarMap (0, &r);
	SetSemaphore (GraphicsSem);
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

static BOOLEAN
DoMoveCursor (INPUT_STATE InputState, PMENU_STATE pMS)
{
#define MIN_ACCEL_DELAY (ONE_SECOND / 60)
#define MAX_ACCEL_DELAY (ONE_SECOND / 8)
	STAMP s;
	UNICODE buf[30];
	static UNICODE last_buf;

	pMS->MenuRepeatDelay = (COUNT)pMS->CurState;
	if (!pMS->Initialized)
	{
		pMS->Initialized = MAKE_WORD (
				KBDToUNICODE (SK_INSERT),
				KBDToUNICODE (SK_DELETE)
				);
		pMS->InputFunc = DoMoveCursor;

		pMS->flash_task = AssignTask (flash_cursor_func, 2048,
				"flash location on star map");
		s.origin.x = UNIVERSE_TO_DISPX (pMS->first_item.x);
		s.origin.y = UNIVERSE_TO_DISPY (pMS->first_item.y);
		last_buf = ~0;
		buf[0] = '\0';
		goto UpdateCursorInfo;
	}
	else if (InputState & DEVICE_BUTTON2)
	{
		if (pMS->flash_task)
		{
			ConcludeTask (pMS->flash_task);
			pMS->flash_task = 0;
		}

		return (FALSE);
	}
	else if (InputState & DEVICE_BUTTON1)
	{
		GLOBAL (autopilot) = pMS->first_item;

		DrawStarMap (0, NULL_PTR);
	}
	else
	{
		SBYTE sx, sy;
		POINT pt;
		SIZE ZoomIn, ZoomOut;

		ZoomIn = ZoomOut = 0;
		if (InputState & DEVICE_LEFTSHIFT)
			ZoomIn = 1;
		else if (InputState & DEVICE_RIGHTSHIFT)
			ZoomOut = 1;

		ZoomStarMap (ZoomOut - ZoomIn);

		sx = GetInputXComponent (InputState);
		sy = GetInputYComponent (InputState);
		if (sx == 0 && sy == 0)
		{
			extern INPUT_STATE OldInputState;

			if (OldInputState == 0)
				pMS->CurState = MAX_ACCEL_DELAY;
		}
		else
		{
			if (pMS->CurState > MIN_ACCEL_DELAY)
				--pMS->CurState;

			pt.x = UNIVERSE_TO_DISPX (pMS->first_item.x);
			pt.y = UNIVERSE_TO_DISPY (pMS->first_item.y);

			s.origin.x = pt.x + sx;
			s.origin.y = pt.y + sy;

			buf[0] = '\0';
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
				SetSemaphore (GraphicsSem);
				EraseCursor (pt.x, pt.y);
				// ClearDrawable ();
				DrawCursor (s.origin.x, s.origin.y);
				ClearSemaphore (GraphicsSem);
			}

UpdateCursorInfo:
			{
				STAR_DESCPTR SDPtr, BestSDPtr;

				SDPtr = BestSDPtr = 0;
				while ((SDPtr = FindStar (SDPtr, &pMS->first_item, 75, 75)))
				{
					if (UNIVERSE_TO_DISPX (SDPtr->star_pt.x) == s.origin.x
							&& UNIVERSE_TO_DISPY (SDPtr->star_pt.y) == s.origin.y
							&& (BestSDPtr == 0
							|| STAR_TYPE (SDPtr->Type) >= STAR_TYPE (BestSDPtr->Type)))
						BestSDPtr = SDPtr;
				}

				if (BestSDPtr)
				{
					pMS->first_item = BestSDPtr->star_pt;
					GetClusterName (BestSDPtr, buf);
				}
			}

			if (GET_GAME_STATE (ARILOU_SPACE))
			{
				if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
				{
					pt.x = ARILOU_SPACE_X;
					pt.y = ARILOU_SPACE_Y;
				}
				else
				{
					pt.x = QUASI_SPACE_X;
					pt.y = QUASI_SPACE_Y;
				}

				if (UNIVERSE_TO_DISPX (pt.x) == s.origin.x
						&& UNIVERSE_TO_DISPY (pt.y) == s.origin.y)
				{
					pMS->first_item = pt;
					wstrcpy (buf, GAME_STRING (STAR_STRING_BASE + 132));
				}
			}

			{
				COUNT fuel_required;
				DWORD f;

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
					fuel_required = square_root (f) + (FUEL_TANK_SCALE >> 1);
				SetSemaphore (GraphicsSem);
				DrawHyperCoords (pMenuState->first_item);
				if (last_buf == (UNICODE) ~0
						|| (buf[0] && !last_buf) || (!buf[0] && last_buf))
				{
					last_buf = buf[0];
					DrawSISMessage (buf);
				}
				wsprintf (buf, "%s %u.%u",
						GAME_STRING (NAVIGATION_STRING_BASE + 4),
						fuel_required / FUEL_TANK_SCALE,
						(fuel_required % FUEL_TANK_SCALE) / 10);
				DrawStatusMessage (buf);
				ClearSemaphore (GraphicsSem);
			}
		}
	}

	return (TRUE);
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
	SOUND OldMenuSounds;
	MENU_STATE MenuState;
	POINT universe;
	//FRAME OldFrame;
	RECT clip_r;
	CONTEXT OldContext;
	extern FRAME misc_data;

	pMenuState = &MenuState;
	memset (pMenuState, 0, sizeof (*pMenuState));

	MenuState.flash_rect1.corner.x = MAX_X_UNIVERSE >> 1;
	MenuState.flash_rect1.corner.y = MAX_Y_UNIVERSE >> 1;
	MenuState.CurFrame = SetAbsFrameIndex (misc_data, 48);
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

	ClearSemaphore (GraphicsSem);
	TaskSwitch ();

	MenuState.InputFunc = DoMoveCursor;
	MenuState.Initialized = FALSE;
	MenuState.CurState = MAX_ACCEL_DELAY;

	transition_pending = TRUE;
	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		UpdateMap ();

	SetSemaphore (GraphicsSem);
	
	DrawStarMap (0, (PRECT)-1);
	transition_pending = FALSE;
	
	BatchGraphics ();
	OldContext = SetContext (SpaceContext);
	GetContextClipRect (&clip_r);
	SetContext (OldContext);
	LoadIntoExtraScreen (&clip_r);
			DrawCursor (
			UNIVERSE_TO_DISPX (pMenuState->first_item.x),
			UNIVERSE_TO_DISPY (pMenuState->first_item.y)
			);
	UnbatchGraphics ();
	ClearSemaphore (GraphicsSem);

	OldMenuSounds = MenuSounds;
	MenuSounds = 0;
	DoInput ((PVOID)&MenuState);
	MenuSounds = OldMenuSounds;

	pMenuState = 0;

	SetSemaphore (GraphicsSem);

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
DoFlagshipCommands (INPUT_STATE InputState, PMENU_STATE pMS)
{
	if (!(pMS->Initialized & 1))
	{
		ChangeSolarSys ();
	}
	else
	{
		SetSemaphore (GraphicsSem);
		while (((PMENU_STATE volatile)pMS)->CurState == 0
				&& (((PMENU_STATE volatile)pMS)->Initialized & 1)
				&& !(GLOBAL (CurrentActivity)
				& (START_ENCOUNTER | END_INTERPLANETARY
				| CHECK_ABORT | CHECK_LOAD))
				&& GLOBAL_SIS (CrewEnlisted) != (COUNT)~0)
		{
			ClearSemaphore (GraphicsSem);
			TaskSwitch ();
			SetSemaphore (GraphicsSem);
		}
		ClearSemaphore (GraphicsSem);

		if (pMS->CurState)
		{
			BYTE NewState;

			NewState = pMS->CurState;
			if ((InputState & DEVICE_BUTTON1) || LastActivity == CHECK_LOAD)
			{
				CONTEXT OldContext;
				if (NewState != SCAN + 1 && NewState != (GAME_MENU) + 1)
				{
					SetSemaphore (GraphicsSem);
					SetFlashRect (NULL_PTR, (FRAME)0);
					ClearSemaphore (GraphicsSem);
				}

				switch (NewState - 1)
				{
					case SCAN:
						ScanSystem ();
						break;
					case EQUIP_DEVICE:
					{
						extern BOOLEAN Devices (PMENU_STATE pMS);

						pMenuState = pMS;
						if (!Devices (pMS))
							InputState &= ~DEVICE_BUTTON1;
						pMenuState = 0;
						if (GET_GAME_STATE (PORTAL_COUNTER))
							return (FALSE);
						break;
					}
					case CARGO:
					{
						extern void Cargo
								(PMENU_STATE
								pMS);

						Cargo (pMS);
						break;
					}
					case ROSTER:
					{
						
								extern
								BOOLEAN
								Roster
								(void);

						if (!Roster ())
							InputState &= ~DEVICE_BUTTON1;
						break;
					}
					case GAME_MENU:
						if (GameOptions () == 0)
							return (FALSE);
						
						if (LOBYTE (GLOBAL(CurrentActivity)) == IN_INTERPLANETARY)
						{
							SetSemaphore(GraphicsSem);
							ClearSISRect(DRAW_SIS_DISPLAY);
							ClearSemaphore(GraphicsSem);
							DrawSISFrame();
							SetSemaphore(GraphicsSem);
							RepairSISBorder();
							DrawSISMessage(NULL_PTR);
							DrawSISTitle(GLOBAL_SIS(PlanetName));
							DrawStarBackGround(TRUE);
							OldContext = SetContext(SpaceContext);
							SetContextDrawState(DEST_PIXMAP | DRAW_REPLACE);
							BatchGraphics();
							DrawPlanet(SIS_SCREEN_WIDTH - MAP_WIDTH, SIS_SCREEN_HEIGHT - MAP_HEIGHT, 0, 0);
							UnbatchGraphics();
							SetContext(OldContext);
							ClearSemaphore(GraphicsSem);
						}
// else
// DrawMenuStateStrings (PM_SCAN, ROSTER + 1);
						break;
					case STARMAP:
					{
						BOOLEAN AutoPilotSet;

						SetSemaphore (GraphicsSem);
						if (++pMS->Initialized > 3) {
							pSolarSysState->PauseRotate = 1;
							RepairSISBorder ();
						}

						AutoPilotSet = DoStarMap ();

						if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE
								|| (GLOBAL (CurrentActivity) & CHECK_ABORT))
						{
							ClearSemaphore (GraphicsSem);
							return (FALSE);
						}
						else if (pMS->Initialized <= 3)
						{
							ZoomSystem ();
							--pMS->Initialized;
						}
						ClearSemaphore (GraphicsSem);

						if (!AutoPilotSet && pMS->Initialized >= 3)
						{
							LoadPlanet (FALSE);
							--pMS->Initialized;
							pSolarSysState->PauseRotate = 0;
							SetSemaphore (GraphicsSem);
							SetFlashRect ((PRECT)~0L, (FRAME)0);
							ClearSemaphore (GraphicsSem);
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
							SetSemaphore (GraphicsSem);
							LoadSolarSys ();
							ZoomSystem ();
							ClearSemaphore (GraphicsSem);
						}

						SetSemaphore (GraphicsSem);
						pMS->CurState = 0;
						ClearSemaphore (GraphicsSem);
						break;
				}
				
				if (GLOBAL (CurrentActivity) & CHECK_ABORT)
					;
				else if (pMS->CurState)
				{
					SetSemaphore (GraphicsSem);
					SetFlashRect ((PRECT)~0L, (FRAME)0);
					ClearSemaphore (GraphicsSem);
					if (InputState & DEVICE_BUTTON1)
					{
						pMS->CurState = (NAVIGATION) + 1;
						DrawMenuStateStrings ((BYTE)(pMS->Initialized <= 1 ? PM_STARMAP : PM_SCAN),
								NAVIGATION);
					}
				}
				else
				{
					SetSemaphore (GraphicsSem);
					SetFlashRect (NULL_PTR, (FRAME)0);
					ClearSemaphore (GraphicsSem);
					DrawMenuStateStrings (PM_STARMAP, -NAVIGATION);
				}
			}
			else
			{
				BYTE FirstState;

				FirstState = (BYTE)(pMS->Initialized <= 1 ? STARMAP : SCAN);
				if (GetInputXComponent (InputState) < 0
						|| GetInputYComponent (InputState) < 0)
				{
					if (NewState-- == (BYTE)(FirstState + 1))
						NewState = (NAVIGATION) + 1;
				}
				else if (GetInputXComponent (InputState) > 0
						|| GetInputYComponent (InputState) > 0)
				{
					if (NewState++ == (NAVIGATION) + 1)
						NewState = (BYTE)(FirstState + 1);
				}

				if (NewState != pMS->CurState)
				{
					DrawMenuStateStrings ((BYTE)(pMS->Initialized <= 1 ? PM_STARMAP : PM_SCAN),
							(BYTE)(NewState - 1));

					pMS->CurState = NewState;
				}
			}
		}
	}

	return (!(GLOBAL (CurrentActivity)
			& (START_ENCOUNTER | END_INTERPLANETARY
			| CHECK_ABORT | CHECK_LOAD))
			&& GLOBAL_SIS (CrewEnlisted) != (COUNT)~0);
}

