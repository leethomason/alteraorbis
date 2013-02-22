#ifndef SQUISHER_INCLUDED
#define SQUISHER_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"

class Squisher
{
public:
	Squisher();
	~Squisher();

	const U8* Encode( const U8* in, int nIn, int* nCompressedOut );
	const U8* Decode( const U8* in, int nDecompressed, int* nCompressed );

	double Ratio() const { return (double)totalOut / (double)totalIn; }
	int totalIn, totalOut;

private:
	int INDEX( unsigned p1, unsigned p2 ) { return p1*256+p2; }

	U8 byte;
	U8 bit;
	const U8* compBuf;

	U8* table;
	int p1, p2;

	grinliz::CDynArray<U8> buffer;

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
	void FlushBits() {
		if ( bit ) {
			buffer.Push( byte );
			bit = byte = 0;
		}
	}

	int PopBit() {
		int result = ((*compBuf) & (1<<bit)) ? 1 : 0;
		++bit;
		if ( bit == 8 ) {
			++compBuf;
			bit = 0;
		}
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
