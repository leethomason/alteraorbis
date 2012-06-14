#ifndef ENGINE_SHADERS_INCLUDED
#define ENGINE_SHADERS_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glcontainer.h"
#include "gpustatemanager.h"


class EngineShaders
{
public:
	EngineShaders();
	~EngineShaders();

	GPUShader light;
	GPUShader blend;
	GPUShader emissive;

	enum {
		LIGHT, BLEND, EMISSIVE		// base
	};

	GPUShader* GetShader( int base, int flags );
	
	// Special functions for glow:
	void SetEmissiveEx();
	void ClearEmissiveEx();

private:
	// These things can't move in memory.
	struct Node {
		int base;
		GPUShader* shader;
	};

	grinliz::CDynArray<Node> shaderArr;	
};

#endif // ENGINE_SHADERS_INCLUDED