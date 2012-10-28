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