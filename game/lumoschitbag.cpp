#include "lumoschitbag.h"
#include "gameitem.h"
#include "../xegame/rendercomponent.h"
#include "../engine/model.h"
#include "../engine/engine.h"
#include "../engine/loosequadtree.h"

using namespace grinliz;

void LumosChitBag::HandleBolt( const Bolt& bolt, Model* modelHit, const grinliz::Vector3F& at )
{
	GLASSERT( engine );
	Chit* chitHit = modelHit->userData;
	Chit* chitShooter = GetChit( bolt.chitID );	// may be null
 
	if ( !bolt.explosive ) {
		GLASSERT( chitHit );
		GLASSERT( GetChit( chitHit->ID() ) == chitHit );
		DamageDesc dd;
		dd.components = bolt.damage;
		chitHit->SendMessage( ChitMsg( ChitMsg::CHIT_DAMAGE, 0, &dd ), 0 );
	}
	else {
		// Here don't worry abou the chit hit. Just ray cast to see
		// who is in range of the explosion and takes damage.
		
		// Back up the origin of the bolt just a bit, so it doesn't keep
		// intersecting the model it hit. Then do ray checks around to 
		// see what gets hit by the explosion.

		float rewind = Min( 0.1f, 0.5f*bolt.len );
		Vector3F origin = at - bolt.dir * rewind;
		static const float RANGE = 2.0f;

		Rectangle2F rect;
		rect.Set( origin.x, origin.z, origin.x, origin.z );
		rect.Outset( RANGE );
		Model* root = engine->GetSpaceTree()->QueryRect( rect, 0, 0 );

		// Only "top level" Chits have a Spatial, can filter on that.
		// Also, both queries use the same Model linked list, so need
		// to save the result to an array.
		chitList.Clear();
		for( ; root; root=root->next ) {
			Chit* chit = root->userData;
			GLASSERT( chit );
			if ( chit && chit->GetSpatialComponent() ) {
				chitList.Push( chit );
			}
		}

		GLLOG(( "<Explosion> (%.1f,%.1f,%.1f)\n", at.x, at.y, at.z ));
		for( int i=0; i<chitList.Size(); ++i ) {
			Vector3F target = { 0, 0, 0 };
			Chit* chit = chitList[i];
			RenderComponent* rc = chit->GetRenderComponent();
			if ( rc && (chit != chitShooter)) {
				if ( rc->HasMetaData( "target" )) {
					rc->GetMetaData( "target", &target );
				}
				else {
					rc->CalcTarget( &target );
				}
				Vector3F hit;
				Model* m = engine->IntersectModel( origin, target-origin, RANGE, TEST_TRI, 0, 0, 0, &hit );
				if ( m ) {
					// Did we hit the current chit? Use the ignoreList 'in reverse': if
					// we hit any component of the Chit, we hit the chit.
					CArray<const Model*, EL_MAX_METADATA+2> targetList;
					rc->GetIgnoreList( &targetList );
					if ( targetList.Find( m ) ) {
						// HIT!
						float len = (hit-origin).Length();
						// Scale the damage based on the range.
						if ( len < RANGE ) {
							DamageDesc dd;
							dd.components = bolt.damage;
							dd.components.Mult( (RANGE-len)/RANGE );
							chit->SendMessage( ChitMsg( ChitMsg::CHIT_DAMAGE, 0, &dd ), 0 );
						}
					}
				}
			}
		}
		GLLOG(( "</Explosion>\n" ));
	}
}
