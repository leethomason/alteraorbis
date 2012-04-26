#include "engineshaders.h"
#include "shadermanager.h"

void EngineShaders::Generate()
{
	lightTexXForm = light;
	lightTexXForm.SetShaderFlag( ShaderManager::TEXTURE0_TRANSFORM );

	blendTexXForm = blend;
	blendTexXForm.SetShaderFlag( ShaderManager::TEXTURE0_TRANSFORM );

	emissiveTexXForm = emissive;
	emissiveTexXForm.SetShaderFlag( ShaderManager::TEXTURE0_TRANSFORM );
}


GPUShader* EngineShaders::GetShader( int base, int mod )
{
	GPUShader* shader = 0;
	switch( base ) {
		case LIGHT:
			shader = (mod==TEXXFORM) ? &lightTexXForm : &light;
			break;
		case BLEND:
			shader = (mod==TEXXFORM) ? &blendTexXForm : &blend;
			break;
		case EMISSIVE:
			shader = (mod==TEXXFORM) ? &emissiveTexXForm : &emissive;
			break;
	}
	return shader;
}



