// OpenAL driver for mikmod
// based on SDL driver (c) Sam Lantinga

#ifdef SOUNDMODULE_MIXSDL

#include "mikmod_internals.h"


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
    /* does nothing, buffers are updated in the background */
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

#endif
