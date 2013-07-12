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

#ifndef LUMOS_WEATHER_INCLUDED
#define LUMOS_WEATHER_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glrandom.h"

class Weather
{
public:
	Weather( int p_width, int p_height ) : width((float)p_width), height((float)p_height) {}

	float RainFraction( float x, float y ) {

		static const float MIN_RAIN = 0.05f;
		static const float MAX_RAIN = 0.95f;
		static const float FUZZ     = 0.05f;

		// More rain in the West.
		// Prevailing wind from West to East.
		float r = grinliz::Lerp( MAX_RAIN, MIN_RAIN, x / width );
		static const float delta[8] = { -1.0f, -0.75f, -0.50f, -0.25f, 0.25f, 0.50f, 0.75f, 1.0f };
		
		int xi = (int)x;
		int yi = (int)y;
		int sx = xi / SECTOR_SIZE;
		int sy = yi / SECTOR_SIZE;
		int m00 = grinliz::Random::Hash8( sy*NUM_SECTORS    +sx ) & 7;
		int m10 = grinliz::Random::Hash8( (sy+1)*NUM_SECTORS+sx ) & 7;
		int m01 = grinliz::Random::Hash8( sy*NUM_SECTORS    +(sx+1) ) & 7;
		int m11 = grinliz::Random::Hash8( (sy+1)*NUM_SECTORS+(sx+1) ) & 7;

		float q[4] = {
			r + delta[m00]*FUZZ,
			r + delta[m10]*FUZZ,
			r + delta[m01]*FUZZ,
			r + delta[m11]*FUZZ,
		};
		float rain = grinliz::BilinearInterpolate( q[0], q[1], q[2], q[3], x-(float)xi, y-(float)yi );
		return rain;
	}

	float Temperature( float, float y ) {
		return grinliz::Lerp( 0.0f, 1.0f, y/height );
	}

private:
	float width;
	float height;
};

#endif // LUMOS_WEATHER_INCLUDED
