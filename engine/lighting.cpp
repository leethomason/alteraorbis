#include "lighting.h"
#include "serialize.h"
#include "../grinliz/glgeometry.h"

using namespace grinliz;
using namespace tinyxml2;


Lighting::Lighting() 
{
	diffuse.Set( 0.7f, 0.7f, 0.7f );
	ambient.Set( 0.3f, 0.3f, 0.3f );
	shadow.Set( 0.3f, 0.3f, 0.3f );
	direction.Set( 2.0f, 3.0f, 1.0f );
	direction.Normalize();
	hemispheric = false;
	glow.Set( 1, 1, 1 );
	glowRadius = 0.01f;
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


void Lighting::CalcLight( const Vector3F& normal, float shadowAmount, Color3F* light, Color3F* shadowResult )
{
	if ( hemispheric ) {
		float nDotL = DotProduct( normal, direction );
		for( int i=0; i<3; ++i ) {
			light->X(i)  = Lerp( ambient.X(i), diffuse.X(i), (nDotL+1.f)*0.5f );
			shadowResult->X(i) = Lerp( shadow.X(i), light->X(i), 1.f-shadowAmount ); 
		}
	}
	else {
		float nDotL = Max( 0.0f, DotProduct( normal, direction ) );
		for( int i=0; i<3; ++i ) {
			light->X(i)  = ambient.X(i) + diffuse.X(i)*nDotL;
			shadowResult->X(i) = Lerp( shadow.X(i), light->X(i), 1.f-shadowAmount ); 
		}
	}
}



void Lighting::Load( const tinyxml2::XMLElement* ele )
{
	hemispheric = false;
	if ( ele->Attribute( "mode", "hemi" ) ) {
		hemispheric = true;
	}

	const tinyxml2::XMLElement* modeEle = ele->FirstChildElement( hemispheric ? "hemispherical" : "lambert" );
	const tinyxml2::XMLElement* child = 0;

	if ( modeEle ) {
		child = modeEle->FirstChildElement( "ambient" );
		if ( child ) {
			LoadColor( child, &ambient );
		}
		child = modeEle->FirstChildElement( "diffuse" );
		if ( child ) {
			LoadColor( child, &diffuse );
		}
		child = modeEle->FirstChildElement( "shadow" );
		if ( child ) {
			LoadColor( child, &shadow );
		}
	}

	child = ele->FirstChildElement( "direction" );
	if ( child ) {
		LoadVector( child, &direction );
		direction.Normalize();
	}
	child = ele->FirstChildElement( "glow" );
	if ( child ) {
		LoadColor( child, &glow );
		child->QueryFloatAttribute( "radius", &glowRadius );
	}
}

