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

#if !defined(_DEBUG_H) && defined(DEBUG)
#define _DEBUG_H

// Called when the debug key (symbol 'Debug' in the keys.cfg) is pressed.
extern void debugKeyPressed (void);

// Forward time to the next event. If skipHEE is set, the event named
// HYPERSPACE_ENCOUNTER_EVENT, which normally occurs every game day,
// is skipped.
extern void forwardToNextEvent (BOOLEAN skipHEE);
// Generate a list of all events in the event queue.
extern void dumpEvents (FILE *out);
// Describe one event.
extern void dumpEvent (FILE *out, EVENTPTR eventPtr);
// Get the name of one event.
extern const char *eventName (BYTE func_index);

// Give the flagship a decent equipment for debugging.
extern void equipShip (void);

// Show all active spheres of influence.
void showSpheres (void);

// Call a function for all stars.
extern void forAllStars (void (*callBack) (STAR_DESC *, void *),
		void *arg);
// Flags for dumpStar(), dumpStars(), dumpPlanets(), and dumpPlanet().
#define DUMP_PLANETS (1 << 0)
// Describe one star system.
extern void dumpStar(FILE *out, const STAR_DESC *star, UWORD flags);
// Generate a list of all stars.
extern void dumpStars(FILE *out, UWORD flags);
// Get a star color as a string.
extern const char *starColorString (BYTE col);
// Get a star type as a string.
extern const char *starTypeString (BYTE type);
// Get a string describing special presence in the star system.
extern const char *starPresenceString (BYTE index);
// Call a function for all planets in a star system.
extern void forAllPlanets(STAR_DESC *star,
		void (*callback) (PLANET_DESC *, void *), void *arg);
// Get a list describing all planets in a star.
extern void dumpPlanets (FILE *out, const STAR_DESC *star, UWORD flags);
// Describe one planet.
extern void dumpPlanet(FILE *out, const PLANET_DESC *planet, UWORD flags);

// Move instantly across hyperspace/quasispace.
extern BOOLEAN instantMove;

// To add some day:
// - a function to fast forward the game clock to a specifiable time.

#endif  /* _DEBUG_H */

