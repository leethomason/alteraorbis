#include "engineshaders.h"
#include "shadermanager.h"

/*
void EngineShaders::Generate()
{
	lightTexXForm = light;
	lightTexXForm.SetShaderFlag( ShaderManager::TEXTURE0_TRANSFORM );
	lightColor = light;
	lightColor.SetShaderFlag( ShaderManager::COLOR_PARAM );

	blendTexXForm = blend;
	blendTexXForm.SetShaderFlag( ShaderManager::TEXTURE0_TRANSFORM );
	blendColor = blend;
	blendColor.SetShaderFlag( ShaderManager::COLOR_PARAM );

	emissiveTexXForm = emissive;
	emissiveTexXForm.SetShaderFlag( ShaderManager::TEXTURE0_TRANSFORM );
	emissiveColor = emissive;
	emissiveColor.SetShaderFlag( ShaderManager::COLOR_PARAM );
}
*/


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
