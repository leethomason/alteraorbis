
#include "../shared/gamedbreader.h"
#include "../grinliz/glmicrodb.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;
using namespace gamedb;

int main( int argc, const char* argv[] )
{
	Reader reader;

	if ( argc != 2 ) {
		printf( "Usage: gamedbreader filename.db\n" );
		return 1;
	}

	reader.Init( 0, argv[1] );
	reader.RecWalk( reader.Root(), 0 );

	reader.Manifest(2);

	return 0;
}

