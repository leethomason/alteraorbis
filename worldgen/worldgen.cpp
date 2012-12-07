// worldgen.cpp : Defines the entry point for the console application.
//

#include "../shared/lodepng.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glnoise.h"

using namespace grinliz;

int main(int argc, const char* argv[])
{
	static const int WIDTH = 256;
	static const int HEIGHT = 256;
	int seed = 20;

	if ( argc >= 2 ) {
		seed = atoi( argv[1] );
	}

	Color4U8* pixels = new Color4U8[WIDTH*HEIGHT];
	memset( pixels, 0xa0, sizeof(Color4U8)*WIDTH*HEIGHT );

	lodepng_encode32_file( "worldgen.png", (const unsigned char*)pixels, WIDTH, HEIGHT );

	PerlinNoise noise( seed );
	for( int j=0; j<HEIGHT; ++j ) {
		for( int i=0; i<WIDTH; ++i ) {
			float f = noise.Noise( (float)i/(float)(WIDTH/2), (float)j/(float)(HEIGHT/2), 0.0f );
			int v = (int)((f+1.0f)*127.0);
			pixels[j*WIDTH+i].Set( v, v, v, 255 );
		}
	}

	delete [] pixels;
	return 0;
}

