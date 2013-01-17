#ifndef ROCKGEN_INCLUDED
#define ROCKGEN_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glnoise.h"


// Creates a sub-region of rocks.
class RockGen
{
public:
	RockGen( int size );
	~RockGen();

	// HEIGHT style
	enum {
		NOISE_HEIGHT,		// height from separate generator
		USE_HIGH_HEIGHT,	// noise from separate generator, but high
		KEEP_HEIGHT,		// height from noise generator
	};
	int		heightStyle;

	// ROCK style
	enum {
		BOULDERY,			// plasma noise
		CAVEY_SMOOTH,		// fractal noise
		CAVEY_ROUGH
	};

	// Do basic computation.
	void DoCalc( int seed, int rockStyle );
	// Seperate flat land from rock.
	void DoThreshold( int seed, float fractionLand, int heightStyle );

	const U8* Height() const { return heightMap; }

private:
	float Fractal( const grinliz::PerlinNoise* noise, int x, int y, int octave0, int octave1, float octave1Amount );

	int size;
	U8*	heightMap;
};


#endif // ROCKGEN_INCLUDED
