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


EngineShaders::EngineShaders()
{
}


EngineShaders::~EngineShaders()
{
	for( int i=0; i<shaderArr.Size(); ++i ) {
		delete shaderArr[i].shader;
	}
}


GPUShader* EngineShaders::GetShader( int base, int flags )
{
	GPUShader* shader = 0;
	if ( flags == 0 ) {
		switch( base ) {
			case LIGHT: return &light;
			case BLEND: return &blend;
			default: return &emissive;
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
		*node.shader = light;
		break;
	case BLEND:
		*node.shader = blend;
		break;
	default:
		*node.shader = emissive;
		break;
	}
	node.shader->SetShaderFlag( flags );
	shaderArr.Push( node );
	return node.shader;
}



void EngineShaders::SetEmissiveEx()
{
	emissive.SetShaderFlag( ShaderManager::EMISSIVE_EXCLUSIVE ); 
	for( int i=0; i<shaderArr.Size(); ++i ) {
		if ( shaderArr[i].base == EMISSIVE ) {
			shaderArr[i].shader->SetShaderFlag( ShaderManager::EMISSIVE_EXCLUSIVE ); 
		}
	}
}


void EngineShaders::ClearEmissiveEx()
{
	emissive.ClearShaderFlag( ShaderManager::EMISSIVE_EXCLUSIVE ); 
	for( int i=0; i<shaderArr.Size(); ++i ) {
		if ( shaderArr[i].base == EMISSIVE ) {
			shaderArr[i].shader->ClearShaderFlag( ShaderManager::EMISSIVE_EXCLUSIVE ); 
		}
	}
}
