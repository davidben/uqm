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

//#include "Portfolio.h" //A 3DO library we don't care about
//#include "Init3DO.h"
//#include "Parse3DO.h"
//#include "Utils3DO.h"
//#include "filefunctions.h"
//#include "BlockFile.h"
#include "starcon.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#define PLANET_SPIN 0
#define PLANET_SPIN_DEBUG 1
//Added by Chris

//End Added by Chris

//#define KDEBUG
//#define SCALE_ROTATE

//#define SCREEN_WIDTH 320

#define WIDTH 210
#define HEIGHT 67
#define XBANDS 6

#define SPHERE_HEIGHT (HEIGHT + 10)
#define SPHERE_WIDTH SPHERE_HEIGHT

#define WRAP_EXTRA 20

#define LINE_PIXELS ((WIDTH + WRAP_EXTRA + 3) & ~3)

#define SURFACE_WIDTH 210
#define SURFACE_HEIGHT 124

#define SURFACE_SHIFT 2
#define SURFACE_UNIT (1 << SURFACE_SHIFT)
#define SURFACE_ADD (SURFACE_UNIT - 1)

#define NUM_TINTS 4

#define PIXC_DUP(v) (((unsigned int)(v) << PPMP_0_SHIFT) | ((unsigned int)(v) << PPMP_1_SHIFT))
#define PIXC_UNCODED16 (PIXC_DUP (PPMPC_MF_8 | PPMPC_SF_8))
#define PIXC_UNCODED8 (PIXC_DUP (PPMPC_MF_8 | PPMPC_SF_8))
#define PIXC_CODED (PIXC_DUP (PPMPC_MS_PIN | PPMPC_SF_8))

#define SET_CEL_FLAGS(c,f) ((c)->ccb_Flags |= (f))
#define CLR_CEL_FLAGS(c,f) ((c)->ccb_Flags &= ~(f))

#define CEL_FLAGS \
		(CCB_SPABS | CCB_PPABS | CCB_YOXY | CCB_ACW | CCB_ACCW | CCB_LDSIZE | CCB_CCBPRE \
		| CCB_LDPRS | CCB_LDPPMP | CCB_ACE | CCB_LCE | CCB_PLUTPOS | CCB_BGND | CCB_NPABS \
		/*| CCB_NOBLK*/)

#define MAP_CEL myMapCel

#define NUM_ROTATE_CELS (138) // derived empirically!

#define NUM_CELS (NUM_ROTATE_CELS * 2) // must be at least 257 for oscilloscope

#define MIN_Y (-(SURFACE_HEIGHT >> 1))
#define MAX_Y ((HEIGHT << SURFACE_SHIFT) - (SURFACE_HEIGHT >> 1))

#define PULSE_SCALE (2)
#define SCALE_PULSE_RED(r) ((r)<<PULSE_SCALE)
#define MIN_PULSE_RED (SCALE_PULSE_RED(2))
#define MAX_PULSE_RED (SCALE_PULSE_RED(7))
#define PULSE_NEG_BIT (1 << 7)

#define SOL 82
#define EARTH 2
#define LUNA 2

typedef struct
{
	int x0[15], x1[15], y0[15], y1[15];
	int ystep[15];
	int src_ws[XBANDS];

	CCB planet_ccb, surface_ccb, tint_ccb[NUM_TINTS];
	
	CCB black_rect_ccb, black_circ_ccb, *pblack_circ_ccb, *pshield_ccb, *pshadow_ccb;
	
	CCB repair_ccb[2];
	
	int black_circ_plut[16];

	UBYTE rmap[HEIGHT][LINE_PIXELS];

	CCB *cur_ccb;

	int old_sx, old_sy;
	
	CCB plCCB[NUM_CELS];
	
	CCB *last_zoom_ccb;

	UBYTE zoom_batch;

#ifdef SCALE_ROTATE
	int scale;
#endif

	CCB *pcirc_ccb, *pblur_ccb;
} PLANET_STUFF;

static PLANET_STUFF *planet_stuff=NULL;

#define x0 (planet_stuff->x0)
#define x1 (planet_stuff->x1)
#define y0 (planet_stuff->y0)
#define y1 (planet_stuff->y1)
#define ystep (planet_stuff->ystep)
#define src_ws (planet_stuff->src_ws)

#define planet_ccb (planet_stuff->planet_ccb)
#define surface_ccb (planet_stuff->surface_ccb)
#define tint_ccb (planet_stuff->tint_ccb)

#define black_rect_ccb (planet_stuff->black_rect_ccb)

#define black_circ_ccb (planet_stuff->black_circ_ccb)

#define pshadow_ccb (planet_stuff->pshadow_ccb)

#define pshield_ccb (planet_stuff->pshield_ccb)

#define pblack_circ_ccb (planet_stuff->pblack_circ_ccb)

#define black_circ_plut (planet_stuff->black_circ_plut)

#define repair_ccb (planet_stuff->repair_ccb)

#define rmap (planet_stuff->rmap)

#define cur_ccb (planet_stuff->cur_ccb)

#define old_sx (planet_stuff->old_sx)
#define old_sy (planet_stuff->old_sy)

#define plCCB (planet_stuff->plCCB)

#define last_zoom_ccb (planet_stuff->last_zoom_ccb)
#define zoom_batch (planet_stuff->zoom_batch)

#define shield_pulse (zoom_batch)

#ifdef SCALE_ROTATE
#define scale (planet_stuff->scale)

#define SCALE_AMT_SHIFT 7
#define SCALE_AMT_MASK ((1<<(SCALE_AMT_SHIFT+1))-1)
#define SCALE_X_SHIFT 9
#define SCALE_X_MASK (3<<SCALE_X_SHIFT)
#define SCALE_Y_SHIFT 12
#define SCALE_Y_MASK (3<<SCALE_Y_SHIFT)
#define SCALE_MAX (1<<SCALE_AMT_SHIFT)
#define SCALE_STEP ((scale & SCALE_AMT_MASK) < (SCALE_MAX / 2) ? \
						4 : ((scale & SCALE_AMT_MASK) < (SCALE_MAX * 3 / 4) ? \
						3 : ((scale & SCALE_AMT_MASK) < (SCALE_MAX * 7 / 8) ? \
						2 : 1)))

#define SCALE_CEL(ccb,cx,cy) \
		do \
		{ \
			int s, _dx, _dy; \
			\
			s = scale & SCALE_AMT_MASK; \
			_dx = (int)((scale & SCALE_X_MASK) >> SCALE_X_SHIFT) - 1; \
			_dy = (int)((scale & SCALE_Y_MASK) >> SCALE_Y_SHIFT) - 1; \
			(ccb)->ccb_XPos = (((ccb)->ccb_XPos - (cx)) * s) >> SCALE_AMT_SHIFT; \
			(ccb)->ccb_YPos = (((ccb)->ccb_YPos - (cy)) * s) >> SCALE_AMT_SHIFT; \
			if (_dx < 0) \
				(ccb)->ccb_XPos += ((cx) * s) >> SCALE_AMT_SHIFT; \
			else if (_dx > 0) \
				(ccb)->ccb_XPos += ((cx) << 1) - (((cx) * s) >> SCALE_AMT_SHIFT); \
			else \
				(ccb)->ccb_XPos += (cx); \
			if (_dy < 0) \
				(ccb)->ccb_YPos += ((cy) * s) >> SCALE_AMT_SHIFT; \
			else if (_dy > 0) \
				(ccb)->ccb_YPos += ((cy) << 1) - (((cy) * s) >> SCALE_AMT_SHIFT); \
			else \
				(ccb)->ccb_YPos += (cy); \
			(ccb)->ccb_HDX = ((ccb)->ccb_HDX * s) >> SCALE_AMT_SHIFT; \
			(ccb)->ccb_VDY = ((ccb)->ccb_VDY * s) >> SCALE_AMT_SHIFT; \
			(ccb)->ccb_VDX = ((ccb)->ccb_VDX * s) >> SCALE_AMT_SHIFT; \
			(ccb)->ccb_HDY = ((ccb)->ccb_HDY * s) >> SCALE_AMT_SHIFT; \
			(ccb)->ccb_HDDX = ((ccb)->ccb_HDDX * s) >> SCALE_AMT_SHIFT; \
			(ccb)->ccb_HDDY = ((ccb)->ccb_HDDY * s) >> SCALE_AMT_SHIFT; \
		} while (0)
#endif

#define pcirc_ccb (planet_stuff->pcirc_ccb)
#define pblur_ccb (planet_stuff->pblur_ccb)

#if PLANET_SPIN
void add_cel(CCB* cel) {}
void add_cels(CCB* first, CCB* last) {}
void myMapCel(CCB* myCCB, POINT Points[4]) {}
void OpenMathFolio(void) {}

static void
build_steps ()
{
	int y, i, c, inc;

	c = 1;
	inc = 1;
	y = 0;
	for (i = 0; y < (HEIGHT >> 1); ++i)
	{
		ystep[i] = c;
		y += c;
		if (y > (HEIGHT >> 2) && inc == 1)
		{
			y -= c;
			ystep[i] = ((HEIGHT >> 1) - (y << 1));
			if (ystep[i] == 0)
				--i;
			else
				y += ystep[i];
			
			inc = -1;
		}
		c += inc;
	}

	ystep[i] = 1;
	
#if PLANET_SPIN_DEBUG
	y = 0;
	for (i = 0; y < (HEIGHT >> 1); i++)
	{
		fprintf (stderr, "ystep[%ld] = %ld\n", i, ystep[i]);
		y += ystep[i];
	}
#endif
}

static void
build_tables (int da)
{
	int y, i;

	build_steps ();
	
	OpenMathFolio (); // calling this multiply is not a problem, right?
	
	for (y = 0, i = 0; y <= (HEIGHT >> 1); i++)
	{
		int dx, dy, a, h;
		
		h = ystep[i];

		if (y + h > (HEIGHT >> 1)
				&& (h = (HEIGHT >> 1) - y + 1) == 0)
			return;

		dy = (HEIGHT >> 1) - y;
		dx = (int)SqrtF16 (((HEIGHT >> 1) * (HEIGHT >> 1) - dy * dy) << 16);
		dy <<= 16;

		a = (int)Atan2F16 (-dx, -dy);
		a += da << 16;
		x0[i] = (int)(HEIGHT * CosF16 (a));
		y0[i] = (int)(HEIGHT * SinF16 (a));
		a = (int)Atan2F16 (dx, -dy);
		a += da << 16;
		x1[i] = (int)(HEIGHT * CosF16 (a));
		y1[i] = (int)(HEIGHT * SinF16 (a));
		
		x0[i] >>= 1;
		y0[i] >>= 1;
		x1[i] >>= 1;
		y1[i] >>= 1;
		
		y += h;
#if PLANET_SPIN_DEBUG
		fprintf (stderr, "%ld: %ld  ", i, y);
		fprintf (stderr, "%ld,%ld -- ", x0[i] >> 16, y0[i] >> 16);
		fprintf (stderr, "%ld,%ld\n", x1[i] >> 16, y1[i] >> 16);
#endif
	}
	
	x0[i] = x0[i - 1];
	y0[i] = y0[i - 1];
	x1[i] = x1[i - 1];
	y1[i] = y1[i - 1];
}

static void
draw_band (int src_x, int src_y, int dst_cx, int dst_cy)
{
	//I_CANNOT_FIGURE_OUT_HOW_TO_FIX_THIS_FILE___STREAM???();
		
		int *sp;
	int i;
	
	sp = (int *)planet_ccb.ccb_SourcePtr + src_y * (LINE_PIXELS >> 1);

	for (i = 0; i < XBANDS; i++)
	{
#define src_w src_ws[i]
		cur_ccb->ccb_SourcePtr = (CelData *)(sp + (src_x >> 1));
		cur_ccb->ccb_XPos = dst_cx + cur_ccb->ccb_Width;
		cur_ccb->ccb_YPos = dst_cy + cur_ccb->ccb_Height;
#ifdef SCALE_ROTATE
		if (scale)
			SCALE_CEL (cur_ccb, dst_cx, dst_cy);
#endif
		cur_ccb++;
	
		src_x += src_w;
		if (src_x >= WIDTH)
			src_x -= WIDTH;
		else if (src_x < 0)
			src_x += WIDTH;
	}
}

static void
set_band_cels (int src_y, int src_h, int band)
{
	int i, dst_x0, dst_y0, dst_x1, dst_y1, dx0, dy0, dx1, dy1;

	if (src_y > (HEIGHT >> 1))
	{
		if (src_y + src_h > HEIGHT
				&& (src_h = HEIGHT - src_y + 1) == 0)
			return;
		dx0 = x1[band + 1];
		dy0 = y1[band + 1];
		dx1 = x1[band];
		dy1 = y1[band];
		dst_x0 = -dx0;
		dst_y0 = -dy0;
		dst_x1 = -dx1;
		dst_y1 = -dy1;
		dx0 -= x0[band + 1];
		dy0 -= y0[band + 1];
		dx1 -= x0[band];
		dy1 -= y0[band];
	}
	else
	{
		if (src_y + src_h > (HEIGHT >> 1)
				&& (src_h = (HEIGHT >> 1) - src_y + 1) == 0)
			return;
		dx0 = x0[band];
		dy0 = y0[band];
		dx1 = x0[band + 1];
		dy1 = y0[band + 1];
		dst_x0 = dx0;
		dst_y0 = dy0;
		dst_x1 = dx1;
		dst_y1 = dy1;
		dx0 = x1[band] - dx0;
		dy0 = y1[band] - dy0;
		dx1 = x1[band + 1] - dx1;
		dy1 = y1[band + 1] - dy1;
	}
	
	for (i = 0; i < XBANDS; i++)
	{
#define dd d[i]
#define src_w src_ws[i]
		POINT quadpts[4];
		int d[XBANDS] = { 17, 47, 64, 64, 47, 17 };

		quadpts[0].x = dst_x0;
		quadpts[0].y = dst_y0;
		quadpts[3].x = dst_x1;
		quadpts[3].y = dst_y1;

		dst_x0 += (dx0 * dd) >> 8;
		dst_y0 += (dy0 * dd) >> 8;
		dst_x1 += (dx1 * dd) >> 8;
		dst_y1 += (dy1 * dd) >> 8;

		quadpts[1].x = dst_x0;
		quadpts[1].y = dst_y0;
		quadpts[2].x = dst_x1;
		quadpts[2].y = dst_y1;

		cur_ccb->ccb_PRE0 &= ~PRE0_VCNT_MASK;
		cur_ccb->ccb_PRE1 &= ~PRE1_TLHPCNT_MASK;
		cur_ccb->ccb_PRE0 |= (src_h - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT;
		cur_ccb->ccb_PRE1 |= (src_w - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT;

		cur_ccb->ccb_Width = src_w;
		cur_ccb->ccb_Height = src_h;

		MAP_CEL (cur_ccb, quadpts);
		
		cur_ccb->ccb_Width = cur_ccb->ccb_XPos;
		cur_ccb->ccb_Height = cur_ccb->ccb_YPos;
		
		cur_ccb++;
	}
}

static void
init_rotate_cels ()
{
	int i, incr, y;
	
	incr = 1;
	cur_ccb = plCCB;
	for (y = 0, i = 0; y < HEIGHT;)
	{
#define src_h ystep[i]
		set_band_cels (y, src_h, i);
		y += src_h;
		i += incr;
		if (src_h == 0)
		{
			--i;
			incr = -1;
			if (HEIGHT & 1)
				--i;
		}
	}
#if PLANET_SPIN_DEBUG
	fprintf (stderr, "using %d cels for rotate\n", cur_ccb - plCCB);
#endif
}
#endif

//SetPlantTilt has moved to plangen.c
#if PLANET_SPIN
void
SetPlanetTilt (int da)
{
	int w, i;
    int j;
	j=MAX_PULSE_RED;
	if(planet_stuff==NULL) {
		planet_stuff=(PLANET_STUFF *)calloc(sizeof(PLANET_STUFF),1);
    }
	shield_pulse = MAX_PULSE_RED;
	for (i = 0, cur_ccb = plCCB; i < NUM_ROTATE_CELS; i++, cur_ccb++)
	{
		*cur_ccb = planet_ccb;
		cur_ccb->ccb_NextPtr = cur_ccb + 1;
	}
	
	if (!src_ws[0])
	{
		int err;
		
		w = (WIDTH / 2) / XBANDS;
		err = XBANDS;
		i = XBANDS - 1;
		do
		{
			src_ws[i] = w;
			if ((err -= ((WIDTH / 2) % XBANDS)) <= 0)
			{
				src_ws[i]++;
				err += XBANDS;
			}
		} while (i--);

		build_tables (da);
	}
	
	init_rotate_cels ();

	{
		CCB *ccb;
		
		ccb = plCCB;
		cur_ccb = plCCB + NUM_ROTATE_CELS;
		for (i = 0; i < NUM_ROTATE_CELS; i++, cur_ccb++, ccb++)
		{
			*cur_ccb = *ccb;
			cur_ccb->ccb_NextPtr = cur_ccb + 1;
		}
	}
}
#endif

extern FRAME stretch_frame (FRAME FramePtr, int neww, int newh,int destroy);
extern void RenderLevelMasks(int,int);
// RotatePlanet
// This will take care of zooming into a planet on orbit, generating the planet frames
// And applying the shield.
//The initial size of the planet when zooming.  MUST BE ODD
#define PLANET_INIT_ZOOM_SIZE 9
// The speed to zoom in.
#define PLANET_ZOOM_SPEED 2
int
RotatePlanet (int x, int da, int dx, int dy, int zoom)
{
#if !PLANET_SPIN
	STAMP s;
	FRAME pFrame[2];
	COUNT i,num_frames;
	static COUNT scale_amt=PLANET_INIT_ZOOM_SIZE;

	// If thiis frame hasn' been generted, generate it
	if(!pSolarSysState->isPFADefined[x]) {
		RenderLevelMasks(x,da);
		pSolarSysState->isPFADefined[x]=1;
	}
	num_frames=(pSolarSysState->ShieldFrame==0) ? 1 : 2;
	pFrame[0]=pSolarSysState->PlanetFrameArray[x];
	if(num_frames==2) {
		pFrame[1]=pSolarSysState->ShieldFrame;
	}
	if (zoom) {
		//  we're zooming in, take care of scalinng the frames
		for(i=0;i<num_frames;i++) {
			COUNT frameh,this_scale;
			if(pSolarSysState->ScaleFrame[i]) {
				DestroyDrawable (ReleaseDrawable (pSolarSysState->ScaleFrame[i]));
				pSolarSysState->ScaleFrame[i]=0;
			}
			frameh=GetFrameHeight(pFrame[i]);
			this_scale=frameh*scale_amt/MAP_HEIGHT;
			if(! (this_scale & 0x01)) {
				this_scale++;
			}
			pSolarSysState->ScaleFrame[i] = stretch_frame(pFrame[i],this_scale,this_scale,0);
			SetFrameHot (pSolarSysState->ScaleFrame[i], MAKE_HOT_SPOT ((this_scale>>1)+1,(this_scale>>1)+1));
			pFrame[i]=pSolarSysState->ScaleFrame[i];
		}
		scale_amt+=PLANET_ZOOM_SPEED;
		//Translate the planet so it comes from the bottom right corner
		if(scale_amt > MAP_HEIGHT) 
		{
			scale_amt=MAP_HEIGHT;
		}
		dx+=dx*(MAP_HEIGHT-scale_amt)/MAP_HEIGHT;
		dy+=dy*(MAP_HEIGHT-scale_amt)/MAP_HEIGHT;
		if(scale_amt==MAP_HEIGHT) {
			scale_amt=PLANET_INIT_ZOOM_SIZE;
			zoom=0;
		}
	}
	s.origin.x = dx;
	s.origin.y = dy;
	for(i=0;i<num_frames;i++) {
		s.frame=pFrame[i];
		DrawStamp (&s);
	}
	return(zoom);
//	}
#else
	int i, y;
	int incr;
	CCB *first;

	dx <<= 16;
	dy <<= 16;

	if (pblack_circ_ccb && !pshield_ccb->ccb_HDY)
	{
		pblack_circ_ccb->ccb_XPos = dx - (pblack_circ_ccb->ccb_Width << 15);
		pblack_circ_ccb->ccb_YPos = dy - (pblack_circ_ccb->ccb_Height << 15);
		pblack_circ_ccb->ccb_HDX = 1 << 20;
		pblack_circ_ccb->ccb_VDY = 1 << 16;
#ifdef SCALE_ROTATE
		if ((scale & SCALE_AMT_MASK) > SCALE_STEP)
		{
			scale -= SCALE_STEP;
			SCALE_CEL (pblack_circ_ccb, dx, dy);
			scale += SCALE_STEP;
		}
		else if (!scale)
#endif
		add_cel (pblack_circ_ccb);
#ifdef KDEBUG
		fprintf (stderr, "CLEAR...\n");
		while (AnyButtonPress (FALSE))
			;
		while (!AnyButtonPress (FALSE))
			;
#endif /* KDEBUG */
   }

	incr = 1;
#ifdef SCALE_ROTATE
	if (scale)
	{
		CCB *ccb;
		
		ccb = plCCB;
		cur_ccb = plCCB + NUM_ROTATE_CELS;
		for (i = 0; i < NUM_ROTATE_CELS; i++, cur_ccb++, ccb++)
		{
			*cur_ccb = *ccb;
			cur_ccb->ccb_NextPtr = cur_ccb + 1;
		}
		first = plCCB + NUM_ROTATE_CELS;
	}
	else
#endif
		first = plCCB;
	cur_ccb = first;
	for (y = 0, i = 0; y < HEIGHT;)
	{
#define src_h ystep[i]
		draw_band (x, y, dx, dy);
		y += src_h;
		i += incr;
		if (src_h == 0)
		{
			--i;
			incr = -1;
			if (HEIGHT & 1)
				--i;
		}
	}
	
	add_cels (first, cur_ccb - 1);

#ifdef KDEBUG
	fprintf (stderr, "PLANET...\n");
	while (AnyButtonPress (FALSE))
		;
	while (!AnyButtonPress (FALSE))
		;
#endif /* KDEBUG */
	if (pcirc_ccb)
	{
		pcirc_ccb->ccb_XPos = dx - (pcirc_ccb->ccb_Width << 15);
		pcirc_ccb->ccb_YPos = dy - (pcirc_ccb->ccb_Height << 15);
		pcirc_ccb->ccb_HDX = 1 << 20;
		pcirc_ccb->ccb_VDY = 1 << 16;
#ifdef SCALE_ROTATE
		if (scale)
			SCALE_CEL (pcirc_ccb, dx, dy);
#endif
			add_cel (pcirc_ccb);
#ifdef KDEBUG
		fprintf (stderr, "CLIP...\n");
		while (AnyButtonPress (FALSE))
			;
		while (!AnyButtonPress (FALSE))
			;
#endif /* KDEBUG */
	}
	
	if (pblur_ccb)
	{
		pblur_ccb->ccb_XPos = dx - (pblur_ccb->ccb_Width << 15);
		pblur_ccb->ccb_YPos = dy - (pblur_ccb->ccb_Height << 15);
		pblur_ccb->ccb_HDX = 1 << 20;
		pblur_ccb->ccb_VDY = 1 << 16;
#ifdef SCALE_ROTATE
		if (scale)
			SCALE_CEL (pblur_ccb, dx, dy);
#endif
				add_cel (pblur_ccb);
#ifdef KDEBUG
		fprintf (stderr, "BLUR...\n");
		while (AnyButtonPress (FALSE))
			;
		while (!AnyButtonPress (FALSE))
			;
#endif /* KDEBUG */
	}
	
	if (pshadow_ccb)
	{
		pshadow_ccb->ccb_XPos = dx - (pshadow_ccb->ccb_Width << 15);
		pshadow_ccb->ccb_YPos = dy - (pshadow_ccb->ccb_Height << 15);
		pshadow_ccb->ccb_HDX = 1 << 20;
		pshadow_ccb->ccb_VDY = 1 << 16;
#ifdef SCALE_ROTATE
		if (scale)
			SCALE_CEL (pshadow_ccb, dx, dy);
#endif
		add_cel (pshadow_ccb);
#ifdef KDEBUG
		fprintf (stderr, "SHADOW...\n");
		while (AnyButtonPress (FALSE))
			;
		while (!AnyButtonPress (FALSE))
			;
#endif /* KDEBUG */
	}

	if (!pshield_ccb->ccb_HDY)
	{
		pshield_ccb->ccb_XPos = dx - (pshield_ccb->ccb_Width << 15);
		pshield_ccb->ccb_YPos = dy - (pshield_ccb->ccb_Height << 15);
		pshield_ccb->ccb_HDX = 1 << 20;
		pshield_ccb->ccb_VDY = 1 << 16;
#ifdef SCALE_ROTATE
		if (scale)
			SCALE_CEL (pshield_ccb, dx, dy);
#endif

		if (shield_pulse & PULSE_NEG_BIT)
		{
			if (((--shield_pulse) & ~PULSE_NEG_BIT) < MIN_PULSE_RED)
				shield_pulse = (MIN_PULSE_RED + 1);
		}
		else if (((++shield_pulse) & ~PULSE_NEG_BIT) > MAX_PULSE_RED)
			shield_pulse = (MAX_PULSE_RED - 1) | PULSE_NEG_BIT;
		pshield_ccb->ccb_PIXC =
				PIXC_DUP (PPMPC_1S_PDC | PPMPC_MS_CCB
				| (((shield_pulse & ~PULSE_NEG_BIT) >> PULSE_SCALE) << PPMPC_MF_SHIFT)
				| PPMPC_SF_8 | PPMPC_2S_CFBD | PPMPC_2D_1);
				
		add_cel (pshield_ccb);
#ifdef KDEBUG
		fprintf (stderr, "SHIELD...\n");
		while (AnyButtonPress (FALSE))
			;
		while (!AnyButtonPress (FALSE))
			;
		while (AnyButtonPress (FALSE))
			;
#endif /* KDEBUG */
	}
	
#ifdef SCALE_ROTATE
	if (scale && ((scale += SCALE_STEP) & SCALE_AMT_MASK) > SCALE_MAX)
		scale = 0;
		
	return (scale != 0);
#else
	return (0);
#endif
#endif
}

void
DrawPlanet (int x, int y, int dy, unsigned int rgb)
{
#if 1
	STAMP s;
	UBYTE a=128;
	s.origin.x = x;
	s.origin.y = y;
	s.frame = pSolarSysState->TopoFrame;
	DrawStamp (&s);
	if (pSolarSysState->ShieldFrame!=0)
	{
		rgb=0x1f<<10;
		dy=GetFrameHeight(s.frame);
		a=200;
	}
	if (rgb)
	{
		UBYTE r,g,b;
		DWORD **rgba,p;
		COUNT i,j,framew;
		FRAME tintFrame=pSolarSysState->TintFrame;
		if(tintFrame !=0)
		{
			DestroyDrawable (ReleaseDrawable (tintFrame));
			tintFrame=0;
		}
		framew=GetFrameWidth(s.frame);
		tintFrame = CaptureDrawable (
			CreateDrawable (WANT_PIXMAP, framew, (UWORD)dy, 1)
				);
		rgba=(DWORD **)HMalloc(sizeof(DWORD *)*(dy));
		r=(rgb&(0x1f<<10))>>8;
		g=(rgb&(0x1f<<5))>>3;
		b=(rgb&0x1f)<<2;
		p=frame_mapRGBA(tintFrame,r,g,b,a);
		for(i=0;i<dy;i++)
		{
			rgba[i]=(DWORD *)HMalloc(sizeof(DWORD)*framew);
			for(j=0;j<framew;j++)
				rgba[i][j]=p;
		}
		process_rgb_bmp(tintFrame,rgba,framew,dy);
		SetFrameHot (tintFrame, MAKE_HOT_SPOT (0, 0));
		for (i=0;i<dy;i++)
			HFree(rgba[i]);
		HFree(rgba);
		s.frame = tintFrame;
		DrawStamp (&s);
	}


				

		
#else
	if (rgb || !pshield_ccb->ccb_HDY)
	{
		int i, nt, my;

		if (!pshield_ccb->ccb_HDY)
		{
			dy = HEIGHT + NUM_TINTS;
			rgb = 0x1f << 10;
		}
		
		my = dy - NUM_TINTS;
		if (my < 0)
		{
			nt = NUM_TINTS + my + 1;
			my = 0;
		}
		else if (my >= HEIGHT)
		{
			nt = NUM_TINTS;
			my = HEIGHT - 1;
		}
		else
			nt = NUM_TINTS;
		
		my += y;
		
		surface_ccb.ccb_XPos = x << 16;
		surface_ccb.ccb_YPos = y << 16;
		surface_ccb.ccb_SourcePtr = planet_ccb.ccb_SourcePtr;
		surface_ccb.ccb_PRE0 =
				PRE0_BGND
				| (((dy + 1) - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT)
				| PRE0_LINEAR
				| PRE0_BPP_16;
		surface_ccb.ccb_PIXC =
				PIXC_DUP (PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_6
				| PPMPC_SF_8 | PPMPC_2S_0);
		add_cel (&surface_ccb);
		
		nt = NUM_TINTS - nt;
		for (i = nt; i < NUM_TINTS; i++, my++)
		{
			if (my >= y && my < y + HEIGHT)
			{
				if (i == 0)
				{
					tint_ccb[i].ccb_YPos = y << 16;
					tint_ccb[i].ccb_VDY = (my - y + 1) << 16;
				}
				else
					tint_ccb[i].ccb_YPos = my << 16;
				tint_ccb[i].ccb_XPos = x << 16;
				*((DWORD *)tint_ccb[i].ccb_SourcePtr) = rgb | (rgb << 16) | 0x80008000;
				add_cel (&tint_ccb[i]);
			}
		}
	}
	else
	{
		surface_ccb.ccb_XPos = x << 16;
		surface_ccb.ccb_YPos = y << 16;
		surface_ccb.ccb_SourcePtr = planet_ccb.ccb_SourcePtr;
		surface_ccb.ccb_PRE0 =
				PRE0_BGND
				| ((HEIGHT - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT)
				| PRE0_LINEAR
				| PRE0_BPP_16;
		surface_ccb.ccb_PIXC = PIXC_UNCODED16;
		add_cel (&surface_ccb);
	}
#endif
}

CCB *
GetCelArray ()
{
	return (plCCB);
}
