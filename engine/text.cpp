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

#include "text.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "../grinliz/glutil.h"
#include "../grinliz/glrectangle.h"
#include "gpustatemanager.h"
#include "shadermanager.h"

using namespace grinliz;


UFOText* UFOText::instance = 0;
void UFOText::Create(Texture* texture)
{
	instance = new UFOText(texture);
}


void UFOText::Destroy()
{
	delete instance;
	instance = 0;
}


UFOText::UFOText(Texture* texture )
{
	this->texture = texture;
	vbo = new GPUVertexBuffer( 0, VBO_MEM );
	ShaderManager::Instance()->AddDeviceLossHandler( this );
}


UFOText::~UFOText()
{
	delete vbo;
	ShaderManager::Instance()->RemoveDeviceLossHandler( this );
}


void UFOText::DeviceLoss()
{
	delete vbo;
	vbo = new GPUVertexBuffer( 0, VBO_MEM );
}

void UFOText::TextOut( const char* str, int _x, int _y, int _h, int* w )
{
	bool rendering = true;
	float x = (float)_x;
	float y = (float)_y;
	float h = (float)_h;

	float width = (float)h * 0.75f;
	float advance = (float)h * 0.50f;
	if ( w ) {
		*w = int(float(strlen(str)) * width);
		return;
	}

	static const float ROWS = 8.0f;
	static const float COLS = 16.0f;

	while( *str )
	{
		int index = *str - 32;
		int r = index / 16;
		int c = index - r * 16;
		if (r <0 || r >= 8) {
			r = c = 0;
		}
		GLASSERT(c >= 0 && c < 16);
		GLASSERT(r >= 0 && r < 8);

		float tx0 = float(c) / COLS;
		float tx1 = tx0 + 1.0f / COLS;
		float ty0 = 1.0f - float(r) / ROWS;
		float ty1 = ty0 - 1.0f / ROWS;

		PTVertex2* vBuf = quadBuf.PushArr(4);
		vBuf[0].tex.Set( tx0, ty0 );
		vBuf[1].tex.Set( tx0, ty1 );
		vBuf[2].tex.Set( tx1, ty1 );
		vBuf[3].tex.Set( tx1, ty0 );

		vBuf[0].pos.Set(x, y);
		vBuf[1].pos.Set(x, y + h);
		vBuf[2].pos.Set(x + width, y + h);
		vBuf[3].pos.Set(x + width, y);

		x += advance;
		++str;
	}
}


void UFOText::FinalDraw()
{
	int n = quadBuf.Size();
	if ( quadBuf.Size() * sizeof(PTVertex2) > VBO_MEM ) {
		n = VBO_MEM / sizeof(PTVertex2);
	}
	vbo->Upload( quadBuf.Mem(), n*sizeof(PTVertex2), 0 );

	CompositingShader shader( BLEND_NORMAL );

	PTVertex2 v;
	GPUStream stream( v );
	GPUStreamData data;
	data.texture0 = texture;
	data.vertexBuffer = vbo->ID();
	GPUDevice::Instance()->DrawQuads( shader, stream, data, n/4 );

	quadBuf.Clear();
}


void UFOText::Draw( int x, int y, const char* format, ... )
{
    va_list     va;
	const int	size = 1024;
    char		buffer[size];

    //
    //  format and output the message..
    //
    va_start( va, format );
#ifdef _MSC_VER
    vsnprintf_s( buffer, size, _TRUNCATE, format, va );
#else
	// Reading the spec, the size does seem correct. The man pages
	// say it will aways be null terminated (whereas the strcpy is not.)
	// Pretty nervous about the implementation, so force a null after.
    vsnprintf( buffer, size, format, va );
	buffer[size-1] = 0;
#endif
	va_end( va );

    TextOut( buffer, x, y, 16, 0 );
}
