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

#ifndef CENSUS_LUMOS_INCLUDED
#define CENSUS_LUMOS_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../game/gamelimits.h"

class Census
{
public:
	Census() {
		memset( plants, 0, sizeof(int)*NUM_PLANT_TYPES*MAX_PLANT_STAGES );
		ais = 0;
	}

	int									plants[NUM_PLANT_TYPES][MAX_PLANT_STAGES];
	int									ais;	// the number of AIs.


	int CountPlants() const {
		int c =  0;
		for( int i=0; i<NUM_PLANT_TYPES; ++i ) {
			for( int j=0; j<MAX_PLANT_STAGES; ++j ) {
				c += plants[i][j];
			}
		}
		return c;
	}
};

#endif 
