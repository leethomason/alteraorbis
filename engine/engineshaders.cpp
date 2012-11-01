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
}


void EngineShaders::Push( int base, const GPUState& state )
{
	switch( base ) {
	case LIGHT:		light.Push( state ); break;
	case BLEND:		blend.Push( state ); break;
	case EMISSIVE:	emissive.Push( state );	break;
	default: GLASSERT( 0 );
	}
}


void EngineShaders::PushAll( const GPUState& state )
{
	light.Push( state );
	blend.Push( state );
	emissive.Push( state );
}


void EngineShaders::Pop( int base )
{
	switch( base ) {
	case LIGHT:	light.Pop();	break;
	case BLEND: blend.Pop();	break;
	case EMISSIVE: emissive.Pop(); break;
	default: GLASSERT(0);
	}
}


void EngineShaders::PopAll()
{
	light.Pop();
	blend.Pop();
	emissive.Pop();
}


void EngineShaders::GetState( int base, int flags, GPUState* state )
{
	switch( base ) {
	case LIGHT:
		*state = light[light.Size()-1];
		break;
	case BLEND:
		*state = blend[blend.Size()-1];
		break;
	default:
		*state = emissive[emissive.Size()-1];
		break;
	}
	state->SetShaderFlag( flags );
}


