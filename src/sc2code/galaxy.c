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

/* background starfield - used to generate agalaxy.asm */

#include "starcon.h"

//Added by Chris

void InsertPrim (PRIM_LINKS *pLinks, COUNT primIndex, COUNT iPI);

//End Added by Chris

#define BIG_STAR_COUNT 30
#define MED_STAR_COUNT 60
#define SML_STAR_COUNT 90
#define NUM_STARS (BIG_STAR_COUNT \
			+ MED_STAR_COUNT \
			+ SML_STAR_COUNT)

POINT SpaceOrg;
static POINT log_star_array[NUM_STARS];

#define NUM_STAR_PLANES 3

typedef struct
{
	COUNT min_star_index, num_stars;
	PPOINT star_array;
	PPOINT pmin_star, plast_star;
} STAR_BLOCK;
typedef STAR_BLOCK *PSTAR_BLOCK;

STAR_BLOCK StarBlock[NUM_STAR_PLANES] =
{
	{
		0, BIG_STAR_COUNT,
		&log_star_array[0],
		NULL, NULL,
	},
	{
		0, MED_STAR_COUNT,
		&log_star_array[BIG_STAR_COUNT],
		NULL, NULL,
	},
	{
		0, SML_STAR_COUNT,
		&log_star_array[BIG_STAR_COUNT + MED_STAR_COUNT],
		NULL, NULL,
	},
};

static void
SortStarBlock (PSTAR_BLOCK pStarBlock)
{
	COUNT i;

	for (i = 0; i < pStarBlock->num_stars; ++i)
	{
		COUNT j;

		for (j = pStarBlock->num_stars - 1; j > i; --j)
		{
			if (pStarBlock->star_array[i].y > pStarBlock->star_array[j].y)
			{
				POINT temp;

				temp = pStarBlock->star_array[i];
				pStarBlock->star_array[i] = pStarBlock->star_array[j];
				pStarBlock->star_array[j] = temp;
			}
		}
	}

	pStarBlock->min_star_index = 0;
	pStarBlock->pmin_star = &pStarBlock->star_array[0];
	pStarBlock->plast_star =
			&pStarBlock->star_array[pStarBlock->num_stars - 1];
}

static void
WrapStarBlock (SIZE plane, SIZE dx, SIZE dy)
{
	COUNT i;
	PPOINT ppt;
	SIZE offs_y;
	COUNT num_stars;
	PSTAR_BLOCK pStarBlock;

	pStarBlock = &StarBlock[plane];

	i = pStarBlock->min_star_index;
	ppt = pStarBlock->pmin_star;
	num_stars = pStarBlock->num_stars;
	if (dy < 0)
	{
		COUNT first;

		first = i;

		dy = -dy;
		offs_y = (LOG_SPACE_HEIGHT << plane) - dy;

		while (ppt->y < dy)
		{
			ppt->y += offs_y;
			ppt->x += dx;
			if (++i < num_stars)
				++ppt;
			else
			{
				i = 0;
				ppt = &pStarBlock->star_array[0];
			}

			if (i == first)
				return;
		}
		pStarBlock->min_star_index = i;
		pStarBlock->pmin_star = ppt;

		if (first <= i)
		{
			i = num_stars - i;
			do
			{
				ppt->y -= dy;
				ppt->x += dx;
				++ppt;
			} while (--i);
			ppt = &pStarBlock->star_array[0];
		}

		if (first > i)
		{
			i = first - i;
			do
			{
				ppt->y -= dy;
				ppt->x += dx;
				++ppt;
			} while (--i);
		}
	}
	else
	{
		COUNT last;

		--ppt;
		if (i-- == 0)
		{
			i = num_stars - 1;
			ppt = pStarBlock->plast_star;
		}

		last = i;

		if (dy > 0)
		{
			offs_y = (LOG_SPACE_HEIGHT << plane) - dy;

			while (ppt->y >= offs_y)
			{
				ppt->y -= offs_y;
				ppt->x += dx;
				if (i-- > 0)
					--ppt;
				else
				{
					i = num_stars - 1;
					ppt = pStarBlock->plast_star;
				}

				if (i == last)
					return;
			}

			pStarBlock->pmin_star = ppt + 1;
			if ((pStarBlock->min_star_index = i + 1) == num_stars)
			{
				pStarBlock->min_star_index = 0;
				pStarBlock->pmin_star = &pStarBlock->star_array[0];
			}
		}

		if (last >= i)
		{
			++i;
			do
			{
				ppt->y += dy;
				ppt->x += dx;
				--ppt;
			} while (--i);
			i = num_stars - 1;
			ppt = pStarBlock->plast_star;
		}

		if (last < i)
		{
			i = i - last;
			do
			{
				ppt->y += dy;
				ppt->x += dx;
				--ppt;
			} while (--i);
		}
	}
}

void
InitGalaxy (void)
{
	COUNT i, factor;
	PPOINT ppt;
	PRIM_LINKS Links;

// fprintf (stderr, "transition_width = %d transition_height = %d\n", TRANSITION_WIDTH, TRANSITION_HEIGHT);

	Links = MakeLinks (END_OF_LIST, END_OF_LIST);
	factor = ONE_SHIFT + MAX_REDUCTION + (BACKGROUND_SHIFT - 3);
	for (i = 0, ppt = log_star_array; i < NUM_STARS; ++i, ++ppt)
	{
		COUNT p;

		p = AllocDisplayPrim ();

		if (i == BIG_STAR_COUNT || i == BIG_STAR_COUNT + MED_STAR_COUNT)
			++factor;

		ppt->x = (COORD)((UWORD)TFB_Random () % SPACE_WIDTH) << factor;
		ppt->y = (COORD)((UWORD)TFB_Random () % SPACE_HEIGHT) << factor;

		if (i < BIG_STAR_COUNT + MED_STAR_COUNT)
		{
			SetPrimType (&DisplayArray[p], STAMP_PRIM);
			SetPrimColor (&DisplayArray[p],
					BUILD_COLOR (MAKE_RGB15 (0xB, 0xB, 0x1F), 0x09));
			DisplayArray[p].Object.Stamp.frame = stars_in_space;
		}
		else
		{
			SetPrimType (&DisplayArray[p], POINT_PRIM);
			if (LOBYTE (GLOBAL (CurrentActivity)) != IN_HYPERSPACE)
				SetPrimColor (&DisplayArray[p],
						BUILD_COLOR (MAKE_RGB15 (0x15, 0x15, 0x15), 0x07));
			else if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
				SetPrimColor (&DisplayArray[p],
						BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x8C));
			else
				SetPrimColor (&DisplayArray[p],
						BUILD_COLOR (MAKE_RGB15 (0x00, 0x0E, 0x00), 0x8C));
		}

		InsertPrim (&Links, p, GetPredLink (Links));
	}

	SortStarBlock (&StarBlock[0]);
	SortStarBlock (&StarBlock[1]);
	SortStarBlock (&StarBlock[2]);
}

void
MoveGalaxy (VIEW_STATE view_state, SIZE dx, SIZE dy)
{
	PPRIMITIVE pprim;

	if (view_state != VIEW_STABLE)
	{
#ifdef OLD_ZOOM
		BYTE reduction;
#else
		COUNT reduction;
#endif
		COUNT i;
		PPOINT ppt;

#ifdef OLD_ZOOM
		reduction = GLOBAL (cur_state);
#else
		{
			extern COUNT zoom_out;
			
			reduction = zoom_out;
		}
#endif
		if (view_state == VIEW_CHANGE)
		{
			pprim = DisplayArray;
			i = BIG_STAR_COUNT;
			if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE)
			{
				do
				{
					pprim->Object.Stamp.frame =
							SetAbsFrameIndex (stars_in_space,
							((COUNT)TFB_Random () & 31) + (32 + 26));
					++pprim;
				} while (--i);

				i = MED_STAR_COUNT;
				do
				{
					pprim->Object.Stamp.frame =
							SetAbsFrameIndex (stars_in_space,
							((COUNT)TFB_Random () & 31) + 26);
					++pprim;
				} while (--i);
			}
			else
			{
				GRAPHICS_PRIM star_object, big_star_obj;
				FRAME star_frame;

				star_frame = stars_in_space;
#ifdef OLD_ZOOM
				big_star_obj = STAMP_PRIM;
				if (reduction > 0)
					star_object = POINT_PRIM;
				else
				{
					star_frame = IncFrameIndex (star_frame);
					star_object = STAMP_PRIM;
				}
#else
				if (reduction > (1 << ZOOM_SHIFT))
					big_star_obj = star_object = POINT_PRIM;
				else
				{
					big_star_obj = star_object = STAMP_PRIM;
					star_frame = IncFrameIndex (star_frame);
					star_object = POINT_PRIM;
				}
#endif

				do
				{
					SetPrimType (pprim, big_star_obj);
					pprim->Object.Stamp.frame = star_frame;
					++pprim;
				} while (--i);

				pprim = &DisplayArray[BIG_STAR_COUNT];
				i = MED_STAR_COUNT;
				do
				{
					SetPrimType (pprim, star_object);
					++pprim;
				} while (--i);
			}
		}

		if (LOBYTE (GLOBAL (CurrentActivity)) == IN_HYPERSPACE)
		{
			pprim = DisplayArray;
			i = BIG_STAR_COUNT + MED_STAR_COUNT;
			do
			{
				COUNT base_index;

				base_index = GetFrameIndex (pprim->Object.Stamp.frame) - 26;
				pprim->Object.Stamp.frame =
						SetAbsFrameIndex (pprim->Object.Stamp.frame,
						((base_index & ~31) + ((base_index + 1) & 31)) + 26);
				++pprim;
			} while (--i);

			dx <<= 3;
			dy <<= 3;
		}

		WrapStarBlock (2, dx, dy);
		WrapStarBlock (1, dx, dy);
		WrapStarBlock (0, dx, dy);

		if (LOBYTE (GLOBAL (CurrentActivity)) != IN_HYPERSPACE)
		{
			dx = SpaceOrg.x;
			dy = SpaceOrg.y;
#ifdef OLD_ZOOM
			reduction += ONE_SHIFT;
#else
			reduction <<= ONE_SHIFT;
#endif
		}
		else
		{
			dx = (COORD)(LOG_SPACE_WIDTH >> 1)
					- (LOG_SPACE_WIDTH >> ((MAX_REDUCTION + 1)
					- MAX_VIS_REDUCTION));
			dy = (COORD)(LOG_SPACE_HEIGHT >> 1)
					- (LOG_SPACE_HEIGHT >> ((MAX_REDUCTION + 1)
					- MAX_VIS_REDUCTION));
#ifdef OLD_ZOOM
			reduction = (COUNT)(MAX_VIS_REDUCTION + ONE_SHIFT);
#else
			reduction = MAX_ZOOM_OUT << ONE_SHIFT;
#endif
		}

		ppt = log_star_array;
		pprim = DisplayArray;

		if (view_state == VIEW_CHANGE
#ifdef OLD_ZOOM
				|| (int)pprim->Object.Point.x != (int)((ppt->x - dx) >> reduction)
				|| (int)pprim->Object.Point.y != (int)((ppt->y - dy) >> reduction))
#else
				|| (int)pprim->Object.Point.x != (int)(((ppt->x - dx) << ZOOM_SHIFT) / reduction)
				|| (int)pprim->Object.Point.y != (int)(((ppt->y - dy) << ZOOM_SHIFT) / reduction))
#endif
		{
			i = BIG_STAR_COUNT;
			do
			{
// ppt->x &= (LOG_SPACE_WIDTH - 1);
ppt->x = WRAP_VAL (ppt->x, LOG_SPACE_WIDTH);
#ifdef OLD_ZOOM
				pprim->Object.Point.x = (ppt->x - dx) >> reduction;
				pprim->Object.Point.y = (ppt->y - dy) >> reduction;
#else
				pprim->Object.Point.x = ((ppt->x - dx) << ZOOM_SHIFT) / reduction;
				pprim->Object.Point.y = ((ppt->y - dy) << ZOOM_SHIFT) / reduction;
#endif
				++pprim, ++ppt;
			} while (--i);
			dx <<= 1;
			dy <<= 1;
#ifdef OLD_ZOOM
			++reduction;
#else
			reduction <<= 1;
#endif

			if (view_state == VIEW_CHANGE
#ifdef OLD_ZOOM
					|| pprim->Object.Point.x != (ppt->x - dx) >> reduction
					|| pprim->Object.Point.y != (ppt->y - dy) >> reduction)
#else
					|| (int)(pprim->Object.Point.x) != (int)(((ppt->x - dx) << ZOOM_SHIFT) / reduction)
					|| (int)(pprim->Object.Point.y) != (int)(((ppt->y - dy) << ZOOM_SHIFT) / reduction))
#endif
			{
				i = MED_STAR_COUNT;
				do
				{
// ppt->x &= ((LOG_SPACE_WIDTH << 1) - 1);
ppt->x = WRAP_VAL (ppt->x, LOG_SPACE_WIDTH << 1);
#ifdef OLD_ZOOM
					pprim->Object.Point.x = (ppt->x - dx) >> reduction;
					pprim->Object.Point.y = (ppt->y - dy) >> reduction;
#else
					pprim->Object.Point.x = ((ppt->x - dx) << ZOOM_SHIFT) / reduction;
					pprim->Object.Point.y = ((ppt->y - dy) << ZOOM_SHIFT) / reduction;
#endif
					++pprim, ++ppt;
				} while (--i);
				dx <<= 1;
				dy <<= 1;
#ifdef OLD_ZOOM
				++reduction;
#else
				reduction <<= 1;
#endif

				if (view_state == VIEW_CHANGE
#ifdef OLD_ZOOM
						|| pprim->Object.Point.x != (ppt->x - dx) >> reduction
						|| pprim->Object.Point.y != (ppt->y - dy) >> reduction)
#else
						|| (int)(pprim->Object.Point.x) != (int)(((ppt->x - dx) << ZOOM_SHIFT) / reduction)
						|| (int)(pprim->Object.Point.y) != (int)(((ppt->y - dy) << ZOOM_SHIFT) / reduction))
#endif
				{
					i = SML_STAR_COUNT;
					do
					{
// ppt->x &= ((LOG_SPACE_WIDTH << 2) - 1);
ppt->x = WRAP_VAL (ppt->x, LOG_SPACE_WIDTH << 2);
#ifdef OLD_ZOOM
						pprim->Object.Point.x = (ppt->x - dx) >> reduction;
						pprim->Object.Point.y = (ppt->y - dy) >> reduction;
#else
						pprim->Object.Point.x = ((ppt->x - dx) << ZOOM_SHIFT) / reduction;
						pprim->Object.Point.y = ((ppt->y - dy) << ZOOM_SHIFT) / reduction;
#endif
						++pprim, ++ppt;
					} while (--i);
				}
			}
		}
	}

	{
		extern PRIM_LINKS DisplayLinks;

		DisplayLinks = MakeLinks (NUM_STARS - 1, 0);
	}
}

