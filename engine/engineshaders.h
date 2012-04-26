#ifndef ENGINE_SHADERS_INCLUDED
#define ENGINE_SHADERS_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "gpustatemanager.h"


class EngineShaders
{
public:
	GPUShader light;
	GPUShader blend;
	GPUShader emissive;

	// generated:
	GPUShader lightTexXForm;
	GPUShader blendTexXForm;
	GPUShader emissiveTexXForm;

	GPUShader lightColor;
	GPUShader blendColor;
	GPUShader emissiveColor;

	enum {
		LIGHT, BLEND, EMISSIVE,		// base
		NONE, TEXXFORM, COLOR		// mod
	};

	void Generate();
	GPUShader* GetShader( int base, int mod );
};

#endif // ENGINE_SHADERS_INCLUDED