/*
Copyright (c) 2000-2003 Lee Thomason (www.grinninglizard.com)
Grinning Lizard Utilities.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/


#ifndef GRINLIZ_RANDOM_INCLUDED
#define GRINLIZ_RANDOM_INCLUDED

#include "gldebug.h"
#include "gltypes.h"
#include "glutil.h"

namespace grinliz {

/**	Random number generators (including the rand() in C) are often frustratingly
	non-random in their lower bits. This is a fast random number that is random
	in its lower bits.
*/

class Random
{
  public:
	/// Constructor, with optional seed value.
	Random( U32 seed = 0 )				{ SetSeed( seed ); }
	Random( U32 seed, U32 subseed )		{ SetSeed( seed ); lowCount = subseed;  }

	/** The current seed can be set at any time to
		guarentee a certain sequence of random numbers.
	*/
	void SetSeed( U32 seed )	{	x = 123456789 + seed;	// 0<=x<2^32
									y = 362436;
									z = 521288629;
									
									c = 7654321;	// Range restricted - change carefully!

									lowCount = U8(seed & 0xff);
								}		

	void SetSeed( const char* str );
	void SetSeedFromTime();

	/// Returns a 32 bit random number.
	U32 Rand();						

	/** Returns a random number greater than or equal to 0, and less 
		that 'upperBound'.
	*/	
	U32 Rand( U32 upperBound )		{	// Fold if v is smaller.
										U32 v = Rand();
										v = ( upperBound & 0xffff0000 ) ? v : (v ^ (v>>16) );
										return v % upperBound; }

	int Rand(int upperBound) 		{ return int(Rand(U32(upperBound))); }									

	/** "Roll dice." Has the same bell curve distribution as N dice. Dice start with the 
		value '1', so RandD2( 2, 6 ) returns a value from 2-12
	*/
	int Dice( U32 nDice, U32 sides ) {	
		int total = 0;
		for( U32 i=0; i<nDice; ++i ) { total += (int)Rand(sides)+1; }
		return total;
	}

	/**
		Roll dice - this version rolls extra dice as well (nHigh, nLow)
		and throws out nHigh highest and nLow lowest. This weighs
		the result of (for example) 
		- 3D6 to the higher range (nHigh==1, nLow==0)
		- 3D6 to the lower range (nHigh==0, nLow==1)
		- 3D6 to the center (nHigh==1, nLow==1)
	*/
	int WeightedDice(int nDice, int side, int nHigh, int nLow);

	/** Like Dice, but returns a value between 0 and 1.
	*/
	float DiceUniform( U32 nDice, U32 sides );

	/*
	need to test if this actually works. think range may be wrong.
	float Uniform11()	{
		// http://stackoverflow.com/questions/1557208/generating-random-number-between-1-1-in-c
		// http://www.rgba.org/articles/sfrand/sfrand.htm
		U32 a = Rand();
		a = (a & 0x007fffff) | 0x40000000;  
	    float v = ( *((float*)&a) - 3.0f );
		return v;
	}
	*/

	/// Return a random number from 0 to upper: [0.0,1.0].
	float Uniform()	{
///		static const double INV = 1.0 / double(UINT16_MAX);
//		U32 r = Rand() & UINT16_MAX;
//		return float(double(r) * INV);

		static const float INV = 1.0f / 255.0f;
		U32 r = Rand(256);	// random series in the low bits.
		return float(r) * INV;
	}

	/// Return 0 or 1
	int Bit()
	{
		U32 v = Rand();
		return v & 1;
	}

	/// Return a random boolean.
	bool Boolean()					
	{ 
		return Bit() ? true : false;
	}

	/// Return +1 or -1
	int Sign()					
	{ 
		return -1 + 2*Bit();
	}

	/**
		Given a set of scores (weights), choose a random value. The scores don't have
		to add to any particular value. For an array of [2,1] Select() will return
		0 2/3rds of time, and 1 1/3rd of the time.
	*/
	int Select( const float* scores, int nItems );

	void NormalVector2D( float* v );
	void NormalVector3D( float* v );

	/// Fast hash
	static U32 Hash( const void* data, U32 len );
	/// Fast hash that returns an 8-bit result
	static U8 Hash8( U32 data );

	/// Shuffle an array
	template< class T >
	void ShuffleArray( T* mem, int size ) {
		for( int i=0; i<size; ++i ) {
			int j = Rand(size);
			Swap(&mem[i], &mem[j]);
		}
	}

	// Shuffle an array, but tends to
	// be generally small moves.
	template< class T >
	void SmallShuffle(T* mem, int size) {
		for (int i = 0; i < size; ++i) {
			int j = i - 1 + Rand(3);
			if (j >= 0 && j < size) {
				Swap(&mem[i], &mem[j]);
			}
		}
	}

private:
	U32 x, y, z, c, lowCount;
	enum { 
		COUNT_2D = 256
	};
	static const float normal2D[COUNT_2D*2];
	static U8 series[256];
};


template< class T >
void ShuffleArray( T* mem, int size, Random* random ) {
	for( int i=0; i<size; ++i ) {
		int j = random->Rand(size);
		T temp = mem[i];
		mem[i] = mem[j];
		mem[j] = temp;
	}
}

};	// namespace grinliz

#endif
