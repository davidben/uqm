#include "gfx_common.h"
#include "tfb_draw.h"
#include "drawcmd.h"

void
TFB_DrawScreen_Line (int x1, int y1, int x2, int y2, int r, int g, int b, SCREEN dest)
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

/* This value is protected by the DCQ's lock. */
static int _localpal[256][3];

void
TFB_FlushPaletteCache ()
{
	int i;
	Lock_DCQ (-1);
	for (i = 0; i < 256; i++)
	{
		_localpal[i][0] = -1;
		_localpal[i][1] = -1;
		_localpal[i][2] = -1;
	}
	Unlock_DCQ ();
}

void
TFB_DrawScreen_Image (TFB_Image *img, int x, int y, int scale, TFB_Palette *palette, SCREEN dest)
{
	TFB_DrawCommand DC;
	
	DC.Type = TFB_DRAWCOMMANDTYPE_IMAGE;
	DC.data.image.image = img;
	DC.data.image.x = x;
	DC.data.image.y = y;
	DC.data.image.scale = (scale == GetGraphicScaleIdentity ()) ? 0 : scale;

	if (palette != NULL)
	{
		int i, changed;
		Lock_DCQ(257);
		changed = 0;
		for (i = 0; i < 256; i++)
		{
			if ((_localpal[i][0] != palette[i].r) ||
			    (_localpal[i][1] != palette[i].g) ||
			    (_localpal[i][2] != palette[i].b))
			{
				changed++;
				_localpal[i][0] = palette[i].r;
				_localpal[i][1] = palette[i].g;
				_localpal[i][2] = palette[i].b;
				TFB_DrawScreen_SetPalette (i, palette[i].r, palette[i].g, 
						palette[i].b);
			}
		}
		// if (changed) { fprintf (stderr, "Actually changing palette! "); }
		DC.data.image.UsePalette = TRUE;
	} 
	else
	{
		Lock_DCQ (1);
		DC.data.image.UsePalette = FALSE;
	}

	DC.data.image.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
	Unlock_DCQ ();
}

void
TFB_DrawScreen_FilledImage (TFB_Image *img, int x, int y, int scale, int r, int g, int b, SCREEN dest)
{
	TFB_DrawCommand DC;
	
	DC.Type = TFB_DRAWCOMMANDTYPE_FILLEDIMAGE;
	DC.data.filledimage.image = img;
	DC.data.filledimage.x = x;
	DC.data.filledimage.y = y;
	DC.data.filledimage.scale = (scale == GetGraphicScaleIdentity ()) ? 0 : scale;
	DC.data.filledimage.r = r;
	DC.data.filledimage.g = g;
	DC.data.filledimage.b = b;
	DC.data.filledimage.destBuffer = dest;

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
TFB_DrawScreen_WaitForSignal (void)
{
	TFB_DrawCommand DrawCommand;
	int channel;
	DrawCommand.Type = TFB_DRAWCOMMANDTYPE_SENDSIGNAL;
	Lock_DCQ (1);
	channel = FindSignalChannel ();
	TFB_BatchReset ();
	TFB_EnqueueDrawCommand(&DrawCommand);
	Unlock_DCQ();
	WaitForSignal (channel);
}

void
TFB_DrawImage_Line (int x1, int y1, int x2, int y2, int r, int g, int b, TFB_Image *dest)
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
TFB_DrawImage_Image (TFB_Image *img, int x, int y, int scale, TFB_Palette *palette, TFB_Image *target)
{
	LockMutex (target->mutex);
	TFB_DrawCanvas_Image (img, x, y, scale, palette, target->NormalImg);
	target->dirty = TRUE;
	UnlockMutex (target->mutex);
}

void
TFB_DrawImage_FilledImage (TFB_Image *img, int x, int y, int scale, int r, int g, int b, TFB_Image *target)
{
	LockMutex (target->mutex);
	TFB_DrawCanvas_FilledImage (img, x, y, scale, r, g, b, target->NormalImg);
	target->dirty = TRUE;
	UnlockMutex (target->mutex);
}

TFB_Image *
TFB_DrawImage_New (TFB_Canvas canvas)
{
	TFB_Image *img = HMalloc (sizeof (TFB_Image));
	img->mutex = CreateMutex ();
	img->ScaledImg = NULL;
	img->colormap_index = -1;

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

void 
TFB_DrawImage_Delete (TFB_Image *image)
{
	if (image == 0)
	{
		fprintf (stderr, "INTERNAL ERROR: Tried to delete a null image!\n");
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
TFB_DrawImage_FixScaling (TFB_Image *image, int target)
{
	if (image->ScaledImg)
	{
		if (image->scale != target)
		{
			TFB_DrawCanvas_Delete(image->ScaledImg);
			image->ScaledImg = NULL;
		}
	}
	
	if (!image->ScaledImg)
	{
		image->scale = target;
		image->dirty = FALSE;
		image->ScaledImg = TFB_DrawCanvas_New_Scaled (image->NormalImg, target);
		if (TFB_DrawCanvas_IsPaletted (image->ScaledImg))
		{
			int colorkey = TFB_DrawCanvas_GetTransparentIndex (image->NormalImg);

			TFB_DrawCanvas_SetPalette (image->ScaledImg, image->Palette);
			if (colorkey != -1)
			{
				TFB_DrawCanvas_SetTransparentIndex (image->ScaledImg, colorkey);
			}
		}
	}
}
