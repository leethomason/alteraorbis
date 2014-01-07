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
#include "../game/gamelimits.h"

using namespace grinliz;

static const int COUNT = 5;
static const float FRACTION_LAND = 0.3f;
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
	}
	if ( argc >= 3 ) {
		seed1 = atoi( argv[2] );
		printf( "seed1=%d\n", seed1 );
	}

	clock_t startTime = clock();
	clock_t loopTime = startTime;

	WorldGen worldGen;
	worldGen.LoadFeatures( "../res/features.png" );

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

		worldGen.WriteMarker();

		SectorData* sectorData = new SectorData[SECTOR_SIZE*SECTOR_SIZE];
		worldGen.CutRoads( seed0, sectorData );
		worldGen.ProcessSectors( seed0, sectorData );

		clock_t endTime = clock();
		printf( "loop %d: %dms\n", i, endTime - loopTime );
		loopTime = endTime;

		CStr<32> fname, fnameP;
		fname.Format( "worldgen%02d.png", i );
		printf( "Writing %s\n", fname.c_str() );
		fnameP.Format( "worldgen-path%02d.png", i );
		
		static const int SIZE2 = WorldGen::SIZE*WorldGen::SIZE;
		Color4U8* pixels = new Color4U8[SIZE2];

		for( int i=0; i<SIZE2; ++i ) {
			WorldGrid grid;
			memset( &grid, 0, sizeof(grid) );

			int h = *(worldGen.Land() + i);
			if ( h >= WorldGen::WATER && h <= WorldGen::LAND3 ) {
				if ( h == WorldGen::LAND3 ) {
					int debug=1;
				}
				grid.SetLandAndRock( h );
			}
			else if ( h == WorldGen::GRID ) {
				grid.SetGrid();
			}
			else if ( h == WorldGen::PORT ) {
				grid.SetPort();
			}
			else if ( h == WorldGen::CORE ) {
				grid.SetLandAndRock( WorldGen::LAND0 );
			}
			else {
				GLASSERT( 0 );
			}
			pixels[i] = grid.ToColor();
		}
		lodepng_encode32_file( fname.c_str(), (const unsigned char*)pixels, WorldGen::SIZE, WorldGen::SIZE );

		for( int i=0; i<SIZE2; ++i ) {
			int path = *(worldGen.Path() + i);
			int h = *(worldGen.Land() + i);

			if ( h != WorldGen::WATER ) {
				switch( path & 3 ) {
				case 0:	pixels[i].Set( 255, 0, 0, 255  ); break;
				case 1:	pixels[i].Set( 0, 255, 0, 255 ); break;
				case 2:	pixels[i].Set( 0, 0, 255, 255 ); break;
				case 3:	pixels[i].Set( 255, 0, 255, 255 ); break;
				}
			}
			else {
				pixels[i].Set( 0, 0, 0, 255 );
			}
		}
		lodepng_encode32_file( fnameP.c_str(), (const unsigned char*)pixels, WorldGen::SIZE, WorldGen::SIZE );

		delete [] pixels;
		delete [] sectorData;
	}
	printf( "total time %dms\n", clock()-startTime );
	return 0;
}

