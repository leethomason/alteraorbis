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

#include "engineshaders.h"
#include "shadermanager.h"


EngineShaders::EngineShaders() : nLight(0), nBlend(0), nEmissive(0)
{
}


EngineShaders::~EngineShaders()
{
	for( int i=0; i<shaderArr.Size(); ++i ) {
		delete shaderArr[i].shader;
	}
}


void EngineShaders::Push( int base, const GPUShader& shader )
{
	switch ( base ) {
	case LIGHT:
		GLASSERT( nLight < STACK );
		light[nLight++] = shader;
		break;
	case BLEND:
		GLASSERT( nBlend < STACK );
		blend[nBlend++] = shader;
		break;
	case EMISSIVE:
		GLASSERT( nEmissive < STACK );
		emissive[nEmissive++] = shader;
		break;
	default:
		GLASSERT( 0 );
	}
}


void EngineShaders::Pop( int base ) 
{
	switch ( base ) {
	case LIGHT:
		GLASSERT( nLight > 0 );
		--nLight;
		break;
	case BLEND:
		GLASSERT( nBlend > 0 );
		--nBlend;
		break;
	case EMISSIVE:
		GLASSERT( nEmissive > 0 );
		--nEmissive;
		break;
	default:
		GLASSERT( 0 );
		break;
	}
}


void EngineShaders::PushAll( const GPUShader& shader )
{
	Push( LIGHT, shader );
	Push( BLEND, shader );
	Push( EMISSIVE, shader );
}


void EngineShaders::PopAll()
{
	Pop( LIGHT );
	Pop( BLEND );
	Pop( EMISSIVE );
}


GPUShader* EngineShaders::GetShader( int base, int flags )
{
	GPUShader* shader = 0;
	if ( flags == 0 ) {
		switch( base ) {
			case LIGHT: return light + nLight - 1;
			case BLEND: return blend + nBlend - 1;
			case EMISSIVE: return emissive + nEmissive - 1;
			default: GLASSERT( 0 ); return emissive;
		}
	}
	for( int i=0; i<shaderArr.Size(); ++i ) {
		const Node& node = shaderArr[i];
		if ( node.base == base && node.flags == flags ) {
			return shaderArr[i].shader;
		}
	}
	Node node = { base, flags, new GPUShader() };
	switch( base ) {
	case LIGHT:
		*node.shader = light[nLight-1];
		break;
	case BLEND:
		*node.shader = blend[nBlend-1];
		break;
	default:
		*node.shader = emissive[nEmissive-1];
		break;
	}
	node.shader->SetShaderFlag( flags );
	shaderArr.Push( node );
	return node.shader;
}


