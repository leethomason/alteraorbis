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
static const float DEFAULT_MOVE_SPEED	= 1.2f;
static const float GRID_SPEED			= DEFAULT_MOVE_SPEED * 6.0f;
static const float ROTATION_SPEED		= 360.f;		// degrees/second

static const int SECTOR_SIZE		= 64;
static const int SECTOR_SIZE_2		= SECTOR_SIZE * SECTOR_SIZE;
static const int INNER_SECTOR_SIZE	= (SECTOR_SIZE-2);
static const int NUM_SECTORS		= MAX_MAP_SIZE / SECTOR_SIZE;
static const int NUM_SECTORS_2		= NUM_SECTORS * NUM_SECTORS;

static const float METERS_PER_GRID	= 2.0f;
static const int MAX_ACTIVE_ITEMS	= 8;
static const int MAX_ITEM_NAME		= 24;
static const int NUM_PLANT_TYPES	= 8;
static const int MAX_PLANT_STAGES	= 4;
static const int MAX_PASSABLE_PLANT_STAGE = 1;	// 0,1 passable, 2,3 are not
static const int LEVEL_OF_NAMING	= 3;
static const int VALUE_OF_NAMING	= 50;
static const float	PICKUP_RANGE	= 1.1f;	// Make sure center-to-center works
static const int FARM_GROW_RAD		= 2;	// grid squares affected by farm. total region 5x5
static const int TECH_MAX			= 4;	// Tech must be less than this: 0-3 in int, 0-3.99 in double
static const int MAX_WILD_FRUIT		= 300;
static const int GOLD_XFER_TAKEOVER = 500;

// This is in terms of MAX_TECH (not number of temples)
// See CoreScript.cpp for rules.
// Part of implenmentation in AIComponent.cpp
static const int TEMPLES_REPELS_LESSER = 0;	// repels *some* but not all lesser
static const int TEMPLES_REPELS_GREATER = 1;

static const float TECH_ATTRACTS_GREATER = 2.5f;

// General guidelines to the # of things in the world.
static const int MAP_SCALE_FACTOR = (1024 * 1024) / (MAX_MAP_SIZE*MAX_MAP_SIZE);

static const int TYPICAL_DOMAINS	= 120		/ MAP_SCALE_FACTOR;
static const int TYPICAL_AI_DOMAINS = 40		/ MAP_SCALE_FACTOR;
static const int TYPICAL_DENIZENS	= 800		/ MAP_SCALE_FACTOR;
static const int TYPICAL_LESSER		= 1500		/ MAP_SCALE_FACTOR;
static const int TYPICAL_GREATER	= 10		/ MAP_SCALE_FACTOR;		// These guys get overwhelming fast - they can clear a domain.
static const int TYPICAL_PLANTS		= 50 * 1000 / MAP_SCALE_FACTOR;

static const int SPAWN_RATS			= TYPICAL_LESSER / 10;	// below this will aggressively spawn rats

static const int GOLD_PER_DENIZEN  = 100;
static const int GOLD_PER_LESSER   =  10;
static const int GOLD_PER_GREATER  = 100;
static const int ALL_GOLD =	(TYPICAL_DENIZENS * GOLD_PER_DENIZEN
							+ TYPICAL_LESSER * GOLD_PER_LESSER
							+ TYPICAL_GREATER * GOLD_PER_GREATER) * 3 / 2;

// Values to prevent huge accumulation in one very lucky MOB.
static const int MAX_LESSER_GOLD	= 100;
static const int MAX_GREATER_GOLD	= 1000;
static const int MAX_LESSER_MOB_CRYSTAL = 4;
static const int MAX_GREATER_MOB_CRYSTAL = 12;

static const int ALL_CRYSTAL_GREEN  = TYPICAL_DOMAINS * 15;		// was 10, seemed stressed.
static const int ALL_CRYSTAL_RED    = TYPICAL_DOMAINS * 4;
static const int ALL_CRYSTAL_BLUE   = TYPICAL_DOMAINS * 2;
static const int ALL_CRYSTAL_VIOLET = TYPICAL_DOMAINS / 2;


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

static const int MAX_CARRIED_ITEMS = 15;

static const U32 MINUTES_IN_AGE		= 100;
static const U32 AGE_IN_MSEC		= MINUTES_IN_AGE * 60 * 1000;

static const float MIN_EFFECTIVE_RANGE	= 2.0f;
static const float MAX_EFFECTIVE_RANGE	= 25.0f;	

static const int MAX_SQUADS				= 4;
static const int CITIZEN_BASE			= 8;		// Base # of citizens before squads form
static const int SQUAD_SIZE				= 4;
// WARNING: needs to equal or less than size of the CChitArray
static const int MAX_CITIZENS			= CITIZEN_BASE + SQUAD_SIZE * MAX_SQUADS;	

// AI Limits
static const int MAX_HUMAN_TEMPLES		= 4;
static const int MAX_GOBMEN_TEMPLES		= 3;
static const int MAX_KAMAKIRI_TEMPLES	= 2;

// Debugging values:
static const float ROTATION_LIMIT	= 45.0f;
static const float MELEE_RANGE		= 1.0f;			// FIXME: vary per monster, should be a max
static const float MELEE_COS_THETA	= 0.71f;		// How wide the melee swipe is	
static const U32   COOLDOWN_TIME	= 750;
// End debugging values

static const float EXPLOSIVE_RANGE	= 1.5f;			// explosions go through walls. Make this too big,
													// that becomes an issue. Too small, explosions aren't fun.
static const float LONGEST_WEAPON_RANGE  = 20.0f;	// effects AI logic, queries
static const float EFFECT_DAMAGE_PER_SEC =  5.0f;
static const float EFFECT_RADIUS = 1.5f;
static const int   EFFECT_MAX_TIME = 2000;
static const float MASS_TO_EXPLOSION_DAMAGE = 0.40f;

static const float CHANCE_FIRE_NORMAL	= 0.40f;
static const float CHANCE_FIRE_HIGH		= 0.80f;
static const float CHANCE_FIRE_OUT		= 0.10f;
static const float CHANCE_FIRE_SPREAD	= 0.40f;	// once it starts spreading, self reinforcing. 0.20 too low.

static const float SALES_TAX = 0.25f;
static const int   WORKER_BOT_COST = 20;
static const int   MAX_WORKER_BOTS = 4;

// Game fiction consistency.
#define MOB_Created		"Initialized"	// Denizen Initialized
#define MOB_Destroyed	"Deleted"		// Denizen Deleted
#define MOB_created		"initialized"	// Denizen Initialized
#define MOB_destroyed	"deleted"		// Denizen Deleted
#define MOB_KILLS		"Deletes"		// Deletions: mantis
#define DATE_ABBR_BORN	"i"
#define DATE_ABBR_DIED	"d"

#endif // GAMELIMITS_INCLUDED