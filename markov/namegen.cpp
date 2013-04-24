#include <stdlib.h>
#include <stdio.h>

#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;

int main( int argc, const char* argv[] ) 
{
	if ( argc != 2 ) {
		printf( "Usage: markov path/in.txt\n" );
		exit( 1 );
	}

	FILE* fp = fopen( argv[1], "r" );
	if ( !fp ) {
		printf( "Could not open file %s\n", argv[1] );
		exit( 2 );
	}

	char buffer[256];
	CDynArray< StrToken > tokens;

	while( fgets( buffer, 256, fp ) ) {
		GLString str = buffer;
		StrTokenize( str, &tokens, true );
	}

	for( int i=0; i<tokens.Size(); ++i ) {
		printf( "%s\n", tokens[i].str.c_str() );
	}
	fclose( fp );
	return 0;
}