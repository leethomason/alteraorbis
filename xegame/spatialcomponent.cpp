#include "spatialcomponent.h"
#include "chit.h"
#include "chitbag.h"
#include "xegamelimits.h"
#include "../grinliz/glmatrix.h"

using namespace grinliz;


void SpatialComponent::DebugStr( GLString* str )
{
	str->Format( "[Spatial]=%.1f,%.1f,%.1f ", position.x, position.y, position.z );
}


void SpatialComponent::SetPosYRot( float x, float y, float z, float _yRot )
{
	float yRot = NormalizeAngleDegrees( _yRot );
	bool posChange = (x!=position.x) || (y!=position.y) || (z!=position.z);

	if ( posChange && track ) {
		int oldX = (int)position.x;
		int oldY = (int)position.z;
		int newX = (int)x;
		int newY = (int)z;
		parentChit->GetChitBag()->UpdateSpatialHash( parentChit, oldX, oldY, newX, newY );
	}

	if ( posChange || (yRot != yRotation) ) {
		position.Set( x, y, z ); 
		yRotation = yRot;
		RequestUpdate();	// Render triggers off update.
		parentChit->SendMessage( SPATIAL_MSG_CHANGED, this, 0 );
	}
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


void RelativeSpatialComponent::DebugStr( GLString* str )
{
	str->Format( "[RelativeSpatial]=%.1f,%.1f,%.1f ", position.x, position.y, position.z );
}


void RelativeSpatialComponent::OnChitMsg( Chit* chit, int id )
{
	if ( id == SPATIAL_MSG_CHANGED) {
		SpatialComponent* other = chit->GetSpatialComponent();
		GLASSERT( other );

		Matrix4 m, r, t;
		r.SetYRotation(   other->GetYRotation() );

		Vector3F newPos = r * relativePosition;

		this->SetPosYRot( other->GetPosition() + newPos,
						  other->GetYRotation() + relativeYRotation );
	}
}


void RelativeSpatialComponent::SetRelativePosYRot( float x, float y, float z, float rot )
{
	relativePosition.Set( x, y, z );
	relativeYRotation = rot;
}
