#include "engineshaders.h"
#include "shadermanager.h"

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


GPUShader* EngineShaders::GetShader( int base, int mod )
{
	GPUShader* shader = 0;
	switch( base ) {
		case LIGHT:
			shader = &light;
			if ( mod==TEXXFORM )
				shader = &lightTexXForm;
			else if ( mod==COLOR )
				shader = &lightColor;
			break;
		case BLEND:
			shader = &blend;
			if ( mod==TEXXFORM )
				shader = &blendTexXForm;
			else if ( mod==COLOR )
				shader = &blendColor;
			break;
		case EMISSIVE:
			shader = &emissive;
			if ( mod==TEXXFORM )
				shader = &emissiveTexXForm;
			else if ( mod==COLOR )
				shader = &emissiveColor;
			break;
	}
	return shader;
}



