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
#include <math.h>
#include <time.h>

#define PROFILE  1
#define ROTATION_TIME 12

// define USE_ALPHA_SHIELD to use an aloha overlay instead of
// an additive overlay for the shield effect
#undef USE_ALPHA_SHIELD

extern void DeltaTopography (COUNT num_iterations, PSBYTE DepthArray,
		PRECT pRect, SIZE depth_delta);

void SetPlanetMusic (BYTE planet_type);

void SetPlanetTilt (int da);

void RepairBackRect (PRECT pRect);

int RotatePlanet (int x, int angle, int dx, int dy,int zoom);

DWORD frame_mapRGBA (FRAME FramePtr,UBYTE r, UBYTE g,  UBYTE b, UBYTE a);

void process_rgb_bmp (FRAME FramePtr, DWORD *rgba, int maxx, int maxy);

FRAME stretch_frame (FRAME FramePtr, int neww, int newh,int destroy);

void fill_frame_rgb (FRAME FramePtr, DWORD color, int x0, int y0, int x, int y);

void add_sub_frame (FRAME srcFrame, RECT *rsrc, FRAME dstFrame, RECT *rdst, int add_sub);

DWORD **getpixelarray(FRAME FramePtr,int width, int height);

#define NUM_BATCH_POINTS 64
#define USE_3D_PLANET 1
#define RADIUS 37
//2*RADIUS
#define TWORADIUS (RADIUS << 1)
//RADIUS^2
#define RADIUS_2 (RADIUS * RADIUS)
#define DIAMETER (TWORADIUS + 1)
#define DIFFUSE_BITS 24

//#define GET_LIGHT(val, dif, sp) \
//	( (UBYTE)min ((sp) + \
//		( ( ( (DWORD)(val) << DIFFUSE_BITS ) - (DWORD)(val) * (dif) ) >> DIFFUSE_BITS ) \
//		, 255) )
UBYTE GET_LIGHT (UBYTE val, DWORD dif, UBYTE sp)
{
	DWORD i = (DWORD)val << DIFFUSE_BITS;
	i -= val * dif;
	i >>= DIFFUSE_BITS;
	i += sp;
	if (i > 255)
		i = 255;
	return ((UBYTE)i);
}

#ifndef M_TWOPI
  #ifndef M_PI
     #define M_PI 3.14159265358979323846
  #endif
  #define M_TWOPI (M_PI * 2.0)
#endif
#ifndef M_DEG2RAD
#define M_DEG2RAD (M_TWOPI / 360.0)
#endif

DWORD light_diff[DIAMETER][DIAMETER];
UBYTE light_spec[DIAMETER][DIAMETER];
typedef struct 
{
	POINT p[4];
	DWORD m[4];
} MAP3D_POINT;
MAP3D_POINT map_rotate[DIAMETER][DIAMETER];
//POINT map_rotate[DIAMETER][DIAMETER];
typedef struct {
	double x, y, z;
} POINT3;

void
RenderTopography (BOOLEAN Reconstruct)
{
	CONTEXT OldContext;
	FRAME OldFrame;

	OldContext = SetContext (TaskContext);
	OldFrame = SetContextFGFrame (pSolarSysState->TopoFrame);
	SetContextDrawState (DEST_PIXMAP | DRAW_REPLACE);

	if (pSolarSysState->XlatRef == 0)
	{
		STAMP s;

		s.origin.x = s.origin.y = 0;
#if 0
		s.frame = SetAbsFrameIndex (
				pSolarSysState->PlanetFrameArray[2], 1
				);
#endif
		s.frame = pSolarSysState->TopoFrame;
		DrawStamp (&s);
	}
	else
	{
		COUNT i;
		BYTE AlgoType;
		SIZE base, d;
		POINT pt;
		PLANDATAPTR PlanDataPtr;
		PRIMITIVE BatchArray[NUM_BATCH_POINTS];
		register PPRIMITIVE pBatch;
		PBYTE lpDst, xlat_tab, cbase;
		HOT_SPOT OldHot;
		RECT ClipRect;

		OldHot = SetFrameHot (pSolarSysState->TopoFrame, MAKE_HOT_SPOT (0, 0));
		GetContextClipRect (&ClipRect);
		SetContextClipRect (NULL_PTR);
		SetContextClipping (FALSE);

		pBatch = &BatchArray[0];
		for (i = 0; i < NUM_BATCH_POINTS; ++i, ++pBatch)
		{
			SetPrimNextLink (pBatch, (COUNT)(i + 1));
			SetPrimType (pBatch, POINT_PRIM);
		}
		SetPrimNextLink (&pBatch[-1], END_OF_LIST);

		PlanDataPtr = &PlanData[
				pSolarSysState->pOrbitalDesc->data_index & ~PLANET_SHIELDED
				];
		AlgoType = PLANALGO (PlanDataPtr->Type);
		if (Reconstruct && AlgoType != GAS_GIANT_ALGO)
			base = 0;
		else
			base = PlanDataPtr->base_elevation;
		xlat_tab = (PBYTE)((XLAT_DESCPTR)pSolarSysState->XlatPtr)->xlat_tab;
		cbase = GetColorMapAddress (pSolarSysState->OrbitalCMap);

		pBatch = &BatchArray[i = NUM_BATCH_POINTS];
		lpDst = pSolarSysState->lpTopoData;
		for (pt.y = 0; pt.y < MAP_HEIGHT; ++pt.y)
		{
			for (pt.x = 0; pt.x < MAP_WIDTH; ++pt.x)
			{
				d = (SBYTE)*lpDst;
				if (Reconstruct)
				{
					d = (SIZE)((BYTE)((BYTE)d - (BYTE)base));
					++lpDst;
				}
				else if (AlgoType == GAS_GIANT_ALGO)
				{
					*lpDst++ = (BYTE)((SBYTE)d + base);
					d &= 255;
				}
				else
				{
					d += base;
					if (d < 0)
						d = 0;
					else if (d > 255)
						d = 255;
					*lpDst++ = (BYTE)d;
				}

				--pBatch;
				pBatch->Object.Point.x = pt.x;
				pBatch->Object.Point.y = pt.y;
{
				PBYTE ctab;

				d = xlat_tab[d] - cbase[0];
				ctab = (cbase + 2) + d * 3;

				// fixed planet surfaces being too dark
				// ctab shifts were previously >> 3 .. -Mika
				SetPrimColor (pBatch, BUILD_COLOR (MAKE_RGB15 (ctab[0] >> 1, ctab[1] >> 1, ctab[2] >> 1), d));
				
}
				if (--i)
					continue;

				DrawBatch (BatchArray, 0, 0);
				pBatch = &BatchArray[i = NUM_BATCH_POINTS];
			}
		}

		if (i < NUM_BATCH_POINTS)
		{
			DrawBatch (BatchArray, i, 0);
		}

		SetContextClipping (TRUE);
		SetContextClipRect (&ClipRect);
		SetFrameHot (pSolarSysState->TopoFrame, OldHot);
	}

	SetContextFGFrame (OldFrame);
	SetContext (OldContext);
}

void P3mult (POINT3 *res, POINT3 *vec,  double cnst)
{
	res->x = vec->x * cnst;
	res->y = vec->y * cnst;
	res->z = vec->z * cnst;
}
void P3sub (POINT3 *res, POINT3 *v1,  POINT3 *v2)
{
	res->x = v1->x - v2->x;
	res->y = v1->y - v2->y;
	res->z = v1->z - v2->z;
}
double P3dot (POINT3 *v1, POINT3 *v2) 
{
	return (v1->x * v2->x + v1->y * v2->y + v1->z * v2->z);
}
void P3norm (POINT3 *res, POINT3 *vec) 
{
	double mag = sqrt (P3dot (vec, vec));
	P3mult (res, vec, 1/mag);
}

// RenderPhongMask builds a shadow map for the rotating planet
//  loc indicates the planets position relavtive to the sun
static void
RenderPhongMask (POINT loc)
{
	POINT pt;
	POINT3 light, view;
	double lrad;
	DWORD step;
	int y, x;

#define LIGHT_INTENS 0.4
#define AMBIENT_LIGHT 0.1
#define MSHI 2
#define LIGHT_Z 1.2
	// lrad is the distance from the sun to the planet
	lrad = sqrt (loc.x * loc.x + loc.y * loc.y);
	// light is the sun's position.  the z-coordinate is whatever
	// looks good
	light.x = -((double)loc.x);
	light.y = -((double)loc.y);
	light.z = LIGHT_Z * lrad;
	P3norm (&light, &light);
	// always view along the z-axis
	// ideally use a view point, and have the view change per pixel
	// but that is too much effort for now.
	// the view MUST be normalized!
	view.x = 0;
	view.y = 0;
	view.z = 1.0;
	step = 1 << DIFFUSE_BITS;
	for (pt.y = 0, y = -RADIUS; pt.y <= TWORADIUS; ++pt.y, y++)
	{
		DWORD y_2;
		y_2 = y * y;
		for (pt.x = 0, x = -RADIUS; pt.x <= TWORADIUS; ++pt.x, x++)
		{
			DWORD x_2, rad_2, stepint;
			POINT3 norm, rvec;
			double diff, spec = 0.0, fb;
			x_2 = x * x;
			rad_2 = x_2 + y_2;
			if (rad_2 <= RADIUS_2) 
			{
				// norm is the sphere's surface normal.
				norm.x = (double)x;
				norm.y = (double)y;
				norm.z = (sqrt (RADIUS_2 - x_2) * sqrt (RADIUS_2 - y_2)) / RADIUS;
				P3norm(&norm,&norm);
				// diffuse component is norm dot light
				diff =P3dot (&norm, &light);
				// negative diffuse is bad
				if(diff < 0)
					diff = 0.0;
				// specular highlight is the phong equation: (rvec dot view)^MSHI
				// where rvec = (2*diff)*norm - light (reflection of light around norm)
				P3mult (&rvec,&norm,2 * diff);
				P3sub (&rvec, &rvec, &light);
				fb = P3dot (&rvec, &view);
				if (fb > 0.0)
					spec = LIGHT_INTENS * pow (fb, MSHI);
				else
					spec = 0;
				// adjust for the ambient light
				if (diff < AMBIENT_LIGHT)
					diff = AMBIENT_LIGHT;
				// stepint allows us multiply by a ratio without usig  floating-point
				// instead of color*diff, we use ((color << 24) - stepint*color) >> 24
				stepint = step - (DWORD)(diff * step + 0.5);
				// Now we antialias the edge of the spere to look nice
				if(rad_2 > (RADIUS - 1) * (RADIUS - 1)) 
				{
					DWORD r;
					r = rad_2 - (RADIUS - 1) * (RADIUS - 1);
					stepint += (step >> 7) * (r + 1);
					if (stepint > step) 
						stepint = step;
				}
			} else
				stepint = 1 << 31;
			light_diff[pt.y][pt.x] = (DWORD)stepint;
			light_spec[pt.y][pt.x] = (UBYTE)(spec*255);
		}
	}
}

//create_aa_points creates weighted averages for
//  4 points around the 'ideal' point at x,y
//  the concept is to compute the weight based on the
//  distance from the integer location poinnts to the ideal point
static void
create_aa_points (MAP3D_POINT *ppt, double x, double y)
{
	double deltax = 0, deltay = 0, inv_deltax, inv_deltay;
	COORD nextx, nexty;
	COUNT i;
	double d1, d2, d3, d4, m[4];
	// get  the integer value of this point
	ppt->p[0].x = (COORD)(0.5 + x);
	ppt->p[0].y = (COORD)(0.5 + y);
	if (ppt->p[0].x >= TWORADIUS)
		ppt->p[0].x = TWORADIUS;
	else if (ppt->p[0].x != 0)
		deltax = x - ppt->p[0].x;
	if (ppt->p[0].y >= TWORADIUS)
		ppt->p[0].y = TWORADIUS;
	else if (ppt->p[0].y != 0)
		deltay = y - ppt->p[0].y;
	//if this point doesn't need modificaton, set m[0]=0
	if (deltax == 0 && deltay == 0)
		ppt->m[0] = 0;
	else
	{
		//get the neighbboring points surrounding the 'ideal' poinnt
		if (deltax != 0)
			nextx = ppt->p[0].x + ((deltax > 0) ? 1 : -1);
		else
			nextx = ppt->p[0].x;
		if (deltay != 0)
			nexty = ppt->p[0].y + ((deltay > 0) ? 1 : -1);
		else 
			nexty = ppt->p[0].y;
		//(x1,y)
		ppt->p[1].x = nextx;
		ppt->p[1].y = ppt->p[0].y;
		//(x,y1)
		ppt->p[2].x = ppt->p[0].x;
		ppt->p[2].y = nexty;
		//(x1y1)
		ppt->p[3].x = nextx;
		ppt->p[3].y = nexty;
		//the square  1x1, so opposite poinnts are at 1-delta
		inv_deltax = 1.0 - fabs (deltax);
		inv_deltax *= inv_deltax;
		inv_deltay = 1.0 - fabs (deltay);
		inv_deltay *= inv_deltay;
		deltax *= deltax;
		deltay *= deltay;
		//d1-d4 contain the distances from the poinnts to the ideal point
		d1 = sqrt (deltax + deltay);
		d2 = sqrt (inv_deltax + deltay);
		d3 = sqrt (deltax + inv_deltay);
		d4 = sqrt (inv_deltax + inv_deltay);
		//compute the weights.  the sum(ppt->m[])=65536
		m[0] = 1 / (1 + d1 * (1 / d2 + 1 / d3 + 1 / d4));
		m[1] = m[0] * d1 / d2;
		m[2] = m[0] * d1 / d3;
		m[3] = m[0] * d1 / d4;
		for (i=0; i<4; i++)
			ppt->m[i] = (DWORD)((1 << 16) * m[i] + 0.5);
	}
}

//get_avg_rgb creates either a red, green, or blue value by
//computing the  weightd averages of the 4 points in p1
static UBYTE 
get_avg_rgb (DWORD p1[4], DWORD mult[4], COUNT offset) {
	COUNT i, j;
	UBYTE c;
	DWORD ci = 0;
	
	i = offset << 3;
	//sum(mult[])==65536
	//c is the red/green/blue value of this pixel
	for (j = 0; j < 4; j++)
	{
		c = (UBYTE)(p1[j] >> i);
		ci += c * mult[j];
	}
	ci >>= 16;
	//check for overflow
	if ( ci > 255)
		ci = 255;
	return ((UBYTE)ci);
}

// SetPlanetTilt creates 'map_rotate' to map the topo data
//  for a tilted planet.  It also does the sphere->plane mapping
void
SetPlanetTilt (int angle)
{
	int x, y, y_2;
	double multx = (MAP_HEIGHT / M_PI) / RADIUS;
	double multy = (MAP_HEIGHT / M_PI) / RADIUS;
	for (y = -RADIUS; y <= RADIUS; y++)
	{
		y_2 = y * y;
		for (x = -RADIUS; x <= RADIUS; x++)
		{
			double dx, dy, newx, newy;
			double da, rad, rad2;
			MAP3D_POINT *ppt = &map_rotate[y + RADIUS][x + RADIUS];
			rad2 = x * x + y_2;
			if (rad2 <= RADIUS_2) {
				rad = sqrt (rad2);
				da = atan2 ((double)y, (double)x);
				// compute the planet-tilt
				if (angle != 0) {
					dx = rad * cos (da + M_DEG2RAD * angle);
					dy = rad * sin (da + M_DEG2RAD * angle);
				} else {
					dx = x;
					dy = y;
				}
				//Map the sphere onto a plane
				newx = RADIUS * (multx * acos (-dx / RADIUS));
				newy = RADIUS * (multy * acos (-dy / RADIUS));
				create_aa_points (ppt, newx, newy);
			} else {
				ppt->p[0].x = x + RADIUS;
				ppt->p[0].y = y + RADIUS;
				ppt->m[0] = 0;
			}
		}
	}
}

//CreateShieldMask
// The shield is created in two parts.  This routine creates the Halo.
// The red tint of the planet is currently applied in RenderLevelMasks
// This was done because the shield lows, and needs to modfy how the planet
// gets lit.  urrently, the planet area is transparent in the mask made by
// this routine, but a filter can be applied if desired too.

//Outer diameter of HALO
#define SHIELD_RADIUS (RADIUS + 5) 
#define SHIELD_DIAM ((SHIELD_RADIUS << 1) + 1)
#define SHIELD_RADIUS_2 (SHIELD_RADIUS * SHIELD_RADIUS)
static void CreateShieldMask ()
{

	DWORD rad2, clear, *rgba, *p_rgba, p;
	UBYTE red_nt;
	int x, y;
	FRAME ShieldFrame;
	DWORD aa_delta, aa_delta2;

	ShieldFrame = CaptureDrawable (
			CreateDrawable (WANT_PIXMAP, SHIELD_DIAM, SHIELD_DIAM, 1)
				);
	rgba = (DWORD *)HMalloc (sizeof (DWORD *) * (SHIELD_DIAM) * (SHIELD_DIAM));
	p_rgba = rgba;
	// This is a non-transparent red for the halo
	red_nt = 180;
	//  This is 100% transparent.
	clear = frame_mapRGBA (ShieldFrame, 0, 0, 0, 0);
	aa_delta = SHIELD_RADIUS_2 - (SHIELD_RADIUS - 1) * (SHIELD_RADIUS - 1);
	aa_delta2 = (RADIUS + 3) * (RADIUS + 3) - (RADIUS + 2)*(RADIUS + 2);
	for (y = -SHIELD_RADIUS; y <= SHIELD_RADIUS; y++)
	{
		for (x = -SHIELD_RADIUS; x <= SHIELD_RADIUS; x++)
		{
			rad2 = x * x + y * y;
			if (rad2 <= SHIELD_RADIUS_2)
			{
				//Inside the halo
				if (rad2 <= RADIUS_2)
					// The mask for the planet
					p=clear;
				else if (rad2 <= (RADIUS + 2) * (RADIUS + 2))
					// The space between the halo and the planet
					p = frame_mapRGBA (ShieldFrame, 0, 0, 0, 255);
				else
				{
					// The halo itself
					UBYTE red = red_nt;
					if (rad2 > (SHIELD_RADIUS - 1) * (SHIELD_RADIUS - 1))
					{
						DWORD r;
						r = rad2 - (SHIELD_RADIUS - 1) * (SHIELD_RADIUS - 1);
						red = red_nt - (UBYTE)(red_nt * r / aa_delta);
					} 
					else if (rad2 < (RADIUS + 3) * (RADIUS + 3))
					{
						DWORD r;
						r = rad2 - (RADIUS + 2) * (RADIUS + 2);
						red = (UBYTE)(red_nt * r / aa_delta2);
					}
					p = frame_mapRGBA (ShieldFrame, red, 0, 0, 255);

				}
			} 
			else
				p = clear;

			*p_rgba++ = p;
		}
	}
	process_rgb_bmp (ShieldFrame, rgba, SHIELD_DIAM, SHIELD_DIAM);
	SetFrameHot (ShieldFrame, MAKE_HOT_SPOT (SHIELD_RADIUS + 1,SHIELD_RADIUS + 1));
	HFree(rgba);
	pSolarSysState->ShieldFrame = ShieldFrame;
	{
		// Applythe shield to the topo data
		UBYTE a, blit_type;
		FRAME tintFrame = pSolarSysState->TintFrame;
#ifdef USE_ALPHA_SHIELD
		a = 200;
		blit_type = 0;
#else
		a = 255;
		blit_type = 1;
#endif
		p = frame_mapRGBA (tintFrame, 255, 0, 0, a);
		fill_frame_rgb (tintFrame, p, 0, 0, 0, 0);
		add_sub_frame (tintFrame, NULL, pSolarSysState->TopoFrame, NULL, blit_type);
	}

}

// RenderLevelMasks builds a frame for the rotating planet view
// offset is effectively the angle of rotation around the planet's axis
// We use the SDL routines to directly write to the SDL_Surface to improve performance
void
RenderLevelMasks (int offset)
{
	POINT pt;
	DWORD *rgba, *p_rgba;
	int x, y;
	DWORD p, **pixels;
	FRAME MaskFrame;

#if PROFILE
	static clock_t t = 0;
	static int frames_done = 1;
	clock_t t1;
	t1 = clock ();
#endif
	rgba = (DWORD *)HMalloc (sizeof (DWORD *) * (DIAMETER) * (DIAMETER));
	p_rgba = rgba;
	// Choose the correct Frame to wrte to
	MaskFrame = SetAbsFrameIndex (pSolarSysState->PlanetFrameArray, (COUNT)(offset + 1));
	pixels = pSolarSysState->lpTopoMap;
	for (pt.y = 0, y = -RADIUS; pt.y <= TWORADIUS; ++pt.y, ++y)
	{
		for (pt.x = 0,  x = -RADIUS; pt.x <= TWORADIUS; ++pt.x, ++x)
		{
			UBYTE c[3];
			DWORD diffus;
			UBYTE spec;
			COUNT i;
			DWORD p1[4];
			MAP3D_POINT *ppt = &map_rotate[pt.y][pt.x];
			diffus = light_diff[pt.y][pt.x];
			spec = light_spec[pt.y][pt.x];
			if (diffus < 1 << DIFFUSE_BITS)
			{
				if (ppt->m[0] == 0) 
				{
					p = pixels[ppt->p[0].y][ppt->p[0].x + offset];
					c[0] = (UBYTE)(p >> 8);
					c[1] = (UBYTE)(p >> 16);
					c[2] = (UBYTE)(p >> 24);
				} 
				else
				{
					for (i = 0; i < 4; i++)
						p1[i]=pixels[ppt->p[i].y][ppt->p[i].x + offset];
					for (i = 1; i < 4; i++)
						c[i-1] = get_avg_rgb (p1, ppt->m, i);
				}
			// Apply the lighting model.  This also bounds the sphere to make it circular
				if (pSolarSysState->ShieldFrame)
				{
					c[2] = GET_LIGHT (255, diffus, spec);
					c[1] = GET_LIGHT ((UBYTE)(c[1] >> 1), diffus, spec);
					c[0] = GET_LIGHT ((UBYTE)(c[0] >> 1), diffus, spec);
				} 
				else
				{
					c[2] = GET_LIGHT (c[2], diffus, spec);
					c[1] = GET_LIGHT (c[1], diffus, spec);
					c[0] = GET_LIGHT (c[0], diffus, spec);
				}
				*p_rgba++ = frame_mapRGBA (
					MaskFrame, c[2], c[1], c[0], (UBYTE)255);
			} 
			else
				*p_rgba++ = frame_mapRGBA (MaskFrame, 0, 0, 0, 0);
		}
	}
	// Map the rgb bitmap onto the SDL_Surface
	process_rgb_bmp (MaskFrame, rgba, DIAMETER, DIAMETER);
	SetFrameHot (MaskFrame, MAKE_HOT_SPOT (RADIUS + 1, RADIUS + 1));
	HFree(rgba);
#if PROFILE
	t += clock() - t1;
	if (frames_done == MAP_WIDTH)
	{
		fprintf (stderr, "frames/sec: %d/%d(msec)=%f\n", frames_done, t,
			frames_done / ((double)t / CLOCKS_PER_SEC));
		frames_done = 1;
		t = clock () - t1;
	}
	else
		frames_done++;
#endif
}

#ifdef NOTYET
static void
ExpandMap (PSBYTE DepthArray)
{
	BYTE i, j;
	register PSBYTE lpSrc, lpDst;

	lpSrc = &DepthArray[(MAP_WIDTH >> 1) * ((MAP_HEIGHT >> 1) + 1) - 1];
	lpDst = &DepthArray[MAP_WIDTH * MAP_HEIGHT - 1];
	i = (MAP_HEIGHT >> 1) + 1;
	do
	{
		register SBYTE lf, rt;

		rt = lpSrc[-(MAP_WIDTH >> 1) + 1];
		lf = *lpSrc--;
		*lpDst-- = (SBYTE)((rt + lf) >> 1);
		*lpDst-- = lf;

		j = (MAP_WIDTH >> 1) - 1;
		do
		{
			rt = lf;
			lf = *lpSrc--;
			*lpDst-- = (SBYTE)((rt + lf) >> 1);
			*lpDst-- = lf;
		} while (--j);

		lpDst -= MAP_WIDTH;
	} while (--i);

	lpDst = &DepthArray[MAP_WIDTH];
	i = MAP_HEIGHT >> 1;
	do
	{
		register SBYTE top, bot;

		j = MAP_WIDTH;
		do
		{
			top = lpDst[-MAP_WIDTH];
			bot = lpDst[MAP_WIDTH];
			*lpDst++ = (SBYTE)((top + bot) >> 1);
		} while (--j);

		lpDst += MAP_WIDTH;
	} while (--i);
}
#endif /* NOTYET */

#define RANGE_SHIFT 6

static void
DitherMap (PSBYTE DepthArray)
{
	COUNT i;
	register PSBYTE lpDst;

	i = (MAP_WIDTH * MAP_HEIGHT) >> 2;
	lpDst = DepthArray;
	do
	{
		DWORD rand_val;

		rand_val = Random ();
		*lpDst++ += (SBYTE) ((1 << (RANGE_SHIFT - 4))
				- (LOBYTE (LOWORD (rand_val)) & ((1 << (RANGE_SHIFT - 3)) - 1)));
		*lpDst++ += (SBYTE) ((1 << (RANGE_SHIFT - 4))
				- (HIBYTE (LOWORD (rand_val)) & ((1 << (RANGE_SHIFT - 3)) - 1)));
		*lpDst++ += (SBYTE) ((1 << (RANGE_SHIFT - 4))
				- (LOBYTE (HIWORD (rand_val)) & ((1 << (RANGE_SHIFT - 3)) - 1)));
		*lpDst++ += (SBYTE) ((1 << (RANGE_SHIFT - 4))
				- (HIBYTE (HIWORD (rand_val)) & ((1 << (RANGE_SHIFT - 3)) - 1)));
	} while (--i);
}

static void
MakeCrater (PRECT pRect, PSBYTE DepthArray, SIZE rim_delta, SIZE
		crater_delta, BOOLEAN SetDepth)
{
	COORD x, y, lf_x, rt_x;
	SIZE A, B;
	long Asquared, TwoAsquared,
				Bsquared, TwoBsquared;
	long d, dx, dy;
	COUNT TopIndex, BotIndex, rim_pixels;

	A = pRect->extent.width >> 1;
	B = pRect->extent.height >> 1;

	x = 0;
	y = B;

	Asquared = (DWORD)A * A;
	TwoAsquared = Asquared << 1;
	Bsquared = (DWORD)B * B;
	TwoBsquared = Bsquared << 1;

	dx = 0;
	dy = TwoAsquared * B;
	d = Bsquared - (dy >> 1) + (Asquared >> 2);

	A += pRect->corner.x;
	B += pRect->corner.y;
	TopIndex = (B - y) * MAP_WIDTH;
	BotIndex = (B + y) * MAP_WIDTH;
	rim_pixels = 1;
	while (dx < dy)
	{
		if (d > 0)
		{
			lf_x = A - x;
			rt_x = A + x;
			if (SetDepth)
			{
				memset ((PSBYTE)&DepthArray[TopIndex + lf_x], 0, rt_x - lf_x + 1);
				memset ((PSBYTE)&DepthArray[BotIndex + lf_x], 0, rt_x - lf_x + 1);
			}
			if (lf_x == rt_x)
			{
				DepthArray[TopIndex + lf_x] += rim_delta;
				DepthArray[BotIndex + lf_x] += rim_delta;
				rim_pixels = 0;
			}
			else
			{
				do
				{
					DepthArray[TopIndex + lf_x] += rim_delta;
					DepthArray[BotIndex + lf_x] += rim_delta;
					if (lf_x != rt_x)
					{
						DepthArray[TopIndex + rt_x] += rim_delta;
						DepthArray[BotIndex + rt_x] += rim_delta;
					}
					++lf_x;
					--rt_x;
				} while (--rim_pixels);

				while (lf_x < rt_x)
				{
					DepthArray[TopIndex + lf_x] += crater_delta;
					DepthArray[BotIndex + lf_x] += crater_delta;
					DepthArray[TopIndex + rt_x] += crater_delta;
					DepthArray[BotIndex + rt_x] += crater_delta;
					++lf_x;
					--rt_x;
				}

				if (lf_x == rt_x)
				{
					DepthArray[TopIndex + lf_x] += crater_delta;
					DepthArray[BotIndex + lf_x] += crater_delta;
				}
			}
		
			--y;
			TopIndex += MAP_WIDTH;
			BotIndex -= MAP_WIDTH;
			dy -= TwoAsquared;
			d -= dy;
		}

		++rim_pixels;
		++x;
		dx += TwoBsquared;
		d += Bsquared + dx;
	}

	d += ((((Asquared - Bsquared) * 3) >> 1) - (dx + dy)) >> 1;

	while (y > 0)
	{
		lf_x = A - x;
		rt_x = A + x;
		if (SetDepth)
		{
			memset ((PSBYTE)&DepthArray[TopIndex + lf_x], 0, rt_x - lf_x + 1);
			memset ((PSBYTE)&DepthArray[BotIndex + lf_x], 0, rt_x - lf_x + 1);
		}
		if (lf_x == rt_x)
		{
			DepthArray[TopIndex + lf_x] += rim_delta;
			DepthArray[BotIndex + lf_x] += rim_delta;
		}
		else
		{
			do
			{
				DepthArray[TopIndex + lf_x] += rim_delta;
				DepthArray[BotIndex + lf_x] += rim_delta;
				if (lf_x != rt_x)
				{
					DepthArray[TopIndex + rt_x] += rim_delta;
					DepthArray[BotIndex + rt_x] += rim_delta;
				}
				++lf_x;
				--rt_x;
			} while (--rim_pixels);

			while (lf_x < rt_x)
			{
				DepthArray[TopIndex + lf_x] += crater_delta;
				DepthArray[BotIndex + lf_x] += crater_delta;
				DepthArray[TopIndex + rt_x] += crater_delta;
				DepthArray[BotIndex + rt_x] += crater_delta;
				++lf_x;
				--rt_x;
			}

			if (lf_x == rt_x)
			{
				DepthArray[TopIndex + lf_x] += crater_delta;
				DepthArray[BotIndex + lf_x] += crater_delta;
			}
		}
		
		if (d < 0)
		{
			++x;
			dx += TwoBsquared;
			d += dx;
		}

		rim_pixels = 1;
		--y;
		TopIndex += MAP_WIDTH;
		BotIndex -= MAP_WIDTH;
		dy -= TwoAsquared;
		d += Asquared - dy;
	}

	lf_x = A - x;
	rt_x = A + x;
	if (SetDepth)
		memset ((PSBYTE)&DepthArray[TopIndex + lf_x], 0, rt_x - lf_x + 1);
	if (lf_x == rt_x)
	{
		DepthArray[TopIndex + lf_x] += rim_delta;
	}
	else
	{
		do
		{
			DepthArray[TopIndex + lf_x] += rim_delta;
			if (lf_x != rt_x)
				DepthArray[TopIndex + rt_x] += rim_delta;
			++lf_x;
			--rt_x;
		} while (--rim_pixels);

		while (lf_x < rt_x)
		{
			DepthArray[TopIndex + lf_x] += crater_delta;
			DepthArray[TopIndex + rt_x] += crater_delta;
			++lf_x;
			--rt_x;
		}

		if (lf_x == rt_x)
		{
			DepthArray[TopIndex + lf_x] += crater_delta;
		}
	}
}

#define NUM_BAND_COLORS 4

static void
MakeStorms (COUNT storm_count, PSBYTE DepthArray)
{
#define MAX_STORMS 8
	COUNT i;
	RECT storm_r[MAX_STORMS];
	register PRECT pstorm_r;

	pstorm_r = &storm_r[i = storm_count];
	while (i--)
	{
		BOOLEAN intersect;
		DWORD rand_val;
		UWORD loword, hiword;
		SIZE band_delta;

		--pstorm_r;
		do
		{
			COUNT j;

			intersect = FALSE;

			rand_val = Random ();
			loword = LOWORD (rand_val);
			hiword = HIWORD (rand_val);
			switch (HIBYTE (hiword) & 31)
			{
				case 0:
					pstorm_r->extent.height =
							(LOBYTE (hiword) % (MAP_HEIGHT >> 2))
							+ (MAP_HEIGHT >> 2);
					break;
				case 1:
				case 2:
				case 3:
				case 4:
					pstorm_r->extent.height =
							(LOBYTE (hiword) % (MAP_HEIGHT >> 3))
							+ (MAP_HEIGHT >> 3);
					break;
				default:
					pstorm_r->extent.height =
							(LOBYTE (hiword) % (MAP_HEIGHT >> 4))
							+ 4;
					break;
			}

			if (pstorm_r->extent.height <= 4)
				pstorm_r->extent.height += 4;

			rand_val = Random ();
			loword = LOWORD (rand_val);
			hiword = HIWORD (rand_val);

			pstorm_r->extent.width = pstorm_r->extent.height
					+ (LOBYTE (loword) % pstorm_r->extent.height);

			pstorm_r->corner.x = HIBYTE (loword)
					% (MAP_WIDTH - pstorm_r->extent.width);
			pstorm_r->corner.y = LOBYTE (loword)
					% (MAP_HEIGHT - pstorm_r->extent.height);

			for (j = i + 1; j < storm_count; ++j)
			{
				COORD x, y;
				SIZE w, h;

				x = storm_r[j].corner.x - pstorm_r->corner.x;
				y = storm_r[j].corner.y - pstorm_r->corner.y;
				w = x + storm_r[j].extent.width + 4;
				h = y + storm_r[j].extent.height + 4;
				intersect = (BOOLEAN) (w > 0 && h > 0
						&& x < pstorm_r->extent.width + 4
						&& y < pstorm_r->extent.height + 4);
				if (intersect)
					break;
			}

		} while (intersect);

		MakeCrater (pstorm_r, DepthArray, 6, 6, FALSE);
		++pstorm_r->corner.x;
		++pstorm_r->corner.y;
		pstorm_r->extent.width -= 2;
		pstorm_r->extent.height -= 2;

		band_delta = HIBYTE (loword) & ((3 << RANGE_SHIFT) + 20);

		MakeCrater (pstorm_r, DepthArray,
				band_delta, band_delta, TRUE);
		++pstorm_r->corner.x;
		++pstorm_r->corner.y;
		pstorm_r->extent.width -= 2;
		pstorm_r->extent.height -= 2;

		band_delta += 2;
		if (pstorm_r->extent.width > 2 && pstorm_r->extent.height > 2)
		{
			MakeCrater (pstorm_r, DepthArray,
					band_delta, band_delta, TRUE);
			++pstorm_r->corner.x;
			++pstorm_r->corner.y;
			pstorm_r->extent.width -= 2;
			pstorm_r->extent.height -= 2;
		}

		band_delta += 2;
		if (pstorm_r->extent.width > 2 && pstorm_r->extent.height > 2)
		{
			MakeCrater (pstorm_r, DepthArray,
					band_delta, band_delta, TRUE);
			++pstorm_r->corner.x;
			++pstorm_r->corner.y;
			pstorm_r->extent.width -= 2;
			pstorm_r->extent.height -= 2;
		}

		band_delta += 4;
		MakeCrater (pstorm_r, DepthArray,
				band_delta, band_delta, TRUE);
	}
}

static void
MakeGasGiant (COUNT num_bands, PSBYTE DepthArray, PRECT pRect, SIZE
		depth_delta)
{
	COORD last_y, next_y;
	SIZE band_error, band_bump, band_delta;
	COUNT i, j, band_height;
	PSBYTE lpDst;
	UWORD loword, hiword;
	DWORD rand_val;

	band_height = pRect->extent.height / num_bands;
	band_bump = pRect->extent.height % num_bands;
	band_error = num_bands >> 1;
	lpDst = DepthArray;

	band_delta = ((LOWORD (Random ())
			& (NUM_BAND_COLORS - 1)) << RANGE_SHIFT)
			+ (1 << (RANGE_SHIFT - 1));
	last_y = next_y = 0;
	for (i = num_bands; i > 0; --i)
	{
		COORD cur_y;

		rand_val = Random ();
		loword = LOWORD (rand_val);
		hiword = HIWORD (rand_val);

		next_y += band_height;
		if ((band_error -= band_bump) < 0)
		{
			++next_y;
			band_error += num_bands;
		}
		if (i == 1)
			cur_y = pRect->extent.height;
		else
		{
			RECT r;

			cur_y = next_y
					+ ((band_height - 2) >> 1)
					- ((LOBYTE (hiword) % (band_height - 2)) + 1);
			r.corner.x = r.corner.y = 0;
			r.extent.width = pRect->extent.width;
			r.extent.height = 5;
			DeltaTopography (50,
					&DepthArray[(cur_y - 2) * r.extent.width],
					&r, depth_delta);
		}

		for (j = cur_y - last_y; j > 0; --j)
		{
			COUNT k;

			for (k = pRect->extent.width; k > 0; --k)
				*lpDst++ += band_delta;
		}

		last_y = cur_y;
		band_delta = (band_delta
				+ ((((LOBYTE (loword) & 1) << 1) - 1) << RANGE_SHIFT))
				& (((1 << RANGE_SHIFT) * NUM_BAND_COLORS) - 1);
	}

	MakeStorms ((COUNT)(4 + ((COUNT)Random () & 3) + 1), DepthArray);

	DitherMap (DepthArray);
}

static void
ValidateMap (PSBYTE DepthArray)
{
	BYTE state;
	BYTE pixel_count[2], lb[2];
	SBYTE last_byte;
	COUNT i;
	register PSBYTE lpDst;

	i = MAP_WIDTH - 1;
	lpDst = DepthArray;
	last_byte = *lpDst++;
	state = pixel_count[0] = pixel_count[1] = 0;
	do
	{
		if (pixel_count[state]++ == 0)
			lb[state] = last_byte;

		if (last_byte > *lpDst)
		{
			if (last_byte - *lpDst > 128)
				state ^= 1;
		}
		else
		{
			if (*lpDst - last_byte > 128)
				state ^= 1;
		}
		last_byte = *lpDst++;
	} while (--i);

	i = MAP_WIDTH * MAP_HEIGHT;
	lpDst = DepthArray;
	if (pixel_count[0] > pixel_count[1])
		last_byte = lb[0];
	else
		last_byte = lb[1];
	do
	{
		if (last_byte > *lpDst)
		{
			if (last_byte - *lpDst > 128)
				*lpDst = last_byte;
		}
		else
		{
			if (*lpDst - last_byte > 128)
				*lpDst = last_byte;
		}
		last_byte = *lpDst++;
	} while (--i);
}

void
GeneratePlanetMask (PPLANET_DESC pPlanetDesc, BOOLEAN IsEarth)
{
	RECT r;
	DWORD old_seed;
	PLANDATAPTR PlanDataPtr;
	COUNT i, x, y;
	POINT loc;
	CONTEXT OldContext;

	old_seed = SeedRandom (pPlanetDesc->rand_seed);

	OldContext = SetContext (TaskContext);
	pSolarSysState->isPFADefined = (BYTE *)HCalloc (sizeof(BYTE) * 255);
	pSolarSysState->TintFrame = CaptureDrawable (
			CreateDrawable (WANT_PIXMAP, (SWORD)MAP_WIDTH, (SWORD)MAP_HEIGHT, 2)
			);

	pSolarSysState->PlanetFrameArray = CaptureDrawable (
			CreateDrawable (WANT_PIXMAP, DIAMETER, DIAMETER, (COUNT)MAP_WIDTH)
				);
	if (IsEarth)
	{
		//Earth uses a pixmap for the topography
		pSolarSysState->TopoFrame = CaptureDrawable (
				LoadGraphic (EARTH_MASK_ANIM)
				);
		pSolarSysState->TopoFrame = stretch_frame (
				pSolarSysState->TopoFrame, MAP_WIDTH, MAP_HEIGHT, 1
				);

	} else {

		PlanDataPtr = &PlanData[
				pPlanetDesc->data_index & ~PLANET_SHIELDED
				];
		r.corner.x = r.corner.y = 0;
		r.extent.width = MAP_WIDTH;
		r.extent.height = MAP_HEIGHT;
		if (pSolarSysState->hTopoData == 0)
		{
			pSolarSysState->hTopoData = AllocResourceData (MAP_WIDTH * MAP_HEIGHT, 0);
			LockResourceData (pSolarSysState->hTopoData, &pSolarSysState->lpTopoData);
		}
		if (pSolarSysState->lpTopoData)
		{
			memset (pSolarSysState->lpTopoData, 0, MAP_WIDTH * MAP_HEIGHT);
			switch (PLANALGO (PlanDataPtr->Type))
			{
				case GAS_GIANT_ALGO:
					MakeGasGiant (PlanDataPtr->num_faults,
							pSolarSysState->lpTopoData, &r, PlanDataPtr->fault_depth);
					break;
				case TOPO_ALGO:
				case CRATERED_ALGO:
					if (PlanDataPtr->num_faults)
						DeltaTopography (PlanDataPtr->num_faults,
								pSolarSysState->lpTopoData, &r, PlanDataPtr->fault_depth);

					for (i = 0; i < PlanDataPtr->num_blemishes; ++i)
					{
						RECT crater_r;
						UWORD loword;
				
						loword = LOWORD (Random ());
						switch (HIBYTE (loword) & 31)
						{
							case 0:
								crater_r.extent.width =
										(LOBYTE (loword) % (MAP_HEIGHT >> 2))
										+ (MAP_HEIGHT >> 2);
								break;
							case 1:
							case 2:
							case 3:
							case 4:
								crater_r.extent.width =
										(LOBYTE (loword) % (MAP_HEIGHT >> 3))
										+ (MAP_HEIGHT >> 3);
								break;
							default:
								crater_r.extent.width =
										(LOBYTE (loword) % (MAP_HEIGHT >> 4))
										+ 4;
								break;
						}
					
						loword = LOWORD (Random ());
						crater_r.extent.height = crater_r.extent.width;
						crater_r.corner.x = HIBYTE (loword)
								% (MAP_WIDTH - crater_r.extent.width);
						crater_r.corner.y = LOBYTE (loword)
								% (MAP_HEIGHT - crater_r.extent.height);
						MakeCrater (&crater_r, pSolarSysState->lpTopoData,
								PlanDataPtr->fault_depth << 2,
								-(PlanDataPtr->fault_depth << 2),
								FALSE);
					}

					if (PLANALGO (PlanDataPtr->Type) == CRATERED_ALGO)
						DitherMap (pSolarSysState->lpTopoData);
					ValidateMap (pSolarSysState->lpTopoData);
					break;
			}
		} else {
			return;
		}
		pSolarSysState->TopoFrame = CaptureDrawable (
				CreateDrawable (WANT_PIXMAP, (SIZE)MAP_WIDTH, (SIZE)MAP_HEIGHT, 1)
				);
		pSolarSysState->OrbitalCMap = CaptureColorMap (
				LoadColorMap (PlanDataPtr->CMapInstance)
				);
		pSolarSysState->XlatRef = CaptureStringTable (
				LoadStringTable (PlanDataPtr->XlatTabInstance)
				);

		if (pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature > HOT_THRESHOLD)
		{
			pSolarSysState->OrbitalCMap = SetAbsColorMapIndex (
					pSolarSysState->OrbitalCMap, 2
					);
			pSolarSysState->XlatRef = SetAbsStringTableIndex (pSolarSysState->XlatRef, 2);
		}
		else if (pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature > COLD_THRESHOLD)
		{
			pSolarSysState->OrbitalCMap = SetAbsColorMapIndex (
					pSolarSysState->OrbitalCMap, 1
					);
			pSolarSysState->XlatRef = SetAbsStringTableIndex (pSolarSysState->XlatRef, 1);
		}
		pSolarSysState->XlatPtr = GetStringAddress (pSolarSysState->XlatRef);
		RenderTopography (FALSE);
	}
	// Generate a pixel array fromthe Topography map.
	// We use this instead of lpTopoData because it needs to be WAP_WIDTH+MAP_HEIGHT wide
	// and we need this method for Earth anyway.  It may be more efficient to build it
	// from lpTopoData instead of the FRAMPTR though
	x = MAP_WIDTH + MAP_HEIGHT;
	y = MAP_HEIGHT;
	pSolarSysState->lpTopoMap = getpixelarray (
			pSolarSysState->TopoFrame, x, y
			);
	// Extend the width from MAP_WIDTH to MAP_WIDTH+MAP_HEIGHT
	for (y = 0; y < MAP_HEIGHT; y++)
		for (x = 0; x < MAP_HEIGHT; x++)
			pSolarSysState->lpTopoMap[y][x + MAP_WIDTH] = pSolarSysState->lpTopoMap[y][x];
	if (pSolarSysState->pOrbitalDesc->pPrevDesc == &pSolarSysState->SunDesc[0])
	{
		// This is a planet.  Get its location
		loc = pSolarSysState->pOrbitalDesc->location;
	} else {
		// This is a moon.  Get its planet's location
		loc = pSolarSysState->pOrbitalDesc->pPrevDesc->location;
	}
	RenderPhongMask (loc);

	if (pPlanetDesc->data_index & PLANET_SHIELDED)
		CreateShieldMask();

	SetContext (OldContext);

	SeedRandom (old_seed);

	SetPlanetMusic ((UBYTE)(pPlanetDesc->data_index & ~PLANET_SHIELDED));
}

int
rotate_planet_task (void *data)
{
	SIZE i;
	DWORD TimeIn;
	PSOLARSYS_STATE pSS;
	BOOLEAN repair, zooming;
	int angle=0;

	Task task = (Task) data;

	repair = FALSE;
	zooming = TRUE;

	pSS = pSolarSysState;
	while (((PSOLARSYS_STATE volatile)pSS)->MenuState.Initialized < 2 && !Task_ReadState (task, TASK_EXIT))
		TaskSwitch ();

//	SetPlanetTilt ((pSS->SysInfo.PlanetInfo.AxialTilt << 8) / 360);
	SetPlanetTilt (pSS->SysInfo.PlanetInfo.AxialTilt);
//	angle=pSS->SysInfo.PlanetInfo.AxialTilt;
			
	i = 1 - ((pSS->SysInfo.PlanetInfo.AxialTilt & 1) << 1);
	TimeIn = GetTimeCounter ();
	while (!Task_ReadState (task, TASK_EXIT))
	{
		BYTE view_index;
		COORD x;

		if (i > 0)
			x = 0;
		else
			x = MAP_WIDTH - 1;
		view_index = MAP_WIDTH;
		do
		{
			// This SetSemaphore was placed before the RotatePlanet call
			// To prevent the thread from being interrupted by the flash
			// task while computing the Planet Frame.  This should help
			// to smooth out the planet rotation animation.
			// The PauseRotate needs to be placed after the SetSemaphore,
			// to gaurantee that PauseRotate doesn't change while waiting
			// to aquire the GraphicsSem
			SetSemaphore (GraphicsSem);
			if (((PSOLARSYS_STATE volatile)pSS)->PauseRotate !=1
//			if (((PSOLARSYS_STATE volatile)pSS)->MenuState.Initialized <= 3
					&& !(GLOBAL (CurrentActivity) & CHECK_ABORT))
			{
				//PauseRotate == 2 is a single-step
				if (((PSOLARSYS_STATE volatile)pSS)->PauseRotate == 2)
					pSS->PauseRotate = 1;

				repair = RotatePlanet (x, angle, SIS_SCREEN_WIDTH >> 1,
						(148 - SIS_ORG_Y) >> 1, zooming
						);

				if (!repair && zooming)
				{
					zooming = FALSE;
					++pSS->MenuState.Initialized;
				}
				x += i;
			}
			ClearSemaphore (GraphicsSem);
			SleepThreadUntil (TimeIn + (ONE_SECOND * ROTATION_TIME) / (MAP_WIDTH));
//			SleepThreadUntil (TimeIn + (ONE_SECOND * 5 / (MAP_WIDTH-32)));
			TimeIn = GetTimeCounter ();
		} while (--view_index && !Task_ReadState (task, TASK_EXIT));
	}
	FinishTask (task);
	return(0);
}
