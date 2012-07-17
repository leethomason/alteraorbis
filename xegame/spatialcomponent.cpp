#include "spatialcomponent.h"
#include "chit.h"
#include "chitbag.h"
#include "xegamelimits.h"
#include "rendercomponent.h"
#include "../grinliz/glmatrix.h"

using namespace grinliz;


void SpatialComponent::DebugStr( GLString* str )
{
	str->Format( "[Spatial]=%.1f,%.1f,%.1f ", position.x, position.y, position.z );
}


void SpatialComponent::SetPosRot( const Vector3F& v, const Quaternion& _q )
{
	GLASSERT( v.x >= -1 && v.x < EL_MAX_MAP_SIZE );
	GLASSERT( v.z >= -1 && v.z < EL_MAX_MAP_SIZE );
	GLASSERT( v.y >= -1 && v.y <= 10 );	// obviously just general sanity

	Quaternion q = _q;
	q.Normalize();

	bool posChange = (position != v);
	bool rotChange = (rotation != q);

	if ( posChange && track ) {
		int oldX = (int)position.x;
		int oldY = (int)position.z;
		int newX = (int)v.x;
		int newY = (int)v.z;
		parentChit->GetChitBag()->UpdateSpatialHash( parentChit, oldX, oldY, newX, newY );
	}
	
	if ( posChange || rotChange ) {
		position = v;
		rotation = q;

		//RequestUpdate();	// Render triggers off update.
		parentChit->SendMessage( SPATIAL_MSG_CHANGED, this, 0 );
	}
}


void SpatialComponent::SetPosYRot( float x, float y, float z, float yRot )
{
	Vector3F v = { x, y, z };
	Quaternion q;

	static const Vector3F UP = { 0, 1, 0 };
	q.FromAxisAngle( UP, yRot );
	SetPosRot( v, q );
}


Vector3F SpatialComponent::GetHeading() const
{
	Matrix4 r;
	rotation.ToMatrix( &r );
	return r.Row( Matrix4::OUT );
}


float SpatialComponent::GetYRotation() const
{
	Vector3F axis;
	float angle;
	rotation.ToAxisAngle( &axis, &angle );
#ifdef DEBUG
	static const Vector3F UP = {0,1,0};
	GLASSERT( angle == 0 ||  DotProduct( UP, axis ) > 0.99f );	// else probably not what was intended.
#endif
	return angle;
}


Vector2F SpatialComponent::GetHeading2D() const
{
	Vector3F h = GetHeading();
	Vector2F norm = { h.x, h.z };
	// Not normalized; I think this is the intention.
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


#if 0 
void RelativeSpatialComponent::DebugStr( GLString* str )
{
	str->Format( "[RelativeSpatial]=%.1f,%.1f,%.1f ", position.x, position.y, position.z );
}


//void RelativeSpatialComponent::OnChitMsg( Chit* chit, int id, const ChitEvent* event )
//{
//	if ( id == SPATIAL_MSG_CHANGED) {


		RenderComponent* otherRender = chit->GetRenderComponent();

		Matrix4 xform;
		if ( otherRender && !metaData.empty() ) {
			otherRender->GetMetaData( metaData.c_str(), &xform );
		}
		else {
			GLASSERT( 0 );	// need to get the other path working
		}

		Vector3F v;
		Quaternion q;
		Quaternion::Decompose( xform, &v, &q );
		this->SetPosRot( v, q );
#endif
