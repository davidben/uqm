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

#include "build.h"
#include "encount.h"
#include "globdata.h"
#include "state.h"
#include "planets/genall.h"
#include "libs/log.h"


static int
SelectMelnormeRefVar (void)
{
	int RefVar;

	switch (CurStarDescPtr->Index)
	{
		case MELNORME0_DEFINED:
			RefVar = MELNORME0_GRPOFFS0;
			break;
		case MELNORME1_DEFINED:
			RefVar = MELNORME1_GRPOFFS0;
			break;
		case MELNORME2_DEFINED:
			RefVar = MELNORME2_GRPOFFS0;
			break;
		case MELNORME3_DEFINED:
			RefVar = MELNORME3_GRPOFFS0;
			break;
		case MELNORME4_DEFINED:
			RefVar = MELNORME4_GRPOFFS0;
			break;
		case MELNORME5_DEFINED:
			RefVar = MELNORME5_GRPOFFS0;
			break;
		case MELNORME6_DEFINED:
			RefVar = MELNORME6_GRPOFFS0;
			break;
		case MELNORME7_DEFINED:
			RefVar = MELNORME7_GRPOFFS0;
			break;
		case MELNORME8_DEFINED:
			RefVar = MELNORME8_GRPOFFS0;
			break;
		default:
			RefVar = -1;
	}
	return RefVar;
}

static DWORD
GetMelnormeRef (void)
{
	int RefVar = SelectMelnormeRefVar ();
	if (RefVar < 0)
	{
		log_add (log_Warning, "GetMelnormeRef(): reference unknown");
		return 0;
	}

	return GET_GAME_STATE_32 (RefVar);
}

static void
SetMelnormeRef (DWORD Ref)
{
	int RefVar = SelectMelnormeRefVar ();
	if (RefVar < 0)
	{
		log_add (log_Warning, "SetMelnormeRef(): reference unknown");
		return;
	}

	SET_GAME_STATE_32 (RefVar, Ref);
}

void
GenerateMelnorme (BYTE control)
{
	switch (control)
	{
		case INIT_NPCS:
			GLOBAL (BattleGroupRef) = GetMelnormeRef ();
			if (GLOBAL (BattleGroupRef) == 0)
			{
				CloneShipFragment (MELNORME_SHIP,
						&GLOBAL (npc_built_ship_q), 0);
				GLOBAL (BattleGroupRef) = PutGroupInfo (GROUPS_ADD_NEW, 1);
				SetMelnormeRef (GLOBAL (BattleGroupRef));
			}
			GenerateRandomIP (INIT_NPCS);
			break;
		default:
			GenerateRandomIP (control);
			break;
	}
}


