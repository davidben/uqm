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

#include "starcon.h"
#include "commglue.h"
#include "libs/sound/trackplayer.h"
#include <stdarg.h>

void
NPCPhrase (int index)
{
	UNICODE *pStr, numbuf[100];
	void *pClip;

	switch (index)
	{
		case GLOBAL_PLAYER_NAME:
			pStr = GLOBAL_SIS (CommanderName);
			pClip = 0;
			break;
		case GLOBAL_SHIP_NAME:
			pStr = GLOBAL_SIS (ShipName);
			pClip = 0;
			break;
		case GLOBAL_PLAYER_LOCATION:
		{
			SIZE dx, dy;
			COUNT adx, ady;

			dx = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x)) - 333;
			adx = dx >= 0 ? dx : -dx;
			dy = 9812 - LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
			ady = dy >= 0 ? dy : -dy;
			wsprintf (numbuf,
					"%+04d.%01u,%+04d.%01u",
					(SIZE)(dy / 10), (COUNT)(ady % 10),
					(SIZE)(dx / 10), (COUNT)(adx % 10));
			pStr = numbuf;
			pClip = 0;
			break;
		}
		default:
			if (index < 0)
			{
				if (index > UNREASONABLE_NUMBER)
					sprintf (numbuf, "%d", -index);
				else
				{
					COUNT i;
					STRING S;

					index -= GLOBAL_ALLIANCE_NAME;

					i = GET_GAME_STATE (NEW_ALLIANCE_NAME);
					S = SetAbsStringTableIndex (CommData.ConversationPhrases, (index - 1) + i);
					wstrcpy (numbuf, (UNICODE *)GetStringAddress (S));
					if (i == 3)
						wstrcat (numbuf, GLOBAL_SIS (CommanderName));
				}
				pStr = numbuf;
				pClip = 0;
			}
			else
			{
				pStr = (UNICODE *)GetStringAddress (
						SetAbsStringTableIndex (CommData.ConversationPhrases, index - 1)
						);
				pClip = GetStringSoundClip (
						SetAbsStringTableIndex (CommData.ConversationPhrases, index - 1)
						);
			}
			break;
	}

	SpliceTrack (pClip, pStr);
}

void
GetAllianceName (UNICODE *buf, RESPONSE_REF name_1)
{
	COUNT i;
	STRING S;

	i = GET_GAME_STATE (NEW_ALLIANCE_NAME);
	S = SetAbsStringTableIndex (CommData.ConversationPhrases, (name_1 - 1) + i);
	wstrcpy (buf, (UNICODE *)GetStringAddress (S));
	if (i == 3)
	{
		wstrcat (buf, GLOBAL_SIS (CommanderName));
		wstrcat (buf, (UNICODE *)GetStringAddress (SetRelStringTableIndex (S, 1)));
	}
}

void
construct_response (UNICODE *buf, RESPONSE_REF R, ...)
{
	UNICODE *name;
	va_list vlist;

	va_start (vlist, R);
	do
	{
		COUNT len;
		STRING S;
		
		S = SetAbsStringTableIndex (CommData.ConversationPhrases, R - 1);
		wstrcpy (buf, (UNICODE *)GetStringAddress (S));
		len = wstrlen (buf);
		buf += len;
		name = va_arg (vlist, UNICODE *);
		if (name)
		{
			len = wstrlen (name);
			wstrncpy (buf, name, len);
			buf += len;

			if ((R = va_arg (vlist, RESPONSE_REF)) == (RESPONSE_REF)-1)
				name = 0;
		}
	} while (name);
	va_end (vlist);
	
	*buf = '\0';
}

LOCDATAPTR
init_race (RESOURCE comm_id)
{
	switch (comm_id)
	{
		case ARILOU_CONVERSATION:
			return ((LOCDATAPTR)init_arilou_comm ());
		case BLACKURQ_CONVERSATION:
			return ((LOCDATAPTR)init_blackurq_comm ());
		case CHMMR_CONVERSATION:
			return ((LOCDATAPTR)init_chmmr_comm ());
		case COMMANDER_CONVERSATION:
			if (!GET_GAME_STATE (STARBASE_AVAILABLE))
				return ((LOCDATAPTR)init_commander_comm ());
			else
				return ((LOCDATAPTR)init_starbase_comm ());
		case DRUUGE_CONVERSATION:
			return ((LOCDATAPTR)init_druuge_comm ());
		case ILWRATH_CONVERSATION:
			return ((LOCDATAPTR)init_ilwrath_comm ());
		case MELNORME_CONVERSATION:
			return ((LOCDATAPTR)init_melnorme_comm ());
		case MYCON_CONVERSATION:
			return ((LOCDATAPTR)init_mycon_comm ());
		case ORZ_CONVERSATION:
			return ((LOCDATAPTR)init_orz_comm ());
		case PKUNK_CONVERSATION:
			return ((LOCDATAPTR)init_pkunk_comm ());
		case SHOFIXTI_CONVERSATION:
			return ((LOCDATAPTR)init_shofixti_comm ());
		case SLYLANDRO_CONVERSATION:
			return ((LOCDATAPTR)init_slyland_comm ());
		case SLYLANDRO_HOME_CONVERSATION:
			return ((LOCDATAPTR)init_slylandro_comm ());
		case SPATHI_CONVERSATION:
			if (!(GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7)))
				return ((LOCDATAPTR)init_spathi_comm ());
			else
				return ((LOCDATAPTR)init_spahome_comm ());
		case SUPOX_CONVERSATION:
			return ((LOCDATAPTR)init_supox_comm ());
		case SYREEN_CONVERSATION:
			return ((LOCDATAPTR)init_syreen_comm ());
		case TALKING_PET_CONVERSATION:
			return ((LOCDATAPTR)init_talkpet_comm ());
		case THRADD_CONVERSATION:
			return ((LOCDATAPTR)init_thradd_comm ());
		case UMGAH_CONVERSATION:
			return ((LOCDATAPTR)init_umgah_comm ());
		case URQUAN_CONVERSATION:
			return ((LOCDATAPTR)init_urquan_comm ());
		case UTWIG_CONVERSATION:
			return ((LOCDATAPTR)init_utwig_comm ());
		case VUX_CONVERSATION:
			return ((LOCDATAPTR)init_vux_comm ());
		case YEHAT_REBEL_CONVERSATION:
			return ((LOCDATAPTR)init_rebel_yehat_comm ());
		case YEHAT_CONVERSATION:
			return ((LOCDATAPTR)init_yehat_comm ());
		case ZOQFOTPIK_CONVERSATION:
			return ((LOCDATAPTR)init_zoqfot_comm ());
		default:
			return ((LOCDATAPTR)init_chmmr_comm ());
	}
}
