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



/****************************************

  Star Control II: 3DO => SDL Port

  Copyright Toys for Bob, 2002

  starcon2.cpp programmer: Chris Nelson

*****************************************/

#include "SDL_wrapper.h"

//Global Variables for the wrapper.

Uint8 KeyboardDown[512];
#define KBDBUFSIZE (1 << 8)
static int kbdhead, kbdtail;
static Uint16 kbdbuf[KBDBUFSIZE];

UBYTE ControlA;
UBYTE ControlB;
UBYTE ControlC;
UBYTE ControlX;
UBYTE ControlStart;
UBYTE ControlLeftShift;
UBYTE ControlRightShift;

int ScreenWidth;
int ScreenHeight;
int ScreenWidthActual;
int ScreenHeightActual;
int ScreenBPPActual;

SDL_Surface *ExtraScreen;

int TFB_DEBUG_HALT;

Uint16
GetUNICODEKey ()
{
	if (kbdtail != kbdhead)
	{
		Uint16 ch;

		ch = kbdbuf[kbdhead];
		kbdhead = (kbdhead + 1) & (KBDBUFSIZE - 1);
		return (ch);
	}

	return (0);
}

void
FlushInput ()
{
	kbdtail = kbdhead = 0;
}

void
ProcessKeyboardEvent(const SDL_Event *Event)
{
	if(Event->key.keysym.sym == SDLK_TAB && Event->type == SDL_KEYDOWN)
	{
		SDL_GL_SwapBuffers(); //Debug: Hit tab to swap buffers
	}
	else if(Event->key.keysym.sym == SDLK_BACKQUOTE)
	{
		exit(0);
	}
	else//if(Event->type == SDL_KEYDOWN)
	{
		if (Event->type == SDL_KEYDOWN)
		{
			if ((kbdbuf[kbdtail] = Event->key.keysym.unicode)
					|| (kbdbuf[kbdtail] = Event->key.keysym.sym) <= 0x7F)
			{
				kbdtail = (kbdtail + 1) & (KBDBUFSIZE - 1);
				if (kbdtail == kbdhead)
					kbdhead = (kbdhead + 1) & (KBDBUFSIZE - 1);
			}
			KeyboardDown[Event->key.keysym.sym]=TRUE;
		}
		else
		{
			KeyboardDown[Event->key.keysym.sym]=FALSE;
		}
	}

	if (Event->key.keysym.sym == SDLK_BACKSPACE && Event->type == SDL_KEYDOWN)
	{
		//Stop + Start Starcon running.

		//TFB_DEBUG_HALT = !TFB_DEBUG_HALT;
	}

	if (Event->type == SDL_KEYDOWN && TFB_DEBUG_HALT)
	{
		//Used for debugging Buffer copy operations
				
		if (Event->key.keysym.sym==SDLK_d)
		{
			//Draw

			glMatrixMode (GL_PROJECTION);
			glPushMatrix ();

			//

			glDrawBuffer (GL_BACK_LEFT);
			glLoadIdentity ();
			glOrtho (0, ScreenWidth, ScreenHeight, 0, -1, 1);
			glMatrixMode (GL_MODELVIEW);
			glLoadIdentity ();
			glDisable (GL_SCISSOR_TEST);

			glBegin (GL_QUADS);
			{
				glColor3f (rand() % 2, rand () % 2, rand () % 2);
				glVertex2i (0, ScreenHeight);
				//glColor3f (rand() % 2, rand() % 2, rand() % 2);
				glVertex2i(0, 0);
				//glColor3f (rand() % 2, rand () % 2, rand () % 2);
				glVertex2i (ScreenWidth, 0);
				glColor3f (rand() % 2, rand () % 2, rand () % 2);
				glVertex2i(ScreenWidth, ScreenHeight);
			}
			glEnd();

			//

			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
		}
		if (Event->key.keysym.sym == SDLK_s)
		{
			SDL_GL_SwapBuffers();
		}
		if (Event->key.keysym.sym == SDLK_c)
		{
			//Copy from front to back
			glReadBuffer(GL_FRONT);
			glDrawBuffer(GL_BACK);

			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();

			glOrtho (0, ScreenWidthActual, ScreenHeightActual, 0, -1, 1);
			glMatrixMode (GL_MODELVIEW);
			glLoadIdentity ();
			glRasterPos2d (0, ScreenHeightActual - .1);
					//Re "-.1": All values 0 < x < .5 work. Strange!!

			glDisable (GL_SCISSOR_TEST);
			glCopyPixels (0, 0, ScreenWidthActual, ScreenHeightActual,
					GL_COLOR);

			glPopMatrix ();
			glMatrixMode (GL_MODELVIEW);
		}
	}
}

void ProcessJoystickEvent (const SDL_Event* Event)
{
	SDL_Event PseudoEvent;

	return;

	if (Event->type == SDL_JOYAXISMOTION)
	{
		if (Event->jaxis.axis == 0)
		{
			//x-axis

			if (Event->jaxis.value <= -5) //Left
			{
				PseudoEvent.type = SDL_KEYDOWN;
				PseudoEvent.key.keysym.sym = SDLK_LEFT;
				ProcessKeyboardEvent (&PseudoEvent);

				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_RIGHT;
				ProcessKeyboardEvent (&PseudoEvent);
			}
			if (Event->jaxis.value >= 5) //Right
			{
				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_LEFT;
				ProcessKeyboardEvent (&PseudoEvent);

				PseudoEvent.type = SDL_KEYDOWN;
				PseudoEvent.key.keysym.sym = SDLK_RIGHT;
				ProcessKeyboardEvent (&PseudoEvent);
			}
			if (Event->jaxis.value > -5 && Event->jaxis.value < 5)
					//Neither Left nor Right
			{
				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_LEFT;
				ProcessKeyboardEvent (&PseudoEvent);

				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_RIGHT;
				ProcessKeyboardEvent (&PseudoEvent);
			}
		}
		if (Event->jaxis.axis == 1)
		{
			//y-axis

			if (Event->jaxis.value <= -5) //Down
			{
				PseudoEvent.type = SDL_KEYDOWN;
				PseudoEvent.key.keysym.sym = SDLK_DOWN;
				ProcessKeyboardEvent (&PseudoEvent);

				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_UP;
				ProcessKeyboardEvent (&PseudoEvent);
			}
			if (Event->jaxis.value >= 5) //Up
			{
				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_DOWN;
				ProcessKeyboardEvent (&PseudoEvent);

				PseudoEvent.type = SDL_KEYDOWN;
				PseudoEvent.key.keysym.sym = SDLK_UP;
				ProcessKeyboardEvent (&PseudoEvent);
			}
			if (Event->jaxis.value >-5 && Event->jaxis.value < 5)
					//Neither Down nor Up
			{
				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_DOWN;
				ProcessKeyboardEvent (&PseudoEvent);

				PseudoEvent.type = SDL_KEYUP;
				PseudoEvent.key.keysym.sym = SDLK_UP;
				ProcessKeyboardEvent (&PseudoEvent);
			}
		}
	}
	if (Event->type == SDL_JOYBUTTONDOWN)
	{
		printf("Joystick %i down\n", Event->jbutton.button);

		if (Event->jbutton.button % 3 == 0)
		{
			//Mimic the Primary key

			PseudoEvent.type=SDL_KEYDOWN;
			PseudoEvent.key.keysym.sym = ControlA;
			ProcessKeyboardEvent (&PseudoEvent);
		}
		if (Event->jbutton.button % 3 == 1)
		{
			//Mimic the Secondary key

			PseudoEvent.type = SDL_KEYDOWN;
			PseudoEvent.key.keysym.sym = ControlB;
			ProcessKeyboardEvent (&PseudoEvent);
		}
		if (Event->jbutton.button % 3 == 2)
		{
			//Mimic the Tertiary key

			PseudoEvent.type = SDL_KEYDOWN;
			PseudoEvent.key.keysym.sym = ControlC;
			ProcessKeyboardEvent (&PseudoEvent);
		}
	}
	if (Event->type == SDL_JOYBUTTONUP)
	{
		if (Event->jbutton.button % 3 == 0)
		{
			//Mimic the Primary key

			PseudoEvent.type = SDL_KEYUP;
			PseudoEvent.key.keysym.sym = ControlA;
			ProcessKeyboardEvent (&PseudoEvent);
		}
		if (Event->jbutton.button % 3 == 1)
		{
			//Mimic the Secondary key

			PseudoEvent.type = SDL_KEYUP;
			PseudoEvent.key.keysym.sym = ControlB;
			ProcessKeyboardEvent (&PseudoEvent);
		}
		if (Event->jbutton.button % 3 == 2)
		{
			//Mimic the Tertiary key

			PseudoEvent.type = SDL_KEYUP;
			PseudoEvent.key.keysym.sym = ControlC;
			ProcessKeyboardEvent (&PseudoEvent);
		}
	}
}


int
main(int argc, char *argv[])
{
	SDL_Surface* Screen;
	SDL_Joystick *Joystick1;
	SDL_Event Event;
	SDL_version compile_version;

	char SoundcardName[256];
	char VideoName[256];
	int audio_rate,audio_channels;
	Uint16 audio_format;

	int a,i;

	//FILE* Debug;
	//Debug = fopen ("Debug.txt","w"); _dup2(fileno(Debug),1);

	//Begin System Initialization

	printf("Initializing SDL.\n");

	if ((SDL_Init (SDL_INIT_VIDEO
			| SDL_INIT_AUDIO
			| SDL_INIT_JOYSTICK
			| SDL_INIT_NOPARACHUTE //For Debugging in MSVC++
			) == -1))
	{
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		exit(-1);
	}
	SDL_VideoDriverName (VideoName, sizeof (VideoName));
	printf("SDL driver used: %s\n", VideoName);
			// Set the environment variable SDL_VIDEODRIVER to override
			// For Linux: x11 (default), dga, fbcon, directfb, svgalib,
			//            ggi, aalib
			// For Windows: directx (default), windib
	atexit (SDL_Quit);

	printf ("SDL initialized.\n");

	SDL_EnableUNICODE (1);

	printf ("Initializing Screen.\n");

	ScreenWidth = 320;
	ScreenHeight = 240;
	ScreenWidthActual = 640;
	ScreenHeightActual = 480;
	ScreenBPPActual = 16;

	switch(ScreenBPPActual) {
		case 8:
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 2);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 3);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 3);
			break;
		case 15:
		case 16:
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
			break;
		case 24:
		case 32:
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			break;
	}
	SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, ScreenBPPActual);
	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

	Screen = SDL_SetVideoMode (
		ScreenWidthActual, ScreenHeightActual, ScreenBPPActual,
		0
// | SDL_HWSURFACE
// | SDL_DOUBLEBUF
		| SDL_OPENGL
		| SDL_ANYFORMAT
// | SDL_FULLSCREEN
	);
	if (Screen == NULL)
	{
		printf ("Couldn't set %ix%ix%i video mode: %s\n",
			ScreenWidthActual, ScreenHeightActual, ScreenBPPActual,
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

	//glShadeModel (GL_SMOOTH);
	glClearColor (0,0,0,0);
	glViewport (0, 0, ScreenWidthActual, ScreenHeightActual);
	glDrawBuffer (GL_FRONT_LEFT);
	glClear (GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers ();  //Clear both buffers at init
	glClear (GL_COLOR_BUFFER_BIT);
	glEnable (GL_ALPHA_TEST);
	glDisable (GL_DITHER);
	glAlphaFunc (GL_GREATER, 0);

	printf ("%i joysticks were found.\n", SDL_NumJoysticks ());
	if(SDL_NumJoysticks() > 0)
	{
		printf ("The names of the joysticks are:\n");
		for(i = 0; i < SDL_NumJoysticks (); i++)
		{
			printf("    %s\n", SDL_JoystickName (i));
		}
		SDL_JoystickEventState (SDL_ENABLE);
		Joystick1 = SDL_JoystickOpen (0);
		if (SDL_NumJoysticks () > 1)
			SDL_JoystickOpen(1);
	}

	if (Mix_OpenAudio(44100, AUDIO_S16, 2, 4096))
	{
		printf ("Unable to open audio!\n");
		exit (-1);
	}
	atexit (Mix_CloseAudio);

	SDL_AudioDriverName (SoundcardName, sizeof (SoundcardName));

	Mix_QuerySpec (&audio_rate, &audio_format, &audio_channels);
	printf ("Opened %s at %d Hz %d bit %s, %d bytes audio buffer\n",
			SoundcardName, audio_rate, audio_format & 0xFF,
			audio_channels > 1 ? "stereo" : "mono", 4096);

	MIX_VERSION (&compile_version);
	printf ("Compiled with SDL_mixer version: %d.%d.%d\n",
			compile_version.major,
			compile_version.minor,
			compile_version.patch);
	printf ("Running with SDL_mixer version: %d.%d.%d\n",
			Mix_Linked_Version()->major,
			Mix_Linked_Version()->minor,
			Mix_Linked_Version()->patch);

	//SDL_ShowCursor (SDL_DISABLE);

	GraphicsSem = SDL_CreateSemaphore (1);
	_MemorySem = SDL_CreateSemaphore (1);

	//End System Initialization

	//Begin Game Initialization

	for(a = 0; a < 512; a++)
	{
		KeyboardDown[a] = FALSE;
		//KeyboardStroke[a] = FALSE;
	}
	ControlA =          SDLK_f;
	ControlB =          SDLK_d;
	ControlC =          SDLK_s;
	ControlX =          SDLK_a;
	ControlStart =      SDLK_RETURN;
	ControlLeftShift =  SDLK_LSHIFT;
	ControlRightShift = SDLK_RSHIFT;

	//End Game Initialization

	TFB_DEBUG_HALT = 0;

	//Begin MultiThreading

	printf ("\n---\n\n");

	SDL_CreateThread (Starcon2Main, NULL);

	//End MultiThreading

	//Start Test Code

	//End Test Code

	//Start main game loop here

	for (;;)
	{
		//Process Events
		
		while (SDL_PollEvent (&Event))
		{
			switch (Event.type) {
				case SDL_ACTIVEEVENT:    /* Loose/gain visibility */
					// TODO
					break;
				case SDL_KEYDOWN:        /* Key pressed */
				case SDL_KEYUP:          /* Key released */
					ProcessKeyboardEvent (&Event);
					break;
				case SDL_JOYAXISMOTION:  /* Joystick axis motion */
				case SDL_JOYBUTTONDOWN:  /* Joystick button pressed */
				case SDL_JOYBUTTONUP:    /* Joystick button released */
					ProcessJoystickEvent (&Event);
					break;
				case SDL_QUIT:
					exit (0);
					break;
				case SDL_VIDEORESIZE:    /* User resized video mode */
					// TODO
					break;
				case SDL_VIDEOEXPOSE:    /* Screen needs to be redrawn */
					// TODO
					break;
				default:
					break;
			}
		}

		TFB_FlushGraphics ();

		//Still testing this... Should fix sticky input soon...
		/*
		{
			Uint8* Temp;

			SDL_PumpEvents ();
			Temp = SDL_GetKeyState (NULL);
			for(a = 0; a < 512; a++)
			{
				KeyboardDown[a] = Temp[a];
			}
		}
		*/
	}
}
