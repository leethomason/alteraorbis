#include "squisher.h"
#include <string.h>

Squisher::Squisher()
{
	table = new U8[256*256];
	memset( table, 0, 256*256 );
	totalIn = totalOut = 0;
	stream = 0;
}


Squisher::~Squisher()
{
	delete [] table;
}


void Squisher::FlushBits() {
	if ( bit ) {
		if ( stream ) {
			fputc( byte, stream );
		}
		else {
			buffer.Push( byte );
		}
		bit = byte = 0;
	}
}


U8 Squisher::ReadByte()
{
	if ( stream ) {
		return fgetc( stream );
	}
	else {
		U8 b = *compBuf;
		++compBuf;
		return b;
	}
}

const U8* Squisher::Encode( const U8* in, int nIn, int* nCompressedOut )
{
	buffer.Clear();
	int p1 = 0, p2 = 0;
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


void Squisher::StreamEncode( const void* in, int nIn, FILE* fp, int* nWritten )
{
	stream = fp;
	Encode( (const U8*) in, nIn, nWritten );
	stream = 0;
}


void Squisher::StreamDecode( void* _write, int nDecom, FILE* fp )
{
	int p1 = 0, p2 = 0;
	bit = byte = 0;
	U8* write = (U8*)_write;
	stream = fp;

	for( int i=0; i<nDecom; ++i ) {
		int b = PopBit();

		int c = 0;
		if ( b == 1 ) {
			c = table[INDEX(p1,p2)];
			*write = c;
			++write;
		}
		else {
			c = Pop8Bits();
			*write = c;
			++write;
			table[INDEX(p1,p2)] = c;
		}
		p2 = p1;
		p1 = c;
	}
	stream = 0;
}


const U8* Squisher::Decode( const U8* in, int nDecom, int* nCompressed )
{
	buffer.Clear();
	compBuf = in;
	int p1=0, p2=0;
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


