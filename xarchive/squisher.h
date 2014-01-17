#ifndef SQUISHER_INCLUDED
#define SQUISHER_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"

#include <stdio.h>


class Squisher
{
public:
	Squisher();
	~Squisher();

	// Encode / Decode to memory.
	const U8* Encode( const U8* in, int nIn, int* nCompressedOut );
	const U8* Decode( const U8* in, int nDecompressed, int* nCompressed );

	// Encode / Decode a block to a file.
	// Use StreamEncode( 0, 0, fp )	for final flush.
	void StreamEncode( const void* in, int nIn, FILE* fp );
	void StreamDecode( void* write, int size, FILE* fp );

	int encodedU, encodedC, decodedU, decodedC;

private:
	int INDEX( unsigned p1, unsigned p2 ) { return p1*256+p2; }

	U8 byte;
	U8 bit;
	const U8* compBuf;
	int streamP1;
	int streamP2;

	U8* table;

	grinliz::CDynArray<U8> buffer;
	FILE* stream;

	void PushBit( int b ) {
		byte |= (b<<bit);
		++bit;
		if ( bit == 8 ) {
			FlushBits();
		}
	}
	void Push8Bits( int b ) {
		for( int i=0; i<8; ++i ) {
			PushBit( (b>>i) & 1 );
		}
	}

	void FlushBits();
	U8 ReadByte();

	int PopBit() {
		if ( bit == 0 ) { 
			byte = ReadByte();
			bit = 8;
		}
		// read low bit first: hence 8-bit
		int result = (byte & (1<<(8-bit))) ? 1 : 0;
		--bit;
		return result;
	}

	int Pop8Bits() {
		int result = 0;
		for( int i=0; i<8; ++i ) {
			result |= (PopBit() << i);
		}
		return result;
	}
};

#endif // SQUISHER_INCLUDED
