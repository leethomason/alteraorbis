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

static const int MAX_MAP_SIZE = 1024;
static const int MAX_MOVE_PATH = MAX_MAP_SIZE/2;	// repath if we need to

static const float METERS_PER_GRID = 2.0f;
static const float PATH_AVOID_DISTANCE = 2.0f;

static const int MODEL_USER	= 0x1000;					// from model.h
static const int MODEL_USER_AVOIDS	= MODEL_USER << 0;	// for finding other models we need to avoid in the space tree

// Debugging values:
static const float MOVE_SPEED = 2.0f;			// grid/second
static const float AVOID_SPEED_FRACTION = 0.5f;	// how fast to side step collisions, as a factor of MOVE_SPEED
static const float ROTATION_SPEED = 360.f;		// degrees/second
static const float ROTATION_LIMIT = 45.0f;

#endif // GAMELIMITS_INCLUDED