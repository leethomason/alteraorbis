/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
		NOISE_HIGH_HEIGHT,	// noise from separate generator, but high
		KEEP_HEIGHT,		// height from noise generator
	};
	int		heightStyle;

	// ROCK style
	enum {
		BOULDERY,			// plasma noise
		CAVEY
	};

	// Do basic computation.
	void StartCalc( int seed );
	void DoCalc( int y );
	void EndCalc();

	// Seperate flat land from rock.
	void DoThreshold( int seed, float fractionLand, int heightStyle );

	const U8* Height() const { return heightMap; }

private:
	float Fractal( int x, int y, float octave0, float octave1, float octave1Amount );

	grinliz::PerlinNoise* noise0;
	grinliz::PerlinNoise* noise1;

	int size;
	U8*	heightMap;
};


#endif // ROCKGEN_INCLUDED
