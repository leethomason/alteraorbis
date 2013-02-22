#include "squisher.h"
#include <string.h>

Squisher::Squisher()
{
	table = new U8[256*256];
	memset( table, 0, 256*256 );
	totalIn = totalOut = 0;
}


Squisher::~Squisher()
{
	delete [] table;
}


const U8* Squisher::Encode( const U8* in, int nIn, int* nCompressedOut )
{
	buffer.Clear();
	p1 = p2 = 0;
	bit = byte = 0;

	const U8* p = in;
	for( int i=0; i<nIn; ++i, ++p ) {
		U8 ref = table[INDEX(p1,p2)];
		if ( ref == *p ) {
			PushBit( 1 );
		}
		else {
			PushBit( 0 );
			Push8Bits( *p );
			table[INDEX(p1,p2)] = *p;
		}
		p2 = p1;
		p1 = *p;
	}
	FlushBits();
	*nCompressedOut = buffer.Size();

	totalIn += nIn;
	totalOut += buffer.Size();

	return buffer.Mem();
}


const U8* Squisher::Decode( const U8* in, int nDecom, int* nCompressed )
{
	buffer.Clear();
	compBuf = in;
	p1 = p2 = 0;
	bit = byte = 0;

	for( int i=0; i<nDecom; ++i ) {
		int b = PopBit();

		int c = 0;
		if ( b == 1 ) {
			c = table[INDEX(p1,p2)];
			buffer.Push( c );
		}
		else {
			c = Pop8Bits();
			buffer.Push( c );
			table[INDEX(p1,p2)] = c;
		}
		p2 = p1;
		p1 = c;
	}
	*nCompressed = compBuf - in;
	if ( bit ) {
		*nCompressed += 1;
	}
	return buffer.Mem();
}


