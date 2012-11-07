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

#include "../grinliz/glrandom.h"

#include "../game/healthcomponent.h"
#include "../game/physicsmovecomponent.h"

#include "../xegame/rendercomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/inventorycomponent.h"

using namespace grinliz;

void ItemComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( msg.ID() == ChitMsg::CHIT_DAMAGE ) {
		parentChit->SetTickNeeded();
		const DamageDesc* dd = (const DamageDesc*) msg.Ptr();
		GLASSERT( dd );
		GLLOG(( "Chit %3d '%s' (origin=%d) ", parentChit->ID(), item.Name(), msg.originID ));

		// See what is in the inventory that can absorb damage.
		// The inventory can't handle the message, because it has
		// to come first, and this code needs the return value.
		// Messy.

		float delta = 0;

		InventoryComponent* ic = parentChit->GetInventoryComponent();
		DamageDesc dd2 = *dd;
		if ( ic ) {
			ic->AbsorbDamage( *dd, &dd2, "DAMAGE" );
			delta += dd->damage - dd2.damage;
		}

		float hp = item.hp;	
		item.AbsorbDamage( false, dd2, 0, "DAMAGE" );
		GLLOG(( "\n" ));
		delta += hp - item.hp;

		if ( delta || (item.hp != hp) ) {
			HealthComponent* hc = GET_COMPONENT( parentChit, HealthComponent );
			if ( hc ) {
				hc->DeltaHealth();

				// Can this be knocked back??
				ComponentSet thisComp( chit, ComponentSet::IS_ALIVE | Chit::SPATIAL_BIT | Chit::RENDER_BIT );
				if ( thisComp.okay ) {
					// Do we apply knockback?
					bool explosion = msg.Data() != 0;
					bool knockback = delta > item.TotalHP() * ( explosion ? 0.1f : 0.4f );

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
						Random random( ((int)v.x*1000)^((int)v.y*1000)^((int)v.z*1000) );
						random.Rand();
						float r = -400.0f + (float)random.Rand( 800 );

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
	bool needsTick = item.DoTick( delta );

	HealthComponent* hc = GET_COMPONENT( parentChit, HealthComponent );
	if ( hc ) {
		hc->DeltaHealth();
	}
	return needsTick;
}


void ItemComponent::EmitEffect( Engine* engine, U32 deltaTime )
{
	LowerEmitEffect( engine, item, deltaTime );
}


void ItemComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
	GLASSERT( !item.parentChit );
	item.parentChit = parentChit;
}


void ItemComponent::OnRemove() 
{
	GLASSERT( item.parentChit == parentChit );
	item.parentChit = 0;
	super::OnRemove();
}

