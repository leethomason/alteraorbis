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
	if ( track ) {
		int oldX = (int)position.x;
		int oldY = (int)position.z;
		int newX = (int)x;
		int newY = (int)z;
		parentChit->GetChitBag()->UpdateSpatialHash( parentChit, oldX, oldY, newX, newY );
	}

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


void SpatialComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
	if ( track ) {
		GLASSERT( chit == parentChit );
		parentChit->GetChitBag()->AddToSpatialHash( chit, (int)position.x, (int)position.z );
	}
}


void SpatialComponent::OnRemove()
{
	if ( track ) {
		parentChit->GetChitBag()->RemoveFromSpatialHash( parentChit, (int)position.x, (int)position.z );
	}
	Component::OnRemove();
}
