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

#ifndef XENOENGINE_LIGHTING_INCLUDED
#define XENOENGINE_LIGHTING_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glcolor.h"
#include "../tinyxml2/tinyxml2.h"

class Lighting
{
public:
	Lighting();

	struct Light {
		grinliz::Color3F	diffuse;		// main diffuse light
		grinliz::Color3F	ambient;		// ambient or key light
		grinliz::Color3F	shadow;			// color of stuff in shadow (which is only flat planes)
	};

	Light midLight;
	Light warm;
	Light cool;

	grinliz::Vector3F	direction;		// normal vector
	bool hemispheric;
	grinliz::Color3F	glow;			// additive glow multiplier

	// Direction from world TO sun. (y is positive). If null, sets the default.
	void SetLightDirection( const grinliz::Vector3F* lightDir );

	// Calculate the light hitting a point with the given surface normal
	// temp: -1 cool, 0 mid, 1 warm
	void CalcLight(	const grinliz::Vector3F& surfaceNormal,
					float shadowAmount,				// 1.0: fully in shadow
					grinliz::Color3F* light, 
					grinliz::Color3F* shadow,
					float temp );
	
	// query the values for shader setup
	void Query( grinliz::Color4F* diffuse,
				grinliz::Color4F* ambient,
				grinliz::Color4F* shadow,
				float temperature,
				grinliz::Vector4F* _dir );
	void Load( const tinyxml2::XMLElement* element );

private:
	void LoadDAS( const tinyxml2::XMLElement* element, Light* light );
};


#endif // XENOENGINE_LIGHTING_INCLUDED