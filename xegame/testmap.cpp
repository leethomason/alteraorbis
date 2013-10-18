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


void TestMap::Draw3D(  const Color3F& colorMult, StencilMode mode, bool /*useSaturation*/ )
{
	GPUStream stream;
	stream.posOffset = 0;
	stream.nPos = 3;
	stream.stride = sizeof( Vector3F );

	FlatShader shader;
	shader.SetColor( colorMult.r*color.r, colorMult.g*color.g, colorMult.b*color.b );
	shader.SetStencilMode( mode );

	GPUDevice* device = GPUDevice::Instance();
	GPUStreamData data;
	device->DrawPtr( shader, stream, data, width*height*4, vertex, width*height*6, index );

	{
		// Debugging coordinate system:
		Vector3F origin = { 0, 0.05f, 0 };
		Vector3F xaxis = { 5, 0, 0 };
		Vector3F zaxis = { 0, 0.05f, 5 };

		FlatShader debug;
		debug.SetColor( 1, 0, 0, 1 );
		device->DrawArrow( debug, origin, xaxis, false, 0.1f );
		debug.SetColor( 0, 0, 1, 1 );
		device->DrawArrow( debug, origin, zaxis, false, 0.1f );
	}
}
