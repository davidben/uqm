#include <stdio.h>
#include <stdlib.h>
#include "sndlib.h"
#include "starcon.h"
#include "libs/sound/sound_common.h"

int
main (int argc, char *argv[])
{
//	InitSound (argc, argv);
	TFB_InitSound (TFB_SOUNDDRIVER_SDL, 0, 44100);

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
