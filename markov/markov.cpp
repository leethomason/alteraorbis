#include "markov.h"
using namespace grinliz;

void MarkovBuilder::Push( const grinliz::GLString& str )
{
	names.Push( str );
}


void MarkovBuilder::Process()
{
	for( int i=0; i<names.Size(); ++i ) {
		const GLString& name = names[i];
		if ( name.size() > 2 ) {
			{
				// Add first
				Triplet t;
				t.value[2] = name[0];
				table.Add( t,t );
			}
			{ 
				// Add 2nd
				Triplet t;
				t.value[1] = name[0];
				t.value[2] = name[1];
				table.Add( t,t );
			}
			{ 
				// Add terminator
				Triplet t;
				t.value[0] = name[name.size()-2];
				t.value[1] = name[name.size()-1];
				table.Add( t,t );
			}
			for( unsigned k=0; k<name.size()-2; ++k ) {
				Triplet  t;
				t.value[0] = name[k];
				t.value[1] = name[k+1];
				t.value[2] = name[k+2];
				table.Add( t,t );
			}
		}
	}

	for( int i=0; i<table.NumValues(); ++i ) {
		sortedArr.Push( table.GetValue(i) );
	}
	Sort< Triplet, DoubletSort >( &sortedArr[0], sortedArr.Size() );
}



void MarkovGenerator::FindPair( char a, char b, int* start, int* count )
{
	*start = 0;
	*count = 0;
	const MarkovBuilder::Triplet* mem = this->arr;

	for( int i=0; i<nTriplets; ++i ) {
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


MarkovGenerator::MarkovGenerator( const char* data, int nBytes, int seed )
{
	random.SetSeed( seed );
	GLASSERT( nBytes % 4 == 0 );

	arr = (MarkovBuilder::Triplet*)data;
	nTriplets = nBytes / 4;
}


bool MarkovGenerator::Name( GLString* name, int maxLen )
{
	*name = "";

	char a=0;
	char b=0;
	int n = 0;
	int start=0, count=0;

	while( n < maxLen ) {
		FindPair( a, b, &start, &count );
		if ( count == 0 )
			break;
		int index = start + random.Rand( count );
		char c = arr[index].value[2];
		if ( c ) {
			name->append( &c, 1 );
			a = b;
			b = c;
			c = 0;
			++n;
		}
		else {
			return n < maxLen;
		}
	}
	return false;
}