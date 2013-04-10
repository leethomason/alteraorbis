#include "physicsmovecomponent.h"
#include "worldmap.h"
#include "gamelimits.h"
#include "pathmovecomponent.h"

#include "../engine/serialize.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/chit.h"
#include "../xegame/chitbag.h"

using namespace grinliz;

PhysicsMoveComponent::PhysicsMoveComponent( WorldMap* _map ) : GameMoveComponent( _map ), deleteWhenDone( false )
{
	velocity.Zero();
}


void PhysicsMoveComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[PhysicsMove] " );
}


void PhysicsMoveComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	XARC_SER( xs, rotation );
	XARC_SER( xs, deleteWhenDone );
	XARC_SER( xs, velocity );
	this->EndSerialize( xs );
}


void PhysicsMoveComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
}


void PhysicsMoveComponent::OnRemove()
{
	super::OnRemove();
}


bool PhysicsMoveComponent::IsMoving() const
{
	return velocity.LengthSquared() > 0 || rotation != 0;
}


int PhysicsMoveComponent::DoTick( U32 delta, U32 since )
{
	ComponentSet thisComp( parentChit, Chit::SPATIAL_BIT );
	if ( thisComp.okay ) {
		Vector3F pos = thisComp.spatial->GetPosition();
		float rot = thisComp.spatial->GetYRotation();

		float dtime = (float)delta * 0.001f;
		Vector3F travel = velocity * dtime;
		float tRot = rotation * dtime;
		pos += travel;
		rot += tRot;

		// Did we hit the ground?
		if ( pos.y < 0 ) {
			pos.y = 0;
			if ( travel.y < 0 ) {
				travel.y = -travel.y;
			}
			static const float scale = 0.5f;
			velocity = velocity * scale;
			rotation = rotation * scale;
		}

		static const float VDONE = 0.1f;
		static const float RDONE = 10.0f;
		static const float GRAVITY = 10.0f*METERS_PER_GRID;
		static const float EPS = 0.01f;

		if (    pos.y < EPS 
			 && velocity.XZ().LengthSquared() < (VDONE*VDONE)
			 && rotation < RDONE ) 
		{
			velocity.Zero();
			pos.y = 0;
			rotation = 0;
		}
		else {
			float grav = GRAVITY * dtime;
			velocity.y -= grav;
		}

		// FIXME: rebound off blocks?
		Vector2F pos2 = { pos.x, pos.z };
		ApplyBlocks( &pos2, 0, 0 );
		pos.XZ( pos2 );
		thisComp.spatial->SetPosYRot( pos, rot );
	}
	bool isMoving = IsMoving();
	if ( !isMoving && deleteWhenDone ) {
		parentChit->Remove( this );
		parentChit->Add( new PathMoveComponent( map ));
		delete this;
	}
	return isMoving ? 0 : VERY_LONG_TICK;
}


