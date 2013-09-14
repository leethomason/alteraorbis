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

// 4 miles/hour = 6000m/hr = 1.6m/s = 0.8grid/s
// Which looks like a mellow walk.
static const float MOVE_SPEED		= 1.2f;			// grid/second
static const float GRID_SPEED		= MOVE_SPEED * 6.0f;
static const float ROTATION_SPEED	= 360.f;		// degrees/second

static const int SECTOR_SIZE		= 64;
static const int INNER_SECTOR_SIZE	= (SECTOR_SIZE-2);
static const int NUM_SECTORS		= MAX_MAP_SIZE / SECTOR_SIZE;

static const float METERS_PER_GRID	= 2.0f;
static const int MAX_ACTIVE_ITEMS	= 8;
static const int MAX_ITEM_NAME		= 24;
static const int NUM_PLANT_TYPES	= 8;
static const int MAX_PLANT_STAGES	= 4;
static const int MAX_PASSABLE_PLANT_STAGE = 1;	// 0,1 passable, 2,3 are not

// General guidelines to the # of things in the world.
static const int TYPICAL_DOMAINS	= 100;
static const int TYPICAL_DENIZENS	= TYPICAL_DOMAINS * 20;		// fixme: not used or tracked
static const int TYPICAL_BEASTMEN	= TYPICAL_DOMAINS * 10;		// fixme: not used or tracked
static const int TYPICAL_MONSTERS	= TYPICAL_DOMAINS * 15;
static const int TYPICAL_GREATER	= 10;						// These guys get overwhelming fast - they can clear a domain.

enum {
	CRYSTAL_GREEN,			// basic weapon crystal
	CRYSTAL_RED,			// fire weapon
	CRYSTAL_BLUE,			// shock weapon
	CRYSTAL_VIOLET,			// advanced weapon, combo fire/shock, explosive, etc.
	NUM_CRYSTAL_TYPES,
	NO_CRYSTAL = NUM_CRYSTAL_TYPES
};

// Objects can be 1x1 or 2x2. Simplifies code. Either 1x1 or 2x2 objects can have
// porches, which essentially adds an extra space at the front door. But that
// doesn't impact the pather. Plants must be 1x1
static const int MAX_BUILDING_SIZE	= 2;	

static const U32 MINUTE				= 1000*60;						// game time and real time
static const U32 MINUTES_IN_AGE		= 100;
static const U32 AGE				= MINUTE * MINUTES_IN_AGE;		// 1st age, 2nd age, etc.
static const float MIN_EFFECTIVE_RANGE = 2.0f;
static const float MAX_EFFECTIVE_RANGE = 25.0f;	

static const int MODEL_USER		= (1<<16);				// from model.h
static const int MODEL_CLICK_THROUGH	= (MODEL_USER<<1);

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


#endif // GAMELIMITS_INCLUDED