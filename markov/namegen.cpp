#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glrandom.h"

#include "markov.h"

using namespace grinliz;


int main( int argc, const char* argv[] )
{
	if ( argc < 2 ) {
		printf( "Usage: markov path/in.txt [maxLen]\n" );
		exit( 1 );
	}
#if 0
	FILE* f = fopen( "markov.txt", "w" );
	fputs( "hello", f );
	fclose( f );
#endif

	FILE* fp = fopen( argv[1], "r" );
	if ( !fp ) {
		printf( "Could not open file %s\n", argv[1] );
		exit( 2 );
	}

	int maxLen = 12;
	if (argc >= 3) maxLen = atoi(argv[2]);

	MarkovBuilder builder;

	char buffer[256];
	while( fgets( buffer, 256, fp ) ) {
		const char* p = buffer;
		while ( *p && isspace( *p )) {
			++p;
		}
		if ( *p == '#' )
			continue;
		
		GLString str = buffer;
		CDynArray< StrToken > tokens;		
		StrTokenize( str, &tokens );

		for( int i=0; i<tokens.Size(); ++i ) {
			builder.Push( tokens[i].str );
		}
	}
	builder.Process();

	const CDynArray< MarkovBuilder::Triplet >& sortedArr = builder.Triplets();
	int line = 0;
	for( int i=0; i<sortedArr.Size(); ++i ) {
		MarkovBuilder::Triplet t = sortedArr[i];
		if ( t.value[0] == 0 ) t.value[0] = '_';
		if ( t.value[1] == 0 ) t.value[1] = '_';
		printf( "%s ", t.value );

		++line;
		if ( line == 18 ) {
			printf( "\n" );
			line = 0;
		}
	}
	printf( "\n" );

	Random random;
	random.SetSeedFromTime();
	MarkovGenerator generator( builder.Data(), builder.NumBytes(), random.Rand() );
	GLString name;

	line = 0;
	for( int k=0; k<800; ++k ) {
		if ( generator.Name( &name, maxLen )) {
			int len = name.size();
			if ( len > 3 ) {
				printf( "%s ", name.c_str() );
				line += len + 2;
			}
			if ( line > 70 ) {
				printf( "\n" );
				line = 0;
			}
		}
	}

	generator.Analyze();

	fclose( fp );
	return 0;
}