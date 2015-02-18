#include "physicsmovecomponent.h"
#include "worldmap.h"
#include "gamelimits.h"
#include "pathmovecomponent.h"
#include "lumosmath.h"

#include "../engine/serialize.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/chit.h"
#include "../game/lumoschitbag.h"

using namespace grinliz;

static const float TRACK_SPEED	= DEFAULT_MOVE_SPEED * 4.0f;

PhysicsMoveComponent::PhysicsMoveComponent()
{
	rotation = 0;
	velocity.Zero();
}


void PhysicsMoveComponent::DebugStr( grinliz::GLString* str )
{
	str->AppendFormat( "[PhysicsMove] " );
}


void PhysicsMoveComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	XARC_SER( xs, rotation );
	XARC_SER( xs, velocity );
	this->EndSerialize( xs );
}


void PhysicsMoveComponent::OnAdd( Chit* chit, bool init )
{
	super::OnAdd( chit, init );
}


void PhysicsMoveComponent::OnRemove()
{
	super::OnRemove();
}


void PhysicsMoveComponent::Set( const grinliz::Vector3F& _velocity, float _rotation )		{ 
	velocity = _velocity; 
	rotation = _rotation; 
	parentChit->SetTickNeeded();
}


void PhysicsMoveComponent::Add( const grinliz::Vector3F& _velocity, float _rotation )		{ 
	velocity += _velocity; 
	rotation += _rotation; 
	parentChit->SetTickNeeded();
}


bool PhysicsMoveComponent::IsMoving() const
{
	return velocity.LengthSquared() > 0 || rotation != 0;
}


int PhysicsMoveComponent::DoTick(U32 delta)
{
	if (delta > MAX_FRAME_TIME) delta = MAX_FRAME_TIME;

	Vector3F pos = parentChit->Position();
	float rot = YRotation(parentChit->Rotation());

	float dtime = (float)delta * 0.001f;
	Vector3F travel = velocity * dtime;
	float tRot = rotation * dtime;
	pos += travel;
	rot += tRot;

	// Did we hit the ground?
	if (pos.y < 0) {
		pos.y = 0;
		if (travel.y < 0) {
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

	if (pos.y < EPS
		&& velocity.XZ().LengthSquared() < (VDONE*VDONE)
		&& rotation < RDONE)
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
	ApplyBlocks(&pos2, 0);
	pos.XZ(pos2);
	Quaternion q = Quaternion::MakeYRotation(rot);
	parentChit->SetPosRot(pos, q);

	bool isMoving = IsMoving();

	if (!isMoving && parentChit->StackedMoveComponent()) {
		Context()->chitBag->QueueRemoveAndDeleteComponent(this);
	}
	return isMoving ? 0 : VERY_LONG_TICK;
}


TrackingMoveComponent::TrackingMoveComponent() : GameMoveComponent(), target( 0 )
{
}

TrackingMoveComponent::~TrackingMoveComponent()		
{
	int debug=1;
}


void TrackingMoveComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	XARC_SER( xs, target );
	this->EndSerialize( xs );
}


Vector3F TrackingMoveComponent::Velocity()
{
	Vector3F v = { 0, 0, 0 };

	Chit* chit = Context()->chitBag->GetChit( target );
	if ( !chit )
		return v;

	Vector3F targetPos = chit->Position();
	Vector3F pos = parentChit->Position();

	Vector3F delta = targetPos - pos;
	delta.Normalize();
	v = delta * TRACK_SPEED;
	return v;
}


bool TrackingMoveComponent::IsMoving()
{
	Chit* chit = Context()->chitBag->GetChit(target);
	if (!chit )
		return false;

	Vector3F targetPos = chit->Position();
	Vector3F pos = parentChit->Position();
	return targetPos != pos;
}


int TrackingMoveComponent::DoTick( U32 deltaTime )
{
	Chit* chit = Context()->chitBag->GetChit(target);
	if (!chit ) {
		GLASSERT(parentChit->StackedMoveComponent()); // not required, but should have a GameMoveComponent??
		Context()->chitBag->QueueRemoveAndDeleteComponent( this );
		return VERY_LONG_TICK;
	}

	Vector3F targetPos = chit->Position();
	Vector3F pos = parentChit->Position();

	Vector3F delta = targetPos - pos;
	float len = delta.Length();
	// This component gets added to non-moving components, so the first
	// tick time can be really big.
	float travel = Travel( TRACK_SPEED, Min( deltaTime, MAX_FRAME_TIME ));

	if ( travel >= len ) {
		parentChit->SetPosition( targetPos );
		ChitMsg msg( ChitMsg::CHIT_TRACKING_ARRIVED, 0, parentChit );
		chit->SendMessage( msg );

		GLASSERT(parentChit->StackedMoveComponent());
		Context()->chitBag->QueueRemoveAndDeleteComponent( this );
	}
	else {
		delta.Normalize();
		parentChit->SetPosition( pos + delta*travel );
	}
	return 0;
}

