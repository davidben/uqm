#include <stdio.h>
#include <stdlib.h>
#include "sndlib.h"
#include "SDL_wrapper.h"

int
main (int argc, char *argv[])
{
	InitSound (argc, argv);

	while (1)
	{
		COUNT sound_index;

		fprintf (stderr, "Sound #: ");
		if (fscanf (stdin, "%u", &sound_index) != 1)
			break;

		PlaySound (sound_index, GAME_SOUND_PRIORITY);
	}

	UninitSound ();

	exit (EXIT_SUCCESS);
}
