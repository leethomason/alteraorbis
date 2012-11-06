#include "lumoschitbag.h"
#include "gameitem.h"
#include "gamelimits.h"

#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"

#include "../engine/model.h"
#include "../engine/engine.h"
#include "../engine/loosequadtree.h"

#include "../script/battlemechanics.h"


//#define DEBUG_EXPLOSION

using namespace grinliz;

void LumosChitBag::HandleBolt( const Bolt& bolt, Model* modelHit, const grinliz::Vector3F& at )
{
	GLASSERT( engine );
	Chit* chitShooter = GetChit( bolt.chitID );	// may be null
	int shooterTeam = -1;
	if ( chitShooter && chitShooter->GetItemComponent() ) {
		shooterTeam = chitShooter->GetItemComponent()->GetItem()->primaryTeam;
	}
	int explosive = bolt.effect & GameItem::EFFECT_EXPLOSIVE;
 
	if ( !explosive ) {
		if ( modelHit ) {
			Chit* chitHit = modelHit->userData;
			GLASSERT( chitHit );
			GLASSERT( GetChit( chitHit->ID() ) == chitHit );
			if ( chitHit->GetItemComponent() &&
				 chitHit->GetItemComponent()->GetItem()->primaryTeam == shooterTeam ) 
			{
				// do nothing. don't shoot own team.
			}
			else {
				DamageDesc dd( bolt.damage, bolt.effect );
		
				ChitMsg msg( ChitMsg::CHIT_DAMAGE, 0, &dd );
				// 'vector' copied from BattleMechanics::GenerateExplosionMsgs
				// This code is strange: the vector doesn't contain the origin,
				// but the impulse (velocity). Weird.
				msg.vector = bolt.dir;
				msg.vector.Normalize();
				msg.vector.Multiply( 2.0f );
				msg.originID = bolt.chitID;
				chitHit->SendMessage( msg, 0 );
			}
		}
	}
	else {
		// Here don't worry about the chit hit. Just ray cast to see
		// who is in range of the explosion and takes damage.
		
		// Back up the origin of the bolt just a bit, so it doesn't keep
		// intersecting the model it hit. Then do ray checks around to 
		// see what gets hit by the explosion.

		//GLLOG(( "Explosion: " ));

		float rewind = Min( 0.1f, 0.5f*bolt.len );
		GLASSERT( Equal( bolt.dir.Length(), 1.f, 0.001f ));
		Vector3F origin = at - bolt.dir * rewind;

		DamageDesc dd( bolt.damage, bolt.effect );
		BattleMechanics::GenerateExplosionMsgs( dd, origin, bolt.chitID, engine, &chitList );
	}
}
