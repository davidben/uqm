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

#include "reslib.h"
#include "comm/zoqfot/resinst.h"
#include "starcon.h"
#include "commglue.h"
#include "comm/zoqfot/strings.h"

#define ZOQ_FG_COLOR WHITE_COLOR
#define ZOQ_BG_COLOR BLACK_COLOR
#define ZOQ_BASE_X (TEXT_X_OFFS + ((SIS_TEXT_WIDTH >> 1) >> 1))
#define ZOQ_BASE_Y 24
#define ZOQ_TALK_INDEX 18
#define ZOQ_TALK_FRAMES 5
#define FOT_TO_ZOQ 23

#define PIK_FG_COLOR WHITE_COLOR
#define PIK_BG_COLOR BLACK_COLOR
#define PIK_BASE_X (SIS_SCREEN_WIDTH - (TEXT_X_OFFS + ((SIS_TEXT_WIDTH >> 1) >> 1)))
#define PIK_BASE_Y 24
#define PIK_TALK_INDEX 29
#define PIK_TALK_FRAMES 2
#define FOT_TO_PIK 26

static LOCDATA zoqfot_desc =
{
	NULL_PTR, /* init_encounter_func */
	NULL_PTR, /* uninit_encounter_func */
	(FRAME)ZOQFOTPIK_PMAP_ANIM, /* AlienFrame */
	(FONT)ZOQFOTPIK_FONT, /* AlienFont */
	0, /* AlienTextFColor */
	0, /* AlienTextBColor */
	0, /* SIS_SCREEN_WIDTH, */ /* AlienTextWidth */
	0, /* AlienColorMap */
	ZOQFOTPIK_MUSIC, /* AlienSong */
	ZOQFOTPIK_CONVERSATION_PHRASES, /* PlayerPhrases */
	3, /* NumAnimations */
	{
		{ /* Eye blink */
			1, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM /* AnimFlags */
			| WAIT_TALKING,
			5, 0, /* FrameRate */
			0, ONE_SECOND * 10, /* RestartRate */
		},
		{ /* Blow smoke */
			5, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			7, 0, /* FrameRate */
			ONE_SECOND * 2, 0, /* RestartRate */
		},
		{ /* Gulp */
			10, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			8, 0, /* FrameRate */
			0, ONE_SECOND * 10, /* RestartRate */
		},
	},
	{ /* Move Eye */
		FOT_TO_ZOQ, /* StartIndex */
		3, /* NumFrames */
		0, /* AnimFlags */
		4, 0, /* FrameRate */
		0, 0, /* RestartRate */
	},
	{
		ZOQ_TALK_INDEX, /* StartIndex */
		ZOQ_TALK_FRAMES, /* NumFrames */
		0, /* AnimFlags */
		8, 0, /* FrameRate */
		10, 0, /* RestartRate */
	},
	NULL_PTR, /* AlienNumberSpeech - none */
};

enum
{
	ZOQ_ALIEN,
	FOT_ALIEN,
	PIK_ALIEN
};

static void
SelectAlien (COUNT index, BYTE which_alien)
{
	switch (which_alien)
	{
		case ZOQ_ALIEN:
			CommData.AlienTransitionDesc.StartIndex = FOT_TO_ZOQ;
			CommData.AlienTalkDesc.StartIndex = ZOQ_TALK_INDEX;
			CommData.AlienTalkDesc.NumFrames = ZOQ_TALK_FRAMES;
			CommData.AlienAmbientArray[1].AnimFlags &= ~WAIT_TALKING;

			CommData.AlienTextTemplate.baseline.x = ZOQ_BASE_X;
			CommData.AlienTextTemplate.baseline.y = ZOQ_BASE_Y;
			CommData.AlienTextFColor = ZOQ_FG_COLOR;
			CommData.AlienTextBColor = ZOQ_BG_COLOR;
			break;
		case FOT_ALIEN:
			break;
		case PIK_ALIEN:
			CommData.AlienTransitionDesc.StartIndex = FOT_TO_PIK;
			CommData.AlienTalkDesc.StartIndex = PIK_TALK_INDEX;
			CommData.AlienTalkDesc.NumFrames = PIK_TALK_FRAMES;
			CommData.AlienAmbientArray[1].AnimFlags |= WAIT_TALKING;

			CommData.AlienTextTemplate.baseline.x = PIK_BASE_X;
			CommData.AlienTextTemplate.baseline.y = PIK_BASE_Y;
			CommData.AlienTextFColor = PIK_FG_COLOR;
			CommData.AlienTextBColor = PIK_BG_COLOR;
			break;
	}

	AlienTalkSegue (index);
}

static void
ExitConversation (RESPONSE_REF R)
{
	SET_GAME_STATE (BATTLE_SEGUE, 0);

	if (PLAYER_SAID (R, bye_homeworld))
	{
		NPCPhrase (GOODBYE_HOME0);
		NPCPhrase (GOODBYE_HOME1);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);
	}
	else if (PLAYER_SAID (R, decide_later))
	{
		NPCPhrase (PLEASE_HURRY0);
		NPCPhrase (PLEASE_HURRY1);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);
	}
	else if (PLAYER_SAID (R, valuable_info))
	{
		NPCPhrase (GOODBYE0);
		NPCPhrase (GOODBYE1);
		NPCPhrase (GOODBYE2);
		NPCPhrase (GOODBYE3);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);
	}
	else if (PLAYER_SAID (R, how_can_i_help))
	{
		NPCPhrase (EMMISSARIES0);
		NPCPhrase (EMMISSARIES1);
		NPCPhrase (EMMISSARIES2);
		NPCPhrase (EMMISSARIES3);
		NPCPhrase (EMMISSARIES4);
		NPCPhrase (EMMISSARIES5);
		NPCPhrase (EMMISSARIES6);
		NPCPhrase (EMMISSARIES7);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien (6, PIK_ALIEN);
		SelectAlien (7, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);
	}
	else if (PLAYER_SAID (R, sure))
	{
		NPCPhrase (WE_ALLY0);
		NPCPhrase (WE_ALLY1);
		NPCPhrase (WE_ALLY2);
		NPCPhrase (WE_ALLY3);
		NPCPhrase (WE_ALLY4);
		NPCPhrase (WE_ALLY5);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);
		ActivateStarShip (ZOQFOTPIK_SHIP, 0);
		AddEvent (RELATIVE_EVENT, 3, 0, 0, ZOQFOT_DISTRESS_EVENT);
		SET_GAME_STATE (ZOQFOT_HOME_VISITS, 0);
	}
	else if (PLAYER_SAID (R, all_very_interesting))
	{
		NPCPhrase (SEE_TOLD_YOU0);
		NPCPhrase (SEE_TOLD_YOU1);
		NPCPhrase (SEE_TOLD_YOU2);
		NPCPhrase (SEE_TOLD_YOU3);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		SET_GAME_STATE (ZOQFOT_HOSTILE, 1);
		SET_GAME_STATE (ZOQFOT_HOME_VISITS, 0);
		SET_GAME_STATE (BATTLE_SEGUE, 1);
	}
	else if (PLAYER_SAID (R, never))
	{
		NPCPhrase (WE_ENEMIES0);
		NPCPhrase (WE_ENEMIES1);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		SET_GAME_STATE (ZOQFOT_HOME_VISITS, 0);
		SET_GAME_STATE (ZOQFOT_HOSTILE, 1);
		SET_GAME_STATE (BATTLE_SEGUE, 1);
	}
}

static void
FormAlliance (RESPONSE_REF R)
{
	NPCPhrase (ALLY_WITH_US0);
	NPCPhrase (ALLY_WITH_US1);
	NPCPhrase (ALLY_WITH_US2);
	NPCPhrase (ALLY_WITH_US3);
	NPCPhrase (ALLY_WITH_US4);
	NPCPhrase (ALLY_WITH_US5);
	SelectAlien (1, ZOQ_ALIEN);
	SelectAlien (2, PIK_ALIEN);
	SelectAlien (3, ZOQ_ALIEN);
	SelectAlien (4, PIK_ALIEN);
	SelectAlien (5, ZOQ_ALIEN);
	SelectAlien ((COUNT)~0, PIK_ALIEN);

	Response (sure, ExitConversation);
	Response (never, ExitConversation);
	Response (decide_later, ExitConversation);
}

static void
ZoqFotIntro (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, we_are_vindicator0))
	{
		NPCPhrase (WE_GLAD0);
		NPCPhrase (WE_GLAD1);
		NPCPhrase (WE_GLAD2);
		NPCPhrase (WE_GLAD3);
		NPCPhrase (WE_GLAD4);
		NPCPhrase (WE_GLAD5);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);
		
		DISABLE_PHRASE (we_are_vindicator0);
	}
	else if (PLAYER_SAID (R, your_race))
	{
		NPCPhrase (YEARS_AGO0);
		NPCPhrase (YEARS_AGO1);
		NPCPhrase (YEARS_AGO2);
		NPCPhrase (YEARS_AGO3);
		NPCPhrase (YEARS_AGO4);
		NPCPhrase (YEARS_AGO5);
		NPCPhrase (YEARS_AGO6);
		NPCPhrase (YEARS_AGO7);
		NPCPhrase (YEARS_AGO8);
		NPCPhrase (YEARS_AGO9);
		NPCPhrase (YEARS_AGO10);
		NPCPhrase (YEARS_AGO11);
		NPCPhrase (YEARS_AGO12);
		NPCPhrase (YEARS_AGO13);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien (6, PIK_ALIEN);
		SelectAlien (7, ZOQ_ALIEN);
		SelectAlien (8, PIK_ALIEN);
		SelectAlien (9, ZOQ_ALIEN);
		SelectAlien (10, PIK_ALIEN);
		SelectAlien (11, ZOQ_ALIEN);
		SelectAlien (12, PIK_ALIEN);
		SelectAlien (13, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);
		
		DISABLE_PHRASE (your_race);
	}
	else if (PLAYER_SAID (R, where_from))
	{
		NPCPhrase (TRAVELED_FAR0);
		NPCPhrase (TRAVELED_FAR1);
		NPCPhrase (TRAVELED_FAR2);
		NPCPhrase (TRAVELED_FAR3);
		NPCPhrase (TRAVELED_FAR4);
		NPCPhrase (TRAVELED_FAR5);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		DISABLE_PHRASE (where_from);
	}
	else if (PLAYER_SAID (R, what_emergency))
	{
		NPCPhrase (UNDER_ATTACK0);
		NPCPhrase (UNDER_ATTACK1);
		NPCPhrase (UNDER_ATTACK2);
		NPCPhrase (UNDER_ATTACK3);
		NPCPhrase (UNDER_ATTACK4);
		NPCPhrase (UNDER_ATTACK5);
		NPCPhrase (UNDER_ATTACK6);
		NPCPhrase (UNDER_ATTACK7);
		NPCPhrase (UNDER_ATTACK8);
		NPCPhrase (UNDER_ATTACK9);
		NPCPhrase (UNDER_ATTACK10);
		NPCPhrase (UNDER_ATTACK11);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien (6, PIK_ALIEN);
		SelectAlien (7, ZOQ_ALIEN);
		SelectAlien (8, PIK_ALIEN);
		SelectAlien (9, ZOQ_ALIEN);
		SelectAlien (10, PIK_ALIEN);
		SelectAlien (11, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		DISABLE_PHRASE (what_emergency);
	}
	else if (PLAYER_SAID (R, tough_luck))
	{
		NPCPhrase (NOT_HELPFUL0);
		NPCPhrase (NOT_HELPFUL1);
		NPCPhrase (NOT_HELPFUL2);
		NPCPhrase (NOT_HELPFUL3);
		NPCPhrase (NOT_HELPFUL4);
		NPCPhrase (NOT_HELPFUL5);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		DISABLE_PHRASE (tough_luck);
	}
	else if (PLAYER_SAID (R, what_look_like))
	{
		NPCPhrase (LOOK_LIKE0);
		NPCPhrase (LOOK_LIKE1);
		NPCPhrase (LOOK_LIKE2);
		NPCPhrase (LOOK_LIKE3);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		DISABLE_PHRASE (what_look_like);
	}
	
// fprintf (stderr, "yr = 0x%x wf = 0x%x\n", alien_text[your_race][0], alien_text[where_from][0]);
// fprintf (stderr, "we = 0x%x tl = 0x%x\n", alien_text[what_emergency][0], alien_text[tough_luck][0]);
// fprintf (stderr, "wll = 0x%x\n", what_look_like);
	if (PHRASE_ENABLED (your_race)
			|| PHRASE_ENABLED (where_from)
			|| PHRASE_ENABLED (what_emergency))
	{
		if (PHRASE_ENABLED (your_race))
			 Response (your_race, ZoqFotIntro);
		if (PHRASE_ENABLED (where_from))
			 Response (where_from, ZoqFotIntro);
		if (PHRASE_ENABLED (what_emergency))
			 Response (what_emergency, ZoqFotIntro);
	}
	else
	{
		if (PHRASE_ENABLED (tough_luck))
			 Response (tough_luck, ZoqFotIntro);
		if (PHRASE_ENABLED (what_look_like))
			 Response (what_look_like, ZoqFotIntro);
		Response (all_very_interesting, ExitConversation);
		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			Response (how_can_i_help, FormAlliance);
		}
		else
		{
			Response (how_can_i_help, ExitConversation);
		}
		Response (valuable_info, ExitConversation);
	}
}

static void
AquaintZoqFot (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, which_fot))
	{
		NPCPhrase (HE_IS0);
		NPCPhrase (HE_IS1);
		NPCPhrase (HE_IS2);
		NPCPhrase (HE_IS3);
		NPCPhrase (HE_IS4);
		NPCPhrase (HE_IS5);
		NPCPhrase (HE_IS6);
		NPCPhrase (HE_IS7);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien (6, PIK_ALIEN);
		SelectAlien (7, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		DISABLE_PHRASE (which_fot);
	}
	else if (PLAYER_SAID (R, quiet_toadies))
	{
		NPCPhrase (TOLD_YOU0);
		NPCPhrase (TOLD_YOU1);
		NPCPhrase (TOLD_YOU2);
		NPCPhrase (TOLD_YOU3);
		NPCPhrase (TOLD_YOU4);
		NPCPhrase (TOLD_YOU5);
		NPCPhrase (TOLD_YOU6);
		NPCPhrase (TOLD_YOU7);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien (6, PIK_ALIEN);
		SelectAlien (7, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		DISABLE_PHRASE (quiet_toadies);
	}

	if (PHRASE_ENABLED (we_are_vindicator0))
	{
		UNICODE buf[80];

		GetAllianceName (buf, name_1);
		construct_response (
				shared_phrase_buf,
				we_are_vindicator0,
				buf,
				we_are_vindicator1,
				GLOBAL_SIS (ShipName),
				we_are_vindicator2,
				0
				);
	}

	if (PHRASE_ENABLED (which_fot))
		Response (which_fot, AquaintZoqFot);
	if (PHRASE_ENABLED (we_are_vindicator0))
		DoResponsePhrase (we_are_vindicator0, ZoqFotIntro, shared_phrase_buf);
	if (PHRASE_ENABLED (quiet_toadies))
		Response (quiet_toadies, AquaintZoqFot);
	Response (all_very_interesting, ExitConversation);
	Response (valuable_info, ExitConversation);
}

static void ZoqFotHome (RESPONSE_REF R);

static void
ZoqFotInfo (RESPONSE_REF R)
{
	BYTE InfoLeft;

	if (PLAYER_SAID (R, want_specific_info))
	{
		NPCPhrase (WHAT_SPECIFIC_INFO0);
		NPCPhrase (WHAT_SPECIFIC_INFO1);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);
	}
	else if (PLAYER_SAID (R, what_about_others))
	{
		NPCPhrase (ABOUT_OTHERS0);
		NPCPhrase (ABOUT_OTHERS1);
		NPCPhrase (ABOUT_OTHERS2);
		NPCPhrase (ABOUT_OTHERS3);
		NPCPhrase (ABOUT_OTHERS4);
		NPCPhrase (ABOUT_OTHERS5);
		NPCPhrase (ABOUT_OTHERS6);
		NPCPhrase (ABOUT_OTHERS7);
		NPCPhrase (ABOUT_OTHERS8);
		NPCPhrase (ABOUT_OTHERS9);
		NPCPhrase (ABOUT_OTHERS10);
		NPCPhrase (ABOUT_OTHERS11);
		NPCPhrase (ABOUT_OTHERS12);
		NPCPhrase (ABOUT_OTHERS13);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien (6, PIK_ALIEN);
		SelectAlien (7, ZOQ_ALIEN);
		SelectAlien (8, PIK_ALIEN);
		SelectAlien (9, ZOQ_ALIEN);
		SelectAlien (10, PIK_ALIEN);
		SelectAlien (11, ZOQ_ALIEN);
		SelectAlien (12, PIK_ALIEN);
		SelectAlien (13, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		DISABLE_PHRASE (what_about_others);
	}
	else if (PLAYER_SAID (R, what_about_zebranky))
	{
		NPCPhrase (ABOUT_ZEBRANKY0);
		NPCPhrase (ABOUT_ZEBRANKY1);
		NPCPhrase (ABOUT_ZEBRANKY2);
		NPCPhrase (ABOUT_ZEBRANKY3);
		NPCPhrase (ABOUT_ZEBRANKY4);
		NPCPhrase (ABOUT_ZEBRANKY5);
		NPCPhrase (ABOUT_ZEBRANKY6);
		NPCPhrase (ABOUT_ZEBRANKY7);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien (6, PIK_ALIEN);
		SelectAlien (7, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		DISABLE_PHRASE (what_about_zebranky);
	}
	else if (PLAYER_SAID (R, what_about_stinger))
	{
		NPCPhrase (ABOUT_STINGER0);
		NPCPhrase (ABOUT_STINGER1);
		NPCPhrase (ABOUT_STINGER2);
		NPCPhrase (ABOUT_STINGER3);
		NPCPhrase (ABOUT_STINGER4);
		NPCPhrase (ABOUT_STINGER5);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		DISABLE_PHRASE (what_about_stinger);
	}
	else if (PLAYER_SAID (R, what_about_guy_in_back))
	{
		NPCPhrase (ABOUT_GUY0);
		NPCPhrase (ABOUT_GUY1);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		DISABLE_PHRASE (what_about_guy_in_back);
	}
	else if (PLAYER_SAID (R, what_about_past))
	{
		NPCPhrase (ABOUT_PAST0);
		NPCPhrase (ABOUT_PAST1);
		NPCPhrase (ABOUT_PAST2);
		NPCPhrase (ABOUT_PAST3);
		NPCPhrase (ABOUT_PAST4);
		NPCPhrase (ABOUT_PAST5);
		NPCPhrase (ABOUT_PAST6);
		NPCPhrase (ABOUT_PAST7);
		NPCPhrase (ABOUT_PAST8);
		NPCPhrase (ABOUT_PAST9);
		NPCPhrase (ABOUT_PAST10);
		NPCPhrase (ABOUT_PAST11);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien (6, PIK_ALIEN);
		SelectAlien (7, ZOQ_ALIEN);
		SelectAlien (8, PIK_ALIEN);
		SelectAlien (9, ZOQ_ALIEN);
		SelectAlien (10, PIK_ALIEN);
		SelectAlien (11, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		DISABLE_PHRASE (what_about_past);
	}

	InfoLeft = FALSE;
	if (PHRASE_ENABLED (what_about_others))
	{
		Response (what_about_others, ZoqFotInfo);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_zebranky))
	{
		Response (what_about_zebranky, ZoqFotInfo);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_stinger))
	{
		Response (what_about_stinger, ZoqFotInfo);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_guy_in_back))
	{
		Response (what_about_guy_in_back, ZoqFotInfo);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_past))
	{
		Response (what_about_past, ZoqFotInfo);
		InfoLeft = TRUE;
	}
	Response (enough_info, ZoqFotHome);

	if (!InfoLeft)
	{
		DISABLE_PHRASE (want_specific_info);
	}
}

static void
ZoqFotHome (RESPONSE_REF R)
{
	BYTE NumVisits;

	if (PLAYER_SAID (R, whats_up_homeworld))
	{
		NumVisits = GET_GAME_STATE (ZOQFOT_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (GENERAL_INFO_10);
				NPCPhrase (GENERAL_INFO_11);
				NPCPhrase (GENERAL_INFO_12);
				NPCPhrase (GENERAL_INFO_13);
				SelectAlien (1, ZOQ_ALIEN);
				SelectAlien (2, PIK_ALIEN);
				SelectAlien (3, ZOQ_ALIEN);
				SelectAlien ((COUNT)~0, PIK_ALIEN);
				break;
			case 1:
				NPCPhrase (GENERAL_INFO_20);
				NPCPhrase (GENERAL_INFO_21);
				NPCPhrase (GENERAL_INFO_22);
				NPCPhrase (GENERAL_INFO_23);
				NPCPhrase (GENERAL_INFO_24);
				NPCPhrase (GENERAL_INFO_25);
				NPCPhrase (GENERAL_INFO_26);
				NPCPhrase (GENERAL_INFO_27);
				SelectAlien (1, ZOQ_ALIEN);
				SelectAlien (2, PIK_ALIEN);
				SelectAlien (3, ZOQ_ALIEN);
				SelectAlien (4, PIK_ALIEN);
				SelectAlien (5, ZOQ_ALIEN);
				SelectAlien (6, PIK_ALIEN);
				SelectAlien (7, ZOQ_ALIEN);
				SelectAlien ((COUNT)~0, PIK_ALIEN);
				break;
			case 2:
				NPCPhrase (GENERAL_INFO_30);
				NPCPhrase (GENERAL_INFO_31);
				NPCPhrase (GENERAL_INFO_32);
				NPCPhrase (GENERAL_INFO_33);
				NPCPhrase (GENERAL_INFO_34);
				NPCPhrase (GENERAL_INFO_35);
				SelectAlien (1, ZOQ_ALIEN);
				SelectAlien (2, PIK_ALIEN);
				SelectAlien (3, ZOQ_ALIEN);
				SelectAlien (4, PIK_ALIEN);
				SelectAlien (5, ZOQ_ALIEN);
				SelectAlien ((COUNT)~0, PIK_ALIEN);
				break;
			case 3:
				NPCPhrase (GENERAL_INFO_40);
				NPCPhrase (GENERAL_INFO_41);
				NPCPhrase (GENERAL_INFO_42);
				NPCPhrase (GENERAL_INFO_43);
				NPCPhrase (GENERAL_INFO_44);
				NPCPhrase (GENERAL_INFO_45);
				NPCPhrase (GENERAL_INFO_46);
				NPCPhrase (GENERAL_INFO_47);
				NPCPhrase (GENERAL_INFO_48);
				NPCPhrase (GENERAL_INFO_49);
				NPCPhrase (GENERAL_INFO_410);
				NPCPhrase (GENERAL_INFO_411);
				SelectAlien (1, ZOQ_ALIEN);
				SelectAlien (2, PIK_ALIEN);
				SelectAlien (3, ZOQ_ALIEN);
				SelectAlien (4, PIK_ALIEN);
				SelectAlien (5, ZOQ_ALIEN);
				SelectAlien (6, PIK_ALIEN);
				SelectAlien (7, ZOQ_ALIEN);
				SelectAlien (8, PIK_ALIEN);
				SelectAlien (9, ZOQ_ALIEN);
				SelectAlien (10, PIK_ALIEN);
				SelectAlien (11, ZOQ_ALIEN);
				SelectAlien ((COUNT)~0, PIK_ALIEN);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (ZOQFOT_INFO, NumVisits);

		DISABLE_PHRASE (whats_up_homeworld);
	}
	else if (PLAYER_SAID (R, any_war_news))
	{
#define UTWIG_BUY_TIME (1 << 0)
#define KOHR_AH_WIN (1 << 1)
#define URQUAN_LOSE (1 << 2)
#define KOHR_AH_KILL (1 << 3)
#define KNOW_ALL (UTWIG_BUY_TIME | KOHR_AH_WIN | URQUAN_LOSE | KOHR_AH_KILL)
		BYTE KnowMask;

		NumVisits = GET_GAME_STATE (UTWIG_SUPOX_MISSION);
		KnowMask = GET_GAME_STATE (ZOQFOT_KNOW_MASK);
		if (!(KnowMask & KOHR_AH_KILL) && GET_GAME_STATE (KOHR_AH_FRENZY))
		{
			NPCPhrase (KOHRAH_FRENZY0);
			NPCPhrase (KOHRAH_FRENZY1);
			NPCPhrase (KOHRAH_FRENZY2);
			NPCPhrase (KOHRAH_FRENZY3);
			NPCPhrase (KOHRAH_FRENZY4);
			NPCPhrase (KOHRAH_FRENZY5);
			NPCPhrase (KOHRAH_FRENZY6);
			NPCPhrase (KOHRAH_FRENZY7);
			NPCPhrase (KOHRAH_FRENZY8);
			NPCPhrase (KOHRAH_FRENZY9);
			NPCPhrase (KOHRAH_FRENZY10);
			NPCPhrase (KOHRAH_FRENZY11);
			SelectAlien (1, ZOQ_ALIEN);
			SelectAlien (2, PIK_ALIEN);
			SelectAlien (3, ZOQ_ALIEN);
			SelectAlien (4, PIK_ALIEN);
			SelectAlien (5, ZOQ_ALIEN);
			SelectAlien (6, PIK_ALIEN);
			SelectAlien (7, ZOQ_ALIEN);
			SelectAlien (8, PIK_ALIEN);
			SelectAlien (9, ZOQ_ALIEN);
			SelectAlien (10, PIK_ALIEN);
			SelectAlien (11, ZOQ_ALIEN);
			SelectAlien ((COUNT)~0, PIK_ALIEN);

			KnowMask = KNOW_ALL;
		}
		else if (!(KnowMask & UTWIG_BUY_TIME)
				&& NumVisits > 0 && NumVisits < 5)
		{
			NPCPhrase (UTWIG_DELAY0);
			NPCPhrase (UTWIG_DELAY1);
			NPCPhrase (UTWIG_DELAY2);
			NPCPhrase (UTWIG_DELAY3);
			NPCPhrase (UTWIG_DELAY4);
			NPCPhrase (UTWIG_DELAY5);
			NPCPhrase (UTWIG_DELAY6);
			NPCPhrase (UTWIG_DELAY7);
			NPCPhrase (UTWIG_DELAY8);
			NPCPhrase (UTWIG_DELAY9);
			NPCPhrase (UTWIG_DELAY10);
			NPCPhrase (UTWIG_DELAY11);
			NPCPhrase (UTWIG_DELAY12);
			NPCPhrase (UTWIG_DELAY13);
			SelectAlien (1, ZOQ_ALIEN);
			SelectAlien (2, PIK_ALIEN);
			SelectAlien (3, ZOQ_ALIEN);
			SelectAlien (4, PIK_ALIEN);
			SelectAlien (5, ZOQ_ALIEN);
			SelectAlien (6, PIK_ALIEN);
			SelectAlien (7, ZOQ_ALIEN);
			SelectAlien (8, PIK_ALIEN);
			SelectAlien (9, ZOQ_ALIEN);
			SelectAlien (10, PIK_ALIEN);
			SelectAlien (11, ZOQ_ALIEN);
			SelectAlien (12, PIK_ALIEN);
			SelectAlien (13, ZOQ_ALIEN);
			SelectAlien ((COUNT)~0, PIK_ALIEN);

			KnowMask |= UTWIG_BUY_TIME;
		}
		else
		{
			SIZE i;

			i = START_YEAR + YEARS_TO_KOHRAH_VICTORY;
			if (NumVisits)
				++i;
			if ((i -= GLOBAL (GameClock.year_index)) == 1
					&& GLOBAL (GameClock.month_index) > 2)
				i = 0;
			if (!(KnowMask & URQUAN_LOSE) && i <= 0)
			{
				NPCPhrase (URQUAN_NEARLY_GONE0);
				NPCPhrase (URQUAN_NEARLY_GONE1);
				NPCPhrase (URQUAN_NEARLY_GONE2);
				NPCPhrase (URQUAN_NEARLY_GONE3);
				NPCPhrase (URQUAN_NEARLY_GONE4);
				NPCPhrase (URQUAN_NEARLY_GONE5);
				SelectAlien (1, ZOQ_ALIEN);
				SelectAlien (2, PIK_ALIEN);
				SelectAlien (3, ZOQ_ALIEN);
				SelectAlien (4, PIK_ALIEN);
				SelectAlien (5, ZOQ_ALIEN);
				SelectAlien ((COUNT)~0, PIK_ALIEN);

				KnowMask |= KOHR_AH_WIN | URQUAN_LOSE;
			}
			else if (!(KnowMask & KOHR_AH_WIN) && i == 1)
			{
				NPCPhrase (KOHRAH_WINNING0);
				NPCPhrase (KOHRAH_WINNING1);
				NPCPhrase (KOHRAH_WINNING2);
				NPCPhrase (KOHRAH_WINNING3);
				NPCPhrase (KOHRAH_WINNING4);
				NPCPhrase (KOHRAH_WINNING5);
				NPCPhrase (KOHRAH_WINNING6);
				NPCPhrase (KOHRAH_WINNING7);
				NPCPhrase (KOHRAH_WINNING8);
				NPCPhrase (KOHRAH_WINNING9);
				SelectAlien (1, ZOQ_ALIEN);
				SelectAlien (2, PIK_ALIEN);
				SelectAlien (3, ZOQ_ALIEN);
				SelectAlien (4, PIK_ALIEN);
				SelectAlien (5, ZOQ_ALIEN);
				SelectAlien (6, PIK_ALIEN);
				SelectAlien (7, ZOQ_ALIEN);
				SelectAlien (8, PIK_ALIEN);
				SelectAlien (9, ZOQ_ALIEN);
				SelectAlien ((COUNT)~0, PIK_ALIEN);

				KnowMask |= KOHR_AH_WIN;
			}
			else
			{
				NPCPhrase (NO_WAR_NEWS0);
				NPCPhrase (NO_WAR_NEWS1);
				SelectAlien (1, ZOQ_ALIEN);
				SelectAlien ((COUNT)~0, PIK_ALIEN);
			}
		}
		SET_GAME_STATE (ZOQFOT_KNOW_MASK, KnowMask);

		DISABLE_PHRASE (any_war_news);
	}
	else if (PLAYER_SAID (R, i_want_alliance))
	{
		NPCPhrase (GOOD0);
		NPCPhrase (GOOD1);
		NPCPhrase (GOOD2);
		NPCPhrase (GOOD3);
		NPCPhrase (GOOD4);
		NPCPhrase (GOOD5);
		NPCPhrase (GOOD6);
		NPCPhrase (GOOD7);
		NPCPhrase (GOOD8);
		NPCPhrase (GOOD9);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien (6, PIK_ALIEN);
		SelectAlien (7, ZOQ_ALIEN);
		SelectAlien (8, PIK_ALIEN);
		SelectAlien (9, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		ActivateStarShip (ZOQFOTPIK_SHIP, 0);
		AddEvent (RELATIVE_EVENT, 3, 0, 0, ZOQFOT_DISTRESS_EVENT);
	}
	else if (PLAYER_SAID (R, enough_info))
	{
		NPCPhrase (OK_ENOUGH_INFO);
		SelectAlien ((COUNT)~0, ZOQ_ALIEN);
	}

	if (PHRASE_ENABLED (whats_up_homeworld))
		Response (whats_up_homeworld, ZoqFotHome);
	if (PHRASE_ENABLED (any_war_news))
		Response (any_war_news, ZoqFotHome);
	if (!(ActivateStarShip (ZOQFOTPIK_SHIP, CHECK_ALLIANCE) & GOOD_GUY))
		Response (i_want_alliance, ZoqFotHome);
	else if (PHRASE_ENABLED (want_specific_info))
	{
		Response (want_specific_info, ZoqFotInfo);
	}
	Response (bye_homeworld, ExitConversation);
}

static void
Intro (void)
{
	BYTE NumVisits;

	if (LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
#ifdef NOT_YET
		NPCPhrase (OUT_TAKES0);
		NPCPhrase (OUT_TAKES1);
		NPCPhrase (OUT_TAKES2);
		NPCPhrase (OUT_TAKES3);
		NPCPhrase (OUT_TAKES4);
		NPCPhrase (OUT_TAKES5);
		NPCPhrase (OUT_TAKES6);
		NPCPhrase (OUT_TAKES7);
		NPCPhrase (OUT_TAKES8);
		NPCPhrase (OUT_TAKES9);
		NPCPhrase (OUT_TAKES10);
		NPCPhrase (OUT_TAKES11);
		NPCPhrase (OUT_TAKES12);
		NPCPhrase (OUT_TAKES13);
		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien (6, PIK_ALIEN);
		SelectAlien (7, ZOQ_ALIEN);
		SelectAlien (8, PIK_ALIEN);
		SelectAlien (9, ZOQ_ALIEN);
		SelectAlien (10, PIK_ALIEN);
		SelectAlien (11, ZOQ_ALIEN);
		SelectAlien (12, PIK_ALIEN);
		SelectAlien (13, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);
#else /* YET */
		NPCPhrase (WE_ARE0);
		NPCPhrase (WE_ARE1);
		NPCPhrase (WE_ARE2);
		NPCPhrase (WE_ARE3);
		NPCPhrase (WE_ARE4);
		NPCPhrase (WE_ARE5);
		NPCPhrase (WE_ARE6);
		NPCPhrase (WE_ARE7);
		NPCPhrase (SCOUT_HELLO0);
		NPCPhrase (SCOUT_HELLO1);
		NPCPhrase (SCOUT_HELLO2);
		NPCPhrase (SCOUT_HELLO3);

		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien (6, PIK_ALIEN);
		SelectAlien (7, ZOQ_ALIEN);
		SelectAlien (8, PIK_ALIEN);
		SelectAlien (9, ZOQ_ALIEN);
		SelectAlien (10, PIK_ALIEN);
		SelectAlien (11, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);
#endif /* NOT_YET */
		SET_GAME_STATE (BATTLE_SEGUE, 0);
		goto ExitIntro;
	}

	if (GET_GAME_STATE (ZOQFOT_HOSTILE))
	{
		NumVisits = GET_GAME_STATE (ZOQFOT_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (HOSTILE_HELLO_10);
				NPCPhrase (HOSTILE_HELLO_11);
				SelectAlien (1, ZOQ_ALIEN);
				SelectAlien ((COUNT)~0, PIK_ALIEN);
				break;
			case 1:
				NPCPhrase (HOSTILE_HELLO_20);
				NPCPhrase (HOSTILE_HELLO_21);
				NPCPhrase (HOSTILE_HELLO_22);
				NPCPhrase (HOSTILE_HELLO_23);
				NPCPhrase (HOSTILE_HELLO_24);
				NPCPhrase (HOSTILE_HELLO_25);
				SelectAlien (1, ZOQ_ALIEN);
				SelectAlien (2, PIK_ALIEN);
				SelectAlien (3, ZOQ_ALIEN);
				SelectAlien (4, PIK_ALIEN);
				SelectAlien (5, ZOQ_ALIEN);
				SelectAlien ((COUNT)~0, PIK_ALIEN);
				break;
			case 2:
				NPCPhrase (HOSTILE_HELLO_30);
				NPCPhrase (HOSTILE_HELLO_31);
				SelectAlien (1, ZOQ_ALIEN);
				SelectAlien ((COUNT)~0, PIK_ALIEN);
				break;
			case 3:
				NPCPhrase (HOSTILE_HELLO_40);
				NPCPhrase (HOSTILE_HELLO_41);
				SelectAlien (1, ZOQ_ALIEN);
				SelectAlien ((COUNT)~0, PIK_ALIEN);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (ZOQFOT_HOME_VISITS, NumVisits);

		SET_GAME_STATE (BATTLE_SEGUE, 1);
	}
	else if (!GET_GAME_STATE (MET_ZOQFOT))
	{
		SET_GAME_STATE (MET_ZOQFOT, 1);

		NPCPhrase (WE_ARE0);
		NPCPhrase (WE_ARE1);
		NPCPhrase (WE_ARE2);
		NPCPhrase (WE_ARE3);
		NPCPhrase (WE_ARE4);
		NPCPhrase (WE_ARE5);
		NPCPhrase (WE_ARE6);
		NPCPhrase (WE_ARE7);

		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NPCPhrase (INIT_HOME_HELLO0);
			NPCPhrase (INIT_HOME_HELLO1);
			NPCPhrase (INIT_HOME_HELLO2);
			NPCPhrase (INIT_HOME_HELLO3);
		}
		else
		{
			NPCPhrase (SCOUT_HELLO0);
			NPCPhrase (SCOUT_HELLO1);
			NPCPhrase (SCOUT_HELLO2);
			NPCPhrase (SCOUT_HELLO3);
		}

		SelectAlien (1, ZOQ_ALIEN);
		SelectAlien (2, PIK_ALIEN);
		SelectAlien (3, ZOQ_ALIEN);
		SelectAlien (4, PIK_ALIEN);
		SelectAlien (5, ZOQ_ALIEN);
		SelectAlien (6, PIK_ALIEN);
		SelectAlien (7, ZOQ_ALIEN);
		SelectAlien (8, PIK_ALIEN);
		SelectAlien (9, ZOQ_ALIEN);
		SelectAlien (10, PIK_ALIEN);
		SelectAlien (11, ZOQ_ALIEN);
		SelectAlien ((COUNT)~0, PIK_ALIEN);

		AquaintZoqFot (0);
	}
	else
	{
		if (GET_GAME_STATE (ZOQFOT_DISTRESS))
		{
#define MAX_ZFP_SHIPS 4
			NPCPhrase (THANKS_FOR_RESCUE0);
			NPCPhrase (THANKS_FOR_RESCUE1);
			NPCPhrase (THANKS_FOR_RESCUE2);
			NPCPhrase (THANKS_FOR_RESCUE3);
			NPCPhrase (THANKS_FOR_RESCUE4);
			NPCPhrase (THANKS_FOR_RESCUE5);
			NPCPhrase (THANKS_FOR_RESCUE6);
			NPCPhrase (THANKS_FOR_RESCUE7);
			NPCPhrase (THANKS_FOR_RESCUE8);
			NPCPhrase (THANKS_FOR_RESCUE9);
			NPCPhrase (THANKS_FOR_RESCUE10);
			NPCPhrase (THANKS_FOR_RESCUE11);
			SelectAlien (1, ZOQ_ALIEN);
			SelectAlien (2, PIK_ALIEN);
			SelectAlien (3, ZOQ_ALIEN);
			SelectAlien (4, PIK_ALIEN);
			SelectAlien (5, ZOQ_ALIEN);
			SelectAlien (6, PIK_ALIEN);
			SelectAlien (7, ZOQ_ALIEN);
			SelectAlien (8, PIK_ALIEN);
			SelectAlien (9, ZOQ_ALIEN);
			SelectAlien (10, PIK_ALIEN);
			SelectAlien (11, ZOQ_ALIEN);
			SelectAlien ((COUNT)~0, PIK_ALIEN);

			SET_GAME_STATE (ZOQFOT_DISTRESS, 0);
			ActivateStarShip (ZOQFOTPIK_SHIP, MAX_ZFP_SHIPS);
		}
		else
		{
			NumVisits = GET_GAME_STATE (ZOQFOT_HOME_VISITS);
			if (!(ActivateStarShip (ZOQFOTPIK_SHIP, CHECK_ALLIANCE) & GOOD_GUY))
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase (NEUTRAL_HOME_HELLO_10);
						NPCPhrase (NEUTRAL_HOME_HELLO_11);
						NPCPhrase (NEUTRAL_HOME_HELLO_12);
						NPCPhrase (NEUTRAL_HOME_HELLO_13);
						break;
					case 1:
						NPCPhrase (NEUTRAL_HOME_HELLO_20);
						NPCPhrase (NEUTRAL_HOME_HELLO_21);
						NPCPhrase (NEUTRAL_HOME_HELLO_22);
						NPCPhrase (NEUTRAL_HOME_HELLO_23);
						--NumVisits;
						break;
				}
				SelectAlien (1, ZOQ_ALIEN);
				SelectAlien (2, PIK_ALIEN);
				SelectAlien (3, ZOQ_ALIEN);
				SelectAlien ((COUNT)~0, PIK_ALIEN);
			}
			else
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase (ALLIED_HOME_HELLO_10);
						NPCPhrase (ALLIED_HOME_HELLO_11);
						NPCPhrase (ALLIED_HOME_HELLO_12);
						NPCPhrase (ALLIED_HOME_HELLO_13);
						SelectAlien (1, ZOQ_ALIEN);
						SelectAlien (2, PIK_ALIEN);
						SelectAlien (3, ZOQ_ALIEN);
						SelectAlien ((COUNT)~0, PIK_ALIEN);
						break;
					case 1:
						NPCPhrase (ALLIED_HOME_HELLO_20);
						NPCPhrase (ALLIED_HOME_HELLO_21);
						NPCPhrase (ALLIED_HOME_HELLO_22);
						NPCPhrase (ALLIED_HOME_HELLO_23);
						NPCPhrase (ALLIED_HOME_HELLO_24);
						NPCPhrase (ALLIED_HOME_HELLO_25);
						NPCPhrase (ALLIED_HOME_HELLO_26);
						NPCPhrase (ALLIED_HOME_HELLO_27);
						SelectAlien (1, ZOQ_ALIEN);
						SelectAlien (2, PIK_ALIEN);
						SelectAlien (3, ZOQ_ALIEN);
						SelectAlien (4, PIK_ALIEN);
						SelectAlien (5, ZOQ_ALIEN);
						SelectAlien (6, PIK_ALIEN);
						SelectAlien (7, ZOQ_ALIEN);
						SelectAlien ((COUNT)~0, PIK_ALIEN);
						break;
					case 2:
						NPCPhrase (ALLIED_HOME_HELLO_30);
						NPCPhrase (ALLIED_HOME_HELLO_31);
						SelectAlien (1, ZOQ_ALIEN);
						SelectAlien ((COUNT)~0, PIK_ALIEN);
						break;
					case 3:
						NPCPhrase (ALLIED_HOME_HELLO_40);
						NPCPhrase (ALLIED_HOME_HELLO_41);
						SelectAlien (1, ZOQ_ALIEN);
						SelectAlien ((COUNT)~0, PIK_ALIEN);
						--NumVisits;
						break;
				}
			}
			SET_GAME_STATE (ZOQFOT_HOME_VISITS, NumVisits);
		}

		ZoqFotHome (0);
	}
ExitIntro:
	;
}

static COUNT
uninit_zoqfot (void)
{
	return (0);
}

LOCDATAPTR
init_zoqfot_comm (void)
{
	LOCDATAPTR retval;

	zoqfot_desc.init_encounter_func = Intro;
	zoqfot_desc.uninit_encounter_func = uninit_zoqfot;

	zoqfot_desc.AlienTextTemplate.align = ALIGN_CENTER;
	zoqfot_desc.AlienTextWidth = (SIS_TEXT_WIDTH >> 1) - TEXT_X_OFFS;

	if ((ActivateStarShip (ZOQFOTPIK_SHIP, CHECK_ALLIANCE) & GOOD_GUY)
			|| LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		SET_GAME_STATE (BATTLE_SEGUE, 0);
	}
	else
	{
		SET_GAME_STATE (BATTLE_SEGUE, 1);
	}
	
	retval = &zoqfot_desc;

	return (retval);
}

