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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "gfx_common.h"
#include "tfb_draw.h"
#include "drawcmd.h"
#include "units.h"
#include "libs/log.h"

static const HOT_SPOT NullHs = {0, 0};

void
TFB_DrawScreen_Line (int x1, int y1, int x2, int y2, int r, int g, int b,
		SCREEN dest)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_LINE;
	DC.data.line.x1 = x1;
	DC.data.line.y1 = y1;
	DC.data.line.x2 = x2;
	DC.data.line.y2 = y2;
	DC.data.line.r = r;
	DC.data.line.g = g;
	DC.data.line.b = b;
	DC.data.line.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_Rect (PRECT rect, int r, int g, int b, SCREEN dest)
{
	RECT locRect;
	TFB_DrawCommand DC;

	if (!rect)
	{
		locRect.corner.x = locRect.corner.y = 0;
		locRect.extent.width = SCREEN_WIDTH;
		locRect.extent.height = SCREEN_HEIGHT;
		rect = &locRect;
	}

	DC.Type = TFB_DRAWCOMMANDTYPE_RECTANGLE;
	DC.data.rect.rect = *rect;
	DC.data.rect.r = r;
	DC.data.rect.g = g;
	DC.data.rect.b = b;
	DC.data.rect.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_SetPalette (int paletteIndex, int r, int g, int b)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_SETPALETTE;
	DC.data.setpalette.r = r;
	DC.data.setpalette.g = g;
	DC.data.setpalette.b = b;
	DC.data.setpalette.index = paletteIndex;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_Image (TFB_Image *img, int x, int y, int scale,
		TFB_ColorMap *cmap, SCREEN dest)
{
	TFB_DrawCommand DC;
	
	DC.Type = TFB_DRAWCOMMANDTYPE_IMAGE;
	DC.data.image.image = img;
	DC.data.image.colormap = cmap;
	DC.data.image.x = x;
	DC.data.image.y = y;
	DC.data.image.scale = (scale == GSCALE_IDENTITY) ? 0 : scale;
	DC.data.image.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_FilledImage (TFB_Image *img, int x, int y, int scale,
		int r, int g, int b, SCREEN dest)
{
	TFB_DrawCommand DC;
	
	DC.Type = TFB_DRAWCOMMANDTYPE_FILLEDIMAGE;
	DC.data.filledimage.image = img;
	DC.data.filledimage.x = x;
	DC.data.filledimage.y = y;
	DC.data.filledimage.scale = (scale == GSCALE_IDENTITY) ? 0 : scale;
	DC.data.filledimage.r = r;
	DC.data.filledimage.g = g;
	DC.data.filledimage.b = b;
	DC.data.filledimage.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_FontChar (TFB_Char *fontChar, TFB_Image *backing,
		int x, int y, SCREEN dest)
{
	TFB_DrawCommand DC;
	
	DC.Type = TFB_DRAWCOMMANDTYPE_FONTCHAR;
	DC.data.fontchar.fontchar = fontChar;
	DC.data.fontchar.backing = backing;
	DC.data.fontchar.x = x;
	DC.data.fontchar.y = y;
	DC.data.fontchar.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_CopyToImage (TFB_Image *img, PRECT lpRect, SCREEN src)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_COPYTOIMAGE;
	DC.data.copytoimage.x = lpRect->corner.x;
	DC.data.copytoimage.y = lpRect->corner.y;
	DC.data.copytoimage.w = lpRect->extent.width;
	DC.data.copytoimage.h = lpRect->extent.height;
	DC.data.copytoimage.image = img;
	DC.data.copytoimage.srcBuffer = src;
	
	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_Copy (PRECT r, SCREEN src, SCREEN dest)
{
	RECT locRect;
	TFB_DrawCommand DC;

	if (!r)
	{
		locRect.corner.x = locRect.corner.y = 0;
		locRect.extent.width = SCREEN_WIDTH;
		locRect.extent.height = SCREEN_HEIGHT;
		r = &locRect;
	}

	DC.Type = TFB_DRAWCOMMANDTYPE_COPY;
	DC.data.copy.x = r->corner.x;
	DC.data.copy.y = r->corner.y;
	DC.data.copy.w = r->extent.width;
	DC.data.copy.h = r->extent.height;
	DC.data.copy.srcBuffer = src;
	DC.data.copy.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_DeleteImage (TFB_Image *img)
{
	if (img)
	{
		TFB_DrawCommand DC;

		DC.Type = TFB_DRAWCOMMANDTYPE_DELETEIMAGE;
		DC.data.deleteimage.image = img;

		TFB_EnqueueDrawCommand (&DC);
	}
}

void
TFB_DrawScreen_DeleteData (void *data)
		// data must be a result of HXalloc() call
{
	if (data)
	{
		TFB_DrawCommand DC;

		DC.Type = TFB_DRAWCOMMANDTYPE_DELETEDATA;
		DC.data.deletedata.data = data;

		TFB_EnqueueDrawCommand (&DC);
	}
}

void
TFB_DrawScreen_WaitForSignal (void)
{
	TFB_DrawCommand DrawCommand;
	Semaphore s;
	s = GetMyThreadLocal ()->flushSem;
	DrawCommand.Type = TFB_DRAWCOMMANDTYPE_SENDSIGNAL;
	DrawCommand.data.sendsignal.sem = s;
	Lock_DCQ (1);
	TFB_BatchReset ();
	TFB_EnqueueDrawCommand(&DrawCommand);
	Unlock_DCQ();
	SetSemaphore (s);	
}

void
TFB_DrawScreen_ReinitVideo (int driver, int flags, int width, int height)
{
	TFB_DrawCommand DrawCommand;
	DrawCommand.Type = TFB_DRAWCOMMANDTYPE_REINITVIDEO;
	DrawCommand.data.reinitvideo.driver = driver;
	DrawCommand.data.reinitvideo.flags = flags;
	DrawCommand.data.reinitvideo.width = width;
	DrawCommand.data.reinitvideo.height = height;
	TFB_EnqueueDrawCommand(&DrawCommand);
}

void
TFB_DrawImage_Line (int x1, int y1, int x2, int y2, int r, int g, int b,
		TFB_Image *dest)
{
	LockMutex (dest->mutex);
	TFB_DrawCanvas_Line (x1, y1, x2, y2, r, g, b, dest->NormalImg);
	dest->dirty = TRUE;
	UnlockMutex (dest->mutex);
}

void
TFB_DrawImage_Rect (PRECT rect, int r, int g, int b, TFB_Image *image)
{
	LockMutex (image->mutex);
	TFB_DrawCanvas_Rect (rect, r, g, b, image->NormalImg);
	image->dirty = TRUE;
	UnlockMutex (image->mutex);
}

void
TFB_DrawImage_Image (TFB_Image *img, int x, int y, int scale,
		TFB_ColorMap *cmap, TFB_Image *target)
{
	LockMutex (target->mutex);
	TFB_DrawCanvas_Image (img, x, y, scale, cmap, target->NormalImg);
	target->dirty = TRUE;
	UnlockMutex (target->mutex);
}

void
TFB_DrawImage_FilledImage (TFB_Image *img, int x, int y, int scale,
		int r, int g, int b, TFB_Image *target)
{
	LockMutex (target->mutex);
	TFB_DrawCanvas_FilledImage (img, x, y, scale, r, g, b, target->NormalImg);
	target->dirty = TRUE;
	UnlockMutex (target->mutex);
}

void
TFB_DrawImage_FontChar (TFB_Char *fontChar, TFB_Image *backing,
		int x, int y, TFB_Image *target)
{
	LockMutex (target->mutex);
	TFB_DrawCanvas_FontChar (fontChar, backing, x, y, target->NormalImg);
	target->dirty = TRUE;
	UnlockMutex (target->mutex);
}


TFB_Image *
TFB_DrawImage_New (TFB_Canvas canvas)
{
	TFB_Image *img = HMalloc (sizeof (TFB_Image));
	img->mutex = CreateMutex ("image lock", SYNC_CLASS_VIDEO);
	img->ScaledImg = NULL;
	img->MipmapImg = NULL;
	img->FilledImg = NULL;
	img->colormap_index = -1;
	img->colormap_version = 0;
	img->NormalHs = NullHs;
	img->MipmapHs = NullHs;
	img->last_scale_hs = NullHs;
	img->last_scale_type = -1;
	TFB_DrawCanvas_GetExtent (canvas, &img->extent);

	img->Palette = TFB_DrawCanvas_ExtractPalette (canvas);
	
	if (img->Palette)
	{
		img->NormalImg = canvas;
	}
	else
	{
		img->NormalImg = TFB_DrawCanvas_ToScreenFormat (canvas);
	}

	return img;
}

TFB_Image*
TFB_DrawImage_CreateForScreen (int w, int h, BOOLEAN withalpha)
{
	TFB_Image* img = HMalloc (sizeof (TFB_Image));
	img->mutex = CreateMutex ("image lock", SYNC_CLASS_VIDEO);
	img->ScaledImg = NULL;
	img->MipmapImg = NULL;
	img->FilledImg = NULL;
	img->colormap_index = -1;
	img->colormap_version = 0;
	img->NormalHs = NullHs;
	img->MipmapHs = NullHs;
	img->last_scale_hs = NullHs;
	img->last_scale_type = -1;
	img->Palette = NULL;
	img->extent.width = w;
	img->extent.height = h;

	img->NormalImg = TFB_DrawCanvas_New_ForScreen (w, h, withalpha);

	return img;
}

TFB_Image *
TFB_DrawImage_New_Rotated (TFB_Image *img, int angle)
{
	TFB_Canvas dst;
	EXTENT size;
	TFB_Image* newimg;

	/* sanity check */
	if (!img->NormalImg)
	{
		log_add (log_Warning, "TFB_DrawImage_New_Rotated: "
				"source canvas is NULL! Failing.");
		return NULL;
	}

	TFB_DrawCanvas_GetRotatedExtent (img->NormalImg, angle, &size);
	dst = TFB_DrawCanvas_New_RotationTarget (img->NormalImg, angle);
	if (!dst)
	{
		log_add (log_Warning, "TFB_DrawImage_New_Rotated: "
				"rotation target canvas not created! Failing.");
		return NULL;
	}
	TFB_DrawCanvas_Rotate (img->NormalImg, dst, angle, size);
	
	newimg = TFB_DrawImage_New (dst);
	return newimg;
}

void 
TFB_DrawImage_Delete (TFB_Image *image)
{
	if (image == 0)
	{
		log_add (log_Warning, "INTERNAL ERROR: Tried to delete a null image!");
		/* Should we die here? */
		return;
	}
	LockMutex (image->mutex);

	TFB_DrawCanvas_Delete (image->NormalImg);
			
	if (image->ScaledImg) {
		TFB_DrawCanvas_Delete (image->ScaledImg);
	}

	if (image->Palette)
		HFree (image->Palette);

	UnlockMutex (image->mutex);
	DestroyMutex (image->mutex);
			
	HFree (image);
}

void
TFB_DrawImage_FixScaling (TFB_Image *image, int target, int type)
{
	EXTENT old = image->extent;
	HOT_SPOT oldhs = image->last_scale_hs;
	TFB_DrawCanvas_GetScaledExtent (image->NormalImg, image->NormalHs,
			image->MipmapImg, image->MipmapHs, target,
			&image->extent, &image->last_scale_hs);

	if ((old.width != image->extent.width) ||
			(old.height != image->extent.height) || image->dirty ||
			!image->ScaledImg || type != image->last_scale_type ||
			(oldhs.x != image->last_scale_hs.x) ||
			(oldhs.y != image->last_scale_hs.y))
	{
		image->dirty = FALSE;
		image->ScaledImg = TFB_DrawCanvas_New_ScaleTarget (image->NormalImg,
			image->ScaledImg, type, image->last_scale_type);
		image->last_scale_type = type;
		
		if (type == TFB_SCALE_NEAREST)
			TFB_DrawCanvas_Rescale_Nearest (image->NormalImg,
					image->ScaledImg, image->extent);
		else
			TFB_DrawCanvas_Rescale_Trilinear (image->NormalImg,
					image->ScaledImg, image->MipmapImg, image->extent);
	}
}
