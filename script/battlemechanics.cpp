#include "battlemechanics.h"

#include "../game/gameitem.h"
#include "../game/gamelimits.h"
#include "../game/healthcomponent.h"

#include "../xegame/chitbag.h"
#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/inventorycomponent.h"

#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"

#include "../engine/engine.h"
#include "../engine/camera.h"
#include "../engine/particle.h"

using namespace grinliz;


/*static*/ int BattleMechanics::PrimaryTeam( Chit* src )
{
	int primaryTeam = -1;
	if ( src->GetItemComponent() ) {
		primaryTeam = src->GetItemComponent()->item.primaryTeam;
	}
	return primaryTeam;
}


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

	Vector2F normalToTarget = targetSpatial->GetPosition2D() - spatial->GetPosition2D();
	const float distToTarget = normalToTarget.Length();
	normalToTarget.SafeNormalize( 1, 0 );

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


void BattleMechanics::MeleeAttack( Engine* engine, Chit* src, IMeleeWeaponItem* weapon )
{
	GLASSERT( engine && src && weapon );
	ChitBag* chitBag = src->GetChitBag();
	GLASSERT( chitBag );

	U32 absTime = chitBag->AbsTime();
	if( !weapon->Ready( absTime )) {
		return;
	}
	weapon->Use( absTime );
	int primaryTeam = PrimaryTeam( src );

	// Get origin and direction of melee attack,
	// then send messages to everyone hit. Everything
	// with a spatial component is tracked by the 
	// chitBag, so it's a very handly query.

	Vector2F srcPos = src->GetSpatialComponent()->GetPosition2D();
	Rectangle2F b;
	b.min = srcPos; b.max = srcPos;
	b.Outset( MELEE_RANGE + MAX_BASE_RADIUS );
	chitBag->QuerySpatialHash( &hashQuery, b, src, 0, false );

	for( int i=0; i<hashQuery.Size(); ++i ) {
		Chit* target = hashQuery[i];

		// Melee damage is chaos. Don't hit your own team.
		if ( PrimaryTeam( target ) == primaryTeam ) {
			continue;
		}

		if ( InMeleeZone( engine, src, target )) {
			// FIXME: account for armor, shields, etc. etc.
			// FIXME: account for knockback (physics move), catching fire, etc.
			HealthComponent* targetHealth = GET_COMPONENT( target, HealthComponent );
			if ( targetHealth ) {
				DamageDesc dd;
				CalcMeleeDamage( src, weapon, &dd );
				targetHealth->DeltaHealth( -dd.Total() );
			}
		}
	}
}



void BattleMechanics::CalcMeleeDamage( Chit* src, IMeleeWeaponItem* weapon, DamageDesc* dd )
{
	GameItem* item = weapon->GetItem();

	InventoryComponent* inv = src->GetInventoryComponent();
	GLASSERT( inv && item );
	if ( !inv ) return;

	grinliz::CArray< GameItem*, 4 > chain;
	inv->GetChain( item, &chain );
	GLASSERT( !chain.Empty() );

	MathVector<float,DamageDesc::NUM_COMPONENTS> components = item->meleeDamage.components;
	
	static const float CHAIN_FRACTION = 0.5f;

	// Compute the multiplier. It is the maximum of
	// the item itself, and a percentage of the value 
	// of the chain items. So a creature of fire
	// will do some fire damage, even when using a
	// normal sword.
	for( int i=0; i<DamageDesc::NUM_COMPONENTS; ++i ) {
		for( int j=1; j<chain.Size(); ++j ) {
			components[i] = Max( components[i], CHAIN_FRACTION*chain[j]->meleeDamage.components[i] );
		}
	}

	// That was the multiplier; the actual damage is based on the mass.
	// How many strikes does it take a unit of equal
	// mass to destroy a unit of the same mass?
	static const float STRIKE_RATIO = 5.0f;

	for( int i=0; i<DamageDesc::NUM_COMPONENTS; ++i ) {
		dd->components[i] = components[i] * item->mass / STRIKE_RATIO;
	}
}
