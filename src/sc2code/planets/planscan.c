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

#include "graphics/gfxintrn.h"
#include "scan.h"
#include "units.h"

void
GenerateRotatedScans (FRAMEPTR FramePtr, PSCAN_BLOCK pscan_block, COUNT
		angle)
{
	SIZE width, height, dst_mask_incr;
	PBYTE lpBase;
	SIZE cosA1xcosA2, sinA1xcosA2,
				sinA1xsinA2, cosA1xsinA2;
	POINT center;
	LINE left_edge, right_edge;
	SIZE delta_x0, delta_y0, delta_x1, delta_y1;
	SIZE xincr0, yincr0, xincr1, yincr1;
	SIZE error0;
	SIZE xerror1, yerror1;
	SIZE bump0, bump1, nd, num_dots0, num_dots1;
	PPOINT point_array;
	PSCAN_DESC scan_array;

	width = GetFrameWidth (FramePtr);
	height = GetFrameHeight (FramePtr);
	dst_mask_incr = SCAN_WIDTH (width);
	lpBase = (PBYTE)FramePtr + FramePtr->DataOffs;
	if (0)
		lpBase += width * height;

	center.x = width >> 1;
	center.y = height >> 1;

	if (width > height)
		bump0 = (width >> 1);
	else
		bump0 = (height >> 1);
	bump0 += 4; /* allow for a little extra room */

	cosA1xcosA2 = sinA1xcosA2 = COSINE (angle, bump0);
	sinA1xsinA2 = cosA1xsinA2 = SINE (angle, bump0);
	left_edge.first.x = (-cosA1xcosA2 - -sinA1xsinA2) + center.x;
	left_edge.first.y = (-cosA1xsinA2 + -sinA1xcosA2) + center.y;
	left_edge.second.x = (-cosA1xcosA2 - sinA1xsinA2) + center.x;
	left_edge.second.y = (-cosA1xsinA2 + sinA1xcosA2) + center.y;
	right_edge.first.x = (cosA1xcosA2 - -sinA1xsinA2) + center.x;
	right_edge.first.y = (cosA1xsinA2 + -sinA1xcosA2) + center.y;
	right_edge.second.x = (cosA1xcosA2 - sinA1xsinA2) + center.x;
	right_edge.second.y = (cosA1xsinA2 + sinA1xcosA2) + center.y;

	delta_x0 = left_edge.second.x - left_edge.first.x;
	if (delta_x0 >= 0)
		xincr0 = 1;
	else
	{
		xincr0 = -1;
		delta_x0 = -delta_x0;
	}
	delta_y0 = left_edge.second.y - left_edge.first.y;
	if (delta_y0 >= 0)
		yincr0 = 1;
	else
	{
		yincr0 = -1;
		delta_y0 = -delta_y0;
	}

	num_dots0 = delta_x0 + delta_y0; /* four connect */
	delta_x0 <<= 1;
	error0 = bump0 = num_dots0 << 1;

	delta_x1 = right_edge.first.x - left_edge.first.x;
	if (delta_x1 >= 0)
		xincr1 = 1;
	else
	{
		xincr1 = -1;
		delta_x1 = -delta_x1;
	}
	delta_y1 = right_edge.first.y - left_edge.first.y;
	if (delta_y1 >= 0)
		yincr1 = 1;
	else
	{
		yincr1 = -1;
		delta_y1 = -delta_y1;
	}

	if (delta_x1 >= delta_y1)
	{
		num_dots1 = delta_x1;
		delta_x1 <<= 1;
		delta_y1 <<= 1;
		bump1 = delta_x1;
	}
	else
	{
		num_dots1 = delta_y1;
		delta_x1 <<= 1;
		delta_y1 <<= 1;
		bump1 = delta_y1;
	}

#ifdef DEBUG
{
LINE loc_line, loc_left_edge, loc_right_edge;

loc_left_edge = left_edge;
loc_left_edge.first.x += 160 - center.x;
loc_left_edge.first.y += 100 - center.y;
loc_left_edge.second.x += 160 - center.x;
loc_left_edge.second.y += 100 - center.y;
loc_right_edge = right_edge;
loc_right_edge.first.x += 160 - center.x;
loc_right_edge.first.y += 100 - center.y;
loc_right_edge.second.x += 160 - center.x;
loc_right_edge.second.y += 100 - center.y;

loc_line = loc_left_edge;
DrawLine (&loc_line);
loc_line.second = loc_right_edge.first;
DrawLine (&loc_line);
loc_line.first = loc_right_edge.second;
DrawLine (&loc_line);
loc_line.second = loc_left_edge.second;
DrawLine (&loc_line);
}
#endif /* DEBUG */

	pscan_block->num_same_scans = 0;

	point_array = pscan_block->line_base;
	left_edge.second.x = left_edge.second.y = 0;
	xerror1 = yerror1 = nd = num_dots1;
	do
	{
		*point_array++ = left_edge.second;
		if ((xerror1 -= delta_x1) <= 0)
		{
			left_edge.second.x += xincr1;
			xerror1 += bump1;
		}
		if ((yerror1 -= delta_y1) <= 0)
		{
			left_edge.second.y += yincr1 * dst_mask_incr;
			yerror1 += bump1;
		}

	} while (nd--);

	scan_array = pscan_block->scan_base;
	do
	{
		BYTE on;
		PBYTE lpScan;

		on = 0;
		left_edge.second = left_edge.first;
		lpScan = lpBase + (left_edge.second.y * dst_mask_incr);

		xerror1 = yerror1 = nd = num_dots1;
#ifdef DEBUG
SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x1F, 0xA, 0xA), 0x0C));
#endif /* DEBUG */
		do
		{
#ifdef DEBUG
POINT pt;
pt.x = left_edge.second.x + 160 - center.x;
pt.y = left_edge.second.y + 100 - center.y;
#endif /* DEBUG */
			if (!(on & 1))
			{
				if (left_edge.second.x >= 0 && left_edge.second.y >= 0
						&& left_edge.second.x < width
						&& left_edge.second.y < height
						&& (lpScan[BYTE_POS (left_edge.second.x)]
						& (BYTE)((BYTE)0x80 >> BYTE_OFFS (left_edge.second.x))))
				{
#ifdef DEBUG
SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));
DrawPoint (&pt);
#endif /* DEBUG */
					scan_array->start = left_edge.first;
					scan_array->start.y *= dst_mask_incr;

					scan_array->start_dot = num_dots1 - nd;
					scan_array->num_dots = nd;

					++on;
				}
#ifdef DEBUG
else
DrawPoint (&pt);
#endif /* DEBUG */
			}
			else
			{
				if (left_edge.second.x < 0 || left_edge.second.y < 0
						|| left_edge.second.x >= width
						|| left_edge.second.y >= height
						|| !(lpScan[BYTE_POS (left_edge.second.x)]
						& (BYTE)((BYTE)0x80 >> BYTE_OFFS (left_edge.second.x))))
				{
#ifdef DEBUG
SetContextForeGroundColor (BUILD_COLOR (MAKE_RGB15 (0x1F, 0xA, 0xA), 0x0C));
#endif /* DEBUG */
					scan_array->num_dots -= nd;
						/* calculate number of dots in semi-circle */
					scan_array->dots_per_semi = (SIZE)((DWORD)(scan_array->num_dots
							* SCALED_ROOT_TWO) >> SCALE_FACTOR);
					if (on > 1)
					{
						scan_array->num_dots |= SAME_SCAN;
						++pscan_block->num_same_scans;
					}
#ifdef DEBUG
					fprintf (stderr, "<%d, %d>-<%d, %d>, %d, %d, %u%s\n",
							scan_array->start.x, scan_array->start.y,
							scan_array->end.x, scan_array->end.y,
							scan_array->xerror, scan_array->yerror,
							scan_array->num_dots & ~SAME_SCAN,
							(scan_array->num_dots & SAME_SCAN) ?
							", SAME SCAN" : "");
#endif /* DEBUG */
					++scan_array;

					++on;
				}
#ifdef DEBUG
DrawPoint (&pt);
#endif /* DEBUG */
			}

			if ((xerror1 -= delta_x1) <= 0)
			{
				left_edge.second.x += xincr1;
				xerror1 += bump1;
			}
			if ((yerror1 -= delta_y1) <= 0)
			{
				left_edge.second.y += yincr1;
				yerror1 += bump1;

				lpScan = lpBase + (left_edge.second.y * dst_mask_incr);
			}

		} while (nd--);
		if (on & 1)
		{
			scan_array->num_dots -= nd;
				/* calculate number of dots in semi-circle */
			scan_array->dots_per_semi = (SIZE)((DWORD)(scan_array->num_dots
					* SCALED_ROOT_TWO) >> SCALE_FACTOR);
			if (on > 1)
			{
				scan_array->num_dots |= SAME_SCAN;
				++pscan_block->num_same_scans;
			}
#ifdef DEBUG
			fprintf (stderr, "<%d, %d>-<%d, %d>, %d, %d, %u\n",
					scan_array->start.x, scan_array->start.y,
					scan_array->end.x, scan_array->end.y,
					scan_array->xerror, scan_array->yerror,
					scan_array->num_dots & ~SAME_SCAN,
					(scan_array->num_dots & SAME_SCAN) ? ", SAME SCAN" : "");
#endif /* DEBUG */
			++scan_array;
		}

		if ((error0 -= delta_x0) > 0)
			left_edge.first.y += yincr0;
		else
		{
			left_edge.first.x += xincr0;
			error0 += bump0;
		}

	} while (num_dots0--);

	pscan_block->num_scans = (COUNT)(scan_array - pscan_block->scan_base);
#ifdef DEBUG
	{
		extern DWORD AnyButtonPress (BOOLEAN CheckSpecial);
		fprintf (stderr, "num_scans = %u\n", pscan_block->num_scans);
		while (!AnyButtonPress (TRUE))
			;
	}
#endif /* DEBUG */
}



