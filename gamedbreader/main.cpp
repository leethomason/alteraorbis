
#include "../shared/gamedbreader.h"
#include "../grinliz/glmicrodb.h"

using namespace grinliz;
using namespace gamedb;

int main( int argc, const char* argv[] )
{
	// Tacked on DB testing:
	{
		MicroDB microdb;

		microdb.Set( "test-i", 
			         "S", 
					 StringPool::Intern( "valueI" ) );
		microdb.Set( "test-d", "d", -3 );

		IString s;
		int i;

		microdb.Fetch( "test-i", "S", &s );
		printf( "Fetch: (valueI) %s\n", s.c_str() );

		microdb.Fetch( "test-d", "d", &i );
		printf( "Fetch: (-3) %d\n", i );
	}


	Reader reader;

	if ( argc != 2 ) {
		printf( "Usage: gamedbreader filename.db\n" );
		return 1;
	}

	reader.Init( 0, argv[1] );
	reader.RecWalk( reader.Root(), 0 );

	return 0;
}

