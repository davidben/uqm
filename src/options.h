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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "port.h"
#include "libs/uio.h"

#define OPT_3DO 0x01
#define OPT_PC  0x02
#define OPT_ALL 0xFF

extern int optWhichMusic;
extern int optWhichCoarseScan;
extern int optWhichMenu;
extern int optWhichFonts;
extern int optWhichIntro;
extern int optSmoothScroll;
extern int optMeleeScale;

extern BOOLEAN optSubtitles;
extern BOOLEAN optStereoSFX;

extern uio_DirHandle *contentDir;
extern uio_DirHandle *configDir;
extern uio_DirHandle *saveDir;
extern uio_DirHandle *meleeDir;

void prepareContentDir (const char *contentDirName, const char **addons);
void prepareConfigDir (const char *configDirName);
void prepareMeleeDir (void);
void prepareSaveDir (void);
void initLanguage (void);

#endif

