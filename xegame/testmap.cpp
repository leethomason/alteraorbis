#include "testmap.h"

using namespace grinliz;

TestMap::TestMap( int w, int h ) : Map( w, h )
{
	int size = w*h;
	color.Set( 1, 1, 1 );
	vertex = new Vector3F[ size*4 ];
	index  = new U16[ size*6 ];

	for( int j=0; j<h; ++j ) {
		for( int i=0; i<w; ++i ) {
			int idx = j*w+i;
			GLASSERT( idx < size );
			vertex[ idx*4 + 0 ].Set( (float)i,      0, (float)j );
			vertex[ idx*4 + 1 ].Set( (float)i+0.9f, 0, (float)j );
			vertex[ idx*4 + 2 ].Set( (float)i+0.9f, 0, (float)j+0.9f );
			vertex[ idx*4 + 3 ].Set( (float)i,      0, (float)j+0.9f );

			index[ idx*6 + 0 ] = idx*4 + 0;
			index[ idx*6 + 2 ] = idx*4 + 1;
			index[ idx*6 + 1 ] = idx*4 + 2;
			index[ idx*6 + 3 ] = idx*4 + 0;
			index[ idx*6 + 5 ] = idx*4 + 2;
			index[ idx*6 + 4 ] = idx*4 + 3;
		}
	}
	int *test = new int[14];
	delete [] test;
}


TestMap::~TestMap()
{
	delete [] vertex;
	delete [] index;
}


void TestMap::Draw3D(  const Color3F& colorMult, GPUShader::StencilMode mode )
{
	GPUStream stream;

	stream.posOffset = 0;
	stream.nPos = 3;
	stream.stride = sizeof( Vector3F );

	FlatShader shader;
	shader.SetColor( colorMult.r*color.r, colorMult.g*color.g, colorMult.b*color.b );
	shader.SetStencilMode( mode );
	shader.SetStream( stream, vertex, width*height*6, index );
	shader.Draw();
}
