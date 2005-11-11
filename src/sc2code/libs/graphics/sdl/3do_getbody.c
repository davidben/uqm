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

#ifdef GFXMODULE_SDL

#ifdef WIN32
#include <io.h>
#endif
#include <fcntl.h>

#include "options.h"
#include "port.h"
#include "sdl_common.h"
#include "sdluio.h"
#include "libs/file.h"
#include "libs/reslib.h"
#include "../font.h"
#include "primitives.h"


typedef struct anidata
{
	int transparent_color;
	int colormap_index;
	int hotspot_x;
	int hotspot_y;
} AniData;


static void
process_image (FRAMEPTR FramePtr, SDL_Surface *img[], AniData *ani, int cel_ct)
{
	TFB_Image *tfbimg;
	int hx, hy;

	TYPE_SET (FramePtr->TypeIndexAndFlags, ROM_DRAWABLE);
	INDEX_SET (FramePtr->TypeIndexAndFlags, cel_ct);

	// handle transparency cases
	if (img[cel_ct]->format->palette)
	{	// indexed color image
		if (ani[cel_ct].transparent_color != -1)
		    SDL_SetColorKey (img[cel_ct], SDL_SRCCOLORKEY,
					ani[cel_ct].transparent_color);
	}
	else if (img[cel_ct]->format->BitsPerPixel > 8)
	{	// special transparency cases for truecolor images
		if (ani[cel_ct].transparent_color == 0)
			// make RGB=0,0,0 transparent
		    SDL_SetColorKey (img[cel_ct], SDL_SRCCOLORKEY,
					SDL_MapRGBA (img[cel_ct]->format, 0, 0, 0, 0));
	}
	if (ani[cel_ct].transparent_color == -1)
	{	// enforce -1 to mean 'no transparency'
		SDL_SetColorKey (img[cel_ct], 0, 0);
		// set transparent_color == -2 to use PNG tRNS transparency
	}
	
	hx = ani[cel_ct].hotspot_x;
	hy = ani[cel_ct].hotspot_y;

	FramePtr->image = TFB_DrawImage_New (img[cel_ct]);

	tfbimg = FramePtr->image;
	tfbimg->colormap_index = ani[cel_ct].colormap_index;
	img[cel_ct] = (SDL_Surface *)tfbimg->NormalImg;
	
	FramePtr->HotSpot = MAKE_HOT_SPOT (hx, hy);
	SetFrameBounds (FramePtr, img[cel_ct]->w, img[cel_ct]->h);

#ifdef CLIPDEBUG
	{
		/* for debugging clipping:
		   draws white (or most matching color from palette) pixels to
	       every corner of the image
		 */
		Uint32 color = SDL_MapRGB (img[cel_ct]->format, 255, 255, 255);
		SDL_Rect r = {0, 0, 1, 1};
		if (img[cel_ct]->w > 2 && img[cel_ct]->h > 2)
		{
			SDL_FillRect (img[cel_ct], &r, color);
			r.x = img[cel_ct]->w - 1;
			SDL_FillRect (img[cel_ct], &r, color);
			r.y = img[cel_ct]->h - 1;
			SDL_FillRect (img[cel_ct], &r, color);
			r.x = 0;
			SDL_FillRect (img[cel_ct], &r, color);
		}
	}
#endif
}

static void
processFontChar (FRAMEPTR FramePtr, SDL_Surface *surf, int charInd)
{
	int x,y;
	Uint8 r,g,b,a;
	Uint32 p;
	Uint32 clear, opaque;
	SDL_Color colors[256];
	SDL_Surface *new_surf;				
	SDL_PixelFormat* srcfmt = surf->format;
	SDL_PixelFormat* dstfmt;
	GetPixelFn getpix;
	PutPixelFn putpix;

	TYPE_SET (FramePtr->TypeIndexAndFlags, ROM_DRAWABLE);
	INDEX_SET (FramePtr->TypeIndexAndFlags, charInd);
			// XXX: Is this still relevant?

//#define ALPHA_FONTS
//#define RGB_FONTS

	SDL_LockSurface (surf);

	clear = 0;
	opaque = 1;

#if defined(ALPHA_FONTS)
	// convert png font to truecolor + alpha
	new_surf = TFB_DrawCanvas_New_TrueColor (surf->w, surf->h, TRUE);

#elif defined(RGB_FONTS)
	// convert png font to truecolor w/ colorkey
	new_surf = TFB_DrawCanvas_New_TrueColor (surf->w, surf->h, FALSE);
	opaque = SDL_MapRGB (new_surf->format, 255, 255, 255);

#else // indexed fonts
	// convert 32-bit png font to indexed
	new_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, surf->w, surf->h,
			8, 0, 0, 0, 0);
#endif

	dstfmt = new_surf->format;
	getpix = getpixel_for (surf);
	putpix = putpixel_for (new_surf);

	for (y = 0; y < surf->h; ++y)
	{
		for (x = 0; x < surf->w; ++x)
		{
			p = getpix (surf, x, y);
			SDL_GetRGBA (p, srcfmt, &r, &g, &b, &a);

#if defined(ALPHA_FONTS)
			if (!srcfmt->Amask)
			{	// produce alpha from intensity (Y component)
				// using a fast approximation
				// contributions to Y are: R=2, G=4, B=1
				a = (((int)r << 1) + ((int)g << 2) + b) / 7;
			}
			// normalize font pixel
			putpix (new_surf, x, y, SDL_MapRGBA (dstfmt,
					255, 255, 255, a));

#else // indexed/rgb colorkey fonts
			// normalize font pixel
			if (r == 0 && g == 0 && b == 0)
				putpix (new_surf, x, y, clear);
			else
				putpix (new_surf, x, y, opaque);
#endif
		}
	}

#if !defined(ALPHA_FONTS)
#if !defined(RGB_FONTS)	
	colors[0].r = 0;
	colors[0].g = 0;
	colors[0].b = 0;
	for (x = 1; x < 256; ++x)
	{
		colors[x].r = 255;
		colors[x].g = 255;
		colors[x].b = 255;
	}

	SDL_SetColors (new_surf, colors, 0, 256);
#endif /* !RGB_FONTS */
	SDL_SetColorKey (new_surf, SDL_SRCCOLORKEY, clear);
#endif /* !ALPHA_FONTS */

	SDL_UnlockSurface (surf);
	SDL_FreeSurface (surf);

	FramePtr->image = TFB_DrawImage_New (new_surf);
	surf = FramePtr->image->NormalImg;
	
	SetFrameBounds (FramePtr, surf->w + 1, surf->h + 1);
			// XXX: why the +1?
			// I brought it into this function from the only calling
			// function, but I don't know why it was there in the first
			// place.
			// XXX: the +1 appears to be for character and line spacing
			//  text_blt just adds the frame width to move to the next char
	
	{
		// This tunes the font positioning to be about what it should
		// TODO: prolly needs a little tweaking still

		int tune_amount = 0;

		if (surf->h == 8)
			tune_amount = -1;
		else if (surf->h == 9)
			tune_amount = -2;
		else if (surf->h > 9)
			tune_amount = -3;

		FramePtr->HotSpot = MAKE_HOT_SPOT (0, surf->h + tune_amount);
	}
}

// stretch_frame
// create a new frame of size neww x newh, and blit a scaled version FramePtr
// into it.
// destroy the old frame if 'destroy' is 1
FRAMEPTR stretch_frame (FRAMEPTR FramePtr, int neww, int newh, int destroy)
{
	FRAMEPTR NewFrame;
	CREATE_FLAGS flags;
	TFB_Image *tfbImg;
	TFB_Canvas src, dst;
	EXTENT ext;

	flags = TYPE_GET (GetFrameParentDrawable (FramePtr)
			->FlagsAndIndex) >> FTYPE_SHIFT;
	NewFrame = CaptureDrawable (
				CreateDrawable (flags, (SIZE)neww, (SIZE)newh, 1));
	tfbImg = FramePtr->image;
	LockMutex (tfbImg->mutex);
	src = tfbImg->NormalImg;
	dst = NewFrame->image->NormalImg;
	ext.width = neww;
	ext.height = newh;
	TFB_DrawCanvas_Rescale_Nearest (src, dst, ext);
	UnlockMutex (tfbImg->mutex);
	if (destroy)
		DestroyDrawable (ReleaseDrawable (FramePtr));
	return (NewFrame);
}

void process_rgb_bmp (FRAMEPTR FramePtr, Uint32 *rgba, int maxx, int maxy)
{
	int x, y;
	TFB_Image *tfbImg;
	SDL_Surface *img;
	PutPixelFn putpix;

	tfbImg = FramePtr->image;
	LockMutex (tfbImg->mutex);
	img = (SDL_Surface *)tfbImg->NormalImg;
	SDL_LockSurface (img);

	putpix = putpixel_for (img);

	for (y = 0; y < maxy; ++y)
		for (x = 0; x < maxx; ++x)
			putpix (img, x, y, *rgba++);
	SDL_UnlockSurface (img);
	UnlockMutex (tfbImg->mutex);
}

void fill_frame_rgb (FRAMEPTR FramePtr, Uint32 color, int x0, int y0,
		int x, int y)
{
	SDL_Surface *img;
	TFB_Image *tfbImg;
	SDL_Rect rect;

	tfbImg = FramePtr->image;
	LockMutex (tfbImg->mutex);
	img = (SDL_Surface *)tfbImg->NormalImg;
	SDL_LockSurface (img);
	if (x0 == 0 && y0 == 0 && x == 0 && y == 0)
		SDL_FillRect(img, NULL, color);
	else
	{
		rect.x = x0;
		rect.y = y0;
		rect.w = x - x0;
		rect.h = y - y0;
		SDL_FillRect(img, &rect, color);
	}
	SDL_UnlockSurface (img);
	UnlockMutex (tfbImg->mutex);
}

void arith_frame_blit (FRAMEPTR srcFrame, RECT *rsrc, FRAMEPTR dstFrame, RECT *rdst, int num,int denom)
{
	TFB_Image *srcImg, *dstImg;
	SDL_Surface *src, *dst;
	SDL_Rect srcRect, dstRect, *srp = NULL, *drp = NULL;
	srcImg = srcFrame->image;
	dstImg = dstFrame->image;
	LockMutex (srcImg->mutex);
	LockMutex (dstImg->mutex);
	src = (SDL_Surface *)srcImg->NormalImg;
	dst = (SDL_Surface *)dstImg->NormalImg;
	if (rdst)
	{
		dstRect.x = rdst->corner.x;
		dstRect.y = rdst->corner.y;
		dstRect.w = rdst->extent.width;
		dstRect.h = rdst->extent.height;
		drp = &dstRect;
	}
	if (rsrc)
	{
		srcRect.x = rsrc->corner.x;
		srcRect.y = rsrc->corner.y;
		srcRect.w = rsrc->extent.width;
		srcRect.h = rsrc->extent.height;
		srp = &srcRect;
	}
	else if (srcFrame->HotSpot.x || srcFrame->HotSpot.y)
	{
		if (rdst)
		{
			dstRect.x -= srcFrame->HotSpot.x;
			dstRect.y -= srcFrame->HotSpot.y;
		}
		else
		{
			dstRect.x = -srcFrame->HotSpot.x;
			dstRect.y = -srcFrame->HotSpot.y;
			dstRect.w = GetFrameWidth (srcFrame);
			dstRect.h = GetFrameHeight (srcFrame);
			drp =&dstRect;
		}

	}
	TFB_BlitSurface (src, srp, dst, drp, num, denom);
	UnlockMutex (srcImg->mutex);
	UnlockMutex (dstImg->mutex);
}

// Generate an array of all pixels in FramePtr
// The pixel format is :
//  bits 25-32 : red
//  bits 17-24 : green
//  bits 9-16  : blue
//  bits 1-8   : alpha
void getpixelarray(Uint32 *map, FRAMEPTR FramePtr, int width, int height)
{
	Uint8 r,g,b,a;
	Uint32 p, pos, row;
	TFB_Image *tfbImg;
	SDL_Surface *img;
	GetPixelFn getpix;
	int x,y;

	tfbImg = FramePtr->image;
	LockMutex (tfbImg->mutex);
	img = (SDL_Surface *)tfbImg->NormalImg;
	SDL_LockSurface (img);
	getpix = getpixel_for (img);
	for (y = 0, row = 0; y < height; y++, row += width)
	{
		if(y >= img->h)
			continue;
		for(x = 0, pos = row; x < img->w; x++, pos++)
		{
				p = getpix (img, x, y);
				SDL_GetRGBA (p,img->format, &r, &g, &b, &a);
				map[pos] = r << 24 | g << 16 | b << 8 | a;
		}
	}
	SDL_UnlockSurface (img);
	UnlockMutex (tfbImg->mutex);
}

FRAMEPTR Build_Font_Effect (FRAMEPTR FramePtr, Uint32 from, Uint32 to, BYTE type)
{
	FRAMEPTR NewFrame;
	int x, y;
	int width, height;
	Uint32 color, clear;
	Uint32 srcmask, threshold;
	BYTE from_r, from_g, from_b, from_a;
	BYTE to_r, to_g, to_b, to_a;
	TFB_Image *tfbImg, *tfbOrigImg;
	SDL_Surface *img, *OrigImg;
	PutPixelFn putpix;
	GetPixelFn getpix;

	tfbOrigImg = FramePtr->image;
	OrigImg = (SDL_Surface *)tfbOrigImg->NormalImg;
	SDL_LockSurface (OrigImg);

	width = OrigImg->w;
	height = OrigImg->h;
	NewFrame = CaptureDrawable (CreateDrawable ((CREATE_FLAGS)RAM_DRAWABLE, 
			(SIZE)width, (SIZE)height, 1));
	NewFrame->Bounds = FramePtr->Bounds;
	NewFrame->HotSpot = FramePtr->HotSpot;

	tfbImg = NewFrame->image;
	img = (SDL_Surface *)tfbImg->NormalImg;
	SDL_LockSurface (img);

	putpix = putpixel_for (img);
	getpix = getpixel_for (OrigImg);

	from_r = (from >> 24);
	from_g = (from >> 16) & 0xFF;
	from_b = (from >> 8) & 0xFF;
	from_a = from & 0xFF;

	to_r = (to >> 24);
	to_g = (to >> 16) & 0xFF;
	to_b = (to >> 8) & 0xFF;
	to_a = to & 0xFF;

	clear = SDL_MapRGBA (img->format, 0, 0, 0, 0);
	TFB_DrawCanvas_SetTransparentColor (img, 0, 0, 0, FALSE);
#if defined(ALPHA_FONTS)
	srcmask = OrigImg->format->Amask;
	threshold = (srcmask >> 1) & srcmask;
#else // indexed fonts
	srcmask = 255;
	threshold = 0; // any index != 0
#endif

	if (type == GRADIENT_EFFECT)
	{
		int clrstep_r, clrstep_g, clrstep_b, clrstep_a;
		
		clrstep_r = (to_r - from_r) / (height - 1);
		clrstep_g = (to_g - from_g) / (height - 1);
		clrstep_b = (to_b - from_b) / (height - 1);
		clrstep_a = (to_a - from_a) / (height - 1);

		for (y = 0; y < height; y++)
		{
			color = SDL_MapRGBA (img->format, from_r, from_g, from_b, from_a);
			from_r += clrstep_r;
			from_g += clrstep_g;
			from_b += clrstep_b;
			from_a += clrstep_a;
			for (x = 0; x < width; x++)
			{
				Uint32 p = getpix (OrigImg, x, y);

				if ((p & srcmask) > threshold)
					putpix (img, x, y, color);
				else
					putpix (img, x, y, clear);
			}
		}
	}
	else if (type == ALTERNATE_EFFECT)
	{
		Uint32 color1, color2;
		
		color1 = SDL_MapRGBA (img->format, from_r, from_g, from_b, from_a);
		color2 = SDL_MapRGBA (img->format, to_r, to_g, to_b, to_a);
		
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				Uint32 p = getpix (OrigImg, x, y);

				if ((p & srcmask) > threshold)
				{
					color = ((y & 0x01) ^ (x & 0x01)) ? color2 : color1;
					putpix (img, x, y, color);
				}
				else
					putpix (img, x, y, clear);
			}
		}
	}

	SDL_UnlockSurface (img);
	SDL_UnlockSurface (OrigImg);

	return (NewFrame);
}

// Generate a pixel (in the correct format to be applied to FramePtr) from the
// r,g,b,a values supplied
Uint32 frame_mapRGBA (FRAMEPTR FramePtr,Uint8 r, Uint8 g,  Uint8 b, Uint8 a)
{
	SDL_Surface *img= (SDL_Surface *)FramePtr->image->NormalImg;
	return (SDL_MapRGBA (img->format, r, g, b, a));
}

MEM_HANDLE
_GetCelData (uio_Stream *fp, DWORD length)
{
	int cel_ct, n;
	DWORD opos;
	char CurrentLine[1024], filename[1024];
#define MAX_CELS 256
	SDL_Surface *img[MAX_CELS];
	AniData ani[MAX_CELS];
	DRAWABLE Drawable;
	
	opos = uio_ftell (fp);

	{
		char *s1, *s2;

		if (_cur_resfile_name == 0
				|| (((s2 = 0), (s1 = strrchr (_cur_resfile_name, '/')) == 0)
						&& (s2 = strrchr (_cur_resfile_name, '\\')) == 0))
			n = 0;
		else
		{
			if (s2 > s1)
				s1 = s2;
			n = s1 - _cur_resfile_name + 1;
			strncpy (filename, _cur_resfile_name, n);
		}
	}

	cel_ct = 0;
	while (uio_fgets (CurrentLine, sizeof (CurrentLine), fp) && cel_ct < MAX_CELS)
	{
		sscanf (CurrentLine, "%s %d %d %d %d", &filename[n], 
			&ani[cel_ct].transparent_color, &ani[cel_ct].colormap_index, 
			&ani[cel_ct].hotspot_x, &ani[cel_ct].hotspot_y);
	
		img[cel_ct] = sdluio_loadImage (contentDir, filename);
		if (img[cel_ct] == NULL)
		{
			const char *err;

			err = SDL_GetError();
			fprintf (stderr, "_GetCelData: Unable to load image!\n");
			if (err != NULL)
				fprintf (stderr, "SDL reports: %s\n", err);
			SDL_FreeSurface (img[cel_ct]);
		}
		else if (img[cel_ct]->w < 0 || img[cel_ct]->h < 0 ||
				img[cel_ct]->format->BitsPerPixel < 8)
		{
			fprintf (stderr, "_GetCelData: Bad file!\n");
			SDL_FreeSurface (img[cel_ct]);
		}
		else
		{
			++cel_ct;
		}

		if ((int)uio_ftell (fp) - (int)opos >= (int)length)
			break;
	}

	Drawable = 0;
	if (cel_ct && (Drawable = AllocDrawable (cel_ct)))
	{
		DRAWABLEPTR DrawablePtr;

		if ((DrawablePtr = LockDrawable (Drawable)) == 0)
		{
			while (cel_ct--)
				SDL_FreeSurface (img[cel_ct]);

			mem_release ((MEM_HANDLE)Drawable);
			Drawable = 0;
		}
		else
		{
			FRAMEPTR FramePtr;

			DrawablePtr->hDrawable = GetDrawableHandle (Drawable);
			TYPE_SET (DrawablePtr->FlagsAndIndex,
					(DRAWABLE_TYPE)WANT_PIXMAP << FTYPE_SHIFT);
			INDEX_SET (DrawablePtr->FlagsAndIndex, cel_ct - 1);

			FramePtr = &DrawablePtr->Frame[cel_ct];
			while (--FramePtr, cel_ct--)
				process_image (FramePtr, img, ani, cel_ct);

			UnlockDrawable (Drawable);
		}
	}

	if (Drawable == 0)
		fprintf (stderr, "Couldn't get cel data for '%s'\n",
				_cur_resfile_name);
	return (GetDrawableHandle (Drawable));
}

BOOLEAN
_ReleaseCelData (MEM_HANDLE handle)
{
	DRAWABLEPTR DrawablePtr;
	int cel_ct;
	FRAMEPTR FramePtr = NULL;

	if ((DrawablePtr = LockDrawable (handle)) == 0)
		return (FALSE);

	cel_ct = INDEX_GET (DrawablePtr->FlagsAndIndex)+1;

	if (DrawablePtr->Frame)
	{
		FramePtr = DrawablePtr->Frame;
		if (TYPE_GET (FramePtr->TypeIndexAndFlags) == SCREEN_DRAWABLE)
		{
			FramePtr = NULL;
		}
	}

	UnlockDrawable (handle);
	if (mem_release (handle) && FramePtr)
	{
		int i;
		for (i = 0; i < cel_ct; i++)
		{
			TFB_Image *img = FramePtr[i].image;
			if (img)
			{
				FramePtr[i].image = NULL;
				TFB_DrawScreen_DeleteImage (img);
			}
		}
		HFree (FramePtr);
	}

	return (TRUE);
}

typedef struct BuildCharDesc {
	SDL_Surface *surface;
	wchar_t index;
} BuildCharDesc;

int
compareBCDIndex(const BuildCharDesc *bcd1, const BuildCharDesc *bcd2) {
	return (int) bcd1->index - (int) bcd2->index;
}

MEM_HANDLE
_GetFontData (uio_Stream *fp, DWORD length)
{
	COUNT numDirEntries;
	FONT_REF fontRef = 0;
	DIRENTRY fontDir = 0;
	BuildCharDesc *bcds = NULL;
	size_t numBCDs = 0;
	int dirEntryI;
	uio_DirHandle *fontDirHandle = NULL;
	FONTPTR fontPtr;

	if (_cur_resfile_name == 0)
		goto err;

	fontDir = CaptureDirEntryTable (LoadDirEntryTable (contentDir,
			_cur_resfile_name, ".", match_MATCH_SUBSTRING, &numDirEntries));
	if (fontDir == 0)
		goto err;

	fontDirHandle = uio_openDirRelative (contentDir, _cur_resfile_name, 0);
	if (fontDirHandle == NULL)
		goto err;
		
	bcds = HMalloc (numDirEntries * sizeof (BuildCharDesc));
	if (bcds == NULL)
		goto err;

	// Load the surfaces for all dir Entries
	for (dirEntryI = 0; dirEntryI < numDirEntries; dirEntryI++)
	{
		char *char_name;
		unsigned int charIndex;
		SDL_Surface *surface;

		char_name = GetDirEntryAddress (SetAbsDirEntryTableIndex (
				fontDir, dirEntryI));
		if (sscanf (char_name, "%u.", &charIndex) != 1)
			continue;
			
		if (charIndex > 0xffff)
			continue;

		surface = sdluio_loadImage (fontDirHandle, char_name);
		if (surface == NULL)
			continue;

		if (surface->w == 0 || surface->h == 0 ||
				surface->format->BitsPerPixel < 8) {
			SDL_FreeSurface (surface);
			continue;
		}

		bcds[numBCDs].surface = surface;
		bcds[numBCDs].index = charIndex;
		numBCDs++;
	}
	uio_closeDir (fontDirHandle);
	DestroyDirEntryTable (ReleaseDirEntryTable (fontDir));

#if 0
	if (numBCDs == 0)
		goto err;
#endif

	// sort on the character index
	qsort (bcds, numBCDs, sizeof (BuildCharDesc),
			(int (*)(const void *, const void *)) compareBCDIndex);

	fontRef = AllocFont (0);
	if (fontRef == 0)
		goto err;

	fontPtr = LockFont (fontRef);
	if (fontPtr == NULL)
		goto err;
	
	fontPtr->FontRef = fontRef;
	fontPtr->Leading = 0;

	{
		size_t startBCD = 0;
		int pageStart;
		FONT_PAGE **pageEndPtr = &fontPtr->fontPages;
		while (startBCD < numBCDs)
		{
			// Process one character page.
			size_t endBCD;
			pageStart = bcds[startBCD].index & CHARACTER_PAGE_MASK;

			endBCD = startBCD;
			while (endBCD < numBCDs &&
					(bcds[endBCD].index & CHARACTER_PAGE_MASK) == pageStart)
				endBCD++;

			{
				size_t bcdI;
				int numChars = bcds[endBCD - 1].index + 1
						- bcds[startBCD].index;
				FONT_PAGE *page = AllocFontPage (numChars);
				page->pageStart = pageStart;
				page->firstChar = bcds[startBCD].index;
				page->numChars = numChars;
				*pageEndPtr = page;
				pageEndPtr = &page->next;

				for (bcdI = startBCD; bcdI < endBCD; bcdI++)
				{
					// Process one character.
					BuildCharDesc *bcd = &bcds[bcdI];
					FRAME_DESC *destFrame =
							&page->charDesc[bcd->index - page->firstChar];
				
					if (destFrame->image != NULL)
					{
						// There's already an image for this character.
#ifdef DEBUG
						fprintf (stderr, "Duplicate image for character %d "
								"for font %s.\n", (int) bcd->index,
								_cur_resfile_name);
#endif
						SDL_FreeSurface (bcd->surface);
						continue;
					}
					
					processFontChar (destFrame, bcd->surface,
							bcd->index - page->firstChar);
					// bcd->surface is handed over to destFrame.
					// Don't access it through bcd->surface any more.
					// It may be freed from within ProcessFontChar().

					if (GetFrameHeight (destFrame) > fontPtr->Leading)
						fontPtr->Leading = GetFrameHeight (destFrame);
				}
			}

			startBCD = endBCD;
		}
		*pageEndPtr = NULL;
	}

	fontPtr->Leading++;
	UnlockFont (fontRef);

	HFree (bcds);

	(void) fp;  /* Satisfying compiler (unused parameter) */
	(void) length;  /* Satisfying compiler (unused parameter) */
	return fontRef;

err:
	if (fontRef != 0)
		mem_release (fontRef);
	
	if (bcds != NULL)
	{
		size_t bcdI;
		for (bcdI = 0; bcdI < numBCDs; bcdI++)
			SDL_FreeSurface (bcds[bcdI].surface);
		HFree (bcds);
	}
	
	if (fontDirHandle != NULL)
		uio_closeDir (fontDirHandle);
	
	if (fontDir != 0)
		DestroyDirEntryTable (ReleaseDirEntryTable (fontDir));
	return 0;
}

BOOLEAN
_ReleaseFontData (MEM_HANDLE handle)
{
	FONTPTR font = LockFont (handle);
	if (font == NULL)
		return FALSE;

	{
		FONT_PAGE *page;
		FONT_PAGE *nextPage;

		for (page = font->fontPages; page != NULL; page = nextPage)
		{
			size_t charI;
			for (charI = 0; charI < page->numChars; charI++)
			{
				FRAME_DESC *frame = &page->charDesc[charI];
				
				if (frame->image == NULL)
					continue;
				
				TFB_DrawScreen_DeleteImage (frame->image);
			}
		
			nextPage = page->next;
			FreeFontPage (page);
		}
	}

	UnlockFont (handle);
	mem_release (handle);

	return TRUE;
}

// _request_drawable was in libs/graphics/drawable.c before modularization

DRAWABLE
_request_drawable (COUNT NumFrames, DRAWABLE_TYPE DrawableType,
		CREATE_FLAGS flags, SIZE width, SIZE height)
{
	DRAWABLE Drawable;

	Drawable = AllocDrawableImage (
			NumFrames, DrawableType, flags, width, height
			);
	if (Drawable)
	{
		DRAWABLEPTR DrawablePtr;

		if ((DrawablePtr = LockDrawable (Drawable)) == 0)
		{
			FreeDrawable (Drawable);
			Drawable = 0;
		}
		else
		{
			int imgw, imgh;
			FRAMEPTR FramePtr;

			DrawablePtr->hDrawable = GetDrawableHandle (Drawable);
			TYPE_SET (DrawablePtr->FlagsAndIndex, flags << FTYPE_SHIFT);
			INDEX_SET (DrawablePtr->FlagsAndIndex, NumFrames - 1);

			imgw = width;
			imgh = height;
			
			FramePtr = &DrawablePtr->Frame[NumFrames - 1];
			while (NumFrames--)
			{
				TFB_Image *Image;

				if (DrawableType == RAM_DRAWABLE && imgw > 0 && imgh > 0
						&& (Image = TFB_DrawImage_New (TFB_DrawCanvas_New_TrueColor (
							imgw, imgh, (flags & WANT_ALPHA) ? TRUE : FALSE))))
				{
					FramePtr->image = Image;
				}

				TYPE_SET (FramePtr->TypeIndexAndFlags, DrawableType);
				INDEX_SET (FramePtr->TypeIndexAndFlags, NumFrames);
				SetFrameBounds (FramePtr, width, height);
				--FramePtr;
			}
			UnlockDrawable (Drawable);
		}
	}

	return (Drawable);
}

#endif
