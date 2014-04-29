#include "towcomponent.h"
#include "../game/lumoschitbag.h"
#include "../xegame/chit.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/spatialcomponent.h"

using namespace grinliz;

static const float TOW_SPEED		= DEFAULT_MOVE_SPEED * 3.0f;
static const float TOW_ROTATION		= ROTATION_SPEED * 2.0f;
static const float TOW_DISTANCE		= 0.5f;

void TowComponent::OnAdd( Chit* chit, bool init )
{
	super::OnAdd( chit, init );
}


void TowComponent::OnRemove()
{
	super::OnRemove();
}


void TowComponent::Serialize( XStream* xs )
{
	BeginSerialize( xs, Name() );
	XARC_SER( xs, towingID );
	EndSerialize( xs );
}


void TowComponent::DebugStr( grinliz::GLString* str )
{
	str->AppendFormat( "[Tow] id=%d ", towingID );
}


int TowComponent::DoTick( U32 delta )
{
	if ( towingID == 0 ) {
		return VERY_LONG_TICK;
	}

	ComponentSet thisComp( parentChit, Chit::SPATIAL_BIT | Chit::RENDER_BIT | ComponentSet::IS_ALIVE );
	if ( !thisComp.okay ) {
		return VERY_LONG_TICK;
	}

	const ChitContext* context = this->Context();
	LumosChitBag* chitBag = context->chitBag;
	Chit* tow = chitBag->GetChit( towingID );
	if ( !tow ) {
		towingID = 0;
		return VERY_LONG_TICK;
	}

	ComponentSet towComp( tow, Chit::SPATIAL_BIT | ComponentSet::IS_ALIVE );
	if ( !towComp.okay ) {
		return 0;	// try again next frame?
	}

	if ( delta > MAX_FRAME_TIME ) delta = MAX_FRAME_TIME;

	Vector3F myTarget = thisComp.spatial->GetPosition();
	thisComp.render->CalcTarget( &myTarget );
	Vector3F myHeading = thisComp.spatial->GetHeading();
	Quaternion myRot = thisComp.spatial->GetRotation();

	Vector3F towTarget = myTarget - myHeading * TOW_DISTANCE;
	Vector3F towPos = towComp.spatial->GetPosition();
	Vector3F towDir = towTarget - towPos;
	towDir.Normalize();

	Vector3F v = towTarget;
	float travel = Travel( TOW_SPEED, delta );
	if ( (towPos - towTarget).Length() <= travel ) {
		// All good; will reach target.
		//towComp.spatial->SetPosition( towTarget );
	}
	else {
		v = towPos + towDir * travel;
	}

	// Not sure how to reasonably do constant rotation...see how this does:
	Quaternion towRot = towComp.spatial->GetRotation();
	Quaternion rot;
	Quaternion::SLERP( towRot, myRot, 0.2f, &rot );
	towComp.spatial->SetPosRot( v, rot );

	return 0;
}

