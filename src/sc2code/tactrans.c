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

BOOLEAN
OpponentAlive (STARSHIPPTR TestStarShipPtr)
{
	HELEMENT hElement, hSuccElement;

	for (hElement = GetHeadElement (); hElement; hElement = hSuccElement)
	{
		ELEMENTPTR ElementPtr;
		STARSHIPPTR StarShipPtr;

		LockElement (hElement, &ElementPtr);
		hSuccElement = GetSuccElement (ElementPtr);
		GetElementStarShip (ElementPtr, &StarShipPtr);
		UnlockElement (hElement);

		if (StarShipPtr
				&& StarShipPtr != TestStarShipPtr
				&& StarShipPtr->RaceDescPtr->ship_info.crew_level == 0)
		{
			return (FALSE);
		}
	}

	return (TRUE);
}

void
new_ship (PELEMENT DeadShipPtr)
{
	STARSHIPPTR DeadStarShipPtr;

	ProcessSound ((SOUND)~0);

	GetElementStarShip (DeadShipPtr, &DeadStarShipPtr);
	if (!(DeadShipPtr->state_flags & PLAYER_SHIP))
	{
		if (DeadShipPtr->life_span) /* must be pre-processing */
			return;
	}
	else
	{
		BOOLEAN MusicStarted;
		HELEMENT hElement, hSuccElement;

		DeadStarShipPtr->ShipFacing =
				DeadStarShipPtr->RaceDescPtr->ship_info.var2;
		DeadStarShipPtr->special_counter =
				DeadStarShipPtr->RaceDescPtr->ship_info.crew_level;
		if (DeadStarShipPtr)
		{
			StopMusic ();
			DeadStarShipPtr->RaceDescPtr->ship_info.crew_level = 0;
		}

		MusicStarted = FALSE;
		DeadShipPtr->turn_wait = (BYTE)(
				DeadShipPtr->state_flags & (GOOD_GUY | BAD_GUY)
				);
		for (hElement = GetHeadElement (); hElement; hElement = hSuccElement)
		{
			ELEMENTPTR ElementPtr;
			STARSHIPPTR StarShipPtr;

			LockElement (hElement, &ElementPtr);
			hSuccElement = GetSuccElement (ElementPtr);
			GetElementStarShip (ElementPtr, &StarShipPtr);
			if (StarShipPtr == DeadStarShipPtr)
			{
				SetElementStarShip (ElementPtr, 0);

				if (!(ElementPtr->state_flags & CREW_OBJECT)
						|| ElementPtr->preprocess_func != crew_preprocess)
				{
					SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
					ElementPtr->life_span = 0;
					ElementPtr->state_flags =
							NONSOLID | DISAPPEARING | FINITE_LIFE;
					ElementPtr->preprocess_func =
							ElementPtr->postprocess_func =
							ElementPtr->death_func = 0;
					ElementPtr->collision_func = 0;
				}
			}

			if (StarShipPtr
					&& (StarShipPtr->cur_status_flags & PLAY_VICTORY_DITTY))
			{
				MusicStarted = TRUE;
				PlayMusic ((MUSIC_REF)StarShipPtr->RaceDescPtr->
						ship_data.victory_ditty, FALSE, 3);
				StarShipPtr->cur_status_flags &= ~PLAY_VICTORY_DITTY;
			}

			UnlockElement (hElement);
		}

		DeadShipPtr->life_span = MusicStarted ?
				(ONE_SECOND * 3) / BATTLE_FRAME_RATE : 1;
		DeadShipPtr->death_func = new_ship;
		DeadShipPtr->preprocess_func = new_ship;
		SetElementStarShip (DeadShipPtr, DeadStarShipPtr);
	}

	if (DeadShipPtr->life_span
#if !DEMO_MODE
			|| PLRPlaying ((MUSIC_REF)~0)
#endif /* DEMO_MODE */
			)
	{
		DeadShipPtr->state_flags &= ~DISAPPEARING;
		++DeadShipPtr->life_span;
	}
	else
	{
		BOOLEAN RestartMusic;

		StopMusic ();
		StopSound ();

		SetElementStarShip (DeadShipPtr, 0);
		RestartMusic = OpponentAlive (DeadStarShipPtr);

		free_ship (DeadStarShipPtr, TRUE);
UnbatchGraphics ();
		if (GetNextStarShip (DeadStarShipPtr,
				WHICH_SIDE (DeadShipPtr->turn_wait)) && RestartMusic)
			BattleSong (TRUE);
		else if (LOBYTE (battle_counter) == 0
				|| HIBYTE (battle_counter) == 0)
			GLOBAL (CurrentActivity) &= ~IN_BATTLE;
BatchGraphics ();
	}
}

void
explosion_preprocess (PELEMENT ShipPtr)
{
	BYTE i;

	i = (NUM_EXPLOSION_FRAMES * 3) - ShipPtr->life_span;
	switch (i)
	{
		case 25:
			ShipPtr->preprocess_func = NULL_PTR;
		case 0:
		case 1:
		case 2:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
			i = 1;
			break;
		case 3:
		case 4:
		case 5:
		case 18:
		case 19:
			i = 2;
			break;
		case 15:
			SetPrimType (&DisplayArray[ShipPtr->PrimIndex], NO_PRIM);
			ShipPtr->state_flags |= CHANGING;
		default:
			i = 3;
			break;
	}

	do
	{
		HELEMENT hElement;

		hElement = AllocElement ();
		if (hElement)
		{
			COUNT angle, dist;
			DWORD rand_val;
			ELEMENTPTR ElementPtr;
			extern FRAME explosion[];

			PutElement (hElement);
			LockElement (hElement, &ElementPtr);
			ElementPtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
			ElementPtr->life_span = 9;
			SetPrimType (&DisplayArray[ElementPtr->PrimIndex], STAMP_PRIM);
			ElementPtr->current.image.farray = explosion;
			ElementPtr->current.image.frame = explosion[0];
			rand_val = Random ();
			angle = LOBYTE (HIWORD (rand_val));
			dist = DISPLAY_TO_WORLD (LOBYTE (LOWORD (rand_val)) % 8);
			if (HIBYTE (LOWORD (rand_val)) < 256 * 1 / 3)
				dist += DISPLAY_TO_WORLD (8);
			ElementPtr->current.location.x =
					ShipPtr->current.location.x
					+ COSINE (angle, dist);
			ElementPtr->current.location.y =
					ShipPtr->current.location.y
					+ SINE (angle, dist);
			ElementPtr->preprocess_func = animation_preprocess;
			rand_val = Random ();
			angle = LOBYTE (LOWORD (rand_val));
			dist = WORLD_TO_VELOCITY (
					DISPLAY_TO_WORLD (HIBYTE (LOWORD (rand_val)) % 5)
					);
			SetVelocityComponents (&ElementPtr->velocity,
					COSINE (angle, dist),
					SINE (angle, dist));
			UnlockElement (hElement);
		}
	} while (--i);
}

void
ship_death (PELEMENT ShipPtr)
{
	STARSHIPPTR StarShipPtr, VictoriousStarShipPtr;
	HELEMENT hElement, hNextElement;
	ELEMENTPTR ElementPtr;

	StopMusic ();

	if (ShipPtr->mass_points <= MAX_SHIP_MASS)
	{
		if (WHICH_SIDE (ShipPtr->state_flags))
			battle_counter -= MAKE_WORD (0, 1);
		else
			battle_counter -= MAKE_WORD (1, 0);
	}

	VictoriousStarShipPtr = 0;
	for (hElement = GetHeadElement (); hElement; hElement = hNextElement)
	{
		LockElement (hElement, &ElementPtr);
		if ((ElementPtr->state_flags & PLAYER_SHIP)
				&& ElementPtr != (ELEMENTPTR)ShipPtr
						/* and not running away */
				&& ElementPtr->mass_points <= MAX_SHIP_MASS)
		{
			GetElementStarShip (ElementPtr, &VictoriousStarShipPtr);
			if (VictoriousStarShipPtr->RaceDescPtr->ship_info.crew_level == 0)
				VictoriousStarShipPtr = 0;
			else
				VictoriousStarShipPtr->cur_status_flags |= PLAY_VICTORY_DITTY;

			UnlockElement (hElement);
			break;
		}
		hNextElement = GetSuccElement (ElementPtr);
		UnlockElement (hElement);
	}

	GetElementStarShip (ShipPtr, &StarShipPtr);
	StarShipPtr->cur_status_flags &= ~PLAY_VICTORY_DITTY;

	DeltaEnergy ((ELEMENTPTR)ShipPtr,
			-(SIZE)StarShipPtr->RaceDescPtr->ship_info.energy_level);

	ShipPtr->life_span = NUM_EXPLOSION_FRAMES * 3;
	ShipPtr->state_flags &= ~DISAPPEARING;
	ShipPtr->state_flags |= FINITE_LIFE | NONSOLID;
	ShipPtr->postprocess_func = PostProcessStatus;
	ShipPtr->death_func = new_ship;
	ShipPtr->hTarget = 0;
	ZeroVelocityComponents (&((ELEMENTPTR)ShipPtr)->velocity);
	if (ShipPtr->crew_level) /* only happens for shofixti self-destruct */
	{

		PlaySound (SetAbsSoundIndex (
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1
				), GAME_SOUND_PRIORITY + 1);

		DeltaCrew ((ELEMENTPTR)ShipPtr, -(SIZE)ShipPtr->crew_level);
		if (VictoriousStarShipPtr == 0)
			StarShipPtr->cur_status_flags |= PLAY_VICTORY_DITTY;
	}
	else
	{
		ShipPtr->preprocess_func = explosion_preprocess;

		PlaySound (SetAbsSoundIndex (GameSounds, SHIP_EXPLODES),
				GAME_SOUND_PRIORITY + 1);
	}
}

#define START_ION_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x15, 0x00), 0x7A)

void
spawn_ion_trail (PELEMENT ElementPtr)
{
		  
	if (ElementPtr->state_flags & PLAYER_SHIP)
	{
		HELEMENT hIonElement;

		hIonElement = AllocElement ();
		if (hIonElement)
		{
#define ION_LIFE 1
			COUNT angle;
			RECT r;
			ELEMENTPTR IonElementPtr;
			STARSHIPPTR StarShipPtr;

			GetElementStarShip (ElementPtr, &StarShipPtr);
			angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing) + HALF_CIRCLE;
			GetFrameRect (StarShipPtr->RaceDescPtr->ship_data.ship[0], &r);
			r.extent.height = DISPLAY_TO_WORLD (r.extent.height + r.corner.y);

			InsertElement (hIonElement, GetHeadElement ());
			LockElement (hIonElement, &IonElementPtr);
			IonElementPtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
			IonElementPtr->life_span = IonElementPtr->thrust_wait = ION_LIFE;
			SetPrimType (&DisplayArray[IonElementPtr->PrimIndex], POINT_PRIM);
			SetPrimColor (
					&DisplayArray[IonElementPtr->PrimIndex], START_ION_COLOR
					);
			IonElementPtr->current.image.frame = DecFrameIndex (stars_in_space);
			IonElementPtr->current.image.farray = &stars_in_space;
			IonElementPtr->current.location = ElementPtr->current.location;
			IonElementPtr->current.location.x +=
					(COORD)COSINE (angle, r.extent.height);
			IonElementPtr->current.location.y +=
					(COORD)SINE (angle, r.extent.height);
			IonElementPtr->death_func = spawn_ion_trail;

			SetElementStarShip (IonElementPtr, StarShipPtr);

			{
					/* normally done during preprocess, but because
					 * object is being inserted at head rather than
					 * appended after tail it may never get preprocessed.
					 */
				IonElementPtr->next = IonElementPtr->current;
				--IonElementPtr->life_span;
				IonElementPtr->state_flags |= PRE_PROCESS;
			}

			UnlockElement (hIonElement);
		}
	}
	else
	{
			COLOR color_tab[] =
				  { BUILD_COLOR (MAKE_RGB15 (0x1F, 0x00, 0x00), 0x2a),
				  BUILD_COLOR (MAKE_RGB15 (0x1B, 0x00, 0x00), 0x2b),
				  BUILD_COLOR (MAKE_RGB15 (0x17, 0x00, 0x00), 0x2c),
				  BUILD_COLOR (MAKE_RGB15 (0x13, 0x00, 0x00), 0x2d),
				  BUILD_COLOR (MAKE_RGB15 (0xF, 0x00, 0x00),  0x2e),
				  BUILD_COLOR (MAKE_RGB15 (0xB, 0x00, 0x00),  0x2f),
				  BUILD_COLOR (MAKE_RGB15 (0x1F, 0x15, 0x00), 0x7a),
				  BUILD_COLOR (MAKE_RGB15 (0x1F, 0x11, 0x00), 0x7b),
				  BUILD_COLOR (MAKE_RGB15 (0x1F, 0xE, 0x00),  0x7c),
				  BUILD_COLOR (MAKE_RGB15 (0x1F, 0xA, 0x00),  0x7d),
				  BUILD_COLOR (MAKE_RGB15 (0x1F, 0x7, 0x00),  0x7e),
				  BUILD_COLOR (MAKE_RGB15 (0x1F, 0x3, 0x00),  0x7f), };
#define NUM_TAB_COLORS (sizeof (color_tab) / sizeof (color_tab[0]))
				
		COUNT color_index = 0;
		COLOR Color;

		Color = COLOR_256 (GetPrimColor (&DisplayArray[ElementPtr->PrimIndex]));
		if (Color != 0x2F)
		{
			ElementPtr->life_span = ElementPtr->thrust_wait;
		   
			++Color;
			if (Color > 0x7F)
				Color = 0x2A;
			if (Color <= 0x2f && Color >= 0x2a)
					color_index = Color - 0x2a;
			else /* color is between 0x7a and 0x7f */
					color_index = (Color - 0x7a) + (NUM_TAB_COLORS >> 1);
			SetPrimColor (
					&DisplayArray[ElementPtr->PrimIndex], color_tab[color_index]
					);

			ElementPtr->state_flags &= ~DISAPPEARING;
			ElementPtr->state_flags |= CHANGING;
		}
	}
}

void
ship_transition (PELEMENT ElementPtr)
{
	if (ElementPtr->state_flags & PLAYER_SHIP)
	{
		if (ElementPtr->state_flags & APPEARING)
		{
			ElementPtr->life_span = HYPERJUMP_LIFE;
			ElementPtr->preprocess_func = ship_transition;
			ElementPtr->postprocess_func = NULL_PTR;
			SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
			ElementPtr->state_flags |= NONSOLID | FINITE_LIFE | CHANGING;
		}
		else if (ElementPtr->life_span < HYPERJUMP_LIFE)
		{
			if (ElementPtr->life_span == NORMAL_LIFE
					&& ElementPtr->crew_level)
			{
				ElementPtr->current.image.frame =
						ElementPtr->next.image.frame =
						SetEquFrameIndex (
						ElementPtr->current.image.farray[0],
						ElementPtr->current.image.frame);
				SetPrimType (&DisplayArray[ElementPtr->PrimIndex], STAMP_PRIM);
				InitIntersectStartPoint (ElementPtr);
				InitIntersectEndPoint (ElementPtr);
				InitIntersectFrame (ElementPtr);
				ZeroVelocityComponents (&((ELEMENTPTR)ElementPtr)->velocity);
				ElementPtr->state_flags &= ~(NONSOLID | FINITE_LIFE);
				ElementPtr->state_flags |= CHANGING;

				ElementPtr->preprocess_func = ship_preprocess;
				ElementPtr->postprocess_func = ship_postprocess;
			}

			return;
		}
	}

	{
		HELEMENT hShipImage;
		ELEMENTPTR ShipImagePtr;
		STARSHIPPTR StarShipPtr;
		
		GetElementStarShip (ElementPtr, &StarShipPtr);
		LockElement (StarShipPtr->hShip, &ShipImagePtr);

		if (!(ShipImagePtr->state_flags & NONSOLID))
		{
			ElementPtr->preprocess_func = NULL_PTR;
		}
		else if ((hShipImage = AllocElement ()))
		{
#define TRANSITION_SPEED DISPLAY_TO_WORLD (40)
#define TRANSITION_LIFE 1
			COUNT angle;

			PutElement (hShipImage);

			angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);

			LockElement (hShipImage, &ShipImagePtr);
			ShipImagePtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
			ShipImagePtr->life_span = ShipImagePtr->thrust_wait = TRANSITION_LIFE;
			SetPrimType (&DisplayArray[ShipImagePtr->PrimIndex], STAMPFILL_PRIM);
			SetPrimColor (
					&DisplayArray[ShipImagePtr->PrimIndex], START_ION_COLOR
					);
			ShipImagePtr->current.image = ElementPtr->current.image;
			ShipImagePtr->current.location = ElementPtr->current.location;
			if (!(ElementPtr->state_flags & PLAYER_SHIP))
			{
				ShipImagePtr->current.location.x +=
						COSINE (angle, TRANSITION_SPEED);
				ShipImagePtr->current.location.y +=
						SINE (angle, TRANSITION_SPEED);
				ElementPtr->preprocess_func = NULL_PTR;
			}
			else if (ElementPtr->crew_level)
			{
				ShipImagePtr->current.location.x -=
						COSINE (angle, TRANSITION_SPEED)
						* (ElementPtr->life_span - 1);
				ShipImagePtr->current.location.y -=
						SINE (angle, TRANSITION_SPEED)
						* (ElementPtr->life_span - 1);

				ShipImagePtr->current.location.x =
						WRAP_X (ShipImagePtr->current.location.x);
				ShipImagePtr->current.location.y =
						WRAP_Y (ShipImagePtr->current.location.y);
			}
			ShipImagePtr->preprocess_func = ship_transition;
			ShipImagePtr->death_func = spawn_ion_trail;
			SetElementStarShip (ShipImagePtr, StarShipPtr);

			UnlockElement (hShipImage);
		}

		UnlockElement (StarShipPtr->hShip);
	}
}

void
flee_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	if (--ElementPtr->turn_wait == 0)
	{
		SIZE dir;
		COLOR c;
		COLOR color_tab[] =
		{
			BUILD_COLOR (MAKE_RGB15 (31, 25, 25), 0x24),
			BUILD_COLOR (MAKE_RGB15 (31, 19, 19), 0x25),
			BUILD_COLOR (MAKE_RGB15 (31, 15, 15), 0x26),
			BUILD_COLOR (MAKE_RGB15 (31, 10, 10), 0x27),
			BUILD_COLOR (MAKE_RGB15 (31, 4, 4), 0x28),
			BUILD_COLOR (MAKE_RGB15 (31, 0, 0), 0x29),
			BUILD_COLOR (MAKE_RGB15 (27, 0, 0), 0x2A),
			BUILD_COLOR (MAKE_RGB15 (23, 0, 0), 0x2B),
			BUILD_COLOR (MAKE_RGB15 (19, 0, 0), 0x2C),
			BUILD_COLOR (MAKE_RGB15 (14, 0, 0), 0x2D),
			BUILD_COLOR (MAKE_RGB15 (10, 0, 0), 0x2E),
		};

		dir = HINIBBLE (ElementPtr->thrust_wait) - 1;

		c = COLOR_256 (GetPrimColor (&DisplayArray[ElementPtr->PrimIndex]));
		if (c == 0x24)
			dir = -dir;
		c += dir;
		c = color_tab[c - 0x24];
		SetPrimColor (&DisplayArray[ElementPtr->PrimIndex], c);

		if (COLOR_256 (c) == 0x2E)
		{
			dir = -dir;
			--ElementPtr->thrust_wait;
		}
		dir += 1;

		ElementPtr->turn_wait = LONIBBLE (ElementPtr->thrust_wait);
		if (ElementPtr->turn_wait)
		{
			ElementPtr->thrust_wait = MAKE_BYTE (ElementPtr->turn_wait, dir);
			ElementPtr->turn_wait = ((ElementPtr->turn_wait - 1) >> 1) + 1;
		}
		else if (COLOR_256 (c) != 0x24)
		{
			ElementPtr->thrust_wait = MAKE_BYTE (0, dir);
			ElementPtr->turn_wait = 1;
		}
		else
		{
			ElementPtr->death_func = new_ship;
			ElementPtr->crew_level = 0;

			ElementPtr->life_span = HYPERJUMP_LIFE + 1;
			ElementPtr->preprocess_func = ship_transition;
			ElementPtr->postprocess_func = NULL_PTR;
			SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
			ElementPtr->state_flags |= NONSOLID | FINITE_LIFE | CHANGING;
		}
	}

	GetElementStarShip (ElementPtr, &StarShipPtr);
	StarShipPtr->cur_status_flags
			&= ~(LEFT | RIGHT | THRUST | WEAPON | SPECIAL);
	PreProcessStatus (ElementPtr);
}
