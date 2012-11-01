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

	enum {
		LIGHT=1, BLEND=2, EMISSIVE=4		// base
	};

	void GetState( int base, int flags, GPUState* state );

	void Push( int base, const GPUState& state );
	void PushAll( const GPUState& state );
	void Pop( int base );
	void PopAll();

private:
	grinliz::CArray< GPUState, 2 > light;
	grinliz::CArray< GPUState, 2 > blend;
	grinliz::CArray< GPUState, 2 > emissive;
};
#endif // ENGINE_SHADERS_INCLUDED