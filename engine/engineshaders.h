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

	// generated if needed:
	GPUShader lightTexXForm;
	GPUShader blendTexXForm;
	GPUShader emissiveTexXForm;

	enum {
		LIGHT, BLEND, EMISSIVE,		// base
		NONE, TEXXFORM				// mod
	};

	void Generate();
	GPUShader* GetShader( int base, int mod );
};

#endif // ENGINE_SHADERS_INCLUDED