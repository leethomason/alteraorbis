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
		int flags;		// the shader flags that we set (a subset of all the flags the shader is actually using)
		GPUShader* shader;
	};

	// FIXME: switch to HashTable
	grinliz::CDynArray<Node> shaderArr;	
};

#endif // ENGINE_SHADERS_INCLUDED