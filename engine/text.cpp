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
void UFOText::Create( const gamedb::Reader* db, Texture* texture )
{
	instance = new UFOText( db, texture );
}


void UFOText::Destroy()
{
	delete instance;
	instance = 0;
}


UFOText::UFOText( const gamedb::Reader* database, Texture* texture )
{
	this->database = database;
	this->texture = texture;
	this->fixedWidth = false;

	memset( metricCache, 0, sizeof(Metric)*CHAR_RANGE );
	memset( kerningCache, 100, CHAR_RANGE*CHAR_RANGE );

	const gamedb::Item* fontItem = database->Root()->Child( "data" )
												   ->Child( "fonts" )
												   ->Child( "font" );
	fontItem = database->ChainItem( fontItem );
	const gamedb::Item* infoItem = fontItem->Child( "info" );
	const gamedb::Item* commonItem = fontItem->Child( "common" );

	fontSize = (float)infoItem->GetInt( "size" );
	texWidthInv = 1.0f / (float)commonItem->GetInt( "scaleW" );
	texHeight = (float)commonItem->GetInt( "scaleH" );
	texHeightInv = 1.0f / texHeight;

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


void UFOText::CacheMetric( int c )
{

	char buffer[] = "char0";
	buffer[4] = (char)c;

	const gamedb::Item* fontItem = database->Root()->Child( "data" )
												   ->Child( "fonts" )
												   ->Child( "font" );
	fontItem = database->ChainItem( fontItem );
	const gamedb::Item* charItem = fontItem->Child( "chars" )->Child( buffer );

	int index = MetricIndex( c );
	GLASSERT( index >= 0 && index < CHAR_RANGE );
	Metric* mc = &metricCache[ index ];
	mc->isSet = 1;

	if ( charItem ) {
		mc->advance = charItem->GetInt( "xadvance" );

		mc->x =		 charItem->GetInt( "x" );
		mc->y =		 charItem->GetInt( "y" );
		mc->width =  charItem->GetInt( "width" );
		mc->height = charItem->GetInt( "height" );

		mc->xoffset = charItem->GetInt( "xoffset" );
		mc->yoffset = charItem->GetInt( "yoffset" );
	}
}


void UFOText::CacheKern( int c, int cPrev )
{
	char kernbuf[] = "kerning00";
	kernbuf[7] = (char)cPrev;
	kernbuf[8] = (char)c;

	const gamedb::Item* fontItem = database->Root()->Child( "data" )
												   ->Child( "fonts" )
												   ->Child( "font" );

	const gamedb::Item* kernsItem = fontItem->Child( "kernings" );
	const gamedb::Item* kernItem = 0;
	
	int index = KernIndex( c, cPrev );
	GLASSERT( index >= 0 && index < CHAR_RANGE*CHAR_RANGE );
	kerningCache[index] = 0;
	
	if ( kernsItem ) {
		kernItem = kernsItem->Child( kernbuf );
		if ( kernItem ) {
			kerningCache[index] = kernItem->GetInt( "amount" );
		}
	}
}



void UFOText::Metrics(	int c, int cPrev,
						float lineHeight,
						gamui::IGamuiText::GlyphMetrics* metric )
{
	if ( c < 0 )  c += 256;
	if ( cPrev < 0 ) cPrev += 256;

	float s2 = 1.f;
	if ( c > 128 ) {
		s2 = 0.75f;
		c -= 128;
	}

	if ( c < CHAR_OFFSET || c >= END_CHAR ) {
		c = ' ';
		cPrev = 0;
	}
	const Metric& mc = metricCache[ MetricIndex( c ) ];
	if ( !mc.isSet ) CacheMetric( c );

	float kern = 0;
	if (    c >= CHAR_OFFSET && c < END_CHAR
		 && cPrev >= CHAR_OFFSET && cPrev < END_CHAR ) 
	{
		int kernI = kerningCache[ KernIndex( c, cPrev ) ];
		if ( kernI == 100 ) {
			CacheKern( c, cPrev );
			kernI = kerningCache[ KernIndex( c, cPrev ) ];
		}

		if ( kernI && s2 == 1.0f ) {
			kern = (float)kernI;
		}
	}

	float scale = (float)lineHeight / fontSize;
	if ( mc.width ) {
		metric->advance = ((float)mc.advance+kern) * scale * s2;

		float x = (float)mc.x;
		float y = (float)mc.y;
		float width = (float)mc.width;
		float height = (float)mc.height;

		metric->x = ((float)mc.xoffset+kern) * scale;
		metric->w = width * scale * s2;
		metric->y = (float)mc.yoffset * scale;
		metric->h = height * scale * s2;

		metric->tx0 = x * texWidthInv;
		metric->tx1 = (x + width) * texWidthInv;
		metric->ty0 = (texHeight - 1.f - y) * texHeightInv;
		metric->ty1 = (texHeight - 1.f - y - height) * texHeightInv;
	}
	else {
		metric->advance = lineHeight * 0.4f;

		metric->x = metric->y = metric->w = metric->h = 0;
		metric->tx0 = metric->tx1 = metric->ty0 = metric->ty1 = 0;
	}
}


void UFOText::TextOut( const char* str, int _x, int _y, int _h, int* w )
{
	bool rendering = true;
	float x = (float)_x;
	float y = (float)_y;
	float h = (float)_h;

	if ( w ) {
		*w = 0;
		rendering = false;
	}

	while( *str )
	{
		gamui::IGamuiText::GlyphMetrics metric;
		Metrics( *str, 0, (float)h, &metric );

		if ( fixedWidth ) {
			float cw = (float)h * 0.5f;
			metric.advance = cw;

			if ( metric.w > cw ) {
				metric.x = 0;
				metric.w = cw;
			}
			else {
				metric.x = (cw - metric.w) * 0.5f;
			}
		}

		if ( rendering ) {
			PTVertex2* vBuf = quadBuf.PushArr(4);
			vBuf[0].tex.Set( metric.tx0, metric.ty0 );
			vBuf[1].tex.Set( metric.tx0, metric.ty1 );
			vBuf[2].tex.Set( metric.tx1, metric.ty1 );
			vBuf[3].tex.Set( metric.tx1, metric.ty0 );

			vBuf[0].pos.Set( (float)x+metric.x,				(float)y+metric.y );	
			vBuf[1].pos.Set( (float)x+metric.x,				(float)y+metric.y+metric.h );				
			vBuf[2].pos.Set( (float)x+metric.x+metric.w,	(float)y+metric.y+metric.h );	
			vBuf[3].pos.Set( (float)x+metric.x+metric.w,	(float)y+metric.y );	
		}
		x += metric.advance;
		++str;
	}
	if ( w ) {
		*w = int(x+0.5f) - _x;
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
