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
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "port.h"
#include "libs/graphics/gfx_common.h"
#include "compiler.h"
#include "options.h"
#include "file.h"
#include "config.h"
#include "libs/uio.h"

int optWhichMusic = OPT_3DO;
int optWhichCoarseScan = OPT_3DO;
int optWhichMenu = OPT_3DO;
int optWhichFonts = OPT_PC;
int optSmoothScroll = OPT_PC;
int optMeleeScale = TFB_SCALE_TRILINEAR;

BOOLEAN optSubtitles = TRUE;
BOOLEAN optStereoSFX = FALSE;
uio_DirHandle *contentDir;
uio_DirHandle *configDir;
uio_DirHandle *saveDir;
uio_DirHandle *meleeDir;

extern uio_Repository *repository;
extern uio_DirHandle *rootDir;


static void mountContentDir (uio_Repository *repository,
		const char *contentPath);

void
prepareContentDir (char *contentDirName)
{
	const char *testfile = "version";
	char cwd[PATH_MAX];

	if (!fileExists (testfile))
	{
		if ((chdir (contentDirName) || !fileExists (testfile)) &&
				(chdir ("content") || !fileExists (testfile)) &&
				(chdir ("../../content") || !fileExists (testfile))) {
			fprintf (stderr, "Fatal error: content not available, running from wrong dir?\n");
			exit (EXIT_FAILURE);
		}
	}
	if (getcwd(cwd, sizeof cwd) == NULL) {
		fprintf (stderr, "Fatal error: Could not get the current "
				"directory.\n");
		exit (EXIT_FAILURE);
	}
	mountContentDir (repository, cwd);
}

void
prepareConfigDir (void) {
	char buf[PATH_MAX];
	static uio_AutoMount *autoMount[] = { NULL };
	uio_MountHandle *contentHandle;

	if (expandPath (buf, PATH_MAX - 13, CONFIGDIR) == -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		fprintf (stderr, "Fatal error: Invalid path to config files.\n");
		exit (EXIT_FAILURE);
	}

	// Create the path upto the config dir, if not already existing.
	if (mkdirhier (buf) == -1)
		exit (EXIT_FAILURE);

	contentHandle = uio_mountDir (repository, "/",
			uio_FSTYPE_STDIO, NULL, NULL, buf, autoMount,
			uio_MOUNT_TOP, NULL);
	if (contentHandle == NULL) {
		fprintf (stderr, "Fatal error: Couldn't mount content dir: %s\n",
				strerror (errno));
		exit (EXIT_FAILURE);
	}

	configDir = uio_openDir (repository, "/", 0);
	if (configDir == NULL) {
		fprintf (stderr, "Fatal error: Could not open config dir: %s\n",
				strerror (errno));
		exit (EXIT_FAILURE);
	}
}

void
prepareSaveDir (void) {
	char buf[PATH_MAX];
	if (expandPath (buf, PATH_MAX - 13, SAVEDIR) == -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		fprintf (stderr, "Fatal error: Invalid path to config files.\n");
		exit (EXIT_FAILURE);
	}

	// Create the path upto the save dir, if not already existing.
	if (mkdirhier (buf) == -1)
		exit (EXIT_FAILURE);
//#ifdef DEBUG
	fprintf(stderr, "Saved games are kept in %s.\n", buf);
//#endif

	saveDir = uio_openDirRelative (configDir, "save", 0);
	if (saveDir == NULL) {
		fprintf (stderr, "Fatal error: Could not open save dir: %s\n",
				strerror (errno));
		exit (EXIT_FAILURE);
	}
}

void
prepareMeleeDir (void) {
	char buf[PATH_MAX];

	if (expandPath (buf, PATH_MAX - 13, MELEEDIR) == -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		fprintf (stderr, "Fatal error: Invalid path to config files.\n");
		exit (EXIT_FAILURE);
	}

	// Create the path upto the save dir, if not already existing.
	if (mkdirhier (buf) == -1)
		exit (EXIT_FAILURE);

	meleeDir = uio_openDirRelative (configDir, "teams", 0);
	if (meleeDir == NULL) {
		fprintf (stderr, "Fatal error: Could not open melee teams dir: %s\n",
				strerror (errno));
		exit (EXIT_FAILURE);
	}
}

static void
mountContentDir (uio_Repository *repository, const char *contentPath) {
	uio_MountHandle *contentHandle;
	uio_DirHandle *packagesDir;
	static uio_AutoMount *autoMount[] = { NULL };

	contentHandle = uio_mountDir (repository, "/",
			uio_FSTYPE_STDIO, NULL, NULL, contentPath, autoMount,
			uio_MOUNT_TOP | uio_MOUNT_RDONLY, NULL);
	if (contentHandle == NULL) {
		fprintf (stderr, "Fatal error: Couldn't mount content dir: %s\n",
				strerror (errno));
		exit (EXIT_FAILURE);
	}

	contentDir = uio_openDir (repository, "/", 0);
	if (contentDir == NULL) {
		fprintf (stderr, "Fatal error: Could not open content dir: %s\n",
				strerror (errno));
		exit (EXIT_FAILURE);
	}

	packagesDir = uio_openDir (repository, "/packages", 0);
	if (packagesDir == NULL) {
		// No packages dir means no packages to load.
		return;
	}

	{
		uio_DirList *dirList;

		dirList = uio_getDirList(packagesDir, "", ".zip", match_MATCH_SUFFIX);
		if (dirList != NULL) {
			int i;
			
			for (i = 0; i < dirList->numNames; i++) {
				if (uio_mountDir (repository, "/", uio_FSTYPE_ZIP,
						packagesDir, dirList->names[i], "/", autoMount,
						uio_MOUNT_BELOW | uio_MOUNT_RDONLY,
						contentHandle) == NULL) {
					fprintf(stderr, "Warning: Could not mount '%s': %s.\n",
							dirList->names[i], strerror(errno));
				}
			}
		}
		uio_freeDirList (dirList);
	}
	uio_closeDir(packagesDir);
}


