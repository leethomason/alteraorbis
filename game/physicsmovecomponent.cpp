#include "physicsmovecomponent.h"
#include "worldmap.h"
#include "gamelimits.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/chit.h"

using namespace grinliz;

PhysicsMoveComponent::PhysicsMoveComponent( WorldMap* _map ) : GameMoveComponent( _map )
{
	velocity.Zero();
}


void PhysicsMoveComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[PhysicsMove] " );
}


void PhysicsMoveComponent::OnAdd( Chit* chit )
{
	GameMoveComponent::OnAdd( chit );
}


void PhysicsMoveComponent::OnRemove()
{
	GameMoveComponent::OnRemove();
}


bool PhysicsMoveComponent::IsMoving() const
{
	return velocity.LengthSquared() > 0;
}


bool PhysicsMoveComponent::DoTick( U32 delta )
{
	ComponentSet thisComp( parentChit, Chit::SPATIAL_BIT );
	if ( thisComp.okay ) {
		Vector3F pos = thisComp.spatial->GetPosition();
		float dtime = (float)delta * 0.001f;
		Vector3F travel = velocity * dtime;
		pos += travel;

		// Did we hit the ground?
		if ( pos.y < 0 ) {
			pos.y = 0;
			if ( travel.y < 0 ) {
				travel.y = -travel.y;
			}
			static const float scale = 0.5f;
			velocity = velocity * scale;
		}

		static const float VDONE = 0.1f;
		static const float GRAVITY = 10.0f*METERS_PER_GRID;

		if ( velocity.LengthSquared() < (VDONE*VDONE)) {
			velocity.Zero();
			pos.y = 0;
		}
		else {
			float grav = GRAVITY * dtime;
			velocity.y -= grav;
		}

		// FIXME: rebound off blocks
		Vector2F pos2 = { pos.x, pos.z };
		ApplyBlocks( &pos2, 0, 0 );
		pos.XZ( pos2 );
		thisComp.spatial->SetPosition( pos );
	}
	return IsMoving();
}


