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

//#include "Portfolio.h" //3DO Library we don't get access to
//#include "Init3DO.h"
//#include "Parse3DO.h"
//#include "Utils3DO.h"
//#include "filefunctions.h"
//#include "BlockFile.h"

#include "SDL_wrapper.h"

//Added by Chris

#include "SDL/SDL.h"

typedef Uint8 uint8;
typedef Uint8 uchar;
typedef Sint8 sint8;
typedef Uint16 uint16;
typedef Sint16 sint16;
typedef Uint32 uint32;
typedef Sint32 int32;
typedef Sint32 sint32;

//End Added by Chris

#define CEL_FLAGS \
		(CCB_SPABS | CCB_PPABS | CCB_YOXY | CCB_ACW | CCB_ACCW \
		| CCB_LDSIZE | CCB_CCBPRE | CCB_LDPRS | CCB_LDPPMP | CCB_ACE \
		| CCB_LCE | CCB_PLUTPOS | CCB_BGND | CCB_NPABS)

#define PIXC_DUP(v) (((v) << PPMP_0_SHIFT) | ((v) << PPMP_1_SHIFT))
#define PIXC_UNCODED16 (PIXC_DUP (PPMPC_MF_8 | PPMPC_SF_8))
#define PIXC_CODED8 (PIXC_DUP (PPMPC_MS_PIN | PPMPC_SF_8))

#define NUMBER_OF_PLUTVALS 32
#define NUMBER_OF_PLUT_UINT32s (NUMBER_OF_PLUTVALS >> 1)
#define GET_VAR_PLUT(i) (_varPLUTs + (i) * NUMBER_OF_PLUT_UINT32s)

extern uint32 *_varPLUTs;

#define NUM_THINGS 256

static CCB *plCCB;
static UBYTE flatline;

void
//InitOscilloscope (int32 x, int32 y, int32 width, int32 height, FRAME_DESC *f)
InitOscilloscope (int32 x, int32 y, int32 width, int32 height, void *f)
{
#if 0
	CCB *ccb;
	int32 xpos, xerr;
	uint32 *cel_plut;
	extern CCB *GetCelArray ();

	cel_plut = GET_VAR_PLUT ((f->TypeIndexAndFlags >> 16) & 0xFF);

	plCCB = GetCelArray ();

	memset (plCCB, 0, sizeof (CCB) * (NUM_THINGS + 1));

	ccb = plCCB;
	ccb->ccb_Flags = CEL_FLAGS | CCB_PACKED | CCB_LDPLUT;

	ccb->ccb_HDX = 1 << 20;
	ccb->ccb_HDY = 0;
	ccb->ccb_VDX = 0;
	ccb->ccb_VDY = 1 << 16;

	ccb->ccb_PIXC = PIXC_CODED8;
	ccb->ccb_SourcePtr = (void *)((uchar *)f + f->DataOffs);
	ccb->ccb_PLUTPtr = cel_plut;
	ccb->ccb_Width = f->Bounds & 0xFFFF;
	ccb->ccb_Height = (f->Bounds >> 16);
	ccb->ccb_PRE0 =
			PRE0_LITERAL |
			((ccb->ccb_Height - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT) |
			(PRE0_BPP_8 << PRE0_BPP_SHIFT);
	ccb->ccb_NextPtr = ccb + 1;
	++ccb;

	ccb->ccb_Width = y;
	ccb->ccb_Height = height >> 1;
	xpos = x;
	xerr = NUM_THINGS;
	do
	{
		int32 color;
			
		ccb->ccb_XPos = xpos << 16;
		if ((xerr -= width) <= 0)
		{
			++xpos;
			xerr += NUM_THINGS;
		}
		ccb->ccb_HDX = 1 << 20;
		ccb->ccb_VDY = 1 << 16;
		
		ccb->ccb_Flags = CEL_FLAGS;
		ccb->ccb_PRE0 =
				(PRE0_BPP_16 << PRE0_BPP_SHIFT) |
				PRE0_BGND |
				PRE0_LINEAR;
		ccb->ccb_PRE1 = PRE1_TLLSB_PDC0;
		ccb->ccb_PIXC = PIXC_UNCODED16;
		
		color = ((16 << 10) | (9 << 5) | 31) | 0x8000;
		ccb->ccb_PLUTPtr = (void *)((color << 16) | color);
		ccb->ccb_SourcePtr = (void *)&ccb->ccb_PLUTPtr;
			
		ccb->ccb_NextPtr = ccb + 1;
	} while (++ccb != &plCCB[NUM_THINGS + 1]);
#endif

	flatline = 0;
}

void
Oscilloscope (int32 grab_data)
{
#if 0
	CCB *ccb;
	signed char buf[NUM_THINGS * 2], *data;
	int32 size, ypos, max;

	if (!grab_data
			|| !(size = GetSoundData (buf)))
	{
		if (flatline)
			return;
			
		flatline = 1;
		memset (buf, 0, sizeof (buf));
	}
	else
		flatline = 0;

	ccb = plCCB + 1;
	max = ccb->ccb_Height;
	ypos = ccb->ccb_Width + max;
	data = buf;
	do
	{
		ccb->ccb_YPos = (ypos + ((*data * max) >> 7)) << 16;
#ifndef READ_SAMPLE_DIRECTLY
		data += 2;
#else
		data++;
#endif
	} while (++ccb != &plCCB[NUM_THINGS + 1]);

	add_cels (plCCB, ccb - 1);
//    ClearDrawable ();
	
	FlushGraphics (TRUE);
#endif
}

static CCB slider_cels[2];

void
InitSlider (int32 x, int32 y, int32 width, int32 height,
		int32 bwidth, int32 bheight, void *f)
// int32 bwidth, int32 bheight, FRAME_DESC *f)
{
#if 0
#define SLIDER_X x
#define SLIDER_Y y
#define SLIDER_WIDTH width
#define SLIDER_HEIGHT height
#define BUTTON_WIDTH bwidth
#define BUTTON_HEIGHT bheight
	int32 i;
	CCB *ccb;
	uint32 *cel_plut;

	cel_plut = GET_VAR_PLUT ((f->TypeIndexAndFlags >> 16) & 0xFF);

slider_cels[0].ccb_Width = SLIDER_X;
slider_cels[0].ccb_Height = SLIDER_WIDTH;
slider_cels[0].ccb_SourcePtr = (void *)((uchar *)f + f->DataOffs);
slider_cels[1].ccb_Width = BUTTON_WIDTH;
		
	ccb = slider_cels;
	for (i = 0; i < 2; i++, ccb++)
	{
		ccb->ccb_Flags = CEL_FLAGS | CCB_PACKED | CCB_LDPLUT;

		ccb->ccb_HDX = 1 << 20;
		ccb->ccb_HDY = 0;
		ccb->ccb_VDX = 0;
		ccb->ccb_VDY = 1 << 16;

		ccb->ccb_PIXC = PIXC_CODED8;
		ccb->ccb_PLUTPtr = cel_plut;
		if (i == 0)
		{
			ccb->ccb_XPos = SLIDER_X << 16;
			ccb->ccb_YPos = SLIDER_Y << 16;
			ccb->ccb_PRE0 =
					PRE0_LITERAL |
					((SLIDER_HEIGHT - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT) |
					(PRE0_BPP_8 << PRE0_BPP_SHIFT);
		}
		else
		{
			ccb->ccb_XPos = SLIDER_X << 16;
			ccb->ccb_YPos = (SLIDER_Y - ((BUTTON_HEIGHT - SLIDER_HEIGHT) >> 1)) << 16;
			ccb->ccb_PRE0 =
					PRE0_LITERAL |
					((BUTTON_HEIGHT - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT) |
					(PRE0_BPP_8 << PRE0_BPP_SHIFT);
		}

		ccb->ccb_NextPtr = ccb + 1;
	}
#endif
}

void
SetSliderImage (void *f)
//SetSliderImage (FRAME_DESC *f)
{
#if 0
	slider_cels[1].ccb_SourcePtr = (void *)((uchar *)f + f->DataOffs);
#endif
}

void
Slider (void)
{
#if 0
#undef SLIDER_X
#undef SLIDER_WIDTH
#undef BUTTON_WIDTH
#define SLIDER_X slider_cels[0].ccb_Width
#define SLIDER_WIDTH slider_cels[0].ccb_Height
#define BUTTON_WIDTH slider_cels[1].ccb_Width
	int32 len, offs;
	
	if (GetSoundInfo (&len, &offs))
	{
		if (offs > len) offs = len;

		slider_cels[1].ccb_XPos = (SLIDER_X + ((SLIDER_WIDTH - BUTTON_WIDTH) * offs / len)) << 16;
		add_cels (slider_cels, &slider_cels[1]);
	}
#endif
}

