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

#ifndef TRACKINT_H
#define TRACKINT_H

typedef struct tfb_soundchain
{
	TFB_SoundDecoder *decoder; // points at the decoder to read from
	float start_time;
	int tag_me;
	uint32 track_num;
	void *text;
	TFB_TrackCB callback;
	struct tfb_soundchain *next;
} TFB_SoundChain;

typedef struct tfb_soundchaindata
{
	TFB_SoundChain *read_chain_ptr; // points to chain read poistion
	TFB_SoundChain *play_chain_ptr; // points to chain playing position

} TFB_SoundChainData;

extern TFB_SoundChain *first_chain;

TFB_SoundChain *create_soundchain (TFB_SoundDecoder *decoder, float startTime);
void destroy_soundchain (TFB_SoundChain *chain);
TFB_SoundChain *get_previous_chain (TFB_SoundChain *first_chain, TFB_SoundChain *current_chain);

#endif // TRACKINT_H
