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
#include "comm/chmmr/resinst.h"
#include "starcon.h"
#include "commglue.h"
#include "comm/chmmr/strings.h"

static LOCDATA chmmr_desc =
{
	NULL_PTR, /* init_encounter_func */
	NULL_PTR, /* uninit_encounter_func */
	(FRAME)CHMMR_PMAP_ANIM, /* AlienFrame */
	(FONT)CHMMR_FONT, /* AlienFont */
	WHITE_COLOR, /* AlienTextFColor */
	BLACK_COLOR, /* AlienTextBColor */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	(COLORMAP)CHMMR_COLOR_MAP, /* AlienColorMap */
	CHMMR_MUSIC, /* AlienSong */
	CHMMR_CONVERSATION_PHRASES, /* PlayerPhrases */
	6, /* NumAnimations */
	{
		{
			12, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			6, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
		},
		{
			17, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			6, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
		},
		{
			22, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			6, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
		},
		{
			27, /* StartIndex */
			20, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			6, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
		},
		{
			47, /* StartIndex */
			14, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			6, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
		},
		{
			61, /* StartIndex */
			24, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			6, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
		},
	},
	{
		0, /* StartIndex */
		0, /* NumFrames */
		0, /* AnimFlags */
		0, 0, /* FrameRate */
		0, 0, /* RestartRate */
	},
	{
		1, /* StartIndex */
		11, /* NumFrames */
		0, /* AnimFlags */
		1, 0, /* FrameRate */
		1, 0, /* RestartRate */
	},
};

static void
ExitConversation (RESPONSE_REF R)
{
	SET_GAME_STATE (BATTLE_SEGUE, 0);

	if (PLAYER_SAID (R, bye))
		NPCPhrase (GOODBYE);
	else if (PLAYER_SAID (R, bye_shielded))
		NPCPhrase (GOODBYE_SHIELDED);
	else if (PLAYER_SAID (R, bye_after_bomb))
		NPCPhrase (GOODBYE_AFTER_BOMB);
	else if (PLAYER_SAID (R, proceed))
	{
		BYTE i;

		NPCPhrase (TAKE_2_WEEKS);

		ActivateStarShip (CHMMR_SHIP, 0);

		SET_GAME_STATE (CHMMR_HOME_VISITS, 0);
		SET_GAME_STATE (CHMMR_STACK, 0);
		SET_GAME_STATE (CHMMR_BOMB_STATE, 2);
		SET_GAME_STATE (UTWIG_BOMB_ON_SHIP, 0);
		GLOBAL_SIS (ResUnits) = 1000000L;
		GLOBAL_SIS (FuelOnBoard) = FUEL_RESERVE;
		GLOBAL_SIS (CrewEnlisted) = 0;
		GLOBAL_SIS (NumLanders) = 0;
		GLOBAL (ModuleCost[PLANET_LANDER]) = 0;
		GLOBAL (ModuleCost[STORAGE_BAY]) = 0;
		GLOBAL_SIS (TotalElementMass) = 0;
		for (i = 0; i < NUM_ELEMENT_CATEGORIES; ++i)
			GLOBAL_SIS (ElementAmounts[i]) = 0;
		for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
			GLOBAL_SIS (DriveSlots[i]) = FUSION_THRUSTER;
		for (i = 0; i < NUM_JET_SLOTS; ++i)
			GLOBAL_SIS (JetSlots[i]) = TURNING_JETS;
		for (i = NUM_BOMB_MODULES; i < NUM_MODULE_SLOTS; ++i)
			GLOBAL_SIS (ModuleSlots[i]) = EMPTY_SLOT + 2;
		GLOBAL_SIS (ModuleSlots[0]) = BOMB_MODULE_4;
		GLOBAL_SIS (ModuleSlots[1]) = BOMB_MODULE_5;
		GLOBAL_SIS (ModuleSlots[2]) = BOMB_MODULE_3;
		GLOBAL_SIS (ModuleSlots[3]) = BOMB_MODULE_1;
		GLOBAL_SIS (ModuleSlots[4]) = BOMB_MODULE_0;
		GLOBAL_SIS (ModuleSlots[5]) = BOMB_MODULE_1;
		GLOBAL_SIS (ModuleSlots[6]) = BOMB_MODULE_3;
		GLOBAL_SIS (ModuleSlots[7]) = BOMB_MODULE_4;
		GLOBAL_SIS (ModuleSlots[8]) = BOMB_MODULE_5;
		GLOBAL_SIS (ModuleSlots[9]) = BOMB_MODULE_2;

#define EARTH_INDEX 2 /* earth is 3rd planet --> 3 - 1 = 2 */
		GLOBAL_SIS (log_x) = UNIVERSE_TO_LOGX (SOL_X);
		GLOBAL_SIS (log_y) = UNIVERSE_TO_LOGY (SOL_Y);
		GLOBAL (ShipStamp.frame) =
				(FRAME)MAKE_DWORD (1, EARTH_INDEX + 1);
			/* Magic numbers for Earth */
		GLOBAL (ip_location.x) = -725;
		GLOBAL (ip_location.y) = 597;
		GLOBAL (ShipStamp.origin.x) = 71;
		GLOBAL (ShipStamp.origin.y) = 97;
	}
}

static void
NotReady (RESPONSE_REF R)
{
	if (R == 0)
		NPCPhrase (RETURN_WHEN_READY);
	else if (PLAYER_SAID (R, further_assistance))
	{
		NPCPhrase (NO_FURTHER_ASSISTANCE);

		DISABLE_PHRASE (further_assistance);
	}
	else if (PLAYER_SAID (R, tech_help))
	{
		NPCPhrase (USE_OUR_SHIPS_BEFORE);

		ActivateStarShip (CHMMR_SHIP, 0);
	}
	else if (PLAYER_SAID (R, where_weapon))
	{
		NPCPhrase (PRECURSOR_WEAPON);

		DISABLE_PHRASE (where_weapon);
	}
	else if (PLAYER_SAID (R, where_distraction))
	{
		NPCPhrase (PSYCHIC_WEAPONRY);

		DISABLE_PHRASE (where_distraction);
	}

	if (!(ActivateStarShip (CHMMR_SHIP, CHECK_ALLIANCE) & GOOD_GUY))
		Response (tech_help, NotReady);
	else if (PHRASE_ENABLED (further_assistance))
		Response (further_assistance, NotReady);
	if (PHRASE_ENABLED (where_weapon) && !GET_GAME_STATE (UTWIG_BOMB_ON_SHIP))
		Response (where_weapon, NotReady);
	if (PHRASE_ENABLED (where_distraction) && !GET_GAME_STATE (TALKING_PET_ON_SHIP))
		Response (where_distraction, NotReady);
	Response (bye, ExitConversation);
}

static void
ImproveBomb (RESPONSE_REF R)
{
	if (R == 0)
		NPCPhrase (WE_WILL_IMPROVE_BOMB);
	else if (PLAYER_SAID (R, what_now))
	{
		NPCPhrase (MODIFY_VESSEL);

		DISABLE_PHRASE (what_now);
	}
	else if (PLAYER_SAID (R, wont_hurt_my_ship))
	{
		NPCPhrase (WILL_DESTROY_IT);

		DISABLE_PHRASE (wont_hurt_my_ship);
	}
	else if (PLAYER_SAID (R, bummer_about_my_ship))
	{
		NPCPhrase (DEAD_SILENCE);

		DISABLE_PHRASE (bummer_about_my_ship);
	}
	else if (PLAYER_SAID (R, other_assistance))
	{
		NPCPhrase (USE_OUR_SHIPS_AFTER);

		ActivateStarShip (CHMMR_SHIP, 0);
	}

	if (PHRASE_ENABLED (what_now))
		Response (what_now, ImproveBomb);
	else if (PHRASE_ENABLED (wont_hurt_my_ship))
		Response (wont_hurt_my_ship, ImproveBomb);
	else if (PHRASE_ENABLED (bummer_about_my_ship))
		Response (bummer_about_my_ship, ImproveBomb);
	if (!(ActivateStarShip (CHMMR_SHIP, CHECK_ALLIANCE) & GOOD_GUY))
		Response (other_assistance, ImproveBomb);
	Response (proceed, ExitConversation);
}

static void
ChmmrFree (RESPONSE_REF R)
{
	if (R == 0
			|| PLAYER_SAID (R, i_am_captain0)
			|| PLAYER_SAID (R, i_am_savior)
			|| PLAYER_SAID (R, i_am_silly))
	{
		NPCPhrase (WHY_HAVE_YOU_FREED_US);
		AlienTalkSegue ((COUNT)~0);
		SET_GAME_STATE (CHMMR_EMERGING, 0);

		Response (serious_1, ChmmrFree);
		Response (serious_2, ChmmrFree);
		Response (silly, ChmmrFree);
	}
	else
	{
		NPCPhrase (WILL_HELP_ANALYZE_LOGS);

		if (GET_GAME_STATE (AWARE_OF_SAMATRA))
			NPCPhrase (YOU_KNOW_SAMATRA);
		else
		{
			NPCPhrase (DONT_KNOW_ABOUT_SAMATRA);

			SET_GAME_STATE (AWARE_OF_SAMATRA, 1);
		}

		if (GET_GAME_STATE (TALKING_PET_ON_SHIP))
			NPCPhrase (HAVE_TALKING_PET);
		else
			NPCPhrase (NEED_DISTRACTION);

		if (GET_GAME_STATE (UTWIG_BOMB_ON_SHIP))
			NPCPhrase (HAVE_BOMB);
		else
			NPCPhrase (NEED_WEAPON);

		if (!GET_GAME_STATE (TALKING_PET_ON_SHIP)
				|| !GET_GAME_STATE (UTWIG_BOMB_ON_SHIP))
			NotReady ((RESPONSE_REF)0);
		else
			ImproveBomb ((RESPONSE_REF)0);
	}
}

static void ChmmrShielded (RESPONSE_REF R);

static void
ChmmrAdvice (RESPONSE_REF R)
{
	BYTE AdviceLeft;

	if (PLAYER_SAID (R, need_advice))
		NPCPhrase (WHAT_ADVICE);
	else if (PLAYER_SAID (R, how_defeat_urquan))
	{
		NPCPhrase (DEFEAT_LIKE_SO);

		SET_GAME_STATE (CHMMR_BOMB_STATE, 1);
		DISABLE_PHRASE (how_defeat_urquan);
	}
	else if (PLAYER_SAID (R, what_about_tpet))
	{
		NPCPhrase (SCARY_BUT_USEFUL);

		DISABLE_PHRASE (what_about_tpet);
	}
	else if (PLAYER_SAID (R, what_about_bomb))
	{
		NPCPhrase (ABOUT_BOMB);

		DISABLE_PHRASE (what_about_bomb);
	}
	else if (PLAYER_SAID (R, what_about_sun_device))
	{
		NPCPhrase (ABOUT_SUN_DEVICE);

		DISABLE_PHRASE (what_about_sun_device);
	}
	else if (PLAYER_SAID (R, what_about_samatra))
	{
		NPCPhrase (ABOUT_SUN_DEVICE);

		DISABLE_PHRASE (what_about_samatra);
	}

	AdviceLeft = 0;
	if (PHRASE_ENABLED (how_defeat_urquan))
	{
		Response (how_defeat_urquan, ChmmrAdvice);
		AdviceLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_tpet) && GET_GAME_STATE (TALKING_PET_ON_SHIP))
	{
		Response (what_about_tpet, ChmmrAdvice);
		AdviceLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_bomb) && GET_GAME_STATE (UTWIG_BOMB_ON_SHIP))
	{
		Response (what_about_bomb, ChmmrAdvice);
		AdviceLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_sun_device) && GET_GAME_STATE (SUN_DEVICE_ON_SHIP))
	{
		Response (what_about_sun_device, ChmmrAdvice);
		AdviceLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_samatra) && GET_GAME_STATE (AWARE_OF_SAMATRA))
	{
		Response (what_about_samatra, ChmmrAdvice);
		AdviceLeft = TRUE;
	}
	Response (enough_advice, ChmmrShielded);

	if (!AdviceLeft)
		DISABLE_PHRASE (need_advice);
}

static void
ChmmrShielded (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, find_out_whats_up))
	{
		NPCPhrase (HYBRID_PROCESS);

		DISABLE_PHRASE (find_out_whats_up);
	}
	else if (PLAYER_SAID (R, need_help))
	{
		NPCPhrase (CANT_HELP);

		SET_GAME_STATE (CHMMR_STACK, 1);
	}
	else if (PLAYER_SAID (R, why_no_help))
	{
		NPCPhrase (LONG_TIME);

		SET_GAME_STATE (CHMMR_STACK, 2);
	}
	else if (PLAYER_SAID (R, what_if_more_energy))
	{
		NPCPhrase (DANGER_TO_US);

		SET_GAME_STATE (CHMMR_STACK, 3);
	}
	else if (PLAYER_SAID (R, enough_advice))
		NPCPhrase (OK_ENOUGH_ADVICE);

	switch (GET_GAME_STATE (CHMMR_STACK))
	{
		case 0:
			Response (need_help, ChmmrShielded);
			break;
		case 1:
			Response (why_no_help, ChmmrShielded);
			break;
		case 2:
			Response (what_if_more_energy, ChmmrShielded);
			break;
	}
	if (PHRASE_ENABLED (find_out_whats_up))
		Response (find_out_whats_up, ChmmrShielded);
	if (PHRASE_ENABLED (need_advice))
	{
		Response (need_advice, ChmmrAdvice);
	}
	Response (bye_shielded, ExitConversation);
}

static void
AfterBomb (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, whats_up_after_bomb))
	{
		if (GET_GAME_STATE (CHMMR_STACK))
			NPCPhrase (GENERAL_INFO_AFTER_BOMB_2);
		else
		{
			NPCPhrase (GENERAL_INFO_AFTER_BOMB_1);

			SET_GAME_STATE (CHMMR_STACK, 1);
		}

		DISABLE_PHRASE (whats_up_after_bomb);
	}
	else if (PLAYER_SAID (R, what_do_after_bomb))
	{
		NPCPhrase (DO_AFTER_BOMB);

		DISABLE_PHRASE (what_do_after_bomb);
	}

	if (PHRASE_ENABLED (whats_up_after_bomb))
		Response (whats_up_after_bomb, AfterBomb);
	if (PHRASE_ENABLED (what_do_after_bomb))
		Response (what_do_after_bomb, AfterBomb);
	Response (bye_after_bomb, ExitConversation);
}

static void
Intro (void)
{
	BYTE NumVisits;

	if (GET_GAME_STATE (CHMMR_BOMB_STATE) >= 2)
	{
		NumVisits = GET_GAME_STATE (CHMMR_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (HELLO_AFTER_BOMB_1);
				break;
			case 1:
				NPCPhrase (HELLO_AFTER_BOMB_2);
				break;
		}
		SET_GAME_STATE (CHMMR_HOME_VISITS, NumVisits);

		AfterBomb ((RESPONSE_REF)0);
	}
	else if (GET_GAME_STATE (CHMMR_UNLEASHED))
	{
		if (!GET_GAME_STATE (TALKING_PET_ON_SHIP)
				|| !GET_GAME_STATE (UTWIG_BOMB_ON_SHIP))
			NotReady ((RESPONSE_REF)0);
		else
		{
			NPCPhrase (YOU_ARE_READY);

			ImproveBomb ((RESPONSE_REF)0);
		}
	}
	else
	{
		NumVisits = GET_GAME_STATE (CHMMR_HOME_VISITS);
		if (!GET_GAME_STATE (CHMMR_EMERGING))
		{
			CommData.AlienColorMap = SetAbsColorMapIndex (
					CommData.AlienColorMap, 1
					);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (WHY_YOU_HERE_1);
					break;
				case 1:
					NPCPhrase (WHY_YOU_HERE_2);
					break;
				case 2:
					NPCPhrase (WHY_YOU_HERE_3);
					break;
				case 3:
					NPCPhrase (WHY_YOU_HERE_4);
					--NumVisits;
					break;
			}

			ChmmrShielded ((RESPONSE_REF)0);
		}
		else
		{
			NPCPhrase (WE_ARE_FREE);

			if (NumVisits)
			{
				ChmmrFree ((RESPONSE_REF)0);
				NumVisits = 0;
			}
			else
			{
				NPCPhrase (WHO_ARE_YOU);

				construct_response (shared_phrase_buf,
						i_am_captain0,
							(vararg_dcl)GLOBAL_SIS (CommanderName),
						i_am_captain1,
						GLOBAL_SIS (ShipName),
						i_am_captain2,
						0);
				DoResponsePhrase (i_am_captain0, ChmmrFree, shared_phrase_buf);
				Response (i_am_savior, ChmmrFree);
				Response (i_am_silly, ChmmrFree);
			}

			SET_GAME_STATE (CHMMR_UNLEASHED, 1);
		}
		SET_GAME_STATE (CHMMR_HOME_VISITS, NumVisits);
	}
}

static COUNT
uninit_chmmr (void)
{
	return (0);
}

LOCDATAPTR
init_chmmr_comm (void)
{
	LOCDATAPTR retval;

	chmmr_desc.init_encounter_func = Intro;
	chmmr_desc.uninit_encounter_func = uninit_chmmr;

	chmmr_desc.AlienTextTemplate.baseline.x =
			TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	chmmr_desc.AlienTextTemplate.baseline.y = 0;
	chmmr_desc.AlienTextTemplate.align = ALIGN_CENTER;
	chmmr_desc.AlienTextWidth = SIS_TEXT_WIDTH - 16;

	SET_GAME_STATE (BATTLE_SEGUE, 0);
	retval = &chmmr_desc;

	return (retval);
}
