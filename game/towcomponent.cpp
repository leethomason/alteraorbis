#include "towcomponent.h"
#include "../game/lumoschitbag.h"
#include "../xegame/chit.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/chitcontext.h"

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

	const ChitContext* context = this->Context();
	LumosChitBag* chitBag = context->chitBag;
	Chit* tow = chitBag->GetChit( towingID );
	if ( !tow ) {
		towingID = 0;
		return VERY_LONG_TICK;
	}

	if ( delta > MAX_FRAME_TIME ) delta = MAX_FRAME_TIME;

	RenderComponent* thisRC = parentChit->GetRenderComponent();
	if (!thisRC) return VERY_LONG_TICK;

	Vector3F myTarget = parentChit->Position();
	thisRC->CalcTarget( &myTarget );
	Vector3F myHeading = parentChit->Heading();
	Quaternion myRot = parentChit->Rotation();

	Vector3F towTarget = myTarget - myHeading * TOW_DISTANCE;
	Vector3F towPos = tow->Position();
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
	Quaternion towRot = tow->Rotation();
	Quaternion rot;
	Quaternion::SLERP( towRot, myRot, 0.2f, &rot );
	tow->SetPosRot( v, rot );

	return 0;
}

