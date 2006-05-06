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

#include "nameref.h"
#include "resinst.h"
#include "setup.h"
#include "options.h"
#include "planets/planets.h"
#include "planets/scan.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/mathlib.h"
#include "libs/log.h"
#include <math.h>
#include <time.h>


#define PROFILE_ROTATION  1
#define ROTATION_TIME 12

// The initial size of the planet when zooming.  MUST BE ODD
#define PLANET_INIT_ZOOM_SIZE 3

// define USE_ALPHA_SHIELD to use an aloha overlay instead of
// an additive overlay for the shield effect
#undef USE_ALPHA_SHIELD

// XXX: these are currently defined in libs/graphics/sdl/3do_getbody.c
//  they should be sorted out and cleaned up at some point
extern DWORD frame_mapRGBA (FRAME FramePtr, UBYTE r, UBYTE g,
		UBYTE b, UBYTE a);
extern void process_rgb_bmp (FRAME FramePtr, DWORD *rgba, int maxx, int maxy);
extern FRAME stretch_frame (FRAME FramePtr, int neww, int newh, int destroy);
extern void fill_frame_rgb (FRAME FramePtr, DWORD color, int x0, int y0,
		int x, int y);
extern void arith_frame_blit (FRAME srcFrame, RECT *rsrc, FRAME dstFrame,
		RECT *rdst, int num, int denom);
extern void getpixelarray (void *map, int Bpp, FRAMEPTR FramePtr,
		int width, int height);


#define SHIELD_GLOW_COMP    120
#define SHIELD_REFLECT_COMP 100

#define NUM_BATCH_POINTS 64
#define USE_3D_PLANET 1
#define RADIUS 37
//2*RADIUS
#define TWORADIUS (RADIUS << 1)
//RADIUS^2
#define RADIUS_2 (RADIUS * RADIUS)
// distance beyond which all pixels are transparent (for aa)
#define RADIUS_THRES  ((RADIUS + 1) * (RADIUS + 1))
#define DIAMETER (TWORADIUS + 1)
#if 0
#	define SPHERE_SPAN_X (MAP_WIDTH >> 1)
#else
#	define SPHERE_SPAN_X (MAP_HEIGHT)
#endif
		// XXX: technically, the sphere's span over X should be MAP_WIDTH/2
		// but this causes visible surface compression over X, because
		// the surface dims ratio is H x H*PI, instead of H x 2*H
		// see bug #885

#define DIFFUSE_BITS 16
#define AA_WEIGHT_BITS 16

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

typedef struct 
{
	POINT p[4];
	DWORD m[4];
} MAP3D_POINT;

MAP3D_POINT map_rotate[DIAMETER][DIAMETER];

typedef struct
{
	double x, y, z;
} POINT3;

static void
TransformTopography (FRAME DstFrame, PBYTE pTopoData, int w, int h)
{
	CONTEXT OldContext;
	FRAME OldFrame;

	OldContext = SetContext (TaskContext);
	OldFrame = SetContextFGFrame (DstFrame);

	if (pSolarSysState->XlatRef == 0)
	{
		STAMP s;

		s.origin.x = s.origin.y = 0;
#if 0
		s.frame = SetAbsFrameIndex (
				pSolarSysState->PlanetFrameArray[2], 1
				);
		DrawStamp (&s);
#endif
	}
	else
	{
		COUNT i;
		BYTE AlgoType;
		SIZE base, d;
		POINT pt;
		PLANDATAPTR PlanDataPtr;
		PRIMITIVE BatchArray[NUM_BATCH_POINTS];
		PPRIMITIVE pBatch;
		PBYTE pSrc, xlat_tab, cbase;
		HOT_SPOT OldHot;
		RECT ClipRect;

		OldHot = SetFrameHot (DstFrame, MAKE_HOT_SPOT (0, 0));
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
		base = PlanDataPtr->base_elevation;
		xlat_tab = (PBYTE)((XLAT_DESCPTR)pSolarSysState->XlatPtr)->xlat_tab;
		cbase = GetColorMapAddress (pSolarSysState->OrbitalCMap);

		i = NUM_BATCH_POINTS;
		pBatch = &BatchArray[i];
		pSrc = pTopoData;
		for (pt.y = 0; pt.y < h; ++pt.y)
		{
			for (pt.x = 0; pt.x < w; ++pt.x, ++pSrc)
			{
				PBYTE ctab;

				d = (SBYTE)*pSrc;
				if (AlgoType == GAS_GIANT_ALGO)
				{
					d &= 255;
				}
				else
				{
					d += base;
					if (d < 0)
						d = 0;
					else if (d > 255)
						d = 255;
				}

				--pBatch;
				pBatch->Object.Point.x = pt.x;
				pBatch->Object.Point.y = pt.y;

				d = xlat_tab[d] - cbase[0];
				ctab = (cbase + 2) + d * 3;

				// fixed planet surfaces being too dark
				// ctab shifts were previously >> 3 .. -Mika
				SetPrimColor (pBatch, BUILD_COLOR (MAKE_RGB15 (ctab[0] >> 1,
								ctab[1] >> 1, ctab[2] >> 1), d));
				
				if (--i == 0)
				{	// flush the batch and start the next one
					DrawBatch (BatchArray, 0, 0);
					i = NUM_BATCH_POINTS;
					pBatch = &BatchArray[i];
				}
			}
		}

		if (i < NUM_BATCH_POINTS)
		{
			DrawBatch (BatchArray, i, 0);
		}

		SetContextClipping (TRUE);
		SetContextClipRect (&ClipRect);
		SetFrameHot (DstFrame, OldHot);
	}

	SetContextFGFrame (OldFrame);
	SetContext (OldContext);
}

static void
RenderTopography (BOOLEAN Reconstruct)
		// Reconstruct arg was not used on 3DO and is not needed here either
{
	TransformTopography (pSolarSysState->TopoFrame,
			pSolarSysState->Orbit.lpTopoData, MAP_WIDTH, MAP_HEIGHT);

	(void)Reconstruct; // swallow compiler whining
}

static inline void
P3mult (POINT3 *res, POINT3 *vec, double cnst)
{
	res->x = vec->x * cnst;
	res->y = vec->y * cnst;
	res->z = vec->z * cnst;
}

static inline void
P3sub (POINT3 *res, POINT3 *v1, POINT3 *v2)
{
	res->x = v1->x - v2->x;
	res->y = v1->y - v2->y;
	res->z = v1->z - v2->z;
}

static inline double
P3dot (POINT3 *v1, POINT3 *v2)
{
	return (v1->x * v2->x + v1->y * v2->y + v1->z * v2->z);
}

static inline void
P3norm (POINT3 *res, POINT3 *vec)
{
	double mag = sqrt (P3dot (vec, vec));
	P3mult (res, vec, 1/mag);
}

// RenderPhongMask builds a shadow map for the rotating planet
//  loc indicates the planet's position relative to the sun
static void
RenderPhongMask (POINT loc)
{
	POINT pt;
	POINT3 light;
	double lrad;
	const DWORD step = 1 << DIFFUSE_BITS;
	int y, x;

#define AMBIENT_LIGHT 0.1
#define LIGHT_Z       1.2
	// lrad is the distance from the sun to the planet
	lrad = sqrt (loc.x * loc.x + loc.y * loc.y);
	// light is the sun's position.  the z-coordinate is whatever
	// looks good
	light.x = -((double)loc.x);
	light.y = -((double)loc.y);
	light.z = LIGHT_Z * lrad;
	P3norm (&light, &light);
	
	for (pt.y = 0, y = -RADIUS; pt.y <= TWORADIUS; ++pt.y, y++)
	{
		DWORD y_2 = y * y;

		for (pt.x = 0, x = -RADIUS; pt.x <= TWORADIUS; ++pt.x, x++)
		{
			DWORD x_2 = x * x;
			DWORD rad_2 = x_2 + y_2;
			DWORD diff_int = 0;
			POINT3 norm;
			double diff;
			
			if (rad_2 < RADIUS_THRES) 
			{
				// norm is the sphere's surface normal.
				norm.x = (double)x;
				norm.y = (double)y;
				norm.z = (sqrt (RADIUS_2 - x_2) * sqrt (RADIUS_2 - y_2)) /
						RADIUS;
				P3norm (&norm, &norm);
				// diffuse component is norm dot light
				diff = P3dot (&norm, &light);
				// negative diffuse is bad
				if (diff < 0)
					diff = 0.0;
#if 0
				// Specular is not used in practice and is left here
				//  if someone decides to use it later for some reason.
				//  Specular highlight is only good for perfectly smooth
				//  surfaces, like balls (of which planets are not)
				//  This wouldn't be RenderPhongMask without the Phong eq.
#define LIGHT_INTENS  0.3
#define MSHI          2
				double fb, spec;
				POINT3 rvec;
				POINT3 view;

				// always view along the z-axis
				// ideally use a view point, and have the view change
				// per pixel, but that is too much effort for now.
				// the view MUST be normalized!
				view.x = 0;
				view.y = 0;
				view.z = 1.0;

				// specular highlight is the phong equation:
				// (rvec dot view)^MSHI
				// where rvec = (2*diff)*norm - light (reflection of light
				// around norm)
				P3mult (&rvec, &norm, 2 * diff);
				P3sub (&rvec, &rvec, &light);
				fb = P3dot (&rvec, &view);
				if (fb > 0.0)
					spec = LIGHT_INTENS * pow (fb, MSHI);
				else
					spec = 0;
#endif
				// adjust for the ambient light
				if (diff < AMBIENT_LIGHT)
					diff = AMBIENT_LIGHT;
				// Now we antialias the edge of the spere to look nice
				if (rad_2 > RADIUS_2) 
				{
					diff *= 1 - (sqrt(rad_2) - RADIUS);
					if (diff < 0) 
						diff = 0;
				}
				// diff_int allows us multiply by a ratio without using
				// floating-point.
				diff_int = (DWORD)(diff * step);
			}

			light_diff[pt.y][pt.x] = diff_int;
		}
	}
}

//create_aa_points creates weighted averages for
//  4 points around the 'ideal' point at x,y
//  the concept is to compute the weight based on the
//  distance from the integer location points to the ideal point
static void
create_aa_points (MAP3D_POINT *ppt, double x, double y)
{
	double deltax, deltay, inv_deltax, inv_deltay;
	COORD nextx, nexty;
	COUNT i;
	double d1, d2, d3, d4, m[4];

	if (x < 0)
		x = 0;
	else if (x >= SPHERE_SPAN_X)
		x = SPHERE_SPAN_X - 1;
	if (y < 0)
		y = 0;
	else if (y >= MAP_HEIGHT)
		y = MAP_HEIGHT - 1;

	// get  the integer value of this point
	ppt->p[0].x = (COORD)x;
	ppt->p[0].y = (COORD)y;
	deltax = x - ppt->p[0].x;
	deltay = y - ppt->p[0].y;
	
	// if this point doesn't need modificaton, set m[0]=0
	if (deltax == 0 && deltay == 0)
	{
		ppt->m[0] = 0;
		return;
	}

	// get the neighboring points surrounding the 'ideal' point
	if (deltax != 0)
		nextx = ppt->p[0].x + 1;
	else
		nextx = ppt->p[0].x;
	if (deltay != 0)
		nexty = ppt->p[0].y + 1;
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

	for (i = 0; i < 4; i++)
		ppt->m[i] = (DWORD)(m[i] * (1 << AA_WEIGHT_BITS) + 0.5);
}

//get_avg_rgb creates either a red, green, or blue value by
//computing the  weightd averages of the 4 points in p1
static UBYTE 
get_avg_rgb (DWORD p1[4], DWORD mult[4], COUNT offset)
{
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
	ci >>= AA_WEIGHT_BITS;
	//check for overflow
	if (ci > 255)
		ci = 255;
	
	return ((UBYTE)ci);
}

// SetPlanetTilt creates 'map_rotate' to map the topo data
//  for a tilted planet.  It also does the sphere->plane mapping
void
SetPlanetTilt (int angle)
{
	int x, y;
	const double multx = ((double)SPHERE_SPAN_X / M_PI);
	const double multy = ((double)MAP_HEIGHT / M_PI);
	const double xadj = ((double)SPHERE_SPAN_X / 2.0);

	for (y = -RADIUS; y <= RADIUS; y++)
	{
		int y_2 = y * y;

		for (x = -RADIUS; x <= RADIUS; x++)
		{
			double dx, dy, newx, newy;
			double da, rad, rad_2;
			double xa, ya;
			MAP3D_POINT *ppt = &map_rotate[y + RADIUS][x + RADIUS];
			
			rad_2 = x * x + y_2;

			if (rad_2 >= RADIUS_THRES)
			{	// pixel won't be present
				ppt->p[0].x = x + RADIUS;
				ppt->p[0].y = y + RADIUS;
				ppt->m[0] = 0;

				continue;
			}
			
			rad = sqrt (rad_2);
			// antialiasing goes beyond the actual radius
			if (rad >= RADIUS)
				rad = (double)RADIUS - 0.1;
			
			da = atan2 ((double)y, (double)x);
			// compute the planet-tilt
			da += M_DEG2RAD * angle;
			dx = rad * cos (da);
			dy = rad * sin (da);

			// Map the sphere onto a plane
			xa = acos (-dx / RADIUS);
			ya = acos (-dy / RADIUS);
			newx = multx * xa;
			newy = multy * ya;
			// Adjust for vertical curvature
			if (ya <= 0.05 || ya >= 3.1 /* almost PI */)
				newx = xadj; // exact centerline
			else
				newx = xadj + ((newx - xadj) / sin (ya));

			create_aa_points (ppt, newx, newy);
		}
	}
}

//init_zoom_array
// evaluate the function 5/6*(1-e^(-x/14)) to get a decelerating zoom
// on entering planet orbit.  This gives us nearly equivalent to what
// the 3DO does.
#define ZOOM_TIME (1.13)
#define ZOOM_FACT1 (6.0 / 5)
#define ZOOM_FACT2 (14.0 / 25)
COUNT
init_zoom_array (COUNT *zoom_arr)
{
	float frames_per_sec;
	int num_frames, i;
	int base = GSCALE_IDENTITY;

	frames_per_sec = (float)MAP_WIDTH / ROTATION_TIME;
	num_frames = (int)((frames_per_sec * ZOOM_TIME)  + 0.5);
	for (i = 0; i < num_frames; i++)
	{
		zoom_arr[i] = (COUNT) (base * ZOOM_FACT1 * 
				(1 - exp (-(i + 1) / (ZOOM_FACT2 * num_frames))));
	}
	zoom_arr[i] = base;
	
	return i;
}

//CreateShieldMask
// The shield is created in two parts.  This routine creates the Halo.
// The red tint of the planet is currently applied in RenderLevelMasks
// This was done because the shield glows and needs to modify how the planet
// gets lit. Currently, the planet area is transparent in the mask made by
// this routine, but a filter can be applied if desired too.

// HALO rim size
#define SHIELD_HALO          7
#define SHIELD_RADIUS        (RADIUS + SHIELD_HALO)
#define SHIELD_DIAM          ((SHIELD_RADIUS << 1) + 1)
#define SHIELD_RADIUS_2      (SHIELD_RADIUS * SHIELD_RADIUS)
#define SHIELD_RADIUS_THRES  ((SHIELD_RADIUS + 1) * (SHIELD_RADIUS + 1))
#define SHIELD_HALO_GLOW     (SHIELD_GLOW_COMP + SHIELD_REFLECT_COMP)
#define SHIELD_HALO_GLOW_MIN (SHIELD_HALO_GLOW >> 2)

static FRAME
CreateShieldMask (void)
{
	DWORD clear, *rgba, *p_rgba;
	int x, y;
	FRAME ShieldFrame;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	ShieldFrame = CaptureDrawable (
			CreateDrawable (WANT_PIXMAP | WANT_ALPHA,
				SHIELD_DIAM, SHIELD_DIAM, 1));

	rgba = Orbit->ScratchArray;
	p_rgba = rgba;
	//  This is 100% transparent.
	clear = frame_mapRGBA (ShieldFrame, 0, 0, 0, 0);

	for (y = -SHIELD_RADIUS; y <= SHIELD_RADIUS; y++)
	{
		for (x = -SHIELD_RADIUS; x <= SHIELD_RADIUS; x++, p_rgba++)
		{
			int rad_2 = x * x + y * y;
			// This is a non-transparent red for the halo
			int red = SHIELD_HALO_GLOW;
			int alpha = 255;
			double rad;
			
			if (rad_2 >= SHIELD_RADIUS_THRES)
			{	// outside all bounds
				*p_rgba = clear;
				continue;
			}
			// Inside the halo
			if (rad_2 <= RADIUS_2)
			{	// planet's pixels, ours transparent
				*p_rgba = clear;
				continue;
			}
			
			// The halo itself
			rad = sqrt (rad_2);

			if (rad <= RADIUS + 0.8)
			{	// pixels common between the shield and planet
				// do antialiasing using alpha
				alpha = (int) (red * (rad - RADIUS));
				red = 255;
			}
			else
			{	// shield pixels
				red -= (int) ((red - SHIELD_HALO_GLOW_MIN) * (rad - RADIUS)
						/ SHIELD_HALO);
				if (red < 0)
					red = 0;
			}
			
			*p_rgba = frame_mapRGBA (ShieldFrame, red, 0, 0, alpha);
		}
	}
	
	process_rgb_bmp (ShieldFrame, rgba, SHIELD_DIAM, SHIELD_DIAM);
	SetFrameHot (ShieldFrame, MAKE_HOT_SPOT (SHIELD_RADIUS + 1,
				SHIELD_RADIUS + 1));
	
	return ShieldFrame;
}

// SetShieldThrobEffect adjusts the red levels in the shield glow graphic
//  the throbbing cycle is tied to the planet rotation cycle
#define SHIELD_THROBS 12
		// throb cycles per revolution
#define THROB_CYCLE      ((MAP_WIDTH << 8) / SHIELD_THROBS)
#define THROB_HALF_CYCLE (THROB_CYCLE >> 1)

#define THROB_MAX_LEVEL 256
#define THROB_MIN_LEVEL 100
#define THROB_D_LEVEL   (THROB_MAX_LEVEL - THROB_MIN_LEVEL)

static inline int
shield_level (int offset)
{
	int level;

	offset = (offset << 8) % THROB_CYCLE;
	level = abs (offset - THROB_HALF_CYCLE);
	level = THROB_MIN_LEVEL + level * THROB_D_LEVEL / THROB_HALF_CYCLE;

	return level;
}

// See description above
// offset is effectively the angle of rotation around the planet's axis
static void
SetShieldThrobEffect (FRAME ShieldFrame, int offset, FRAME ThrobFrame)
{
	int i;
	int width, height;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;
	DWORD *rgba;
	int level;

	level = shield_level (offset);

	width = GetFrameWidth (ShieldFrame);
	height = GetFrameHeight (ShieldFrame);
	getpixelarray (Orbit->ScratchArray, 4, ShieldFrame, width, height);
	
	for (i = 0, rgba = Orbit->ScratchArray; i < width * height; ++i, ++rgba)
	{
		DWORD p = *rgba;
		int r, g, b, a;

		r = (UBYTE)(p >> 24);
		g = (UBYTE)(p >> 16);
		b = (UBYTE)(p >> 8);
		a = (UBYTE)(p);

		if (a == 255)
		{	// adjust color data for full-alpha pixels
			r = r * level / THROB_MAX_LEVEL;
			g = g * level / THROB_MAX_LEVEL;
			b = b * level / THROB_MAX_LEVEL;
		}
		else if (a > 0)
		{	// adjust alpha for translucent pixels
			a = a * level / THROB_MAX_LEVEL;
		}

		*rgba = frame_mapRGBA (ThrobFrame, r, g, b, a);
	}
	
	process_rgb_bmp (ThrobFrame, Orbit->ScratchArray, width, height);
	SetFrameHot (ThrobFrame, GetFrameHot (ShieldFrame));
}

// Apply the shield to the topo image
static void
ApplyShieldTint (void)
{
	UBYTE a;
	int blit_type;
	FRAME tintFrame = pSolarSysState->Orbit.TintFrame;
	DWORD p;

#ifdef USE_ALPHA_SHIELD
	a = 200;
	blit_type = 0;
#else
	//additive_blit
	a = 255;
	blit_type = -1;
#endif
	p = frame_mapRGBA (tintFrame, 255, 0, 0, a);
	fill_frame_rgb (tintFrame, p, 0, 0, 0, 0);
	arith_frame_blit (tintFrame, NULL, pSolarSysState->TopoFrame, NULL,
			0, blit_type);
}

static inline UBYTE
calc_map_light (UBYTE val, DWORD dif, int lvf)
{
	int i;

	// apply diffusion
	i = (dif * val) >> DIFFUSE_BITS;
	// apply light variance for 3d lighting effect
	i += (lvf * val) >> 7;

	if (i < 0)
		i = 0;
	else if (i > 255)
		i = 255;

	return ((UBYTE)i);
}

static inline DWORD
get_map_pixel (DWORD *pixels, int x, int y)
{
	return pixels[y * (MAP_WIDTH + SPHERE_SPAN_X) + x];
}

static inline int
get_map_elev (SBYTE *elevs, int x, int y, int offset)
{
	return elevs[y * MAP_WIDTH + (offset + x) % MAP_WIDTH];
}

// RenderLevelMasks builds a frame for the rotating planet view
// offset is effectively the angle of rotation around the planet's axis
// We use the SDL routines to directly write to the SDL_Surface to improve performance
void
RenderLevelMasks (FRAME MaskFrame, int offset, BOOLEAN doThrob)
{
	POINT pt;
	DWORD *rgba, *p_rgba;
	DWORD clear;
	int x, y;
	DWORD *pixels;
	SBYTE *elevs;
	int shLevel;

#if PROFILE_ROTATION
	static clock_t t = 0;
	static int frames_done = 1;
	clock_t t1;
	t1 = clock ();
#endif


	shLevel = shield_level (offset);

	rgba = pSolarSysState->Orbit.ScratchArray;
	p_rgba = rgba;
	clear = frame_mapRGBA (MaskFrame, 0, 0, 0, 0);
	pixels = pSolarSysState->Orbit.lpTopoMap + offset;
	elevs = pSolarSysState->Orbit.lpTopoData;
	
	for (pt.y = 0, y = -RADIUS; pt.y <= TWORADIUS; ++pt.y, ++y)
	{
		for (pt.x = 0, x = -RADIUS; pt.x <= TWORADIUS; ++pt.x, ++x, ++p_rgba)
		{
			UBYTE c[3];
			DWORD diffus = light_diff[pt.y][pt.x];
			int i;
			MAP3D_POINT *ppt = &map_rotate[pt.y][pt.x];
			int lvf; // light variance factor
	
			if (diffus == 0)
			{	// full diffusion
				*p_rgba = clear;
				continue;
			}

			// get pixel from topo map and factor from light variance map
			if (ppt->m[0] == 0) 
			{	// exact pixel from the topo map
				DWORD p = get_map_pixel (pixels, ppt->p[0].x, ppt->p[0].y);
				c[0] = (UBYTE)(p >> 8);
				c[1] = (UBYTE)(p >> 16);
				c[2] = (UBYTE)(p >> 24);

				lvf = get_map_elev (elevs, ppt->p[0].x, ppt->p[0].y, offset);
			}
			else
			{	// fractional pixel -- blend from 4
				DWORD p[4];
				int lvsum;

				// compute 'ideal' pixel
				for (i = 0; i < 4; i++)
					p[i] = get_map_pixel (pixels, ppt->p[i].x, ppt->p[i].y);
				for (i = 1; i < 4; i++)
					c[i - 1] = get_avg_rgb (p, ppt->m, i);

				// compute 'ideal' light variance
				for (i = 0, lvsum = 0; i < 4; i++)
					lvsum += get_map_elev (elevs, ppt->p[0].x, ppt->p[0].y,
							offset) * ppt->m[i];
				lvf = lvsum >> AA_WEIGHT_BITS;
			}
		
			// Apply the lighting model.  This also bounds the sphere
			// to make it circular.
			if (pSolarSysState->pOrbitalDesc->data_index & PLANET_SHIELDED)
			{
				int r;
				
				// add lite red filter (3/4) component
				c[1] = (c[1] >> 1) + (c[1] >> 2);
				c[0] = (c[0] >> 1) + (c[0] >> 2);

				c[2] = calc_map_light (c[2], diffus, lvf);
				c[1] = calc_map_light (c[1], diffus, lvf);
				c[0] = calc_map_light (c[0], diffus, lvf);

				// The shield is glow + reflect (+ filter for others)
				r = calc_map_light (SHIELD_REFLECT_COMP, diffus, 0);
				r += SHIELD_GLOW_COMP;
				
				if (doThrob)
				{	// adjust red level for throbbing shield
					r = r * shLevel / THROB_MAX_LEVEL;
				}

				r += c[2];
				if (r > 255)
					r = 255;
				c[2] = r;
			} 
			else
			{
				c[2] = calc_map_light (c[2], diffus, lvf);
				c[1] = calc_map_light (c[1], diffus, lvf);
				c[0] = calc_map_light (c[0], diffus, lvf);
			}

			*p_rgba = frame_mapRGBA (MaskFrame, c[2], c[1], c[0], 255);
		}
	}
	
	// Map the rgb bitmap onto the SDL_Surface
	process_rgb_bmp (MaskFrame, rgba, DIAMETER, DIAMETER);
	SetFrameHot (MaskFrame, MAKE_HOT_SPOT (RADIUS + 1, RADIUS + 1));

#if PROFILE_ROTATION
	t += clock() - t1;
	if (frames_done == MAP_WIDTH)
	{
		log_add (log_Debug, "Rotation frames/sec: %d/%ld(msec)=%f",
				frames_done,
				(long int) (((double)t / CLOCKS_PER_SEC) * 1000.0 + 0.5),
				frames_done / ((double)t / CLOCKS_PER_SEC + 0.5));
		frames_done = 1;
		t = clock () - t1;
	}
	else
		frames_done++;
#endif
}


#define RANGE_SHIFT 6

static void
DitherMap (PSBYTE DepthArray)
{
	COUNT i;
	PSBYTE lpDst;

	i = (MAP_WIDTH * MAP_HEIGHT) >> 2;
	lpDst = DepthArray;
	do
	{
		DWORD rand_val;

		rand_val = TFB_Random ();
		*lpDst++ += (SBYTE) ((1 << (RANGE_SHIFT - 4))
				- (LOBYTE (LOWORD (rand_val)) &
				((1 << (RANGE_SHIFT - 3)) - 1)));
		*lpDst++ += (SBYTE) ((1 << (RANGE_SHIFT - 4))
				- (HIBYTE (LOWORD (rand_val)) &
				((1 << (RANGE_SHIFT - 3)) - 1)));
		*lpDst++ += (SBYTE) ((1 << (RANGE_SHIFT - 4))
				- (LOBYTE (HIWORD (rand_val)) &
				((1 << (RANGE_SHIFT - 3)) - 1)));
		*lpDst++ += (SBYTE) ((1 << (RANGE_SHIFT - 4))
				- (HIBYTE (HIWORD (rand_val)) &
				((1 << (RANGE_SHIFT - 3)) - 1)));
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
				memset ((PSBYTE)&DepthArray[TopIndex + lf_x], 0,
						rt_x - lf_x + 1);
				memset ((PSBYTE)&DepthArray[BotIndex + lf_x], 0,
						rt_x - lf_x + 1);
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
	PRECT pstorm_r;

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

			rand_val = TFB_Random ();
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

			rand_val = TFB_Random ();
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

	band_delta = ((LOWORD (TFB_Random ())
			& (NUM_BAND_COLORS - 1)) << RANGE_SHIFT)
			+ (1 << (RANGE_SHIFT - 1));
	last_y = next_y = 0;
	for (i = num_bands; i > 0; --i)
	{
		COORD cur_y;

		rand_val = TFB_Random ();
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

	MakeStorms ((COUNT)(4 + ((COUNT)TFB_Random () & 3) + 1), DepthArray);

	DitherMap (DepthArray);
}

static void
ValidateMap (PSBYTE DepthArray)
{
	BYTE state;
	BYTE pixel_count[2], lb[2];
	SBYTE last_byte;
	COUNT i;
	PSBYTE lpDst;

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
planet_orbit_init ()
{
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	Orbit->PlanetFrameArray = CaptureDrawable (
			CreateDrawable (WANT_PIXMAP | WANT_ALPHA, DIAMETER, DIAMETER,
				2));
	Orbit->TintFrame = CaptureDrawable (
			CreateDrawable (WANT_PIXMAP | WANT_ALPHA, (SWORD)MAP_WIDTH,
				(SWORD)MAP_HEIGHT, 2));
	Orbit->ObjectFrame = 0;
	Orbit->WorkFrame = 0;
	Orbit->lpTopoData = HMalloc (MAP_WIDTH * MAP_HEIGHT);
	Orbit->TopoZoomFrame = CaptureDrawable (
			CreateDrawable (WANT_PIXMAP, (COUNT)(MAP_WIDTH << 2),
				(COUNT)(MAP_HEIGHT << 2), 1));
	Orbit->lpTopoMap = HMalloc (sizeof (DWORD)
			* (MAP_HEIGHT * (MAP_WIDTH + SPHERE_SPAN_X)));
	// always allocate the scratch array to largest needed size
	Orbit->ScratchArray = HMalloc (sizeof (DWORD)
			* (SHIELD_DIAM) * (SHIELD_DIAM));
}

static unsigned
frandom ()
{
	static unsigned seed = 0x12345678;
	
	if (seed == 0)
		seed = 15807;
	seed = (seed >> 4) * 227;

	return seed;
}

static inline int
TopoVarianceFactor (int step, int allowed, int min)
{
#define SCALE_SHIFT 8
	return ((abs(step) * allowed) >> SCALE_SHIFT) + min;
}

static inline int
TopoVarianceCalc (int factor)
{
	if (factor == 0)
		return 0;
	else
		return (frandom () % factor) - (factor >> 1);
}

static void
TopoScale4x (PBYTE pDstTopo, PBYTE pSrcTopo, int num_faults, int fault_var)
{
		// Interpolate the topographical data by connecting the elevations
		// to their nearest neighboors using straight lines (in random
		// direction) with random variance factors defined by
		// num_faults and fault_var args
#define AVG_VARIANCE 250
	int x, y;
	const int w = MAP_WIDTH, h = MAP_HEIGHT;
	const int spitch = MAP_WIDTH, dpitch = MAP_WIDTH * 4;
	PSBYTE pSrc, pDst;
	int* prevrow;
	int* prow;
	int elev[5][5];
	int var_allow, var_min;
	static const struct line_def_t
	{
		int x0, y0, x1, y1;
		int dx, dy;
	}
	fill_lines[4][6] = 
	{
		{	// diag set 0
			{ 0,  2,  2,  0,  1, -1},
			{ 0,  3,  3,  0,  1, -1},
			{ 0,  4,  4,  0,  1, -1},
			{ 1,  4,  4,  1,  1, -1},
			{ 2,  4,  4,  2,  1, -1},
			{-1, -1, -1, -1,  0,  0}, // term
		},
		{	// diag set 1
			{ 0,  2,  2,  4,  1,  1},
			{ 0,  1,  3,  4,  1,  1},
			{ 0,  0,  4,  4,  1,  1},
			{ 1,  0,  4,  3,  1,  1},
			{ 2,  0,  4,  2,  1,  1},
			{-1, -1, -1, -1,  0,  0}, // term
		},
		{	// horizontal
			{ 0,  1,  4,  1,  1,  0},
			{ 0,  2,  4,  2,  1,  0},
			{ 0,  3,  4,  3,  1,  0},
			{-1, -1, -1, -1,  0,  0}, // term
		},
		{	// vertical
			{ 1,  0,  1,  4,  0,  1},
			{ 2,  0,  2,  4,  0,  1},
			{ 3,  0,  3,  4,  0,  1},
			{-1, -1, -1, -1,  0,  0}, // term
		},
	};
	
	prevrow = (int *) alloca ((MAP_WIDTH * 4 + 1) * sizeof(prevrow[0]));

	var_allow = (num_faults << SCALE_SHIFT) / AVG_VARIANCE;
	var_min = fault_var << SCALE_SHIFT;

	//memset (pDstTopo, 0, MAP_WIDTH * MAP_HEIGHT * 16);

	// init the first row in advance
	pSrc = pSrcTopo;
	prow = prevrow;
#define STEP_RANGE (4 - 1)
	prow[0] = ((int)pSrc[0]) << SCALE_SHIFT;;
	for (x = 0; x < w; ++x, ++pSrc, prow += 4)
	{
		int x2;
		int val, step, rndfact;

		// next point in row
		if (x < w - 1)
			// one right
			prow[4] = ((int)pSrc[1]) << SCALE_SHIFT;
		else
			// wrap around
			prow[4] = ((int)pSrc[1 - spitch]) << SCALE_SHIFT;

		// compute elevations between 2 points
		val = prow[0];
		step = (prow[4] - val) / STEP_RANGE;
		rndfact = TopoVarianceFactor (step, var_allow, var_min);
		for (x2 = 1, val += step; x2 < 4; ++x2, val += step)
			prow[x2] = val + TopoVarianceCalc (rndfact);
	}

	pSrc = pSrcTopo;
	pDst = pDstTopo;
	for (y = 0; y < h; ++y, pDst += dpitch * 3)
	{
		int x2, y2;
		PSBYTE p;
		int val, step, rndfact;
		const struct line_def_t* pld;

		prow = prevrow;
		// prime the first interpolated column
		elev[4][0] = prow[0];
		if (y < h - 1)
			elev[4][4] = ((int)pSrc[spitch]) << SCALE_SHIFT;
		else
			elev[4][4] = elev[4][0];
		// compute elevations for interpolated column
		val = elev[4][0];
		step = (elev[4][4] - val) / STEP_RANGE;
		rndfact = TopoVarianceFactor (step, var_allow, var_min);
		for (y2 = 1, val += step; y2 < 4; ++y2, val += step)
			elev[4][y2] = val + TopoVarianceCalc (rndfact);

		for (x = 0; x < w; ++x, ++pSrc, pDst += 4, prow += 4)
		{
			// recall the first interpolated row from prevrow
			for (x2 = 0; x2 <= 4; ++x2)
				elev[x2][0] = prow[x2];
			// recall the first interpolated column
			for (y2 = 1; y2 <= 4; ++y2)
				elev[0][y2] = elev[4][y2];
			
			if (y < h - 1)
			{
				if (x < w - 1)
					// one right, one down
					elev[4][4] = ((int)pSrc[1 + spitch]) << SCALE_SHIFT;
				else
					// wrap around, one down
					elev[4][4] = ((int)pSrc[1]) << SCALE_SHIFT;
			}
			else
			{
				elev[4][4] = elev[4][0];
			}

			// compute elevations for the rest of square borders first
			val = elev[0][4];
			step = (elev[4][4] - val) / STEP_RANGE;
			rndfact = TopoVarianceFactor (step, var_allow, var_min);
			for (x2 = 1, val += step; x2 < 4; ++x2, val += step)
				elev[x2][4] = val + TopoVarianceCalc (rndfact);

			val = elev[4][0];
			step = (elev[4][4] - val) / STEP_RANGE;
			rndfact = TopoVarianceFactor (step, var_allow, var_min);
			for (y2 = 1, val += step; y2 < 4; ++y2, val += step)
				elev[4][y2] = val + TopoVarianceCalc (rndfact);

			// fill in the rest by connecting opposing elevations
			// some randomness to determine which elevations to connect
			for (pld = fill_lines[frandom () & 3]; pld->x0 >= 0; ++pld)
			{
				int num_steps;

				x2 = pld->x0;
				y2 = pld->y0;
				val = elev[x2][y2];
				num_steps = pld->x1 - pld->x0;
				if (num_steps == 0)
					num_steps = pld->y1 - pld->y0;
				step = (elev[pld->x1][pld->y1] - val) / num_steps;
				rndfact = TopoVarianceFactor (step, var_allow, var_min);
				
				for (x2 += pld->dx, y2 += pld->dy, val += step;
						x2 != pld->x1 || y2 != pld->y1;
						x2 += pld->dx, y2 += pld->dy, val += step)
				{
					elev[x2][y2] = val + TopoVarianceCalc (rndfact);
				}
			}

			// output the interpolated topography
			for (y2 = 0; y2 < 4; ++y2)
			{
				p = pDst + y2 * dpitch;
				for (x2 = 0; x2 < 4; ++x2, ++p)
				{
					int e = elev[x2][y2] >> SCALE_SHIFT;
					if (e > 127)
						e = 127;
					else if (e < -128)
						e = -128;
					*p = (SBYTE)e;
				}
			}

			// save last interpolated row to prevrow for later
			for (x2 = 0; x2 < 4; ++x2)
				prow[x2] = elev[x2][4];
		}
		// save last row point
		prow[0] = elev[4][4];
	}
}


// GenerateLightMap produces a surface light variance map for the
//  rotating planet by, first, transforming absolute elevation data
//  into normalized relative and then applying a weighted
//  average-median of surrounding points
// Lots of pure Voodoo here ;)
//  the goal is a 3D illusion, not mathematically correct lighting

#define LMAP_AVG_BLOCK    ((MAP_HEIGHT + 4) / 5)
#define LMAP_MAX_DIST     ((LMAP_AVG_BLOCK + 1) >> 1)
#define LMAP_WEIGHT_THRES (LMAP_MAX_DIST * 2 / 3)

typedef struct
{
	int min;
	int max;
	int avg;

} elev_block_t;

static inline void
get_vblock_avg (elev_block_t *pblk, PSBYTE pTopo, int x, int y)
{
	SBYTE *elev = pTopo;
	int y0, y1, i;
	int min = 127, max = -127;
	int avg = 0, total_weight = 0;

	// surface wraps around along x
	x = (x + MAP_WIDTH) % MAP_WIDTH;
	
	y0 = y - LMAP_MAX_DIST;
	y1 = y + LMAP_MAX_DIST;
	if (y0 < 0)
		y0 = 0;
	if (y1 > MAP_HEIGHT)
		y1 = MAP_HEIGHT;

	elev = pTopo + y0 * MAP_HEIGHT + x;
	for (i = y0; i < y1; ++i, elev += MAP_HEIGHT)
	{
		int delta = abs (i - y);
		int weight = 255; // full weight
		int v = *elev;

		if (delta >= LMAP_WEIGHT_THRES)
		{	// too far -- progressively reduced weight
			weight = weight * (LMAP_MAX_DIST - delta + 1)
					/ (LMAP_MAX_DIST - LMAP_WEIGHT_THRES + 2);
		}

		if (v > max)
			max = v;
		if (v < min)
			min = v;
		avg += pblk->avg * weight;
		total_weight += weight;
	}
	avg /= total_weight;

	pblk->min = min;
	pblk->max = max;
	pblk->avg = avg / (y1 - y0);
}

// See description above
static void
GenerateLightMap (PSBYTE pTopo, int w, int h)
{
#define LMAP_BLOCKS       (2 * LMAP_MAX_DIST + 1)
	int x, y;
	elev_block_t vblocks[LMAP_BLOCKS];
			// we use a running block average to reduce the amount of work
			// where a block is a vertical line of map points
	SBYTE *elev;
	int min, max, med;
	int sfact, spread;

	// normalize the topo data
	min = 127;
	max = -128;
	for (x = 0, elev = pTopo; x < w * h; ++x, ++elev)
	{
		int v = *elev;
		if (v > max)
			max = v;
		if (v < min)
			min = v;
	}
	med = (min + max) / 2;
	spread = max - med;

	if (spread == 0)
	{	// perfectly smooth surface -- nothing to do but
		// level it out completely
		if (max != 0)
			memset (pTopo, 0, w * h);
		return;
	}

	// these are whatever looks right
	if (spread < 10)
		sfact = 30; // minimal spread
	else if (spread < 30)
		sfact = 60;
	else
		sfact = 100; // full spread
	
	// apply spread
	for (x = 0, elev = pTopo; x < w * h; ++x, ++elev)
	{
		int v = *elev;
		v = (v - med) * sfact / spread;
		*elev = v;
	}

	// compute and apply weighted averages of surrounding points
	for (y = 0, elev = pTopo; y < h; ++y)
	{
		elev_block_t *pblk;
		int i;

		// prime the running block average
		// get the minimum, maximum and avg elevation for each block
		for (i = -LMAP_MAX_DIST; i < LMAP_MAX_DIST; ++i)
		{
			// blocks wrap around on both sides
			pblk = vblocks + ((i + LMAP_BLOCKS) % LMAP_BLOCKS);

			get_vblock_avg (pblk, pTopo, i, y);
		}

		for (x = 0; x < w; ++x, ++elev)
		{
			int avg = 0, total_weight = 0;

			min = 127;
			max = -127;

			// prepare next block as we move along x
			pblk = vblocks + ((x + LMAP_MAX_DIST) % LMAP_BLOCKS);
			get_vblock_avg (pblk, pTopo, x + LMAP_MAX_DIST, y);

			// compute the min, max and weighted avg of blocks
			for (i = x - LMAP_MAX_DIST; i <= x + LMAP_MAX_DIST; ++i)
			{
				int delta = abs (i - x);
				int weight = 255; // full weight

				pblk = vblocks + ((i + LMAP_BLOCKS) % LMAP_BLOCKS);

				if (delta >= LMAP_WEIGHT_THRES)
				{	// too far -- progressively reduced weight
					weight = weight * (LMAP_MAX_DIST - delta + 1)
							/ (LMAP_MAX_DIST - LMAP_WEIGHT_THRES + 2);
				}

				if (pblk->max > max)
					max = pblk->max;
				if (pblk->min < min)
					min = pblk->min;
				
				avg += pblk->avg * weight;
				total_weight += weight;
			}
			avg /= total_weight;

			// This is mostly Voodoo
			// figure out what kind of relative lighting factor
			// to assign to this point
#if 0
			// relative to median
			med = (min + max) / 2; // median
			*elev = (int)*elev - med;
#else
			// relative to median of (average, median)
			med = (min + max) / 2; // median
			med = (med + avg) / 2;
			*elev = (int)*elev - med;
#endif
		}
	}
}

void
GeneratePlanetMask (PPLANET_DESC pPlanetDesc, FRAME SurfDefFrame)
{
	RECT r;
	DWORD old_seed;
	PLANDATAPTR PlanDataPtr;
	COUNT i, y;
	POINT loc;
	CONTEXT OldContext;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;
	PBYTE pScaledTopo = 0;

	old_seed = TFB_SeedRandom (pPlanetDesc->rand_seed);

	OldContext = SetContext (TaskContext);
	planet_orbit_init ();

	PlanDataPtr = &PlanData[pPlanetDesc->data_index & ~PLANET_SHIELDED];

	if (SurfDefFrame)
	{	// This is a defined planet; pixmap for the topography and
		// elevation data is supplied in Surface Definition frame
		BOOLEAN DeleteDef = FALSE;
		FRAME ElevFrame;

		// surface pixmap
		SurfDefFrame = SetAbsFrameIndex (SurfDefFrame, 0);
		if (GetFrameWidth (SurfDefFrame) != MAP_WIDTH
				|| GetFrameHeight (SurfDefFrame) != MAP_HEIGHT)
		{
			pSolarSysState->TopoFrame = stretch_frame (SurfDefFrame,
				MAP_WIDTH, MAP_HEIGHT, 0);
			// will not need the passed FRAME anymore
			DeleteDef = TRUE;
		}
		else
			pSolarSysState->TopoFrame = SurfDefFrame;

		if (GetFrameCount (SurfDefFrame) > 1)
		{	// 2nd frame is elevation data 
			int i;
			BYTE* elev;

			ElevFrame = SetAbsFrameIndex (SurfDefFrame, 1);
			if (GetFrameWidth (ElevFrame) != MAP_WIDTH
					|| GetFrameHeight (ElevFrame) != MAP_HEIGHT)
			{
				ElevFrame = stretch_frame (ElevFrame, MAP_WIDTH,
						MAP_HEIGHT, 0);
			}

			// grab the elevation data in 1 byte per pixel format
			getpixelarray (Orbit->lpTopoData, 1, ElevFrame,
					MAP_WIDTH, MAP_HEIGHT);
			// the supplied data is in unsigned format, must convert
			for (i = 0, elev = Orbit->lpTopoData;
					i < MAP_WIDTH * MAP_HEIGHT;
					++i, ++elev)
			{
				*elev -= 128;
			}
		}
		else
		{	// no elevation data -- planet flat as a pancake
			memset (Orbit->lpTopoData, 0, MAP_WIDTH * MAP_HEIGHT);
		}

		if (DeleteDef)
			DestroyDrawable (ReleaseDrawable (SurfDefFrame));
	}
	else
	{	// Generate planet surface elevation data and look

		r.corner.x = r.corner.y = 0;
		r.extent.width = MAP_WIDTH;
		r.extent.height = MAP_HEIGHT;
		{
			memset (Orbit->lpTopoData, 0, MAP_WIDTH * MAP_HEIGHT);
			switch (PLANALGO (PlanDataPtr->Type))
			{
				case GAS_GIANT_ALGO:
					MakeGasGiant (PlanDataPtr->num_faults,
							Orbit->lpTopoData, &r, PlanDataPtr->fault_depth);
					break;
				case TOPO_ALGO:
				case CRATERED_ALGO:
					if (PlanDataPtr->num_faults)
						DeltaTopography (PlanDataPtr->num_faults,
								Orbit->lpTopoData, &r,
								PlanDataPtr->fault_depth);

					for (i = 0; i < PlanDataPtr->num_blemishes; ++i)
					{
						RECT crater_r;
						UWORD loword;
				
						loword = LOWORD (TFB_Random ());
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
					
						loword = LOWORD (TFB_Random ());
						crater_r.extent.height = crater_r.extent.width;
						crater_r.corner.x = HIBYTE (loword)
								% (MAP_WIDTH - crater_r.extent.width);
						crater_r.corner.y = LOBYTE (loword)
								% (MAP_HEIGHT - crater_r.extent.height);
						MakeCrater (&crater_r, Orbit->lpTopoData,
								PlanDataPtr->fault_depth << 2,
								-(PlanDataPtr->fault_depth << 2),
								FALSE);
					}

					if (PLANALGO (PlanDataPtr->Type) == CRATERED_ALGO)
						DitherMap (Orbit->lpTopoData);
					ValidateMap (Orbit->lpTopoData);
					break;
			}
		}
		pSolarSysState->TopoFrame = CaptureDrawable (
				CreateDrawable (WANT_PIXMAP, (SIZE)MAP_WIDTH,
				(SIZE)MAP_HEIGHT, 1));
		pSolarSysState->OrbitalCMap = CaptureColorMap (
				LoadColorMap (PlanDataPtr->CMapInstance));
		pSolarSysState->XlatRef = CaptureStringTable (
				LoadStringTable (PlanDataPtr->XlatTabInstance));

		if (pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature >
				HOT_THRESHOLD)
		{
			pSolarSysState->OrbitalCMap = SetAbsColorMapIndex (
					pSolarSysState->OrbitalCMap, 2);
			pSolarSysState->XlatRef = SetAbsStringTableIndex (
					pSolarSysState->XlatRef, 2);
		}
		else if (pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature >
				COLD_THRESHOLD)
		{
			pSolarSysState->OrbitalCMap = SetAbsColorMapIndex (
					pSolarSysState->OrbitalCMap, 1);
			pSolarSysState->XlatRef = SetAbsStringTableIndex (
					pSolarSysState->XlatRef, 1);
		}
		pSolarSysState->XlatPtr = GetStringAddress (pSolarSysState->XlatRef);
		RenderTopography (FALSE);
	}

	if (!(pPlanetDesc->data_index & PLANET_SHIELDED)
			&& pSolarSysState->SysInfo.PlanetInfo.AtmoDensity
				!= GAS_GIANT_ATMOSPHERE)
	{	// produce 4x scaled topo image for IP
		// for the planets that we can land on
		pScaledTopo = HMalloc (MAP_WIDTH * 4 * MAP_HEIGHT * 4);
		if (pScaledTopo)
		{
			TopoScale4x (pScaledTopo, Orbit->lpTopoData,
					PlanDataPtr->num_faults, PlanDataPtr->fault_depth
					* (PLANALGO (PlanDataPtr->Type) == CRATERED_ALGO ? 2 : 1  ));
			TransformTopography (Orbit->TopoZoomFrame, pScaledTopo,
					MAP_WIDTH * 4, MAP_HEIGHT * 4);

			HFree (pScaledTopo);
		}
	}

	// Generate a pixel array from the Topography map.
	// We use this instead of lpTopoData because it needs to be
	// WAP_WIDTH+SPHERE_SPAN_X wide and we need this method for Earth anyway.
	// It may be more efficient to build it from lpTopoData instead of the
	// FRAMPTR though.
	getpixelarray (Orbit->lpTopoMap, 4, pSolarSysState->TopoFrame,
			MAP_WIDTH + SPHERE_SPAN_X, MAP_HEIGHT);
	// Extend the width from MAP_WIDTH to MAP_WIDTH+SPHERE_SPAN_X
	for (y = 0; y < MAP_HEIGHT * (MAP_WIDTH + SPHERE_SPAN_X);
			y += MAP_WIDTH + SPHERE_SPAN_X)
		memcpy (Orbit->lpTopoMap + y + MAP_WIDTH, Orbit->lpTopoMap + y,
				SPHERE_SPAN_X * sizeof (Orbit->lpTopoMap[0]));

	if (PLANALGO (PlanDataPtr->Type) != GAS_GIANT_ALGO)
	{	// convert topo data to a light map, based on relative
		// map point elevations
		GenerateLightMap (Orbit->lpTopoData, MAP_WIDTH, MAP_HEIGHT);
	}
	else
	{	// gas giants are pretty much flat
		memset (Orbit->lpTopoData, 0, MAP_WIDTH * MAP_HEIGHT);
	}
			
	if (pSolarSysState->pOrbitalDesc->pPrevDesc ==
			&pSolarSysState->SunDesc[0])
	{	// this is a planet -- get its location
		loc = pSolarSysState->pOrbitalDesc->location;
	}
	else
	{	// this is a moon -- get its planet's location
		loc = pSolarSysState->pOrbitalDesc->pPrevDesc->location;
	}
	
	RenderPhongMask (loc);

	if (pPlanetDesc->data_index & PLANET_SHIELDED)
	{
		Orbit->ObjectFrame = CreateShieldMask ();
		ApplyShieldTint ();
	}

	SetContext (OldContext);

	TFB_SeedRandom (old_seed);
}

int
rotate_planet_task (void *data)
{
	Task task = (Task) data;
	SIZE spin_dir;
	COORD init_x;
	DWORD TimeIn;
	PSOLARSYS_STATE pSS;
	BOOLEAN zooming;
	RECT r, *repair = &r;
	UBYTE zoom_from;
	COUNT zoom_arr[50];
	COUNT zoom_amt, frame_num = 0, zoom_frames;
	PLANET_ORBIT *Orbit;
	FRAME OldShieldFrame = 0;
	FRAME ShieldFrame = 0;
	BOOLEAN doThrob = FALSE;
	COUNT altfi;  // alternating frame index

	r.extent.width = 0;
	zooming = TRUE;

	pSS = pSolarSysState;
	while (*(volatile SIZE *)&pSS->MenuState.Initialized < 2 &&
			!Task_ReadState (task, TASK_EXIT))
		TaskSwitch ();

	SetPlanetTilt (pSS->SysInfo.PlanetInfo.AxialTilt);
	Orbit = &pSolarSysState->Orbit;

	spin_dir = 1 - ((pSS->SysInfo.PlanetInfo.AxialTilt & 1) << 1);
	init_x = (spin_dir == 1) ? (0) : (MAP_WIDTH - 1);
	altfi = 0;

	if (optWhichShield == OPT_3DO &&
			pSolarSysState->pOrbitalDesc->data_index & PLANET_SHIELDED)
	{	// prepare the shield throb effect
		doThrob = TRUE;

		OldShieldFrame = Orbit->ObjectFrame;
		ShieldFrame = CaptureDrawable (
			CreateDrawable (WANT_PIXMAP | WANT_ALPHA,
				GetFrameWidth (OldShieldFrame),
				GetFrameHeight (OldShieldFrame), 2));

		SetShieldThrobEffect (OldShieldFrame, init_x, ShieldFrame);
	}
	
	// Render the first planet frame
	RenderLevelMasks (Orbit->PlanetFrameArray, init_x, doThrob);

	zoom_from = (UBYTE)TFB_Random () & 0x03;
	zoom_frames = init_zoom_array (zoom_arr);
	zoom_amt = zoom_arr[frame_num];

	// Disable zooming when already in orbit
	if (LastActivity & CHECK_LOAD)
		zoom_amt = 0;

	TimeIn = GetTimeCounter ();
	while (!Task_ReadState (task, TASK_EXIT))
	{
		BYTE view_index;
		COORD x;

		x = init_x;
		view_index = MAP_WIDTH;
		do
		{
			// This lock was placed before the RotatePlanet call
			// To prevent the thread from being interrupted by the flash
			// task while computing the Planet Frame.  This should help
			// to smooth out the planet rotation animation.
			// The PauseRotate needs to be placed after the lock,
			// to guarantee that PauseRotate doesn't change while waiting
			// to acquire the graphics lock
			LockMutex (GraphicsLock);
			if (*(volatile UBYTE *)&pSS->PauseRotate != 1
//			if (((PSOLARSYS_STATE volatile)pSS)->MenuState.Initialized <= 3
					&& !(GLOBAL (CurrentActivity) & CHECK_ABORT))
			{
				//PauseRotate == 2 is a single-step
				if (*(volatile UBYTE *)&pSS->PauseRotate == 2)
					pSS->PauseRotate = 1;

				// go to the next frame
				Orbit->PlanetFrameArray = SetAbsFrameIndex (
						Orbit->PlanetFrameArray, altfi);
				if (doThrob)
				{
					Orbit->ObjectFrame = SetAbsFrameIndex (
							ShieldFrame, altfi);
				}

				repair = RotatePlanet (x, SIS_SCREEN_WIDTH >> 1,
						(148 - SIS_ORG_Y) >> 1, zoom_amt, zoom_from, repair);

				if (!repair && zooming)
				{
					zooming = FALSE;
					zoom_amt = 0;
					++pSS->MenuState.Initialized;
				}
				x += spin_dir;
			}
			else
				view_index++;

			UnlockMutex (GraphicsLock);
			
			// Generate the next rotation frame
			altfi ^= 1;
			RenderLevelMasks (SetAbsFrameIndex (Orbit->PlanetFrameArray,
					altfi), x, doThrob);
			if (doThrob)
			{	// prepare next throb frame
				SetShieldThrobEffect (OldShieldFrame, x,
						SetAbsFrameIndex (ShieldFrame, altfi));
			}
			
			if (zooming)
			{
				frame_num++;
				if (frame_num > zoom_frames)
				{
					log_add (log_Warning, "rotate_planet_task() : zoom frame"
							" out of bounds!");
					frame_num = zoom_frames;
				}
				zoom_amt = zoom_arr[frame_num];
			}


			SleepThreadUntil (TimeIn + (ONE_SECOND * ROTATION_TIME) /
					(MAP_WIDTH));
			TimeIn = GetTimeCounter ();
		} while (--view_index && !Task_ReadState (task, TASK_EXIT));
	}

	if (OldShieldFrame)
	{
		Orbit->ObjectFrame = OldShieldFrame;
		DestroyDrawable (ReleaseDrawable (ShieldFrame));
	}

	FinishTask (task);

	return 0;
}


