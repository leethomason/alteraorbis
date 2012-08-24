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
	// chitBag, so it's a very handy query.

	Vector2F srcPos = src->GetSpatialComponent()->GetPosition2D();
	Rectangle2F b;
	b.min = srcPos; b.max = srcPos;
	b.Outset( MELEE_RANGE + MAX_BASE_RADIUS );
	chitBag->QuerySpatialHash( &hashQuery, b, src, 0 );

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

				GLLOG(( "Chit %3d '%s' using '%s' hit %3d '%s' dd=[%5.1f %5.1f %5.1f]\n", 
						src->ID(), src->GetItemComponent()->GetItem()->Name(),
						weapon->GetItem()->Name(),
						target->ID(), target->GetItemComponent()->GetItem()->Name(),
						dd.components[0], dd.components[1], dd.components[2] ));

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
	GLASSERT( chain.Size() == 2 || chain.Size() == 3 );

	DamageDesc::Vector vec = item->meleeDamage.components;
	DamageDesc::Vector handVec;
	if ( chain.Size() == 3 ) {
		handVec = chain[1]->meleeDamage.components;
	}
	// The parent item doesn't do damage. But
	// give it credit for FLAME, etc.
	GameItem* parentItem = chain[chain.Size()-1];
	DamageDesc::Vector parentVec = chain[chain.Size()-1]->meleeDamage.components;
	if ( parentItem->flags & GameItem::EFFECT_FIRE ) {
		parentVec[DamageDesc::FIRE] = 1;
	}
	
	static const float CHAIN_FRACTION = 0.5f;

	// Compute the multiplier. It is the maximum of
	// the item itself, and a percentage of the value 
	// of the chain items. So a creature of fire
	// will do some fire damage, even when using a
	// normal sword.
	for( int i=0; i<DamageDesc::NUM_COMPONENTS; ++i ) {
		vec[i] = Max( vec[i], CHAIN_FRACTION*handVec[i], CHAIN_FRACTION*parentVec[i] );
	}

	// That was the multiplier; the actual damage is based on the mass.
	// How many strikes does it take a unit of equal
	// mass to destroy a unit of the same mass?
	static const float STRIKE_RATIO = 5.0f;

	for( int i=0; i<DamageDesc::NUM_COMPONENTS; ++i ) {
		dd->components[i] = vec[i] * parentItem->mass / STRIKE_RATIO;
	}
}
