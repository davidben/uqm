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

#include "gfxlib.h"
#include "scan.h"
#include "libs/compiler.h"
#include "libs/graphics/drawable.h"

typedef struct
{
	BYTE bit0, bit1, bit2, bit3;
	COUNT index0, index1, index2, index3;
} MAP_BLOCK;

BOOLEAN
MapMask (FRAMEPTR SrcFramePtr, FRAMEPTR DstFramePtr, PRECT pSrcRect,
		PSCAN_BLOCK pscan_block)
{
	COUNT num_scans;
	COORD loc_x, x;
	PBYTE lpSrcMask, lpDstMask;
	SIZE width, height, src_error, src_delta, src_bump;
	COUNT src_mask_incr, src_byte_count, dst_byte_count;
	MAP_BLOCK SrcBlock;
	PPOINT point_array;
	PSCAN_DESC scan_array;

	loc_x = pSrcRect->corner.x;
	width = pSrcRect->extent.width;
	height = pSrcRect->extent.height;

	src_mask_incr = SCAN_WIDTH (GetFrameWidth (SrcFramePtr));
	lpSrcMask = (BYTE *)SrcFramePtr + SrcFramePtr->DataOffs;
	if (0)
		lpSrcMask += GetFrameWidth (SrcFramePtr)
				* GetFrameHeight (SrcFramePtr);
	src_byte_count = src_mask_incr
			* GetFrameHeight (SrcFramePtr);

	lpDstMask = (BYTE *)DstFramePtr + DstFramePtr->DataOffs;
	if (0)
		lpDstMask += GetFrameWidth (DstFramePtr)
				* GetFrameHeight (DstFramePtr);
	dst_byte_count = SCAN_WIDTH (GetFrameWidth (DstFramePtr))
			* GetFrameHeight (DstFramePtr);
	memset (lpDstMask, 0, dst_byte_count * 3);

	x = loc_x + (width >> 1);
	SrcBlock.bit0 = SrcBlock.bit1 =
			(BYTE)((BYTE)0x80 >> (BYTE)BYTE_OFFS (x));
	SrcBlock.index0 = SrcBlock.index1 = BYTE_POS (x);
	SrcBlock.bit2 = (BYTE)((BYTE)0x80 >> (BYTE)BYTE_OFFS (loc_x));
	SrcBlock.index2 = BYTE_POS (loc_x);
	x = loc_x + width - 1;
	SrcBlock.bit3 = (BYTE)((BYTE)0x80 >> (BYTE)BYTE_OFFS (x));
	SrcBlock.index3 = BYTE_POS (x);

	num_scans = pscan_block->num_scans;

	src_delta = (height - 1) << 1;
	src_bump = (num_scans - pscan_block->num_same_scans - 1) << 1;
	src_error = src_bump;

	point_array = pscan_block->line_base;
	scan_array = pscan_block->scan_base;
	do
	{
		COORD y;

		y = scan_array->num_dots >> 1;
		if (y)
		{
			COORD offs_x, offs_y;
			SIZE error_term, bump, incr;
			PPOINT pdot0, pdot1, pdot2, pdot3;
			SIZE ig, idgr, idgd;
			MAP_BLOCK SBlock, DBlock;

			offs_x = scan_array->start.x;
			offs_y = scan_array->start.y;
			pdot0 = pdot1 = &point_array[scan_array->start_dot + y];
			pdot2 = &point_array[scan_array->start_dot];
			pdot3 = &point_array[scan_array->start_dot + scan_array->num_dots - 1];

			x = pdot0->x + offs_x;
			DBlock.bit0 = DBlock.bit1 =
					(BYTE)((BYTE)0x80 >> (BYTE)BYTE_OFFS (x));
			DBlock.index0 = DBlock.index1 = BYTE_POS (x) + pdot0->y + offs_y;
			x = pdot2->x + offs_x;
			DBlock.bit2 = (BYTE)((BYTE)0x80 >> (BYTE)BYTE_OFFS (x));
			DBlock.index2 = BYTE_POS (x) + pdot2->y + offs_y;
			x = pdot3->x + offs_x;
			DBlock.bit3 = (BYTE)((BYTE)0x80 >> (BYTE)BYTE_OFFS (x));
			DBlock.index3 = BYTE_POS (x) + pdot3->y + offs_y;

			SBlock = SrcBlock;

			incr = scan_array->dots_per_semi << 1;
			bump = width << 1;
			error_term = width;

			x = 0;
			ig = (y << 1) - 3;
			idgr = -6;
			idgd = (y << 2) - 10;
			do
			{
				if (lpSrcMask[SBlock.index0] & SBlock.bit0)
					lpDstMask[DBlock.index0] |= DBlock.bit0;
				SBlock.index0 += src_byte_count;
				DBlock.index0 += dst_byte_count;
				if (lpSrcMask[SBlock.index0] & SBlock.bit0)
					lpDstMask[DBlock.index0] |= DBlock.bit0;
				SBlock.index0 += src_byte_count;
				DBlock.index0 += dst_byte_count;
				if (lpSrcMask[SBlock.index0] & SBlock.bit0)
					lpDstMask[DBlock.index0] |= DBlock.bit0;
				if ((SBlock.bit0 >>= 1) == 0)
				{
					SBlock.bit0 = (BYTE)0x80;
					++SBlock.index0;
				}
				SBlock.index0 -= src_byte_count << 1;
				DBlock.index0 -= dst_byte_count << 1;

				if (lpSrcMask[SBlock.index1] & SBlock.bit1)
					lpDstMask[DBlock.index1] |= DBlock.bit1;
				SBlock.index1 += src_byte_count;
				DBlock.index1 += dst_byte_count;
				if (lpSrcMask[SBlock.index1] & SBlock.bit1)
					lpDstMask[DBlock.index1] |= DBlock.bit1;
				SBlock.index1 += src_byte_count;
				DBlock.index1 += dst_byte_count;
				if (lpSrcMask[SBlock.index1] & SBlock.bit1)
					lpDstMask[DBlock.index1] |= DBlock.bit1;
				if ((SBlock.bit1 <<= 1) == 0)
				{
					SBlock.bit1 = (BYTE)0x01;
					--SBlock.index1;
				}
				SBlock.index1 -= src_byte_count << 1;
				DBlock.index1 -= dst_byte_count << 1;

				if (lpSrcMask[SBlock.index2] & SBlock.bit2)
					lpDstMask[DBlock.index2] |= DBlock.bit2;
				SBlock.index2 += src_byte_count;
				DBlock.index2 += dst_byte_count;
				if (lpSrcMask[SBlock.index2] & SBlock.bit2)
					lpDstMask[DBlock.index2] |= DBlock.bit2;
				SBlock.index2 += src_byte_count;
				DBlock.index2 += dst_byte_count;
				if (lpSrcMask[SBlock.index2] & SBlock.bit2)
					lpDstMask[DBlock.index2] |= DBlock.bit2;
				if ((SBlock.bit2 >>= 1) == 0)
				{
					SBlock.bit2 = (BYTE)0x80;
					++SBlock.index2;
				}
				SBlock.index2 -= src_byte_count << 1;
				DBlock.index2 -= dst_byte_count << 1;

				if (lpSrcMask[SBlock.index3] & SBlock.bit3)
					lpDstMask[DBlock.index3] |= DBlock.bit3;
				SBlock.index3 += src_byte_count;
				DBlock.index3 += dst_byte_count;
				if (lpSrcMask[SBlock.index3] & SBlock.bit3)
					lpDstMask[DBlock.index3] |= DBlock.bit3;
				SBlock.index3 += src_byte_count;
				DBlock.index3 += dst_byte_count;
				if (lpSrcMask[SBlock.index3] & SBlock.bit3)
					lpDstMask[DBlock.index3] |= DBlock.bit3;
				if ((SBlock.bit3 <<= 1) == 0)
				{
					SBlock.bit3 = (BYTE)0x01;
					--SBlock.index3;
				}
				SBlock.index3 -= src_byte_count << 1;
				DBlock.index3 -= dst_byte_count << 1;

				if ((error_term -= incr) <= 0)
				{
					error_term += bump;
					if (ig >= 0)
					{
						ig += idgr;
						idgd -= 4;
					}
					else
					{
						ig += idgd;
						idgd -= 8;
						--y;
						++pdot2;
						loc_x = pdot2->x + offs_x;
						DBlock.bit2 =
								(BYTE)((BYTE)0x80 >> (BYTE)BYTE_OFFS (loc_x));
						DBlock.index2 = BYTE_POS (loc_x) + pdot2->y + offs_y;
						--pdot3;
						loc_x = pdot3->x + offs_x;
						DBlock.bit3 =
								(BYTE)((BYTE)0x80 >> (BYTE)BYTE_OFFS (loc_x));
						DBlock.index3 = BYTE_POS (loc_x) + pdot3->y + offs_y;
					}
					idgr -= 4;
					++x;
					++pdot0;
					loc_x = pdot0->x + offs_x;
					DBlock.bit0 =
							(BYTE)((BYTE)0x80 >> (BYTE)BYTE_OFFS (loc_x));
					DBlock.index0 = BYTE_POS (loc_x) + pdot0->y + offs_y;
					--pdot1;
					loc_x = pdot1->x + offs_x;
					DBlock.bit1 =
							(BYTE)((BYTE)0x80 >> (BYTE)BYTE_OFFS (loc_x));
					DBlock.index1 = BYTE_POS (loc_x) + pdot1->y + offs_y;
				}
			} while (y >= x);
		}

		if ((src_error -= src_delta) <= 0)
		{
			if (--height > 0)
			{
				src_error += src_bump;
				lpSrcMask += src_mask_incr;
			}
		}
		++scan_array;
	} while (--num_scans);

	return (TRUE);
}

