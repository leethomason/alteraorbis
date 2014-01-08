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

	const U8* Encode( const U8* in, int nIn, int* nCompressedOut );
	const U8* Decode( const U8* in, int nDecompressed, int* nCompressed );

	// Stream operation. Decode assumes size of decoded data is known.
	void StreamEncode( const void* in, int nIn, FILE* fp, int* nWritten );
	void StreamDecode( void* write, int size, FILE* fp );

	double Ratio() const { return (double)totalOut / (double)totalIn; }
	int totalIn, totalOut;

private:
	int INDEX( unsigned p1, unsigned p2 ) { return p1*256+p2; }

	U8 byte;
	U8 bit;
	const U8* compBuf;

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
