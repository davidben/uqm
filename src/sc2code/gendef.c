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

#include "encount.h"

PLAN_GEN_FUNC
GenerateIP (BYTE Index)
{
	PLAN_GEN_FUNC GenFunc;

	switch (Index)
	{
		case SOL_DEFINED:
			GenFunc = GenerateSOL;
			break;
		case SHOFIXTI_DEFINED:
			GenFunc = GenerateShofixti;
			break;
		case START_COLONY_DEFINED:
			GenFunc = GenerateColony;
			break;
		case SPATHI_DEFINED:
			GenFunc = GenerateSpathi;
			break;
		case MELNORME0_DEFINED:
		case MELNORME1_DEFINED:
		case MELNORME2_DEFINED:
		case MELNORME3_DEFINED:
		case MELNORME4_DEFINED:
		case MELNORME5_DEFINED:
		case MELNORME6_DEFINED:
		case MELNORME7_DEFINED:
		case MELNORME8_DEFINED:
			GenFunc = GenerateMelnorme;
			break;
		case TALKING_PET_DEFINED:
			GenFunc = GenerateTalkingPet;
			break;
		case CHMMR_DEFINED:
			GenFunc = GenerateChmmr;
			break;
		case SYREEN_DEFINED:
		case MYCON_TRAP_DEFINED:
			GenFunc = GenerateSyreen;
			break;
		case BURVIXESE_DEFINED:
			GenFunc = GenerateBurvixes;
			break;
		case SLYLANDRO_DEFINED:
			GenFunc = GenerateSlylandro;
			break;
		case DRUUGE_DEFINED:
			GenFunc = GenerateDruuge;
			break;
		case BOMB_DEFINED:
		case UTWIG_DEFINED:
			GenFunc = GenerateUtwig;
			break;
		case AQUA_HELIX_DEFINED:
		case THRADD_DEFINED:
			GenFunc = GenerateThradd;
			break;
		case SUN_DEVICE_DEFINED:
		case MYCON_DEFINED:
		case EGG_CASE0_DEFINED:
		case EGG_CASE1_DEFINED:
		case EGG_CASE2_DEFINED:
			GenFunc = GenerateMycon;
			break;
		case ANDROSYNTH_DEFINED:
		case TAALO_PROTECTOR_DEFINED:
		case ORZ_DEFINED:
			GenFunc = GenerateOrz;
			break;
		case SHIP_VAULT_DEFINED:
			GenFunc = GenerateShipVault;
			break;
		case URQUAN_WRECK_DEFINED:
			GenFunc = GenerateUrquanWreck;
			break;
		case MAIDENS_DEFINED:
		case VUX_BEAST_DEFINED:
		case VUX_DEFINED:
			GenFunc = GenerateVUX;
			break;
		case SAMATRA_DEFINED:
			GenFunc = GenerateSamatra;
			break;
		case ZOQFOT_DEFINED:
		case ZOQ_SCOUT_DEFINED:
			GenFunc = GenerateZoqFotPik;
			break;
		case YEHAT_DEFINED:
			GenFunc = GenerateYehat;
			break;
		case PKUNK_DEFINED:
			GenFunc = GeneratePkunk;
			break;
		case SUPOX_DEFINED:
			GenFunc = GenerateSupox;
			break;
		case RAINBOW_DEFINED:
			GenFunc = GenerateRainbow;
			break;
		case ILWRATH_DEFINED:
			GenFunc = GenerateIlwrath;
			break;
		default:
			GenFunc = GenerateRandomIP;
			break;
	}

	return (GenFunc);
}


