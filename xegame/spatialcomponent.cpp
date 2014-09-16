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

#include "spatialcomponent.h"
#include "chit.h"
#include "chitbag.h"
#include "xegamelimits.h"
#include "rendercomponent.h"
#include "istringconst.h"

#include "../engine/engine.h"
#include "../engine/particle.h"
#include "../grinliz/glmatrix.h"
#include "../game/lumoschitbag.h"
#include "../game/pathmovecomponent.h"	// see the Teleport() function.

using namespace grinliz;


void SpatialComponent::DebugStr( GLString* str )
{
	str->AppendFormat( "[Spatial]=%.1f,%.1f,%.1f ", position.x, position.y, position.z );
}


void SpatialComponent::SetPosRot( const Vector3F& v, const Quaternion& _q )
{
	GLASSERT( v.x >= 0 && v.x < EL_MAP_SIZE );
	GLASSERT( v.z >= 0 && v.z < EL_MAP_SIZE );
	GLASSERT( v.y >= -1 && v.y <= 10 );	// obviously just general sanity

	Quaternion q = _q;
	q.Normalize();

	bool posChange = (position != v);
	bool rotChange = (rotation != q);

	if ( posChange ) {
		int oldX = (int)position.x;
		int oldY = (int)position.z;
		int newX = (int)v.x;
		int newY = (int)v.z;
		if (parentChit) {
			// Sometimes called before addad to Chit
			Context()->chitBag->UpdateSpatialHash(parentChit, oldX, oldY, newX, newY);
		}
	}
	
	if ( posChange || rotChange ) {
		position = v;
		rotation = q;

		if (parentChit) {
			parentChit->SetTickNeeded();
		}
	}
}


void SpatialComponent::SetPosYRot( float x, float y, float z, float yRot )
{
	Vector3F v = { x, y, z };
	Quaternion q;

	static const Vector3F UP = { 0, 1, 0 };
	q.FromAxisAngle( UP, NormalizeAngleDegrees( yRot ) );
	SetPosRot( v, q );
}


Vector3F SpatialComponent::GetHeading() const
{
	Matrix4 r;
	rotation.ToMatrix( &r );
	Vector3F v = r * V3F_OUT;
	return v;
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
	return NormalizeAngleDegrees( angle );
}


Vector2F SpatialComponent::GetHeading2D() const
{
	Vector3F h = GetHeading();
	Vector2F norm = { h.x, h.z };
	return norm;	
}


void SpatialComponent::Serialize( XStream* xs )
{
	BeginSerialize( xs, Name() );
	XARC_SER( xs, position );

	static const Quaternion DEFQUAT; 
	XARC_SER_DEF( xs, rotation, DEFQUAT );
	EndSerialize( xs );
}


void SpatialComponent::OnAdd( Chit* chit, bool init )
{
	Component::OnAdd( chit, init );
	GLASSERT( chit == parentChit );
	Context()->chitBag->AddToSpatialHash( chit, (int)position.x, (int)position.z );
}


void SpatialComponent::OnRemove()
{
	Context()->chitBag->RemoveFromSpatialHash( parentChit, (int)position.x, (int)position.z );
	Component::OnRemove();
}


void SpatialComponent::Teleport(const grinliz::Vector3F& pos)
{
	GLASSERT(!GET_SUB_COMPONENT(ParentChit(), SpatialComponent, MapSpatialComponent));
	if (ToWorld2I(pos) != ToWorld2I(position)) {

		Context()->engine->particleSystem->EmitPD(ISC::teleport, position, V3F_UP, 0);
		SetPosition(pos);
		Context()->engine->particleSystem->EmitPD(ISC::teleport, position, V3F_UP, 0);

		// Sigh. Reaching into another component. Didn't think of this when I 
		// added the teleport method.
		while (parentChit->StackedMoveComponent()) {
			Component* c = parentChit->Remove(parentChit->GetMoveComponent());
			delete c;
		}
		GLASSERT(parentChit->GetMoveComponent());
		PathMoveComponent* pmc = GET_SUB_COMPONENT(parentChit, MoveComponent, PathMoveComponent);
		if (pmc) pmc->Stop();
	}
}


void SpatialComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( msg.ID() == ChitMsg::CHIT_DESTROYED_START ) {
		// NOT deleted by destroy sequence.
	}
	else {
		super::OnChitMsg( chit, msg );
	}
}
