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
#include "../game/pathmovecomponent.h"
#include "../game/lumoschitbag.h"

#include "../script/procedural.h"

#include "../xegame/rendercomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/istringconst.h"

using namespace grinliz;


ItemComponent::ItemComponent( Engine* _engine, WorldMap* wm, const GameItem& _item ) 
	: engine(_engine), worldMap(wm), mainItem(_item), slowTick( 500 )
{
	itemArr.Push( &mainItem );	
	pickupMode = 0;
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
	XARC_SER( xs, pickupMode );

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
	wallet.Serialize( xs );
	this->EndSerialize( xs );
}


void ItemComponent::AddGold( int delta )
{
	wallet.gold += delta;
	parentChit->SetTickNeeded();
}


void ItemComponent::AddGold( const Wallet& w )
{
	wallet.gold += w.gold;
	for( int i=0; i<NUM_CRYSTAL_TYPES; ++i ) {
		wallet.crystal[i] += w.crystal[i];
	}
	parentChit->SetTickNeeded();
}


void ItemComponent::AddBattleXP( bool isMelee, int killshotLevel )
{
	int level = mainItem.stats.Level();
	mainItem.stats.AddBattleXP( killshotLevel );
	if ( mainItem.stats.Level() > level ) {
		// Level up!
		// FIXME: show an icon
		mainItem.hp = mainItem.TotalHPF();
	}

	GameItem* weapon = 0;
	if ( isMelee ) {
		IMeleeWeaponItem*  melee  = GetMeleeWeapon();
		if ( melee ) 
			weapon = melee->GetItem();
	}
	else {
		IRangedWeaponItem* ranged = GetRangedWeapon( 0 );
		if ( ranged )
			weapon = ranged->GetItem();
	}
	if ( weapon ) {
		// instrinsic parts don't level up...that is just
		// really an extension of the main item.
		if (( weapon->flags & GameItem::INTRINSIC) == 0 ) {
			weapon->stats.AddBattleXP( killshotLevel );
		}
	}
}


void ItemComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( msg.ID() == ChitMsg::CHIT_DAMAGE ) {
		parentChit->SetTickNeeded();

		const ChitDamageInfo* info = (const ChitDamageInfo*) msg.Ptr();
		GLASSERT( info );
		DamageDesc dd = info->dd;

		// Scale damage to distance, if explosion. And check for impact.
		if ( info->isExplosion ) {
			RenderComponent* rc = parentChit->GetRenderComponent();
			if ( rc ) {
				// First scale damage.
				Vector3F target;
				rc->CalcTarget( &target );
				Vector3F v = (target - info->originOfImpact); 
				float len = v.Length();
				dd.damage = Lerp( dd.damage, dd.damage*0.5f, len / EXPLOSIVE_RANGE );
				v.Normalize();

				bool knockback = dd.damage > (mainItem.TotalHP() *0.25f);

				// Ony apply for phyisics or pathmove
				PhysicsMoveComponent* physics  = GET_SUB_COMPONENT( parentChit, MoveComponent, PhysicsMoveComponent );
				PathMoveComponent*    pathMove = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );

				if ( knockback && (physics || pathMove) ) {
					rc->PlayAnimation( ANIM_IMPACT );

					// Rotation
					float r = -300.0f + (float)chit->random.Rand( 600 );

					if ( pathMove ) {
						physics = new PhysicsMoveComponent( worldMap, true );
						parentChit->Swap( pathMove, physics );
					}
					static const float FORCE = 4.0f;
					physics->Add( v*FORCE, r );
				}
			}
		}

		GLLOG(( "Chit %3d '%s' (origin=%d) ", parentChit->ID(), mainItem.Name(), info->originID ));

		// Run through the inventory, looking for modifiers.

		for( int i=itemArr.Size()-1; i >= 0; --i ) {
			if ( i==0 || itemArr[i]->Active() ) {
				itemArr[i]->AbsorbDamage( i>0, dd, &dd, "DAMAGE" );

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

		// report XP back to what hit us.
		int originID = info->originID;
		Chit* origin = GetChitBag()->GetChit( originID );
		if ( origin && origin->GetItemComponent() ) {
			bool killshot = mainItem.hp == 0 && !(mainItem.flags & GameItem::INDESTRUCTABLE);
			origin->GetItemComponent()->AddBattleXP( info->isMelee, killshot ? mainItem.stats.Level() : 0 ); 
		}

	}
	else if ( msg.ID() == ChitMsg::CHIT_HEAL ) {
		parentChit->SetTickNeeded();
		float heal = msg.dataF;
		GLASSERT( heal >= 0 );

		mainItem.hp += heal;
		if ( mainItem.hp > mainItem.TotalHPF() ) {
			mainItem.hp = mainItem.TotalHPF();
		}

		if ( parentChit->GetSpatialComponent() ) {
			Vector3F v = parentChit->GetSpatialComponent()->GetPosition();
			engine->particleSystem->EmitPD( "heal", v, V3F_UP, engine->camera.EyeDir3(), 30 );
		}
	}
	else if ( msg.ID() >= ChitMsg::CHIT_DESTROYED_START && msg.ID() <= ChitMsg::CHIT_DESTROYED_END ) {
		// FIXME: send to reserve if no pos
		GLASSERT( parentChit->GetSpatialComponent() );
		if ( !wallet.IsEmpty() ) {
			parentChit->GetLumosChitBag()->NewWalletChits( parentChit->GetSpatialComponent()->GetPosition(), wallet );
			wallet.MakeEmpty();
		}
		// Stop picking up things after destroy sequence started.
		this->SetPickup( 0 );
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
			ChitDamageInfo info( dd );

			info.originID = parentChit->ID();
			info.awardXP  = false;
			info.isMelee  = true;
			info.isExplosion = false;
			info.originOfImpact.Zero();

			ChitMsg msg( ChitMsg::CHIT_DAMAGE, 0, &info );
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

		if (    ( i==0 || itemArr[i]->Active()) 
			 && EmitEffect( mainItem, delta )) 
		{
			tick = 0;
		}
	}


	static const float PICKUP_RANGE = 0.5f;
	static const float HOOVER_RANGE = 2.0f;

	if ( parentChit->GetSpatialComponent() ) {
		CChitArray arr;

		Vector2F pos = parentChit->GetSpatialComponent()->GetPosition2D();
		if ( pickupMode == GOLD_PICKUP || pickupMode == GOLD_HOOVER ) {
			GetChitBag()->QuerySpatialHash( &arr, pos, PICKUP_RANGE, 0, LumosChitBag::GoldCrystalFilter );
			for( int i=0; i<arr.Size(); ++i ) {
				wallet.Add( arr[i]->GetItemComponent()->GetWallet() );
				arr[i]->GetItemComponent()->EmptyWallet();
				arr[i]->QueueDelete();
			}
		}
		if ( pickupMode == GOLD_HOOVER ) {
			GetChitBag()->QuerySpatialHash( &arr, pos, HOOVER_RANGE, 0, LumosChitBag::GoldCrystalFilter );
			for( int i=0; i<arr.Size(); ++i ) {
				Chit* gold = arr[i];
				GLASSERT( parentChit != gold );
				TrackingMoveComponent* tc = GET_SUB_COMPONENT( gold, MoveComponent, TrackingMoveComponent );
				if ( !tc ) {
					tc = new TrackingMoveComponent( worldMap );
					tc->SetTarget( parentChit->ID() );
					gold->Add( tc );
				}
			}
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

				ProcRenderInfo info;
				if ( ItemGen::ProceduralRender( item->stats.Hash(), *item, &info )) {
					rc->SetProcedural( item->hardpoint, info );
				}
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

