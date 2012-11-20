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
#include "chitevent.h"
#include "chitbag.h"

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


void ItemComponent::OnChitEvent( const ChitEvent& event )
{
	if ( event.ID() == ChitEvent::EFFECT ) {
		// It's important to remember this event could be
		// coming from ourselves: which is how fire / shock
		// grows, but don't want a positive feedback loop
		// for normal-susceptible.

		if (    parentChit->random.Rand(12) == 0
			 && parentChit->GetSpatialComponent() ) 
		{
			DamageDesc dd( event.factor, event.data );
			ChitMsg msg( ChitMsg::CHIT_DAMAGE, 0, &dd );

			Vector2F v = parentChit->GetSpatialComponent()->GetPosition2D() - event.AreaOfEffect().Center();
			msg.vector.Set( v.x, 0, v.y );
			msg.vector.SafeNormalize( 0,1,0 );
			parentChit->SendMessage( msg );
		}
	}
}


bool ItemComponent::DoSlowTick()
{
	// Look around for fire or shock spread.
	if ( item.accruedFire > 0 || item.accruedShock > 0 ) {
		SpatialComponent* sc = parentChit->GetSpatialComponent();
		if ( sc ) {
			Rectangle2F aoe;
			aoe.max = aoe.min = sc->GetPosition2D();
			aoe.Outset( EFFECT_RADIUS );

			if ( item.accruedFire ) {
				ChitEvent event( ChitEvent::EFFECT, aoe );
				event.data = GameItem::EFFECT_FIRE;
				event.factor = item.accruedFire;
				GetChitBag()->QueueEvent( event );
			}
			if ( item.accruedShock ) {
				ChitEvent event( ChitEvent::EFFECT, aoe );
				event.data = GameItem::EFFECT_SHOCK;
				event.factor = item.accruedShock;
				GetChitBag()->QueueEvent( event );
			}
			return true;
		}
	}
	return false;
}


bool ItemComponent::DoTick( U32 delta )
{
	bool needsTick = item.DoTick( delta );
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

