/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GAMELIMITS_INCLUDED
#define GAMELIMITS_INCLUDED

#include "../xegame/xegamelimits.h"

static const int MAX_MOVE_PATH = MAX_MAP_SIZE/2;	// repath if we need to

static const float METERS_PER_GRID	= 2.0f;
static const int MAX_ACTIVE_ITEMS	= 8;
static const int MAX_ITEM_NAME		= 24;

static const int MODEL_USER	= 0x1000;				// from model.h

// Debugging values:
static const float ROTATION_LIMIT	= 45.0f;
static const float MELEE_RANGE		= 1.0f;			// FIXME: vary per monster, should be a max
static const float MELEE_COS_THETA	= 0.71f;		// How wide the melee swipe is	
static const U32   COOLDOWN_TIME	= 750;
// End debugging values

static const float EXPLOSIVE_RANGE	= 1.5f;			// explosions go through walls. Make this too big,
													// that becomes an issue. Too small, explosions aren't fun.
static const float LONGEST_WEAPON_RANGE = 20.0f;	// effects AI logic, queries
static const float EFFECT_DAMAGE_PER_SEC = 20.0f;
static const float EFFECT_RADIUS = 1.5f;
static const float EFFECT_ACCRUED_MAX = EFFECT_DAMAGE_PER_SEC;

// This arrangement breaks the case where something can attach
// to multiple hardpoints. Fine; fix later.
enum {
	NO_HARDPOINT = -1,
	// The set of hardpoints	
	HARDPOINT_TRIGGER = 0,	// this attaches to the trigger hardpoint
	HARDPOINT_ALTHAND,		// this attaches to the alternate hand (non-trigger) hardpoint
	HARDPOINT_HEAD,			// this attaches to the head hardpoint
	HARDPOINT_SHIELD,		// this attaches to the shield hardpoint
	NUM_HARDPOINTS,
};

enum {
	PROCEDURAL_NONE,
	PROCEDURAL_ROUNDS_TO_GLOW,	// rounds -> glow intensity
	PROCEDURAL_RING,			// layers, etc.

	PROCEDURAL_INIT_MASK = PROCEDURAL_RING,				// mask of procedural rendering done at init time
	PROCEDURAL_TICK_MASK = PROCEDURAL_ROUNDS_TO_GLOW,	// mask of procedural rendering done per tick
};


#endif // GAMELIMITS_INCLUDED