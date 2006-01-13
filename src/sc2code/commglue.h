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

#ifndef _COMMGLUE_H
#define _COMMGLUE_H

#include "globdata.h"
#include "resinst.h"
#include "libs/sound/trackplayer.h"


extern LOCDATA CommData;
extern int cur_comm;
extern UNICODE shared_phrase_buf[2048];

#define PLAYER_SAID(r,i) ((r)==(i))
#define PHRASE_ENABLED(p) \
		(*(UNICODE *)GetStringAddress ( \
				SetAbsStringTableIndex (CommData.ConversationPhrases, (p)-1) \
				) != '\0')
#define DISABLE_PHRASE(p) \
		(*(UNICODE *)GetStringAddress ( \
				SetAbsStringTableIndex (CommData.ConversationPhrases, (p)-1) \
				) = '\0')
#define RESPONSE_TO_REF(R) (R)

#define Response(i,a) \
		DoResponsePhrase(i,(RESPONSE_FUNC)a,0)

enum
{
	GLOBAL_PLAYER_NAME = -1000000,
	GLOBAL_SHIP_NAME,
	GLOBAL_PLAYER_LOCATION,

	GLOBAL_ALLIANCE_NAME,
	UNREASONABLE_NUMBER = GLOBAL_ALLIANCE_NAME + 1000,
};

typedef COUNT RESPONSE_REF;

typedef void (*RESPONSE_FUNC) (RESPONSE_REF R);

extern void DoResponsePhrase (RESPONSE_REF R, RESPONSE_FUNC
		response_func, UNICODE *ContstructStr);
extern void DoNPCPhrase (UNICODE *pStr);

extern void NPCPhrase_cb (int index, TFB_TrackCB cb);
#define NPCPhrase(index) NPCPhrase_cb ((index), NULL)

#define ALLIANCE_NAME_BUFSIZE 256
extern void GetAllianceName (UNICODE *buf, RESPONSE_REF name_1);

extern void construct_response (UNICODE *buf, int R /* promoted from
		RESPONSE_REF */, ...);

extern LOCDATAPTR init_race (RESOURCE comm_id);

extern LOCDATAPTR init_arilou_comm (void);

extern LOCDATAPTR init_blackurq_comm (void);

extern LOCDATAPTR init_chmmr_comm (void);

extern LOCDATAPTR init_commander_comm (void);

extern LOCDATAPTR init_druuge_comm (void);

extern LOCDATAPTR init_ilwrath_comm (void);

extern LOCDATAPTR init_melnorme_comm (void);

extern LOCDATAPTR init_mycon_comm (void);

extern LOCDATAPTR init_orz_comm (void);

extern LOCDATAPTR init_pkunk_comm (void);

extern LOCDATAPTR init_rebel_yehat_comm (void);

extern LOCDATAPTR init_shofixti_comm (void);

extern LOCDATAPTR init_slyland_comm (void);

extern LOCDATAPTR init_slylandro_comm (void);

extern LOCDATAPTR init_spahome_comm (void);

extern LOCDATAPTR init_spathi_comm (void);

extern LOCDATAPTR init_starbase_comm (void);

extern LOCDATAPTR init_supox_comm (void);

extern LOCDATAPTR init_syreen_comm (void);

extern LOCDATAPTR init_talkpet_comm (void);

extern LOCDATAPTR init_thradd_comm (void);

extern LOCDATAPTR init_umgah_comm (void);

extern LOCDATAPTR init_urquan_comm (void);

extern LOCDATAPTR init_utwig_comm (void);

extern LOCDATAPTR init_vux_comm (void);

extern LOCDATAPTR init_yehat_comm (void);

extern LOCDATAPTR init_zoqfot_comm (void);

extern LOCDATAPTR init_umgah_comm (void);

#endif /* _COMMGLUE_H */

