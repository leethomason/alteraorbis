#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glrandom.h"

using namespace grinliz;

class MarkovArray
{
public:
	struct Entry {
		char a;
		char b;
		char c;
	};

};


struct Triplet
{
	static U32 Hash( const Triplet& v)							{ return v.All(); }
	static bool Equal( const Triplet& v0, const Triplet& v1 )	{ return v0.All() == v1.All(); }

	Triplet() {
		for( int i=0; i<4; ++i ) value[i] = 0;
	}

	char value[4];	// a,b,c,0
	U32 All() const { return *((U32*)value); }
};


struct Doublet
{
	static bool Less( const Triplet& v0, const Triplet& v1 )	{ 
		U32 a = (v0.value[0] << 8) + v0.value[1];
		U32 b = (v1.value[0] << 8) + v1.value[1];
		return a < b; 
	}
};


void FindPair( const Triplet* mem, int n, char a, char b, int* start, int* count )
{
	*start = 0;
	*count = 0;
	for( int i=0; i<n; ++i ) {
		while ( mem[i].value[0] == a && mem[i].value[1] == b ) {
			if ( *count == 0 ) 
				*start = i;
			*count += 1;
			++i;
		}
		if ( *count ) {
			return;
		}
	}
}


int main( int argc, const char* argv[] ) 
{
	if ( argc != 2 ) {
		printf( "Usage: markov path/in.txt\n" );
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

	char buffer[256];
	CDynArray< StrToken > tokens;

	while( fgets( buffer, 256, fp ) ) {
		const char* p = buffer;
		while ( *p && isspace( *p )) {
			++p;
		}
		if ( *p == '#' )
			continue;
		
		GLString str = buffer;
		
		StrTokenize( str, &tokens, true );
	}

	HashTable< Triplet, Triplet, Triplet > table;

	for( int i=0; i<tokens.Size(); ++i ) {
		//printf( "%s\n", tokens[i].str.c_str() );

		const GLString& token = tokens[i].str;

		if ( token.size() > 2 ) {
			{
				Triplet t;
				t.value[2] = token[0];
				table.Add( t,t );
			}
			{ 
				Triplet t;
				t.value[1] = token[0];
				t.value[2] = token[1];
				table.Add( t,t );
			}
			{ 
				Triplet t;
				t.value[0] = token[token.size()-2];
				t.value[1] = token[token.size()-1];
				table.Add( t,t );
			}
			for( unsigned k=0; k<token.size()-2; ++k ) {
				Triplet  t;
				t.value[0] = token[k];
				t.value[1] = token[k+1];
				t.value[2] = token[k+2];
				table.Add( t,t );
			}
		}
	}

	Triplet* arr = table.GetValues();
	CDynArray< Triplet > sortedArr;
	for( int i=0; i<table.NumValues(); ++i ) {
		sortedArr.Push( arr[i] );
	}

	Sort< Triplet, Doublet >( &sortedArr[0], sortedArr.Size() );
	for( int i=0; i<sortedArr.Size(); ++i ) {
		Triplet t = sortedArr[i];
		if ( t.value[0] == 0 ) t.value[0] = '_';
		if ( t.value[1] == 0 ) t.value[1] = '_';
		printf( "%s\n", t.value );
	}

	int start=0, count=0;

	FindPair( sortedArr.Mem(), sortedArr.Size(), 0, 0, &start, &count );
	printf( "00 start=%d, count=%d\n", start, count );

	FindPair( sortedArr.Mem(), sortedArr.Size(), 'a', 'n', &start, &count );
	printf( "an start=%d, count=%d\n", start, count );

	Random random;
	random.SetSeedFromTime();

	for( int k=0; k<8; ++k ) {
		GLString name;
		char a=0;
		char b=0;
		while( true ) {
			FindPair( sortedArr.Mem(), sortedArr.Size(), a, b, &start, &count );
			if ( count == 0 )
				break;
			int index = start + random.Rand( count );
			char c = sortedArr[index].value[2];
			if ( c ) {
				name.append( &c, 1 );
				a = b;
				b = c;
				c = 0;
			}
			else {
				printf( "%s\n", name.c_str() );
				break;
			}
		}
	}

	fclose( fp );
	return 0;
}