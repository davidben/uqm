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

#include "genall.h"
#include "../lifeform.h"
#include "../lander.h"
#include "../planets.h"
#include "../scan.h"
#include "../../build.h"
#include "../../comm.h"
#include "../../globdata.h"
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../state.h"
#include "libs/mathlib.h"


static bool GenerateSpathi_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateSpathi_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet);
static bool GenerateSpathi_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateSpathi_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static COUNT GenerateSpathi_generateLife (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateSpathi_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);
static bool GenerateSpathi_pickupLife (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateSpathiFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateSpathi_generatePlanets,
	/* .generateMoons    = */ GenerateSpathi_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateSpathi_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateSpathi_generateEnergy,
	/* .generateLife     = */ GenerateSpathi_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateSpathi_pickupEnergy,
	/* .pickupLife       = */ GenerateSpathi_pickupLife,
};


static bool
GenerateSpathi_generatePlanets (SOLARSYS_STATE *solarSys)
{
	PLANET_DESC *pMinPlanet;
	COUNT angle;

	pMinPlanet = &solarSys->PlanetDesc[0];
	solarSys->SunDesc[0].NumPlanets = 1;
	FillOrbits (solarSys,
			solarSys->SunDesc[0].NumPlanets, pMinPlanet, FALSE);

	pMinPlanet->radius = EARTH_RADIUS * 1150L / 100;
	angle = ARCTAN (pMinPlanet->location.x, pMinPlanet->location.y);
	pMinPlanet->location.x = COSINE (angle, pMinPlanet->radius);
	pMinPlanet->location.y = SINE (angle, pMinPlanet->radius);
	pMinPlanet->data_index = WATER_WORLD;
	if (GET_GAME_STATE (SPATHI_SHIELDED_SELVES))
		pMinPlanet->data_index |= PLANET_SHIELDED;
	pMinPlanet->NumPlanets = 1;

	return true;
}

static bool
GenerateSpathi_generateMoons (SOLARSYS_STATE *solarSys, PLANET_DESC *planet)
{
	COUNT angle;

	GenerateDefault_generateMoons (solarSys, planet);

	if (matchWorld (solarSys, planet, 0, MATCH_PLANET))
	{
#ifdef NOTYET
		utf8StringCopy (GLOBAL_SIS (PlanetName),
				sizeof (GLOBAL_SIS (PlanetName)),
				"Spathiwa");
#endif /* NOTYET */

		solarSys->MoonDesc[0].data_index = PELLUCID_WORLD;
		solarSys->MoonDesc[0].radius = MIN_MOON_RADIUS + MOON_DELTA;
		angle = NORMALIZE_ANGLE (LOWORD (RandomContext_Random (SysGenRNG)));
		solarSys->MoonDesc[0].location.x =
				COSINE (angle, solarSys->MoonDesc[0].radius);
		solarSys->MoonDesc[0].location.y =
				SINE (angle, solarSys->MoonDesc[0].radius);
	}

	return true;
}

static bool
GenerateSpathi_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	DWORD rand_val;

	if (matchWorld (solarSys, world, 0, 0))
	{
		/* Spathiwa's moon */
		if (!GET_GAME_STATE (SPATHI_SHIELDED_SELVES)
				&& StartSphereTracking (SPATHI_SHIP))
		{
			NotifyOthers (SPATHI_SHIP, IPNL_ALL_CLEAR);
			PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue (&GLOBAL (ip_group_q));
			assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

			CloneShipFragment (SPATHI_SHIP, &GLOBAL (npc_built_ship_q),
					INFINITE_FLEET);

			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
			InitCommunication (SPATHI_CONVERSATION);

			if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
			{
				GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
				ReinitQueue (&GLOBAL (npc_built_ship_q));
				GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);
			}
			return true;
		}
		
		DoPlanetaryAnalysis (&solarSys->SysInfo, world);
		rand_val = RandomContext_GetSeed (SysGenRNG);

		solarSys->SysInfo.PlanetInfo.ScanSeed[BIOLOGICAL_SCAN] = rand_val;
		GenerateLifeForms (&solarSys->SysInfo, GENERATE_ALL, NULL);
		rand_val = RandomContext_GetSeed (SysGenRNG);

		solarSys->SysInfo.PlanetInfo.ScanSeed[MINERAL_SCAN] = rand_val;
		GenerateMineralDeposits (&solarSys->SysInfo, GENERATE_ALL, NULL);

		solarSys->SysInfo.PlanetInfo.ScanSeed[ENERGY_SCAN] = rand_val;

		solarSys->SysInfo.PlanetInfo.Weather = 0;
		solarSys->SysInfo.PlanetInfo.Tectonics = 0;
		solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 28;
		if (!GET_GAME_STATE (UMGAH_BROADCASTERS))
		{
			LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
					CaptureDrawable (LoadGraphic (UMGAH_BCS_MASK_PMAP_ANIM));
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					CaptureStringTable (LoadStringTable (UMGAH_BCS_STRTAB));
			if (!GET_GAME_STATE (SPATHI_SHIELDED_SELVES))
			{	// The first report talks extensively about Spathi
				// slave-shielding selves. If they never did so, the report
				// makes no sense, so use an alternate.
				solarSys->SysInfo.PlanetInfo.DiscoveryString =	
						SetAbsStringTableIndex (
						solarSys->SysInfo.PlanetInfo.DiscoveryString, 1);
			}
		}
		LoadPlanet (NULL);
		return true;
	}
	else if (matchWorld (solarSys, world, 0, MATCH_PLANET))
	{
		/* visiting Spathiwa */
		DoPlanetaryAnalysis (&solarSys->SysInfo, world);
		rand_val = RandomContext_GetSeed (SysGenRNG);

		solarSys->SysInfo.PlanetInfo.ScanSeed[MINERAL_SCAN] = rand_val;
		GenerateMineralDeposits (&solarSys->SysInfo, GENERATE_ALL, NULL);
		rand_val = RandomContext_GetSeed (SysGenRNG);

		solarSys->SysInfo.PlanetInfo.ScanSeed[BIOLOGICAL_SCAN] = rand_val;

		solarSys->SysInfo.PlanetInfo.PlanetRadius = 120;
		solarSys->SysInfo.PlanetInfo.SurfaceGravity =
				CalcGravity (&solarSys->SysInfo.PlanetInfo);
		solarSys->SysInfo.PlanetInfo.Weather = 0;
		solarSys->SysInfo.PlanetInfo.Tectonics = 0;
		solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 31;

		LoadPlanet (NULL);
		return true;
	}
	
	GenerateDefault_generateOrbital (solarSys, world);

	return true;
}

static COUNT
GenerateSpathi_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (matchWorld (solarSys, world, 0, 0))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (GET_GAME_STATE (UMGAH_BROADCASTERS))
		{	// already picked up
			return 0;
		}

		return GenerateDefault_generateArtifact (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateSpathi_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (matchWorld (solarSys, world, 0, 0))
	{
		assert (!GET_GAME_STATE (UMGAH_BROADCASTERS) && whichNode == 0);

		GenerateDefault_landerReport (solarSys);
		SetLanderTakeoff ();

		SET_GAME_STATE (UMGAH_BROADCASTERS, 1);
		SET_GAME_STATE (UMGAH_BROADCASTERS_ON_SHIP, 1);

		return true; // picked up
	}

	(void) whichNode;
	return false;
}

static COUNT
GenerateSpathi_generateLife (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (matchWorld (solarSys, world, 0, MATCH_PLANET))
	{
		#define NUM_EVIL_ONES  32
		return GenerateRandomNodes (&solarSys->SysInfo, BIOLOGICAL_SCAN, NUM_EVIL_ONES,
				NUM_CREATURE_TYPES, whichNode, info);
	}

	return 0;
}

static bool
GenerateSpathi_pickupLife (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (matchWorld (solarSys, world, 0, MATCH_PLANET))
	{
		assert (!GET_GAME_STATE (SPATHI_CREATURES_ELIMINATED) &&
				!GET_GAME_STATE (SPATHI_SHIELDED_SELVES));

		SET_GAME_STATE (SPATHI_CREATURES_EXAMINED, 1);
		if (countNodesRetrieved (&solarSys->SysInfo.PlanetInfo, BIOLOGICAL_SCAN)
				+ 1 == NUM_EVIL_ONES)
		{	// last creature picked up
			SET_GAME_STATE (SPATHI_CREATURES_ELIMINATED, 1);
		}

		return true; // picked up
	}

	return GenerateDefault_pickupLife (solarSys, world, whichNode);
}
