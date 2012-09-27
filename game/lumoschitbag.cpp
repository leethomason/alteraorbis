#include "lumoschitbag.h"
#include "gameitem.h"
#include "../xegame/rendercomponent.h"
#include "../engine/model.h"
#include "../engine/engine.h"
#include "../engine/loosequadtree.h"
#include "../script/worldscript.h"

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
		msg.vector = bolt.dir;
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
		static const float RANGE = 2.0f;

		Rectangle2F rect;
		rect.Set( origin.x, origin.z, origin.x, origin.z );
		rect.Outset( RANGE );
		WorldScript::QueryChits( rect, engine, &chitList );

		GLLOG(( "<Explosion> (%.1f,%.1f,%.1f)\n", at.x, at.y, at.z ));
		for( int i=0; i<chitList.Size(); ++i ) {
			Vector3F target = { 0, 0, 0 };
			Chit* chit = chitList[i];
			RenderComponent* rc = chit->GetRenderComponent();
			if ( rc && (chit != chitShooter)) {
				rc->CalcTarget( &target );

				Vector3F hit;
				Model* m = engine->IntersectModel( origin, target-origin, RANGE, TEST_TRI, 0, 0, 0, &hit );
#ifdef DEBUG_EXPLOSION
				if ( m ) 
					DebugLine( origin, hit, 1, 0, 0 );
				else
					DebugLine( origin, target );
#endif
				if ( m ) {
					// Did we hit the current chit? Use the ignoreList 'in reverse': if
					// we hit any component of the Chit, we hit the chit.
					CArray<const Model*, EL_MAX_METADATA+2> targetList;
					rc->GetModelList( &targetList );
					if ( targetList.Find( m ) >= 0 ) {
						// HIT!
						float len = (hit-origin).Length();
						// Scale the damage based on the range.
						if ( len < RANGE ) {
							DamageDesc dd;
							dd.components = bolt.damage;
							dd.components.Mult( (RANGE-len)/RANGE );

							ChitMsg msg( ChitMsg::CHIT_DAMAGE, 1, &dd );
							msg.vector = target - origin;
							msg.vector.Normalize();
							chit->SendMessage( msg, 0 );
						}
					}
				}
			}
		}
		GLLOG(( "</Explosion>\n" ));
	}
}
