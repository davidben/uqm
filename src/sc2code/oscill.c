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

#include "libs/graphics/gfx_common.h"
#include "libs/sound/trackplayer.h"

//Added by Chris
//Modified by Mika (removed use of SDL datatypes)

typedef unsigned char uint8;
typedef unsigned char uchar;
typedef signed char sint8;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned int uint32;
typedef signed int int32;
typedef signed int sint32;

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

//extern uint32 *_varPLUTs;

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

static STAMP sliderStamp;
static STAMP buttonStamp;
int32 sliderSpace;  // slider width - button width

/*
 * Initialise the communication progress bar
 * x - x location of slider
 * y - y location of slider
 * width - width of slider
 * height - height of slider
 * bwidth - width of button indicating current progress
 * bheight - height of button indicating progress
 * f - image for the slider
 */                        
void
InitSlider (int32 x, int32 y, int32 width, int32 height,
		int32 bwidth, int32 bheight, FRAME f)
{
	sliderStamp.origin.x = x;
	sliderStamp.origin.y = y;
	sliderStamp.frame = f;
	buttonStamp.origin.x = x;
	buttonStamp.origin.y = y - ((bheight - height) >> 1);
	sliderSpace = width - bwidth;
}

void
SetSliderImage (FRAME f)
{
	buttonStamp.frame = f;
}

void
Slider (void)
{
	int32 len, offs;
	
	if (GetSoundInfo (&len, &offs))
	{
		if (offs > len)
			offs = len;
		if (len == 0)
			len = 1;
		buttonStamp.origin.x = sliderStamp.origin.x + sliderSpace * offs / len;
		BatchGraphics ();
		DrawStamp (&sliderStamp);
		DrawStamp (&buttonStamp);
		UnbatchGraphics ();
	}
}

