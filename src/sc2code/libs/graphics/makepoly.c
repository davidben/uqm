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

#include "gfxintrn.h"

/* I don't know yet what the deal is with the __WATCOMC__ delimited
 * code, but this will at least make it compile, which is stage 1. - SvdB
 */
#define __WATCOMC__

#define AllocGraphicData(size) \
		mem_allocate ((MEM_SIZE)(size), MEM_ZEROINIT | MEM_GRAPHICS, \
		DEFAULT_MEM_PRIORITY, MEM_SIMPLE)
#define LockData (PBYTE)mem_lock
#define UnlockData mem_unlock
#define FreeData mem_release

typedef enum
{
	UP,
	DOWN
} DIRECTION;

#define POLY_XMIN 1
#define POLY_YMIN 0

static void
xor_dot (FRAMEPTR FramePtr, COORD x, COORD y)
{
	PBYTE lpMaskAddr;

	lpMaskAddr =
#ifndef __WATCOMC__
			GetImageData (&FramePtr->MaskImage)
			+ (y * GetImageLineBytes (&FramePtr->MaskImage))
			+ BYTE_POS (x);
#else /* __WATCOMC__ */
			0;
#endif /* __WATCOMC__ */
	*lpMaskAddr ^= (BYTE)0x80 >> BYTE_OFFS (x);
}

static void
xor_line (FRAMEPTR FramePtr, PBRESENHAM_LINE pLine)
{
	COORD dst_x, dst_y;
	COORD dst_mask_x;
	SIZE dst_mask_incr;
	SIZE count;
	BYTE dst_bit, bit_shift;
	SIZE delta_x, delta_y, error_term;
	PBYTE lpMaskAddr;

	dst_x = pLine->first.x;
	dst_y = pLine->first.y;
	delta_x = pLine->abs_delta_x;
	delta_y = pLine->abs_delta_y;
	error_term = pLine->error_term;

	dst_mask_x = dst_x;
	bit_shift = 1;

#ifndef __WATCOMC__
	lpMaskAddr = GetImageData (&FramePtr->MaskImage);
	dst_mask_incr = GetImageLineBytes (&FramePtr->MaskImage);
	lpMaskAddr += (dst_y * dst_mask_incr) + BYTE_POS (dst_mask_x);
#endif /* __WATCOMC__ */

	if (delta_y < 0)
	{
		delta_y = -delta_y;
		dst_mask_incr = -dst_mask_incr;
	}

	dst_bit = (BYTE)((BYTE)0x80 >> (BYTE)BYTE_OFFS (dst_mask_x));
	if (delta_x > delta_y)
	{
		count = pLine->second.x - pLine->first.x;
		if (!pLine->end_points_exchanged)
		{
			do
			{
				if ((error_term += delta_y) >= 0)
				{
					*lpMaskAddr ^= dst_bit;
					if (count == 0)
						break;

					lpMaskAddr += dst_mask_incr;
					error_term -= delta_x;
				}
				if ((dst_bit >>= bit_shift) == 0)
				{
					dst_bit = (BYTE)0x80;
					++lpMaskAddr;
				}
			} while (count--);

			if (error_term < 0)
			{
				if ((dst_bit <<= bit_shift) == 0)
				{
					dst_bit = (BYTE)0x01;
					--lpMaskAddr;
				}
				*lpMaskAddr ^= dst_bit;
			}
		}
		else
		{
			*lpMaskAddr ^= dst_bit;
			do
			{
				if ((dst_bit >>= bit_shift) == 0)
				{
					dst_bit = (BYTE)0x80;
					++lpMaskAddr;
				}
				if ((error_term += delta_y) >= 0)
				{
					if (count == 0)
						break;

					lpMaskAddr += dst_mask_incr;
					*lpMaskAddr ^= dst_bit;
					error_term -= delta_x;
				}
			} while (count--);
		}
	}
	else
	{
		if ((count = pLine->second.y - pLine->first.y) < 0)
			count = -count;

		do
		{
			*lpMaskAddr ^= dst_bit;
			lpMaskAddr += dst_mask_incr;
			if ((error_term += delta_x) >= 0)
			{
				if ((dst_bit >>= bit_shift) == 0)
				{
					dst_bit = (BYTE)0x80;
					++lpMaskAddr;
				}
				error_term -= delta_y;
			}
		} while (count--);
	}
}

static void
or_line (FRAMEPTR FramePtr, PBRESENHAM_LINE pLine)
{
	COORD dst_x, dst_y;
	COORD dst_mask_x;
	SIZE dst_mask_incr;
	SIZE delta_x, delta_y, error_term, count;
	BYTE dst_bit, dst_byte, bit_mask, bit_shift;
	PBYTE lpMaskAddr;

	dst_x = pLine->first.x;
	dst_y = pLine->first.y;
	delta_x = pLine->abs_delta_x;
	delta_y = pLine->abs_delta_y;
	error_term = pLine->error_term;

	dst_mask_x = dst_x;
	bit_mask = (BYTE)0x80;
	bit_shift = 1;

#ifndef __WATCOMC__
	lpMaskAddr = GetImageData (&FramePtr->MaskImage);
	dst_mask_incr = GetImageLineBytes (&FramePtr->MaskImage);
	lpMaskAddr += (dst_y * dst_mask_incr) + BYTE_POS (dst_mask_x);
#endif /* __WATCOMC__ */

	if (delta_y < 0)
	{
		delta_y = -delta_y;
		dst_mask_incr = -dst_mask_incr;
	}

	dst_bit = (BYTE)(bit_mask >> BYTE_OFFS (dst_mask_x));
	if (delta_x > delta_y)
	{
		count = pLine->second.x - pLine->first.x;
		dst_byte = 0;
		do
		{
			dst_byte |= dst_bit;
			if ((dst_bit >>= bit_shift) == 0)
			{
				dst_bit = bit_mask;
				*lpMaskAddr++ |= dst_byte;
				dst_byte = 0;
			}
			if ((error_term += delta_y) >= 0)
			{
				if (dst_byte)
				{
					*lpMaskAddr |= dst_byte;
					dst_byte = 0;
				}
				lpMaskAddr += dst_mask_incr;
				error_term -= delta_x;
			}
		} while (count--);
		if (dst_byte)
			*lpMaskAddr |= dst_byte;
	}
	else
	{
		if ((count = pLine->second.y - pLine->first.y) < 0)
			count = -count;
		do
		{
			*lpMaskAddr |= dst_bit;
			lpMaskAddr += dst_mask_incr;
			if ((error_term += delta_x) >= 0)
			{
				if ((dst_bit >>= bit_shift) == 0)
				{
					dst_bit = bit_mask;
					++lpMaskAddr;
				}
				error_term -= delta_y;
			}
		} while (count--);
	}
}

static void
on_off (FRAMEPTR FramePtr)
{
	register SIZE width, height;
	register SIZE dst_mask_incr;
	register PBYTE lpScan;

	width = GetFrameWidth (FramePtr);
	height = GetFrameHeight (FramePtr);
#ifndef __WATCOMC__
	lpScan = GetImageData (&FramePtr->MaskImage);
	dst_mask_incr = GetImageLineBytes (&FramePtr->MaskImage);
#endif /* __WATCOMC__ */
	width = BYTE_WIDTH (width);

	do
	{
		register SIZE byte_width;
		register BYTE mask_byte, bit;
		register PBYTE lpMask;

		byte_width = width;
		lpMask = lpScan;
start_scan:
		while (!(mask_byte = *lpMask))
		{
			if (--byte_width == 0)
				goto done_scan;
			++lpMask;
		}

		for (bit = (BYTE)0x80; !(bit & mask_byte); bit >>= 1)
			;

fill_scan:
		while (bit >>= 1)
		{
fill_byte:
			if (!(bit & mask_byte))
				mask_byte |= bit;
			else
			{
				while (bit >>= 1)
				{
					if (bit & mask_byte)
						goto fill_scan;
				}

				*lpMask++ = mask_byte;
				if (--byte_width == 0)
					goto done_scan;
				else
					goto start_scan;
			}
		}
		*lpMask++ = mask_byte;
		if (--byte_width > 0)
		{
			while (!(mask_byte = *lpMask))
			{
				*lpMask++ = 0xFF;
				if (--byte_width == 0)
					goto done_scan;
			}
			bit = (BYTE)0x80;
			goto fill_byte;
		}

done_scan:
		lpScan += dst_mask_incr;
	} while (--height);
}

static void
create_poly (PRECT pClipRect, PPOLYGON_DESC lpPolygon, FRAMEPTR
		FramePtr)
{
	COUNT i;
	COORD xoffs, yoffs;
	BRESENHAM_LINE Line;
	PPOINT pt1;

	xoffs = pClipRect->corner.x - POLY_XMIN;
	yoffs = pClipRect->corner.y - POLY_YMIN;

	if (lpPolygon->PtCount > 2)
	{
		COORD xmax, ymax;
		COORD ox, oy;
		COORD ystart;
		PPOINT pt2;
		DIRECTION last_ydir;
		BRESENHAM_LINE ShadowLine;

		xmax = POLY_XMIN + pClipRect->extent.width - 1;
		ymax = POLY_YMIN + pClipRect->extent.height - 1;
		ShadowLine.first.x = ShadowLine.second.x = POLY_XMIN - 1;
		ShadowLine.error_term = -1;
		ShadowLine.abs_delta_x = 0;
		ShadowLine.end_points_exchanged = FALSE;

		pt1 = lpPolygon->PtList;
		ystart = pt1->y;
		while ((oy = (++pt1)->y) == ystart)
			;
		ox = pt1->x;

		last_ydir = oy > ystart ? UP : DOWN;

		i = lpPolygon->PtCount;
		pt2 = &lpPolygon->PtList[i - 1];
		do
		{
			COORD x1, y1;

			if (pt1 != pt2)
				++pt1;
			else
				pt1 = lpPolygon->PtList;

			x1 = ox;
			y1 = oy;
			ox = pt1->x;
			oy = pt1->y;

			if (oy != y1)
			{
				COORD y2;
				INTERSECT_CODE intersect_code;

				Line.first.x = x1;
				Line.first.y = y1;
				Line.second.x = ox;
				Line.second.y = oy;
				LOGtoDEV ((PPOINT)&Line, (PPOINT)&Line, 2);
				y1 = Line.first.y - yoffs;
				y2 = Line.second.y - yoffs;

				{
					DIRECTION ydir;

					ydir = y2 > y1 ? UP : DOWN;
					if (ydir != last_ydir)
						last_ydir = ydir;
					else if (y1 >= POLY_YMIN && y1 <= ymax)
					{
						x1 = Line.first.x - xoffs;
						if (x1 < POLY_XMIN)
							xor_dot (FramePtr, POLY_XMIN - 1, y1);
						else if (x1 <= xmax)
							xor_dot (FramePtr, x1, y1);
					}
				}

				intersect_code = _clip_line (pClipRect, &Line);
				Line.first.x -= xoffs;
				Line.first.y -= yoffs;
				Line.second.x -= xoffs;
				Line.second.y -= yoffs;
				if (intersect_code)
				{
					xor_line (FramePtr, &Line);

					if (intersect_code & INTERSECT_LEFT)
					{
						if (Line.end_points_exchanged)
							y1 = y2;

						y2 = Line.first.y;
						if (y2 > POLY_YMIN && y1 < y2)
						{
							--y2;
							if (y1 < POLY_YMIN)
								y1 = POLY_YMIN;
							ShadowLine.first.y = y1;
							ShadowLine.second.y = y2;
							ShadowLine.abs_delta_y = y2 - y1;
							xor_line (FramePtr, &ShadowLine);
						}
						else if (y2 < ymax && y1 > y2)
						{
							++y2;
							if (y1 > ymax)
								y1 = ymax;
							ShadowLine.first.y = y2;
							ShadowLine.second.y = y1;
							ShadowLine.abs_delta_y = y1 - y2;
							xor_line (FramePtr, &ShadowLine);
						}
					}
				}
				else if (Line.first.x < POLY_XMIN
						&& (Line.first.y >= POLY_YMIN
						|| Line.second.y >= POLY_YMIN)
						&& (Line.first.y <= ymax
						|| Line.second.y <= ymax))
				{
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
					if (Line.first.y > Line.second.y)
					{
						y1 = MAX (Line.second.y, POLY_YMIN);
						y2 = MIN (Line.first.y, ymax);
					}
					else
					{
						y1 = MAX (Line.first.y, POLY_YMIN);
						y2 = MIN (Line.second.y, ymax);
					}

					if (Line.first.x == Line.second.x) /* vertical line */
					{
						ShadowLine.first.y = y1;
						ShadowLine.second.y = y2;
						ShadowLine.abs_delta_y = y2 - y1;
						xor_line (FramePtr, &ShadowLine);
					}
					else   /* sloped line */
					{
						SIZE line_delta_x, line_delta_y;
						long yinter;   /* y intercept with xmin */

						line_delta_x = Line.second.x - Line.first.x;
						line_delta_y = Line.second.y - Line.first.y;
						yinter = ((long)Line.first.y * line_delta_x)
								+ ((long)(POLY_XMIN - Line.first.x)
								* line_delta_y);
						if ((Line.first.y < Line.second.y
								&& yinter >= (long)(POLY_YMIN * line_delta_x))
								|| (Line.first.y > Line.second.y
								&& yinter <= (long)ymax * line_delta_x))
						{
							ShadowLine.first.y = y1;
							ShadowLine.second.y = y2;
							ShadowLine.abs_delta_y = y2 - y1;
							xor_line (FramePtr, &ShadowLine);
						}
					}
				}
			}
		} while (--i);

		on_off (FramePtr);
	}

	/* now outline the polygon */
	pt1 = lpPolygon->PtList;
	for (i = lpPolygon->PtCount - 1; i > 0; --i)
	{
		LOGtoDEV (pt1++, (PPOINT)&Line, 2);
		if (_clip_line (pClipRect, &Line))
		{
			Line.first.x -= xoffs;
			Line.first.y -= yoffs;
			Line.second.x -= xoffs;
			Line.second.y -= yoffs;
			or_line (FramePtr, &Line);
		}
	}

	Line.first = *pt1;
	Line.second = lpPolygon->PtList[0];
	LOGtoDEV ((PPOINT)&Line, (PPOINT)&Line, 2);
	if (_clip_line (pClipRect, &Line))
	{
		Line.first.x -= xoffs;
		Line.first.y -= yoffs;
		Line.second.x -= xoffs;
		Line.second.y -= yoffs;
		or_line (FramePtr, &Line);
	}
}

void
_fillpoly_blt (PRECT pClipRect, PRIMITIVEPTR PrimPtr)
{
#ifndef __WATCOMC__
	MEM_HANDLE h;
	FRAME_DESC PolyFrame;
	PRIMITIVE locPrim;

	SetFrameX (&PolyFrame, 0);
	SetFrameY (&PolyFrame, 0);
	SetFrameWidth (&PolyFrame, pClipRect->extent.width + POLY_XMIN);
	SetFrameHeight (&PolyFrame, pClipRect->extent.height + POLY_YMIN);
	SetImageShift (&PolyFrame.MaskImage, 0);
	AllocMaskImage (&PolyFrame, RAM_DRAWABLE, 0);
	SubImageFlags (&PolyFrame.PixmapImage, ~0);
	SetImageData (&PolyFrame.PixmapImage, NULL_PTR);
	AllocPixmapImage (&PolyFrame, RAM_DRAWABLE, 0);
	h = AllocGraphicData (GetImageByteCount (&PolyFrame.MaskImage));
	SetImageData (&PolyFrame.MaskImage, LockData (h));

	create_poly (pClipRect,
			(PPOLYGON_DESC)PrimPtr->Object.Polygon, &PolyFrame);

	_save_stamp.origin.x = pClipRect->corner.x - POLY_XMIN;
	_save_stamp.origin.y = pClipRect->corner.y - POLY_YMIN;
	locPrim.Object.Stamp.frame = &PolyFrame;
	SetPrimType (&locPrim, STAMPFILL_PRIM);
	SetPrimColor (&locPrim, GetPrimColor (PrimPtr));

	DrawGraphicsFunc (pClipRect, &locPrim);

	UnlockData (h);
	FreeMaskImage (h);
#endif /* __WATCOMC__ */
}

