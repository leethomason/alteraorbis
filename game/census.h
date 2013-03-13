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
	}

	int									plants[NUM_PLANT_TYPES][MAX_PLANT_STAGES];
	grinliz::CArray< int, MAX_CORES >	cores; 
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
