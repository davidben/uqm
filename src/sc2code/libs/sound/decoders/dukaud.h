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

/* .duk sound track decoder */

#ifndef DUKAUD_H
#define DUKAUD_H

#include "port.h"
#include "types.h"
#include "uio.h"

typedef struct
{
	// public read-only
	uint32 iframe;    // current frame index
	uint32 cframes;   // total count of frames
	uint32 rate;      // frequency
	uint32 channels;  // number of channels
	uint32 pcm_frame; // samples per frame
	float length;     // length in seconds (estimated)

	// private
	uio_Stream* duk;
	uint32* frames;
	// buffer
	void* data;
	uint32 maxdata;
	uint32 cbdata;
	uint32 dataofs;
	// decoder stuff
	sint32 predictors[2];

} DukAud_Track;

typedef enum
{
	// positive values are the same as in errno
	dukae_None = 0,
	dukae_Unknown = -1,
	dukae_BadFile = -2,
	dukae_BadArg = -3,
	dukae_Other = -1000,
} DukAud_Error;

sint32 duka_getError ();
DukAud_Track* duka_openTrack (uio_DirHandle *dir, const char *file);
void duka_closeTrack (DukAud_Track* track);
bool duka_seekTrack (DukAud_Track* track, float pos /* in seconds */);
sint32 duka_readData (DukAud_Track* track, void* buf, sint32 bufsize);
uint32 duka_getFrame (DukAud_Track* track);
uint32 duka_getFrameSize (DukAud_Track* track);

#endif // DUKAUD_H
