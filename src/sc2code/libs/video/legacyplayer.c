#include "vidintrn.h"

BOOLEAN
PlayLegacyVideo (LEGACY_VIDEO vid)
{
	const char *name, *audname, *speechname;
	DWORD loopframe;
	VIDEO_REF VidRef;
	MUSIC_REF AudRef = 0;
	MUSIC_REF SpeechRef = 0;

	if (!vid)
		return FALSE;
	name = vid->video;
	audname = vid->audio;
	speechname = vid->speech;
	loopframe = vid->loop;

	VidRef = LoadVideoFile (name);
	if (!VidRef)
		return FALSE;
	if (audname)
		AudRef = LoadMusicFile (audname);
	if (speechname)
		SpeechRef = LoadMusicFile (speechname);

	VidPlayEx (VidRef, AudRef, SpeechRef, loopframe);
	VidDoInput ();
	VidStop ();
	
	DestroyVideo (VidRef);
	if (SpeechRef)
		DestroyMusic (SpeechRef);
	if (AudRef)
		DestroyMusic (AudRef);

	return TRUE;
}

