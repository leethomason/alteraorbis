// worldgen.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <ctime>

#include "../grinliz/glcolor.h"
#include "../grinliz/glnoise.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glstringutil.h"
#include "../shared/lodepng.h"

#include "../script/worldgen.h"
#include "../game/worldgrid.h"

using namespace grinliz;

static const int COUNT = 5;
static const float FRACTION_LAND = 0.4f;
static const int WIDTH  = WorldGen::SIZE;
static const int HEIGHT = WorldGen::SIZE;


int main(int argc, const char* argv[])
{

	U32 seed0 = 0;
	U32 seed1 = 4321;
	int count = COUNT;

	if ( argc >= 2 ) {
		seed0 = atoi( argv[1] );
		printf( "seed0=%d\n", seed0 );
		seed1 = seed0 + 4321;
		count = 1;
	}
	if ( argc >= 3 ) {
		seed1 = atoi( argv[2] );
		printf( "seed1=%d\n", seed1 );
	}

	clock_t startTime = clock();
	clock_t loopTime = startTime;

	WorldGen worldGen;

	for( int i=0; i<count; ++i ) {
		// Always change seed in case of retry.
		seed0 = seed0*3+7;
		seed1 = seed1*11+2;

		worldGen.StartLandAndWater( seed0, seed1 );
		printf( "Calc" ); fflush( stdout );
		for( int j=0; j<HEIGHT; ++j ) {
			worldGen.DoLandAndWater( j );
			if ( j%32 == 0 )
				printf( "." ); fflush( stdout );
		}
		printf( "Iterating...\n" );
		bool result = worldGen.EndLandAndWater( FRACTION_LAND );
		printf( "Done.\n" );

		if ( !result ) {
			printf( "CalcLandAndWater failed. Retry.\n" );
			--i;
			continue;
		}

		CDynArray<WorldFeature> featureArr;
		result = worldGen.CalColor( &featureArr );
		if ( !result ) {
			printf( "CalcColor failed. Retry.\n" );
			--i;
			continue;
		}

		clock_t endTime = clock();
		printf( "loop %d: %dms\n", i, endTime - loopTime );
		loopTime = endTime;

		for( int j=0; j<Min(featureArr.Size(),10); ++j ) {
			const WorldFeature& wf = featureArr[j];
			if ( wf.land ) {
				printf( "%d  %s area=%d (%d,%d)-(%d,%d)\n", 
						wf.id, 
						wf.land ? "land" : "water", 
						wf.area,
						wf.bounds.min.x, wf.bounds.min.y, wf.bounds.max.x, wf.bounds.max.y );
			}
		}

		CStr<32> fname;
		fname.Format( "worldgen%02d.png", i );
		printf( "Writing %s\n", fname.c_str() );
		
		static const int SIZE2 = WorldGen::SIZE*WorldGen::SIZE;
		WorldGrid* grid = new WorldGrid[SIZE2];
		memset( grid, 0, sizeof(WorldGrid)*SIZE2 );
		for( int i=0; i<SIZE2; ++i ) {
			grid[i].SetLand( *(worldGen.Land() + i) != 0 );
			grid[i].PathColor( *(worldGen.Color() + i) );
		}
		lodepng_encode32_file( fname.c_str(), (const unsigned char*)grid, WorldGen::SIZE, WorldGen::SIZE );
		delete [] grid;
	}
	printf( "total time %dms\n", clock()-startTime );
	return 0;
}

