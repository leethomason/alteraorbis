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

static const float METERS_PER_GRID = 2.0f;
static const int MAX_ACTIVE_ITEMS = 8;

static const int MODEL_USER	= 0x1000;				// from model.h

// Debugging values:
static const float MOVE_SPEED = 2.0f;			// grid/second
static const float ROTATION_SPEED = 360.f;		// degrees/second
static const float ROTATION_LIMIT = 45.0f;
static const float MELEE_RANGE = 1.0f;			// FIXME: vary per monster, should be a max
static const float MELEE_COS_THETA = 0.71f;		// How wide the melee swipe is	
static const U32   COOLDOWN_TIME = 750;


enum {
	GAME_COMPONENT_MSG = GAME_COMPONENT_MSG_START,
	
	PATHMOVE_MSG_DESTINATION_REACHED,
	PATHMOVE_MSG_DESTINATION_BLOCKED,

	HEALTH_MSG_CHANGED,

	AI_EVENT_AWARENESS,		// data0: team to receive bounds of awareness

	EVENT_MELEE_DAMAGE,		// pData0: WeaponItem* 
							// normal: direction of strike
};

#endif // GAMELIMITS_INCLUDED