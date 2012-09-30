/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "itemcomponent.h"
#include "chit.h"

#include "../game/healthcomponent.h"
#include "../game/physicsmovecomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/spatialcomponent.h"

using namespace grinliz;

void ItemComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( msg.ID() == ChitMsg::CHIT_DAMAGE ) {
		const DamageDesc* dd = (const DamageDesc*) msg.Ptr();
		GLASSERT( dd );
		GLLOG(( "Chit %3d '%s' ", parentChit->ID(), item.Name() ));

		// FIXME: see if inventory can absorb first (shields)

		float hp = item.hp;
		float delta = item.AbsorbDamage( *dd, 0, "DAMAGE" );
		GLLOG(( "\n" ));

		if ( item.hp != hp ) {
			HealthComponent* hc = GET_COMPONENT( parentChit, HealthComponent );
			if ( hc ) {
				hc->DeltaHealth();

				// Can this be knocked back??
				ComponentSet thisComp( chit, ComponentSet::IS_ALIVE | Chit::SPATIAL_BIT | Chit::RENDER_BIT );
				if ( thisComp.okay ) {
					// Do we apply knockback? In place or travelling?
					// What are the rules?
					//  - solid hits do knockback
					//  - minor explosions do knockback
					bool knockback = false;
					if ( msg.Data() ) {
						// explosion
						if ( delta > item.TotalHP() * 0.2f ) {
							knockback = true;
						}
					}
					else {
						if ( delta > item.TotalHP() * 0.4f ) {
							knockback = true;
						}
					}

					if ( knockback ) {
						thisComp.render->PlayAnimation( ANIM_HEAVY_IMPACT );

						// Immediately reflect off the ground.
						Vector3F v = msg.vector;

						if ( thisComp.spatial->GetPosition().y < 0.1f ) {
							if ( v.y < 0 )
								v.y = -v.y;
							if ( v.y < 3.0f )
								v.y = 3.0f;		// make more interesting
						}
						// Rotation
						float r = msg.dataF;

						PhysicsMoveComponent* pmc = GET_COMPONENT( parentChit, PhysicsMoveComponent );
						if ( pmc ) {
							pmc->Add( v, r );
						}
						else {
							GameMoveComponent* gmc = GET_COMPONENT( parentChit, GameMoveComponent );
							if ( gmc ) {
								WorldMap* map = gmc->GetWorldMap();
								parentChit->Shelve( gmc );

								pmc = new PhysicsMoveComponent( map );
								parentChit->Add( pmc );

								pmc->Set( v, r );
								pmc->DeleteWhenDone( true );
							}
						}
					}
				}
			}
		}
	}
}


bool ItemComponent::DoTick( U32 delta )
{
	return item.DoTick( delta );
}


void ItemComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
	GLASSERT( !item.parentChit );
	item.parentChit = parentChit;
}


void ItemComponent::OnRemove() 
{
	GLASSERT( item.parentChit == parentChit );
	item.parentChit = 0;
	Component::OnRemove();
}

