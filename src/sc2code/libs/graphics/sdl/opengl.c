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

#if defined (GFXMODULE_SDL) && defined (HAVE_OPENGL)

#include "libs/graphics/sdl/opengl.h"

static SDL_Surface *format_conv_surf;

static int ScreenWidthOpenGL;
static int ScreenHeightOpenGL;
static int ScreenFilterMode;
static unsigned int DisplayTexture;


int
TFB_GL_InitGraphics (int driver, int flags, int width, int height, int bpp)
{
	char VideoName[256];
	int videomode_flags;

	GraphicsDriver = driver;

	printf("Initializing SDL (OpenGL).\n");

	if ((SDL_Init (SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) == -1))
	{
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		exit(-1);
	}

	SDL_VideoDriverName (VideoName, sizeof (VideoName));
	printf("SDL driver used: %s\n", VideoName);
	atexit (SDL_Quit);

	printf ("SDL initialized.\n");
	printf ("Initializing Screen.\n");

	ScreenWidth = 320;
	ScreenHeight = 240;
	ScreenWidthActual = ScreenWidth;
	ScreenHeightActual = ScreenHeight;

	ScreenWidthOpenGL = width;
	ScreenHeightOpenGL = height;

	switch(bpp) {
		case 8:
			printf("bpp of 8 not supported under OpenGL\n");
			exit(-1);

		case 15:
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
			break;

		case 16:
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
			break;

		case 24:
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			break;

		case 32:
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			break;
	}

	SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, bpp);
	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

	videomode_flags = SDL_OPENGL;
	if (flags & TFB_GFXFLAGS_FULLSCREEN)
		videomode_flags |= SDL_FULLSCREEN;

	SDL_Video = SDL_SetVideoMode (ScreenWidthOpenGL, ScreenHeightOpenGL, 
		bpp, videomode_flags);

	if (SDL_Video == NULL)
	{
		printf ("Couldn't set OpenGL %ix%ix%i video mode: %s\n",
			ScreenWidthOpenGL, ScreenHeightOpenGL, bpp,
			SDL_GetError ());
		exit(-1);
	}
	else
	{
		printf ("Set the resolution to: %ix%ix%i\n",
			SDL_GetVideoSurface()->w, SDL_GetVideoSurface()->h,
			SDL_GetVideoSurface()->format->BitsPerPixel);

		printf("OpenGL renderer: %s version: %s\n", glGetString(GL_RENDERER), 
			glGetString(GL_VERSION));
	}

	SDL_Screen = SDL_CreateRGBSurface(SDL_SWSURFACE, ScreenWidthActual, ScreenHeightActual, 32,
		0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000);

	if (SDL_Screen == NULL)
	{
		printf("Couldn't create back buffer: %s\n",
			SDL_GetError());
		exit(-1);
	}

	ExtraScreen = SDL_CreateRGBSurface(SDL_SWSURFACE, ScreenWidthActual, ScreenHeightActual, 32,
		0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000);

	if (ExtraScreen == NULL)
	{
		printf("Couldn't create workspace buffer: %s\n",
			SDL_GetError());
		exit(-1);
	}

	format_conv_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, 0, 0, 32,
		0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

	if (format_conv_surf == NULL)
	{
		printf("Couldn't create format_conv_surf: %s\n",
			SDL_GetError());
		exit(-1);
	}

	if (flags & TFB_GFXFLAGS_BILINEAR_FILTERING)
		ScreenFilterMode = GL_LINEAR;
	else
		ScreenFilterMode = GL_NEAREST;

	glViewport (0, 0, ScreenWidthOpenGL, ScreenHeightOpenGL);
	glClearColor (0,0,0,0);
	glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapBuffers ();
	glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glDisable (GL_DITHER);
	glDepthMask(GL_FALSE);

	glGenTextures(1,&DisplayTexture);
	glBindTexture(GL_TEXTURE_2D, DisplayTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 512, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	return 0;
}

void 
TFB_GL_SwapBuffers ()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho (0,ScreenWidthOpenGL,ScreenHeightOpenGL, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
		
	glBindTexture(GL_TEXTURE_2D, DisplayTexture);

	SDL_LockSurface(SDL_Screen);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ScreenWidth,ScreenHeight, GL_RGBA, GL_UNSIGNED_BYTE, SDL_Screen->pixels);
	SDL_UnlockSurface(SDL_Screen);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ScreenFilterMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ScreenFilterMode);

	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);
	
	glBegin(GL_TRIANGLE_FAN);

	glTexCoord2f(0, 0);
	glVertex2i(0, 0);

	glTexCoord2f(ScreenWidth / 512.0f, 0);
	glVertex2i(ScreenWidthOpenGL, 0);
	
	glTexCoord2f(ScreenWidth / 512.0f, ScreenHeight / 256.0f);
	glVertex2i(ScreenWidthOpenGL, ScreenHeightOpenGL);

	glTexCoord2f(0, ScreenHeight / 256.0f);
	glVertex2i(0, ScreenHeightOpenGL);

	glEnd();

	SDL_GL_SwapBuffers();
}

SDL_Surface* 
TFB_GL_DisplayFormatAlpha (SDL_Surface *surface)
{
	return SDL_ConvertSurface (surface, format_conv_surf->format, surface->flags);
}

#endif
