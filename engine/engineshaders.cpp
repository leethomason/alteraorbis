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
	int f = flags;
	switch( base ) {
	case LIGHT:
		f |= light.ShaderFlags();
		break;
	case BLEND:
		f |= blend.ShaderFlags();
		break;
	default:
		f |= emissive.ShaderFlags();
		break;
	}
	
	GPUShader* shader = ShaderManager::


	for( int i=0; i<shaderArr.Size(); ++i ) {
		const Node& node = shaderArr[i];
		if ( node.base == base && node.flags == flags ) {
			return shaderArr[i].shader;
		}
	}
	Node node = { base, flags, new GPUShader() };

	node.shader->SetShaderFlag( flags );
	shaderArr.Push( node );
	return node.shader;
}


