#include "battlemechanics.h"

#include "../game/gameitem.h"
#include "../game/gamelimits.h"
#include "../game/healthcomponent.h"

#include "../xegame/chitbag.h"
#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"

#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"

#include "../engine/engine.h"
#include "../engine/camera.h"
#include "../engine/particle.h"

using namespace grinliz;


bool BattleMechanics::InMeleeZone(	Engine* engine,
									Chit* src,
									Chit* target )
{
	SpatialComponent* spatial		= src->GetSpatialComponent();
	RenderComponent*  render		= src->GetRenderComponent();
	SpatialComponent* targetSpatial = target->GetSpatialComponent();
	RenderComponent*  targetRender	= target->GetRenderComponent();

	if ( spatial && render && targetSpatial && targetRender ) {}	// all good
	else return false;

	Vector2F normalToTarget = spatial->GetPosition2D() - targetSpatial->GetPosition2D();
	const float distToTarget = normalToTarget.Length();
	normalToTarget.Normalize();

	int test = IntersectRayCircle( targetSpatial->GetPosition2D(),
								   targetRender->RadiusOfBase(),
								   spatial->GetPosition2D(),
								   normalToTarget );

	bool intersect = ( test == INTERSECT || test == INSIDE );

	const float meleeRangeDiff = 0.5f * render->RadiusOfBase();	// FIXME: correct??? better metric?
	const float meleeRange = render->RadiusOfBase() + targetRender->RadiusOfBase() + meleeRangeDiff;

	if ( intersect && distToTarget < meleeRange ) {
		return true;
	}
	return false;
}


void BattleMechanics::MeleeAttack( Engine* engine, Chit* src, WeaponItem* weapon )
{
	GLASSERT( engine && src && weapon );
	ChitBag* chitBag = src->GetChitBag();
	GLASSERT( chitBag );

	U32 absTime = chitBag->AbsTime();
	if( !weapon->CanMelee( absTime )) {
		return;
	}
	weapon->DoMelee( absTime );

	// Get origin and direction of melee attack,
	// then send messages to everyone hit.
	// FIXME:
	// Use the chitBag query, which avoids smacking
	// weapons and such (good) but may not account
	// for objects in air and such (bad...)
	// Does establish that everything in the chitbag
	// query is something that can be hit by melee/etc.
	// FIXME: may never be hitting world objects (blocks and such)
	// that aren't in the ChitBag

	Vector2F srcPos = src->GetSpatialComponent()->GetPosition2D();
	Rectangle2F b;
	b.min = srcPos; b.max = srcPos;
	b.Outset( MELEE_RANGE + MAX_BASE_RADIUS );
	const CDynArray<Chit*>& nearChits = chitBag->QuerySpatialHash( b, src, false );

	for( int i=0; i<nearChits.Size(); ++i ) {
		Chit* target = nearChits[i];

		if ( InMeleeZone( engine, src, target )) {
			// FIXME: account for armor, shields, etc. etc.
			// FIXME: account for knockback (physics move), catching fire, etc.
			// FIXME: account for critical damage
			HealthComponent* targetHealth = GET_COMPONENT( target, HealthComponent );
			if ( targetHealth ) {
				targetHealth->DeltaHealth( -weapon->BaseDamage() );
			}
		}
	}
}
