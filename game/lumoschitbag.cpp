#include "lumoschitbag.h"
#include "gameitem.h"
#include "gamelimits.h"

#include "../xegame/rendercomponent.h"

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
 
	if ( !bolt.explosive ) {
		Chit* chitHit = modelHit->userData;
		GLASSERT( chitHit );
		GLASSERT( GetChit( chitHit->ID() ) == chitHit );
		DamageDesc dd;
		dd.components = bolt.damage;
		
		ChitMsg msg( ChitMsg::CHIT_DAMAGE, 0, &dd );
		msg.vector = at;
		chitHit->SendMessage( msg, 0 );
	}
	else {
		// Here don't worry abou the chit hit. Just ray cast to see
		// who is in range of the explosion and takes damage.
		
		// Back up the origin of the bolt just a bit, so it doesn't keep
		// intersecting the model it hit. Then do ray checks around to 
		// see what gets hit by the explosion.

		float rewind = Min( 0.1f, 0.5f*bolt.len );
		GLASSERT( Equal( bolt.dir.Length(), 1.f, 0.001f ));
		Vector3F origin = at - bolt.dir * rewind;

		DamageDesc dd;
		dd.components = bolt.damage;
		BattleMechanics::GenerateExplosionMsgs( dd, origin, engine, &chitList );
	}
}
