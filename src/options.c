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

int optWhichMusic;
int optWhichCoarseScan;
int optWhichMenu;
int optWhichFonts;
int optSmoothScroll;
int optMeleeScale;

BOOLEAN optSubtitles;
BOOLEAN optStereoSFX;
uio_DirHandle *contentDir;
uio_DirHandle *configDir;
uio_DirHandle *saveDir;
uio_DirHandle *meleeDir;

extern uio_Repository *repository;
extern uio_DirHandle *rootDir;


static const char *findFileInDirs (const char *locs[], int numLocs,
		const char *file);
static void mountContentDir (uio_Repository *repository,
		const char *contentPath, const char **addons);
static void mountDirZips (uio_MountHandle *contentHandle,
		uio_DirHandle *dirHandle);


// Looks for a file 'file' in all 'numLocs' locations from 'locs'.
// returns the first element from locs where 'file' is found.
// If there is no matching location, NULL will be returned and
// errno will be set to 'ENOENT'.
// Entries from 'locs' that together with 'file' are longer than
// PATH_MAX will be ignored, except for a warning given to stderr.
static const char *
findFileInDirs (const char *locs[], int numLocs, const char *file)
{
	int locI;
	char path[PATH_MAX];
	size_t fileLen;

	fileLen = strlen (file);
	for (locI = 0; locI < numLocs; locI++)
	{
		size_t locLen;
		const char *loc;
		bool needSlash;
		
		loc = locs[locI];
		locLen = strlen (loc);

		needSlash = (locLen != 0 && loc[locLen - 1] != '/');
		if (locLen + (needSlash ? 1 : 0) + fileLen + 1 >= sizeof path)
		{
			// This dir plus the file name is too long.
			fprintf (stderr, "Warning: path '%s' is ignored because it is "
					"too long.\n", loc);
			continue;
		}
		
		snprintf (path, sizeof path, "%s%s%s", loc, needSlash ? "/" : "",
				file);
		if (fileExists (path))
			return loc;
	}

	// No matching location was found.
	errno = ENOENT;
	return NULL;
}

void
prepareContentDir (const char *contentDirName, const char **addons)
{
	const char *testFile = "version";
	const char *loc;
	char path[PATH_MAX];

	if (contentDirName == NULL)
	{
		// Try the default content locations.
		const char *locs[] = {
			CONTENTDIR, /* defined in config.h */
			""
			"content",
			"../../content" /* For running from MSVC */
		};
		loc = findFileInDirs (locs, sizeof locs / sizeof locs[0], testFile);
	}
	else
	{
		// Only use the explicitely supplied content dir.
		loc = findFileInDirs (&contentDirName, 1, testFile);
	}
	if (loc == NULL)
	{
		fprintf (stderr, "Fatal error: Could not find content.\n");
		exit (EXIT_FAILURE);
	}

	if (expandPath(path, sizeof path, loc, EP_ALL_SYSTEM) == -1)
	{
		fprintf (stderr, "Fatal error: Could not expand path to content "
				"directory: %s\n", strerror (errno));
		exit (EXIT_FAILURE);
	}
	
#ifdef DEBUG
	fprintf (stderr, "Using '%s' as base content dir.\n", path);
#endif
	mountContentDir (repository, path, addons);
}

void
prepareConfigDir (void) {
	char buf[PATH_MAX];
	static uio_AutoMount *autoMount[] = { NULL };
	uio_MountHandle *contentHandle;

	if (expandPath (buf, PATH_MAX - 13, CONFIGDIR, EP_ALL_SYSTEM) == -1)
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
		fprintf (stderr, "Fatal error: Could not mount config dir: %s\n",
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
	if (expandPath (buf, PATH_MAX - 13, SAVEDIR, EP_ALL_SYSTEM) == -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		fprintf (stderr, "Fatal error: Invalid path to config files.\n");
		exit (EXIT_FAILURE);
	}

	// Create the path upto the save dir, if not already existing.
	if (mkdirhier (buf) == -1)
		exit (EXIT_FAILURE);
#ifdef DEBUG
	fprintf(stderr, "Saved games are kept in %s.\n", buf);
#endif

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

	if (expandPath (buf, PATH_MAX - 13, MELEEDIR, EP_ALL_SYSTEM) == -1)
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
mountContentDir (uio_Repository *repository, const char *contentPath,
		const char **addons) {
	uio_MountHandle *contentHandle;
	uio_DirHandle *packagesDir, *addonsDir, *addonDir;
	static uio_AutoMount *autoMount[] = { NULL };

	contentHandle = uio_mountDir (repository, "/",
			uio_FSTYPE_STDIO, NULL, NULL, contentPath, autoMount,
			uio_MOUNT_TOP | uio_MOUNT_RDONLY, NULL);
	if (contentHandle == NULL) {
		fprintf (stderr, "Fatal error: Could not mount content dir: %s\n",
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
		if (addons[0] != NULL) {
			// addons were specified, but there's no /packages dir,
			// let alone a /packages/addons dir.
			fprintf (stderr, "Warning: There's no 'packages/addons' "
					"directory in the 'content' directory;\n\t'--addon' "
					"options are ignored.\n");
		}
		return;
	}

	mountDirZips (contentHandle, packagesDir);

	// NB: note the difference between addonsDir and addonDir.
	//     the former is the dir 'packages/addons', the latter a directory
	//     in that dir.
	addonsDir = uio_openDirRelative (packagesDir, "addons", 0);
	if (addonsDir == NULL) {
		// No addon dir found.
		fprintf (stderr, "Warning: There's no 'packages/addons' "
				"directory in the 'content' directory;\n\t'--addon' "
				"options are ignored.\n");
		uio_closeDir (packagesDir);
		return;
	}
			
	uio_closeDir (packagesDir);

	for (; *addons != NULL; addons++)
	{
		addonDir = uio_openDirRelative (addonsDir, *addons, 0);
		if (addonDir == NULL) {
			fprintf (stderr, "Warning: directory 'packages/addons/%s' "
					"not found; addon skipped.\n", *addons);
			continue;
		}
		
		mountDirZips (contentHandle, addonDir);

		uio_closeDir (addonDir);
	}

	uio_closeDir (addonsDir);
}

static void
mountDirZips (uio_MountHandle *contentHandle, uio_DirHandle *dirHandle)
{
	static uio_AutoMount *autoMount[] = { NULL };
	uio_DirList *dirList;

	dirList = uio_getDirList (dirHandle, "", ".zip", match_MATCH_SUFFIX);
	if (dirList != NULL) {
		int i;
		
		for (i = 0; i < dirList->numNames; i++) {
			if (uio_mountDir (repository, "/", uio_FSTYPE_ZIP,
					dirHandle, dirList->names[i], "/", autoMount,
					uio_MOUNT_BELOW | uio_MOUNT_RDONLY,
					contentHandle) == NULL) {
				fprintf (stderr, "Warning: Could not mount '%s': %s.\n",
						dirList->names[i], strerror (errno));
			}
		}
	}
	uio_freeDirList (dirList);
}




