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

/*
 * Eventually this should include all configuration stuff, 
 * for now there's few options which indicate 3do/pc flavors.
 * By Mika Kolehmainen.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "options.h"
#include "file.h"
#include "config.h"

int optWhichMusic = MUSIC_3DO;
char *configDir, *saveDir, *meleeDir;
BOOLEAN optSubtitles = TRUE;

void
prepareConfigDir (void) {
	configDir = malloc (PATH_MAX - 13);
	if (expandPath (configDir, PATH_MAX - 13, CONFIGDIR) == -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		fprintf (stderr, "Fatal error: Invalid path to config files.\n");
		exit (EXIT_FAILURE);
	}
	configDir = realloc (configDir, strlen (configDir) + 1);

	// Create the path upto the config dir, if not already existing.
	mkdirhier (configDir);
}

void
prepareSaveDir (void) {
	saveDir = malloc (PATH_MAX - 13);
	if (expandPath (saveDir, PATH_MAX - 13, SAVEDIR) == -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		fprintf (stderr, "Fatal error: Invalid path to config files.\n");
		exit (EXIT_FAILURE);
	}
	saveDir = realloc (saveDir, strlen (saveDir) + 1);

	// Create the path upto the save dir, if not already existing.
	mkdirhier (saveDir);
//#ifdef DEBUG
	fprintf(stderr, "Saved games are kept in %s.\n", saveDir);
//#endif
}

void
prepareMeleeDir (void) {
	meleeDir = malloc (PATH_MAX - 13);
	if (expandPath (meleeDir, PATH_MAX - 13, MELEEDIR) == -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		fprintf (stderr, "Fatal error: Invalid path to config files.\n");
		exit (EXIT_FAILURE);
	}
	meleeDir = realloc (meleeDir, strlen (meleeDir) + 1);

	// Create the path upto the save dir, if not already existing.
	mkdirhier (meleeDir);
}

