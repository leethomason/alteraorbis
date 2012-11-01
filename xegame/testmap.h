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

#ifndef TESTMAP_INCLUDED
#define TESTMAP_INCLUDED

#include "../engine/map.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glcolor.h"

class TestMap : public Map
{
public:
	TestMap( int w, int h );
	~TestMap();

	virtual void Draw3D(  const grinliz::Color3F& colorMult, GPUState::StencilMode );

	void SetColor( const grinliz::Color3F& color ) { this->color = color; }

private:
	grinliz::Vector3F*	vertex;	// 4 vertex quads
	U16*				index;	// 6 index / quad
	grinliz::Color3F	color;
};

#endif // TESTMAP_INCLUDED