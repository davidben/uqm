/*
	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

// Memory driver for mikmod
// based on SDL driver (c) Sam Lantinga

#include "mikmod.h"

static void* buffer;
ULONG bufsize;
ULONG written;

ULONG* ALDRV_SetOutputBuffer(void* buf, ULONG size)
{
	buffer = buf;
	bufsize = size;
	written = 0;
	return &written;
}


static BOOL ALDRV_IsThere(void)
{
    return 1;
}


static BOOL ALDRV_Init(void)
{
    md_mode |= DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX;
    return(VC_Init());
}


static void ALDRV_Exit(void)
{
    VC_Exit();
}


static void ALDRV_Update(void)
{
	if (!buffer || bufsize == 0)
		return;

	written = VC_WriteBytes(buffer, bufsize);
}


static BOOL ALDRV_Reset(void)
{
    return 0;
}


MIKMODAPI MDRIVER drv_openal =
{   NULL,
    "OpenAL",
    "MikMod OpenAL driver v1.0",
    0,255,
    "OpenAL",

    NULL,
    ALDRV_IsThere,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    ALDRV_Init,
    ALDRV_Exit,
    ALDRV_Reset,
    VC_SetNumVoices,
    VC_PlayStart,
    VC_PlayStop,
    ALDRV_Update,
    NULL,               /* FIXME: Pause */
    VC_VoiceSetVolume,
    VC_VoiceGetVolume,
    VC_VoiceSetFrequency,
    VC_VoiceGetFrequency,
    VC_VoiceSetPanning,
    VC_VoiceGetPanning,
    VC_VoicePlay,
    VC_VoiceStop,
    VC_VoiceStopped,
    VC_VoiceGetPosition,
    VC_VoiceRealVolume
};
