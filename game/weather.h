#ifndef LUMOS_WEATHER_INCLUDED
#define LUMOS_WEATHER_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glutil.h"

class Weather
{
public:
	Weather( int p_width, int p_height ) : width((float)p_width), height((float)p_height) {}

	float RainFraction( int p_x, int p_y ) {
		float x = (float)p_x;
		float y = (float)p_y;

		static const float MIN_RAIN = 0.10f;
		static const float MAX_RAIN = 0.90f;
		static const float FUZZ = 0.05f;

		// More rain in the West.
		// Prevailing wind from West to East.
		float r = grinliz::Lerp( MAX_RAIN, MIN_RAIN, x / width );

		int m = p_x ^ p_y;
		static const float delta[8] = { -1.0f, -0.75f, -0.50f, -0.25f, 0.25f, 0.50f, 0.75f, 1.0f };
		r += delta[m&7] * FUZZ;

		return r;
	}

	float Temperature( int, int p_y ) {
		float y = (float)p_y;
		return grinliz::Lerp( 0.0f, 1.0f, y/height );
	}

private:
	float width;
	float height;
};

#endif // LUMOS_WEATHER_INCLUDED
