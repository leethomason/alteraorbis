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

#include "../engine/engine.h"
#include "../engine/particle.h"

#include "../grinliz/glrandom.h"

#include "../game/healthcomponent.h"
#include "../game/physicsmovecomponent.h"

#include "../script/procedural.h"

#include "../xegame/rendercomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/istringconst.h"

using namespace grinliz;


ItemComponent::ItemComponent( Engine* _engine, const GameItem& _item ) : engine(_engine), mainItem(_item), slowTick( 500 )
{
	itemArr.Push( &mainItem );	
}


ItemComponent::~ItemComponent()
{
	for( int i=1; i<itemArr.Size(); ++i ) {
		delete itemArr[i];
	}
}



void ItemComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, "ItemComponent" );
	int nItems = itemArr.Size();
	XARC_SER( xs, nItems );

	if ( xs->Loading() ) {
		GLASSERT( itemArr.Size() == 1 );
		for ( int i=1; i<nItems; ++i  ) {
			GameItem* pItem = new GameItem();	
			itemArr.Push( pItem );
		}
	}
	for( int i=0; i<nItems; ++i ) {
		itemArr[i]->Serialize( xs );
	}
	this->EndSerialize( xs );
}


void ItemComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( msg.ID() == ChitMsg::CHIT_DAMAGE ) {
		parentChit->SetTickNeeded();
		const DamageDesc* dd = (const DamageDesc*) msg.Ptr();
		GLASSERT( dd );
		GLLOG(( "Chit %3d '%s' (origin=%d) ", parentChit->ID(), mainItem.Name(), msg.originID ));

		// Run through the inventory, looking for modifiers.

		DamageDesc dd2 = *dd;

		for( int i=itemArr.Size()-1; i >= 0; --i ) {
			if ( i==0 || itemArr[i]->Active() ) {
				itemArr[i]->AbsorbDamage( i>0, dd2, &dd2, "DAMAGE" );

				if ( itemArr[i]->ToShield() ) {
					GameItem* shield = itemArr[i]->ToShield()->GetItem();
					RenderComponent* rc = parentChit->GetRenderComponent();

					if ( rc && shield->RoundsFraction() > 0 ) {
						ParticleDef def = engine->particleSystem->GetPD( "shield" );
						Vector3F shieldPos = { 0, 0, 0 };
						rc->GetMetaData( IStringConst::kshield, &shieldPos );
					
						float f = shield->RoundsFraction();
						def.color.x *= f;
						def.color.y *= f;
						def.color.z *= f;
						engine->particleSystem->EmitPD( def, shieldPos, V3F_UP, engine->camera.EyeDir3(), 0 );
					}
				}

			}
		}

		HealthComponent* hc = GET_COMPONENT( parentChit, HealthComponent );
		if ( hc ) {
			// Can this be knocked back??
			ComponentSet thisComp( chit, ComponentSet::IS_ALIVE | Chit::SPATIAL_BIT | Chit::RENDER_BIT );
			if ( thisComp.okay ) {
				// Do we apply knockback?
				bool explosion = msg.Data() != 0;
				bool knockback = dd->damage > (mainItem.TotalHP() * ( explosion ? 0.1f : 0.4f ));

				if ( knockback ) {
					thisComp.render->PlayAnimation( ANIM_IMPACT );

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
							parentChit->Remove( gmc );
							delete gmc;
							gmc = 0;
							
							WorldMap* map = gmc->GetWorldMap();

							pmc = new PhysicsMoveComponent( map );
							parentChit->Add( pmc );

							pmc->Set( v, r );
							pmc->DeleteAndRestorePathMCWhenDone( true );
						}
					}
				}
			}
		}
	}
	else if ( msg.ID() == ChitMsg::CHIT_HEAL ) {
		parentChit->SetTickNeeded();
		float heal = msg.dataF;
		GLASSERT( heal >= 0 );

		mainItem.hp += heal;
		if ( mainItem.hp > mainItem.TotalHP() ) {
			mainItem.hp = mainItem.TotalHP();
		}
	}
	else {
		super::OnChitMsg( chit, msg );
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


void ItemComponent::DoSlowTick()
{
	// Look around for fire or shock spread.
	if ( mainItem.accruedFire > 0 || mainItem.accruedShock > 0 ) {
		SpatialComponent* sc = parentChit->GetSpatialComponent();
		if ( sc ) {
			if ( mainItem.accruedFire ) {
				ChitEvent event = ChitEvent::EffectEvent( sc->GetPosition2D(), EFFECT_RADIUS, GameItem::EFFECT_FIRE, mainItem.accruedFire );
				GetChitBag()->QueueEvent( event );
			}
			if ( mainItem.accruedShock ) {
				ChitEvent event = ChitEvent::EffectEvent( sc->GetPosition2D(), EFFECT_RADIUS, GameItem::EFFECT_SHOCK, mainItem.accruedShock );
				GetChitBag()->QueueEvent( event );
			}
		}
	}
}


int ItemComponent::DoTick( U32 delta, U32 since )
{
	if ( slowTick.Delta( since )) {
		DoSlowTick();
	}
	int tick = VERY_LONG_TICK;
	for( int i=0; i<itemArr.Size(); ++i ) {	
		int t = itemArr[i]->DoTick( delta, since );
		tick = Min( t, tick );

		if ( itemArr[i]->Active() && EmitEffect( mainItem, delta )) {
			tick = 0;
		}
	}
	return tick;
}


void ItemComponent::OnAdd( Chit* chit )
{
	GLASSERT( itemArr.Size() >= 1 );	// the one true item
	super::OnAdd( chit );
	GLASSERT( !mainItem.parentChit );
	mainItem.parentChit = parentChit;
}


void ItemComponent::OnRemove() 
{
	GLASSERT( mainItem.parentChit == parentChit );
	mainItem.parentChit = 0;
	super::OnRemove();
}


bool ItemComponent::EmitEffect( const GameItem& it, U32 delta )
{
	ParticleSystem* ps = engine->particleSystem;
	bool emitted = false;

	ComponentSet compSet( parentChit, Chit::ITEM_BIT | Chit::SPATIAL_BIT | ComponentSet::IS_ALIVE );

	if ( compSet.okay ) {
		if ( it.accruedFire > 0 ) {
			ps->EmitPD( "fire", compSet.spatial->GetPosition(), V3F_UP, engine->camera.EyeDir3(), delta );
			ps->EmitPD( "smoke", compSet.spatial->GetPosition(), V3F_UP, engine->camera.EyeDir3(), delta );
			emitted = true;
		}
		if ( it.accruedShock > 0 ) {
			ps->EmitPD( "shock", compSet.spatial->GetPosition(), V3F_UP, engine->camera.EyeDir3(), delta );
			emitted = true;
		}
	}
	return emitted;
}

class InventorySorter
{
public:
	// "Less" in this case means "closer to center"
	static bool Less( GameItem* i0, GameItem* i1 ) {

		// Pack
		// Held + hardpoint
		// Held 
		// Intrinsic + hardpoint
		// Intrinsic
		// (core)

		enum {
			CORE,
			INTRINSIC	= 2,
			HELD		= 4,
			PACK		= 6
		};

		int s[2] = { 0, 0 };
		for( int i=0; i<2; ++i ) {
			GameItem* item = (i==0) ? i0 : i1;

			if ( item->Intrinsic() ) {
				s[i] = INTRINSIC;
			}
			else if ( item->Active() ) {
				s[i] = HELD;
			}
			else {
				s[i] = PACK;
				GLASSERT( !item->Active() );
			}
			if ( item->hardpoint ) {
				s[i] += 1;
			}
		}
		if ( s[0] != s[1] ) {
			return s[0] < s[1];
		}
		return strcmp( i0->Name(), i1->Name() ) < 0;
	}
};

bool ItemComponent::AddToInventory( GameItem* item, bool equip )
{
	bool hardpointOpen = false;
	RenderComponent* rc = parentChit->GetRenderComponent();
	if ( rc ) {
		hardpointOpen = rc->HardpointAvailable( item->hardpoint );
	}

	bool equipped = false;
	
	if ( item->Intrinsic() ) {
		equipped = true;
	}
	else if ( equip && item->hardpoint == 0 ) {
		// Held, but doesn't consume a hardpoint.
		equipped = true;
	} 
	else if ( equip ) {
		GLASSERT( item->hardpoint );
		// Tell the render component to render.
		GLASSERT( rc );

		if ( rc ) {
			if ( rc->HardpointAvailable( item->hardpoint )) {
				equipped = true;
				bool okay = rc->Attach( item->hardpoint, item->ResourceName() );
				GLASSERT( okay );

				/*
				if ( item->procedural & PROCEDURAL_INIT_MASK ) {
					ProcRenderInfo info;
					int result = ItemGen::RenderItem( Game::GetMainPalette(), *item, &info );

					if ( result == ItemGen::PROC4 ) {
						rc->SetProcedural( item->hardpoint, info );
					}
				}
				*/
			}
		}
	}
	GLASSERT( item->parentChit == 0 );
	item->parentChit = parentChit;
	item->isHeld = equipped;
	itemArr.Push( item );

	if ( itemArr.Size() > 2 ) {
		// remember, the 1st item is special. don't move it about.
		Sort<GameItem*, InventorySorter>( itemArr.Mem()+1, itemArr.Size()-1 );
	}

	return equipped;
}


GameItem* ItemComponent::IsCarrying()
{
	for( int i=1; i<itemArr.Size(); ++i ) {
		if (    itemArr[i]->Active()							// in use (not in pack)
			 && itemArr[i]->hardpoint == HARDPOINT_TRIGGER		// at the hardpoint of interest
			 && !itemArr[i]->Intrinsic() )						// not a built-in
		{
			return itemArr[i];
		}
	}
	return 0;
}


IRangedWeaponItem* ItemComponent::GetRangedWeapon( grinliz::Vector3F* trigger )
{
	for( int i=itemArr.Size()-1; i>=0; --i ) {
		if ( itemArr[i]->ToRangedWeapon() ) {
			if ( trigger ) {
				RenderComponent* rc = parentChit->GetRenderComponent();
				GLASSERT( rc );
				if ( rc ) {
					Matrix4 xform;
					rc->GetMetaData( IStringConst::Hardpoint( itemArr[i]->hardpoint ), &xform ); 
					Vector3F pos = xform * V3F_ZERO;
					*trigger = pos;
				}
			}
			return itemArr[i]->ToRangedWeapon();
		}
	}
	return 0;
}


IMeleeWeaponItem* ItemComponent::GetMeleeWeapon()
{
	for( int i=itemArr.Size()-1; i>=0; --i ) {
		if ( itemArr[i]->ToMeleeWeapon() ) {
			return itemArr[i]->ToMeleeWeapon();
		}
	}
	return 0;
}


IShield* ItemComponent::GetShield()
{
	for( int i=itemArr.Size()-1; i>=0; --i ) {
		if ( itemArr[i]->ToShield() ) {
			return itemArr[i]->ToShield();
		}
	}
	return 0;
}

