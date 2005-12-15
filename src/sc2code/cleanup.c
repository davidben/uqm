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

#include "battle.h"
#include "master.h"
#include "nameref.h"
#include "reslib.h"
#include "gamestr.h"
#include "init.h"
#include "hyper.h"
#include "planets/lander.h"
#include "starcon.h"
#include "setup.h"
#include "sounds.h"
#include "libs/sndlib.h"
#include "libs/vidlib.h"

// XXX: we do not current have a header for this prototype to live in
//  should be something like solarsys.h
extern void FreeIPData (void);

static void UninitContexts (void);
static void UninitKernel (BOOLEAN ships);
static void UninitGameKernel (void);


void
FreeKernel (void)
{
	UninitKernel (TRUE);
	UninitContexts ();

	UNINIT_INSTANCES ();
	UninitResourceSystem ();

	UninitPlayerInput ();

	DestroyDrawable (ReleaseDrawable (Screen));
	DestroyContext (ReleaseContext (ScreenContext));

	UninitVideo ();
	UninitSound ();
	UninitGraphics ();
}

static void
UninitContexts (void)
{
	UninitQueue (&disp_q);

	DestroyContext (ReleaseContext (OffScreenContext));
	DestroyContext (ReleaseContext (SpaceContext));
	DestroyContext (ReleaseContext (StatusContext));
}

static void
UninitKernel (BOOLEAN ships)
{
	UninitSpace ();

	DestroySound (ReleaseSound (MenuSounds));
	DestroyFont (ReleaseFont (MicroFont));
	DestroyStringTable (ReleaseStringTable (GameStrings));
	DestroyDrawable (ReleaseDrawable (StatusFrame));
	DestroyDrawable (ReleaseDrawable (ActivityFrame));
	DestroyFont (ReleaseFont (TinyFont));
	DestroyFont (ReleaseFont (StarConFont));

	UninitQueue (&race_q[0]);
	UninitQueue (&race_q[1]);

	if (ships)
		FreeMasterShipList ();
	
	ActivityFrame = 0;
}

void
FreeGameData (void)
{
	FreeSC2Data ();
	FreeLanderData ();
	FreeIPData ();
	FreeHyperData ();
}

static void
UninitGameKernel (void)
{
	// XXX: this function is never called. Why not? (BUG?)
	if (ActivityFrame)
	{
		FreeGameData ();

		UninitKernel (FALSE);
		UninitContexts ();
	}
}


