#include "spatialcomponent.h"
#include "chit.h"
#include "chitbag.h"

using namespace grinliz;


void SpatialComponent::DebugStr( GLString* str )
{
	str->Format( "[Spatial]=%.1f,%.1f,%.1f ", position.x, position.y, position.z );
}


void SpatialComponent::SetPosition( float x, float y, float z )
{
	position.Set( x, y, z ); 
	RequestUpdate();	// Renders triggers off update. May want to reconsider that.
}


Vector3F SpatialComponent::GetHeading() const
{
	Vector3F norm = { sinf(ToRadian(yRotation)), 0.0f, cosf(ToRadian(yRotation)) };
	return norm;	
}

Vector2F SpatialComponent::GetHeading2D() const
{
	Vector2F norm = { sinf(ToRadian(yRotation)), cosf(ToRadian(yRotation)) };
	return norm;	
}

