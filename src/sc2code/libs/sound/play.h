#ifndef _PLAY_H
#define _PLAY_H

enum
{
	MOD_TRACK,
	RED_BOOK_TRACK,

	NUM_TRACK_TYPES
};

typedef struct
{
	MUSIC_REF TrackRef;
	PBYTE TrackPtr;
} TRACK_DESC;
extern TRACK_DESC volatile _TrackList[NUM_TRACK_TYPES];

#define MAX_CHANNELS 4
#define MAX_TRACKS 16
#define MAX_INSTRUMENTS 63
#define MAX_BLOCKS 256
#define MAX_TRACK_VOLUME 64

typedef unsigned short SAMPLE_RATE;

typedef struct
{
	COUNT LoopBegin, LoopLength;
	COUNT SampleLength;
	BYTE volume, transposition;
} INSTRUMENT_DESC;

typedef struct
{
	BYTE LineCommands[MAX_CHANNELS][3];
} LINE_DESC;
typedef LINE_DESC *PLINE_DESC;
#define LINE_DESCPTR PLINE_DESC

typedef struct
{
	BYTE NumTracks, LastLine;
	LINE_DESC LineList[1];
} BLOCK_DESC;
typedef BLOCK_DESC *PBLOCK_DESC;
#define BLOCK_DESCPTR PBLOCK_DESC

#define PLAY_CONTINUOUS (1 << 0)

typedef struct
{
	INSTRUMENT_DESC PresetList[MAX_INSTRUMENTS];
	BYTE NumPhysicalBlocks, NumLogicalBlocks;
	BYTE LogToPhysBlockList[MAX_BLOCKS];
	BYTE Tempo, NumSteps;
	BYTE Priority, Flags;
	BYTE TrackVolumeList[MAX_TRACKS];
	BYTE MasterVolume;
	BYTE NumInstruments;

	DWORD InstrumentList[MAX_INSTRUMENTS];
	DWORD BlockList[1];
} SONG_DESC;
typedef SONG_DESC *PSONG_DESC;
#define SONG_DESCPTR PSONG_DESC

extern void _download_effects (SOUND_REF SoundRef);
extern BOOLEAN _download_instruments (MUSIC_REF MusicRef);

#include "redbook.h"

#endif /* _PLAY_H */

