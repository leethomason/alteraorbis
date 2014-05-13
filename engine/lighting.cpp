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

#include "lighting.h"
#include "serialize.h"
#include "../grinliz/glgeometry.h"

using namespace grinliz;
using namespace tinyxml2;


Lighting::Lighting() 
{
	midLight.diffuse.Set( 0.7f, 0.7f, 0.7f );
	midLight.ambient.Set( 0.3f, 0.3f, 0.3f );
	midLight.shadow.Set( 0.3f, 0.3f, 0.3f );

	warm = cool = midLight;

	direction.Set( 2.0f, 3.0f, 1.0f );
	direction.Normalize();
	glow.Set( 1, 1, 1 );
}


void Lighting::SetLightDirection( const grinliz::Vector3F* lightDir )
{
	if ( lightDir ) {
		direction = *lightDir;
	}
	else {
		direction.Set( 2.f, 3.f, 1.f );
	}
	direction.Normalize();
}


void Lighting::CalcLight( const Vector3F& normal, float shadowAmount, Color3F* light, Color3F* shadowResult, float temp )
{
	Color4F ambient, diffuse, shadow;
	Query( &diffuse, &ambient, &shadow, temp, 0 );

	float nDotL = DotProduct( normal, direction );
	for( int i=0; i<3; ++i ) {
		light->X(i)  = Lerp( ambient.X(i), diffuse.X(i), (nDotL+1.f)*0.5f );
		shadowResult->X(i) = Lerp( shadow.X(i), light->X(i), 1.f-shadowAmount ); 
	}

#if 0
	float nDotL = Max( 0.0f, DotProduct( normal, direction ) );
		for( int i=0; i<3; ++i ) {
			light->X(i)  = ambient.X(i) + diffuse.X(i)*nDotL;
			shadowResult->X(i) = Lerp( shadow.X(i), light->X(i), 1.f-shadowAmount ); 
		}
	}
#endif
}


void Lighting::Query(	grinliz::Color4F* diffuse,
						grinliz::Color4F* ambient,
						grinliz::Color4F* shadow,
						float temperature,
						grinliz::Vector4F* _dir )
{
	if ( temperature == 0 ) {
		diffuse->Set( midLight.diffuse.r, midLight.diffuse.g, midLight.diffuse.b, 1 );
		ambient->Set( midLight.ambient.r, midLight.ambient.g, midLight.ambient.b, 1 );
		shadow->Set(  midLight.shadow.r,  midLight.shadow.g,  midLight.shadow.b,  1 );
	}
	else if ( temperature < 0 ) {
		for( int i=0; i<3; ++i ) {
			diffuse->X(i) = Lerp( midLight.diffuse.X(i), cool.diffuse.X(i), -temperature );
			ambient->X(i) = Lerp( midLight.ambient.X(i), cool.ambient.X(i), -temperature );
			shadow->X(i)  = Lerp( midLight.shadow.X(i),	 cool.shadow.X(i),  -temperature );
		}
	}
	else {
		for( int i=0; i<3; ++i ) {
			diffuse->X(i) = Lerp( midLight.diffuse.X(i), warm.diffuse.X(i), temperature );
			ambient->X(i) = Lerp( midLight.ambient.X(i), warm.ambient.X(i), temperature );
			shadow->X(i)  = Lerp( midLight.shadow.X(i),	 warm.shadow.X(i),  temperature );
		}
	}
	diffuse->a = 1;
	ambient->a = 1;
	shadow->a = 1;

	if ( _dir ) {
		_dir->Set( direction.x, direction.y, direction.z, 0 );
	}
}




void Lighting::Load( const tinyxml2::XMLElement* ele )
{
	const tinyxml2::XMLElement* modeEle = ele->FirstChildElement( "hemispherical" );
	const tinyxml2::XMLElement* child = 0;

	if ( modeEle ) {
		LoadDAS( modeEle, &midLight );
	}
	warm = cool = midLight;

	if ( modeEle && modeEle->FirstChildElement( "warm" ) ) {
		LoadDAS( modeEle->FirstChildElement( "warm" ), &warm );
	}
	if ( modeEle && modeEle->FirstChildElement( "cool" ) ) {
		LoadDAS( modeEle->FirstChildElement( "cool" ), &cool );
	}

	child = ele->FirstChildElement( "direction" );
	if ( child ) {
		LoadVector( child, &direction );
		direction.Normalize();
	}
	child = ele->FirstChildElement( "glow" );
	if ( child ) {
		LoadColor( child, &glow );
	}
}


void Lighting::LoadDAS( const tinyxml2::XMLElement* element, Light* light )
{
	const XMLElement* child = element->FirstChildElement( "ambient" );
	if ( child ) {
		LoadColor( child, &light->ambient );
	}
	child = element->FirstChildElement( "diffuse" );
	if ( child ) {
		LoadColor( child, &light->diffuse );
	}
	child = element->FirstChildElement( "shadow" );
	if ( child ) {
		LoadColor( child, &light->shadow );
	}
}



