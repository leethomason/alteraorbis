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
#include "../game/reservebank.h"
#include "../game/lumosgame.h"

#include "../script/procedural.h"
#include "../script/itemscript.h"
#include "../script/scriptcomponent.h"

#include "../xegame/rendercomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/istringconst.h"

using namespace grinliz;


ItemComponent::ItemComponent( const GameItem& item ) 
	: slowTick( 500 ), hardpointsModified( true ), lastDamageID(0),
	  ranged(0), melee(0), reserve(0), shield(0)
{
	GLASSERT( !item.IName().empty() );
	itemArr.Push( new GameItem( item ) );
}


ItemComponent::ItemComponent( GameItem* item ) 
	: slowTick( 500 ), hardpointsModified( true ), lastDamageID(0),
	  ranged(0), melee(0), reserve(0), shield(0)
{
	// item can be null if loading.
	if ( item ) {
		GLASSERT( !item->IName().empty() );
		itemArr.Push( item );	
	}
}


ItemComponent::~ItemComponent()
{
	for( int i=0; i<itemArr.Size(); ++i ) {
		delete itemArr[i];
	}
}


void ItemComponent::DebugStr( grinliz::GLString* str )
{
	const GameItem* item = itemArr[0];
	str->AppendFormat( "[Item] %s hp=%.1f/%d lvl=%d tm=%d ", 
		item->Name(), item->hp, item->TotalHP(), item->Traits().Level(),
		item->primaryTeam );
}


void ItemComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, "ItemComponent" );
	int nItems = itemArr.Size();
	XARC_SER( xs, nItems );

	if ( xs->Loading() ) {
		for ( int i=0; i<nItems; ++i  ) {
			GameItem* pItem = new GameItem();	
			itemArr.Push( pItem );
		}
	}
	for( int i=0; i<nItems; ++i ) {
		itemArr[i]->Serialize( xs );
		GLASSERT( !itemArr[i]->IName().empty() );
	}

	this->EndSerialize( xs );
	hardpointsModified = true;
	UpdateActive();
}


void ItemComponent::NameItem(GameItem* item)
{
	bool shouldHaveName = item->Traits().Level() >= LEVEL_OF_NAMING;
	if (item->GetValue() >= VALUE_OF_NAMING) {
		shouldHaveName = true;
	}

	const ChitContext* context = this->GetChitContext();

	if ( shouldHaveName ) {
		if ( item->IProperName().empty() ) {
			IString nameGen = item->keyValues.GetIString( "nameGen" );
			if ( !nameGen.empty() ) {
				item->SetProperName( StringPool::Intern( 
					context->game->GenName( nameGen.c_str(), 
											item->ID(),
											4, 10 )));
			}

			/*
			This happens so rarely it isn't worth the bugs.
			// An item that wasn't significant is now significant.
			// In practice, this means adding tracking for lesser mobs.
			if ( item->keyValues.GetIString( "mob" ) == "lesser" ) {
				SpatialComponent* sc = parentChit->GetSpatialComponent();
				if ( sc ) {
					NewsEvent news( NewsEvent::LESSER_MOB_NAMED, sc->GetPosition2D(), item, parentChit ); 
					// Should not have been already added.
					GLASSERT( item->keyValues.GetIString( "destroyMsg" ) == IString() );
					item->keyValues.Set( "destroyMsg", NewsEvent::LESSER_NAMED_MOB_KILLED );
				}
			}
			*/
		}
	}
}


float ItemComponent::PowerRating() const
{
	GameItem* mainItem = itemArr[0];
	int level = mainItem->Traits().Level();

	return float(1+level) * float(mainItem->mass);
}


void ItemComponent::AddCraftXP( int nCrystals )
{
	GameItem* mainItem = itemArr[0];
	int level = mainItem->Traits().Level();
	mainItem->GetTraitsMutable()->AddCraftXP( nCrystals );
	if ( mainItem->Traits().Level() > level ) {
		// Level up!
		mainItem->hp = mainItem->TotalHPF();
		NameItem( mainItem );
		if ( parentChit->GetRenderComponent() ) {
			parentChit->GetRenderComponent()->AddDeco( "levelup", STD_DECO );
		}
	}
}


void ItemComponent::AddBattleXP( bool isMelee, int killshotLevel, const GameItem* loser )
{
	// Loser has to be a MOB. That was a funny bug. kills: plant0 7
	if ( loser->keyValues.GetIString( "mob" ) == IString() ) {
		return;
	}

	GameItem* mainItem = itemArr[0];
	int level = mainItem->Traits().Level();
	mainItem->GetTraitsMutable()->AddBattleXP( killshotLevel );

	bool isGreater = (loser->keyValues.GetIString( IStringConst::mob ) == IStringConst::greater);

	if ( mainItem->Traits().Level() > level ) {
		// Level up!
		mainItem->hp = mainItem->TotalHPF();
		NameItem( mainItem );
		if ( parentChit->GetRenderComponent() ) {
			parentChit->GetRenderComponent()->AddDeco( "levelup", STD_DECO );
		}
	}

	GameItem* weapon[2] = { 0, 0 };	// weapon, shield
	if ( isMelee ) {
		IMeleeWeaponItem*  melee  = GetMeleeWeapon();
		if ( melee ) 
			weapon[0] = melee->GetItem();
	}
	else {
		IRangedWeaponItem* ranged = GetRangedWeapon( 0 );
		if ( ranged )
			weapon[0] = ranged->GetItem();
	}
	weapon[1] = this->GetShield() ? this->GetShield()->GetItem() : 0;

	for (int i = 0; i < 2; ++i) {
		if (weapon[i]) {
			// instrinsic parts don't level up...that is just
			// really an extension of the main item.
			if ((weapon[i]->flags & GameItem::INTRINSIC) == 0) {
				weapon[i]->GetTraitsMutable()->AddBattleXP(killshotLevel);
				NameItem(weapon[i]);
			}
		}
	}

	if ( killshotLevel ) {
		// Credit the main item and weapon. (But not the shield.)
		mainItem->historyDB.Increment( "Kills" );
		if ( isGreater )
			mainItem->historyDB.Increment( "Greater" );

		if ( weapon[0] ) {
			weapon[0]->historyDB.Increment( "Kills" );
			if ( isGreater )
				weapon[0]->historyDB.Increment( "Greater" );
		}

		if ( loser ) {
			CStr< 64 > str;
			str.Format( "Kills: %s", loser->Name() );

			mainItem->historyDB.Increment( str.c_str() );

			if ( weapon[0] ) {
				weapon[0]->historyDB.Increment( str.c_str() );
			}
		}
	}
}


bool ItemComponent::ItemActive( const GameItem* item )
{
	GameItem* gi = const_cast<GameItem*>( item );
	int i = itemArr.Find( gi );
	if ( i >= 0 ) {
		return activeArr[i];
	}
	return false;
}

/* This is a delicate routine, and needs
   to work consistently with SetHardpoints.
   In general, inventory management is tricky stuff,
   and this seems to be a pretty good approach.
   It is decoupled from the RenderComponent, which
   solves a bunch of issues.
*/
void ItemComponent::UpdateActive()
{
	melee = 0;
	ranged = 0;
	reserve = 0;
	shield = 0;
	activeArr.Clear();

	activeArr.Push( true );	// mainitem always active
	bool usedHardpoint[EL_NUM_METADATA];

	const GameItem* mainItem = itemArr[0];
	const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( mainItem->ResourceName(), false );
	int usesWeapons = itemArr[0]->flags & GameItem::AI_USES_BUILDINGS;

	for ( int i=0; i<EL_NUM_METADATA; ++i ) {
		usedHardpoint[i] = !res || !( res->header.metaData[i].InUse() ) || !usesWeapons;	// set the hardpoint in use if it isn't available.
	}

	for( int i=1; i<itemArr.Size(); ++i ) {
		
		GameItem* item = itemArr[i];
		bool active = false;

		if ( item->hardpoint == 0 || item->Intrinsic() || !usedHardpoint[item->hardpoint] ) {	// can we use this weapon?
			if ( item->ToMeleeWeapon() ) {
				if ( !melee ) {
					melee = item->ToMeleeWeapon();
					active = true;
				}
				else if ( melee->GetItem()->Intrinsic() ) {
					melee = item->ToMeleeWeapon();
					active = true;
				}
			}
			else if ( item->ToRangedWeapon() ) {
				if ( !ranged ) {
					ranged = item->ToRangedWeapon();
					active = true;
				}
				else if ( ranged->GetItem()->Intrinsic() ) {
					ranged = item->ToRangedWeapon();
					active = true;
				}
			}
			else if ( item->ToShield() ) {
				if ( !shield ) {
					shield = item->ToShield();
					active = true;
				}
				else if ( shield->GetItem()->Intrinsic() ) {
					shield = item->ToShield();
					active = true;
				}
			}
			else {
				// Amulet, bonus, buff, etc.
				active = true;
			}
		}
		if ( active ) {
			if ( item->hardpoint && !item->Intrinsic() ) {
				GLASSERT( usedHardpoint[item->hardpoint] == false );
				usedHardpoint[item->hardpoint] = true;
			}
		}
		else {
			// NOT active - reserve?
			if ( !reserve ) {
				if ( melee && !melee->GetItem()->Intrinsic() && !item->Intrinsic() && item->ToRangedWeapon() )
					reserve = item->ToWeapon();
				if ( ranged && !ranged->GetItem()->Intrinsic() && !item->Intrinsic() && item->ToMeleeWeapon() )
					reserve = item->ToWeapon();
			}
		}
		activeArr.Push( active );
	}
}


void ItemComponent::NewsDestroy( const GameItem* item )
{
	NewsHistory* history = this->GetChitBag()->GetNewsHistory();
	GLASSERT(history);

	Vector2F pos = { 0, 0 };
	if ( parentChit->GetSpatialComponent() ) {
		pos = parentChit->GetSpatialComponent()->GetPosition2D();
	}

	Chit* destroyer = parentChit->GetChitBag()->GetChit( lastDamageID );

	int msg = 0;
	if ( item->keyValues.Get( ISC::destroyMsg, &msg ) == 0 ) {
		history->Add( NewsEvent( msg, pos, parentChit, destroyer ));
	}
}


void ItemComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	const ChitContext* context = GetChitContext();
	GameItem* mainItem = itemArr[0];
	GLASSERT( !mainItem->IName().empty() );

	if ( msg.ID() == ChitMsg::CHIT_DAMAGE ) {
		parentChit->SetTickNeeded();

		const ChitDamageInfo* info = (const ChitDamageInfo*) msg.Ptr();
		GLASSERT( info );
		const DamageDesc ddorig = info->dd;

		// Scale damage to distance, if explosion. And check for impact.
		if ( info->isExplosion ) {
			RenderComponent* rc = parentChit->GetRenderComponent();
			if ( rc ) {
				// First scale damage.
				Vector3F target;
				rc->CalcTarget( &target );
				Vector3F v = (target - info->originOfImpact); 
				float len = v.Length();
				DamageDesc dd = ddorig;
				dd.damage = Lerp( dd.damage, dd.damage*0.5f, len / EXPLOSIVE_RANGE );
				v.Normalize();

				//bool knockback = dd.damage > (mainItem.TotalHP() *0.25f);
				bool knockback = ddorig.damage > (mainItem->TotalHP() *0.25f);

				// Ony apply for phyisics or pathmove
				PhysicsMoveComponent* physics  = GET_SUB_COMPONENT( parentChit, MoveComponent, PhysicsMoveComponent );
				PathMoveComponent*    pathMove = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );

				if ( knockback && (physics || pathMove) ) {
					rc->PlayAnimation( ANIM_IMPACT );

					// Rotation
					float r = -300.0f + (float)chit->random.Rand( 600 );

					if ( pathMove ) {
						physics = new PhysicsMoveComponent( true );
						parentChit->Remove( pathMove );
						GetChitBag()->DeferredDelete( pathMove );
						parentChit->Add( physics );
					}
					static const float FORCE = 4.0f;
					physics->Add( v*FORCE, r );
				}
			}
		}

		GLLOG(( "Chit %3d '%s' (origin=%d) ", parentChit->ID(), mainItem->Name(), info->originID ));

		// Run through the inventory, looking for modifiers.
		// Be cautious there is no order dependency here (except the
		// inverse order to do the mainItem last)
		DamageDesc dd = ddorig;
		for( int i=itemArr.Size()-1; i>=0; --i ) {
			if ( i==0 || ItemActive(i) ) {
				itemArr[i]->AbsorbDamage( i>0, dd, &dd, this->GetMeleeWeapon(), parentChit );

				if ( itemArr[i]->ToShield() ) {
					GameItem* shield = itemArr[i]->ToShield()->GetItem();
					
					RenderComponent* rc = parentChit->GetRenderComponent();
					if ( rc && shield->RoundsFraction() > 0 ) {
						ParticleDef def = context->engine->particleSystem->GetPD( "shield" );
						Vector3F shieldPos = { 0, 0, 0 };
						rc->GetMetaData( HARDPOINT_SHIELD, &shieldPos );
					
						float f = shield->RoundsFraction();
						def.color.x *= f;
						def.color.y *= f;
						def.color.z *= f;
						context->engine->particleSystem->EmitPD( def, shieldPos, V3F_UP, 0 );
					}
				}

			}
		}

		// report XP back to what hit us.
		int originID = info->originID;
		Chit* origin = GetChitBag()->GetChit( originID );
		if ( origin && origin->GetItemComponent() ) {
			bool killshot = mainItem->hp == 0 && !(mainItem->flags & GameItem::INDESTRUCTABLE);
			origin->GetItemComponent()->AddBattleXP( info->isMelee, 
													 killshot ? Max( 1, mainItem->Traits().Level()) : 0, 
													 mainItem ); 
		}
		lastDamageID = originID;
	}
	else if ( msg.ID() == ChitMsg::CHIT_HEAL ) {
		parentChit->SetTickNeeded();
		float heal = msg.dataF;
		GLASSERT( heal >= 0 );

		mainItem->hp += heal;
		if ( mainItem->hp > mainItem->TotalHPF() ) {
			mainItem->hp = mainItem->TotalHPF();
		}

		if ( parentChit->GetSpatialComponent() ) {
			Vector3F v = parentChit->GetSpatialComponent()->GetPosition();
			context->engine->particleSystem->EmitPD( "heal", v, V3F_UP, 30 );
		}
	}
	else if ( msg.ID() == ChitMsg::CHIT_TRACKING_ARRIVED ) {
		Chit* gold = (Chit*)msg.Ptr();
		GoldCrystalFilter filter;
		if (    filter.Accept( gold )		// it really is gold/crystal
			 && gold->GetItem()				// it has an item component
			 && !parentChit->Destroyed() )	// this item is alive
		{
			Transfer(&mainItem->wallet, &gold->GetItem()->wallet, gold->GetItem()->wallet);

			// Need to delete the gold, else it will track to us again!
			gold->DeRez();

			if ( parentChit->GetRenderComponent() ) {
				parentChit->GetRenderComponent()->AddDeco( "loot", STD_DECO );
			}
		}
	}
	else if ( msg.ID() >= ChitMsg::CHIT_DESTROYED_START && msg.ID() <= ChitMsg::CHIT_DESTROYED_END ) 
	{
		/* A significant GameItem will automatically set it's history in the ItemHistory
		   database when it changes. (Some changes to the item need to be set in the
		   history as well.) The ItemHistory stores what the item is/was not a series of
		   events.

		   The NewsHistory is the series of events, including the creation and destruction
		   of the item. All the dates get inferredd from that.
		*/
		if ( msg.ID() == ChitMsg::CHIT_DESTROYED_START ) {
			NewsDestroy( mainItem );
		}

		// Drop our wallet on the ground or send to the Reserve?
		// MOBs drop items, as does anything with sub-items
		// or carrying crystal.
		MOBIshFilter mobFilter;

		Wallet w = mainItem->wallet.EmptyWallet();
		bool dropItems = false;
		Vector3F pos = { 0, 0, 0 };
		if ( parentChit->GetSpatialComponent() ) {
			pos = parentChit->GetSpatialComponent()->GetPosition();
		}

		if ( mobFilter.Accept( parentChit ) || mainItem->wallet.NumCrystals() || this->NumCarriedItems() ) {
			dropItems = true;
			if ( !w.IsEmpty() ) {
				parentChit->GetLumosChitBag()->NewWalletChits( pos, w );
				w.EmptyWallet();
			}
		}
		if (!w.IsEmpty()) {
			if ( ReserveBank::Instance() ) {	// null in battle mode
				ReserveBank::Instance()->bank.Add( w );
			}
		}

		while( itemArr.Size() > 1 ) {
			if ( itemArr[itemArr.Size()-1]->Intrinsic() ) {
				break;
			}
			GameItem* item = itemArr.Pop();
			GLASSERT( !item->IName().empty() );
			if ( dropItems ) {
				parentChit->GetLumosChitBag()->NewItemChit( pos, item, true, true, 0 );
			}
			else {
				ReserveBank::Instance()->bank.Add( item->wallet.EmptyWallet() );
				NewsDestroy( item );
				delete item;
			}
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
			ChitDamageInfo info( dd );

			info.originID = parentChit->ID();
			info.awardXP  = false;
			info.isMelee  = true;
			info.isExplosion = false;
			info.originOfImpact.Zero();

			ChitMsg msg( ChitMsg::CHIT_DAMAGE, 0, &info );
			parentChit->SendMessage( msg );
			parentChit->SetTickNeeded();
		}
	}
}


void ItemComponent::DoSlowTick()
{
	GameItem* mainItem = itemArr[0];

	if (mainItem->flags & GameItem::INDESTRUCTABLE) {
		mainItem->accruedFire = 0;
		mainItem->accruedShock = 0;
	}

	// Look around for fire or shock spread.
	if ( mainItem->accruedFire > 0 || mainItem->accruedShock > 0 ) {
		SpatialComponent* sc = parentChit->GetSpatialComponent();
		if ( sc ) {
			if ( mainItem->accruedFire ) {
				ChitEvent event = ChitEvent::EffectEvent( sc->GetPosition2D(), EFFECT_RADIUS, GameItem::EFFECT_FIRE, mainItem->accruedFire );
				GetChitBag()->QueueEvent( event );
			}
			if ( mainItem->accruedShock ) {
				ChitEvent event = ChitEvent::EffectEvent( sc->GetPosition2D(), EFFECT_RADIUS, GameItem::EFFECT_SHOCK, mainItem->accruedShock );
				GetChitBag()->QueueEvent( event );
			}
		}
	}

	if ( mainItem->flags & GameItem::GOLD_PICKUP ) {
		ComponentSet thisComp( parentChit, Chit::SPATIAL_BIT | ComponentSet::IS_ALIVE );
		if ( thisComp.okay ) {
			CChitArray arr;

			Vector2F pos = parentChit->GetSpatialComponent()->GetPosition2D();
			GoldCrystalFilter goldCrystalFilter;
			GetChitBag()->QuerySpatialHash( &arr, pos, PICKUP_RANGE, 0, &goldCrystalFilter );
			for( int i=0; i<arr.Size(); ++i ) {
				Chit* gold = arr[i];
				GLASSERT( parentChit != gold );
				TrackingMoveComponent* tc = GET_SUB_COMPONENT( gold, MoveComponent, TrackingMoveComponent );
				if ( !tc ) {
					tc = new TrackingMoveComponent();
					tc->SetTarget( parentChit->ID() );
					gold->Add( tc );
				}
			}
		}
	}
	ApplyLootLimits();
}


int ItemComponent::DoTick( U32 delta )
{
	if ( hardpointsModified && parentChit->GetRenderComponent() ) {
		SetHardpoints();
		hardpointsModified = false;
	}
	const ChitContext* context = GetChitContext();

	GameItem* mainItem = itemArr[0];

#ifdef DEBUG
	{
		// FIXME: argument for moving health component functionality
		// into the item component.
		HealthComponent* hc = parentChit->GetHealthComponent();
		if (mainItem->flags & GameItem::INDESTRUCTABLE) {
			GLASSERT(!hc);
		}
		else {
			GLASSERT(hc);
		}
	}
#endif

	// Slow tick is about 500-600 ms
	if ( slowTick.Delta( delta )) {
		DoSlowTick();
	}
	int tick = VERY_LONG_TICK;

	for( int i=0; i<itemArr.Size(); ++i ) {	
		int t = itemArr[i]->DoTick( delta );
		tick = Min( t, tick );

		// This accounts for accruedFire or accruedShock turning on the timer.
		if (    ( i==0 || ItemActive(i) ) 
			 && EmitEffect( *mainItem, delta )) 
		{
			tick = 0;
		}
	}

	// FIXME: hack for ui. shouldn't use specific names.
	if (    mainItem->IName() == IStringConst::humanMale
		 || mainItem->IName() == IStringConst::humanFemale
		 || mainItem->IName() == IStringConst::worker ) 
	{
		if ( parentChit->GetRenderComponent() ) {
			int index = this->FindItem( IStringConst::fruit );
			if ( index >= 0 ) {
				parentChit->GetRenderComponent()->AddDeco( "fruit", STD_DECO/2 );
			}
			// don't remove - can't see fruit being eaten!
			//else {
			//	parentChit->GetRenderComponent()->RemoveDeco( "fruit" );
			//}
		}
	}	

	return tick;
}


void ItemComponent::OnAdd( Chit* chit )
{
	GameItem* mainItem = itemArr[0];
	GLASSERT( itemArr.Size() >= 1 );	// the one true item
	super::OnAdd( chit );
	hardpointsModified = true;

	if ( parentChit->GetLumosChitBag() ) {
		IString mob = mainItem->keyValues.GetIString( "mob" );
		if ( mob == IStringConst::lesser ) {
			parentChit->GetLumosChitBag()->census.normalMOBs += 1;
		}
		else if ( mob == IStringConst::greater ) {
			parentChit->GetLumosChitBag()->census.greaterMOBs += 1;
		}
	}
	slowTick.SetPeriod( 500 + (chit->ID() & 128));
	UpdateActive();
}


void ItemComponent::OnRemove() 
{
	GameItem* mainItem = itemArr[0];
	if ( parentChit->GetLumosChitBag() ) {
		IString mob;
		mainItem->keyValues.Get( ISC::mob, &mob );
		if ( mob == IStringConst::lesser ) {
			parentChit->GetLumosChitBag()->census.normalMOBs -= 1;
		}
		else if ( mob == IStringConst::greater ) {
			parentChit->GetLumosChitBag()->census.greaterMOBs -= 1;
		}
	}
	super::OnRemove();
}


bool ItemComponent::EmitEffect( const GameItem& it, U32 delta )
{
	const ChitContext* context = GetChitContext();
	ParticleSystem* ps = context->engine->particleSystem;
	bool emitted = false;

	ComponentSet compSet( parentChit, Chit::ITEM_BIT | Chit::SPATIAL_BIT | ComponentSet::IS_ALIVE );

	if ( compSet.okay ) {
		if ( it.accruedFire > 0 ) {
			ps->EmitPD( "fire", compSet.spatial->GetPosition(), V3F_UP, delta );
			ps->EmitPD( "smoke", compSet.spatial->GetPosition(), V3F_UP, delta );
			emitted = true;
		}
		if ( it.accruedShock > 0 ) {
			ps->EmitPD( "shock", compSet.spatial->GetPosition(), V3F_UP, delta );
			emitted = true;
		}
	}
	return emitted;
}


class ItemValueCompare
{
public:
	bool Less( const GameItem* v0, const GameItem* v1 ) const
	{
		int val0 = v0->GetValue();
		int val1 = v1->GetValue();
		// sort descending: (FIXME: should really add a flag for this, in the Sort)
		return val0 > val1;
	}
};


void ItemComponent::SortInventory()
{
	ItemValueCompare compare;

	for( int i=1; i<itemArr.Size(); ++i ) {
		if ( itemArr[i]->Intrinsic() )
			continue;
		if ( itemArr.Size() > i+1 ) {
			::Sort<const GameItem*, ItemValueCompare>( (const GameItem**) &itemArr[i], itemArr.Size() - i, compare );
		}
		break;
	}
}



int ItemComponent::NumCarriedItems() const
{
	int count = 0;
	for( int i=1; i<itemArr.Size(); ++i ) {
		if ( !itemArr[i]->Intrinsic() )
			++count;
	}
	return count;
}


int ItemComponent::ItemToSell() const
{
	// returns 0 or the cheapest item that can be sold
	int index = 0;
	int val = INT_MAX;

	for (int i = 1; i < itemArr.Size(); ++i) {
		GameItem* item = itemArr[i];
		if (!item->Intrinsic() && item->GetValue()) {
			if (!ItemActive(i) 
				&& (!reserve || item != reserve->GetItem()))
			{
				if (item->GetValue() < val) {
					index = i;
					val = item->GetValue();
				}
			}
		}
	}
	return index;
}


bool ItemComponent::CanAddToInventory()
{
	int count = NumCarriedItems();
	return count < INVERTORY_SLOTS;
}


void ItemComponent::AddToInventory( GameItem* item )
{
	GLASSERT( this->CanAddToInventory() );
	itemArr.Push( item );

	// AIs will use the "best" item.
	if ( parentChit && !parentChit->PlayerControlled() ) {
		SortInventory();
	}
	hardpointsModified = true;
	UpdateActive();
}


void ItemComponent::AddSubInventory( ItemComponent* ic, bool addWeapons, grinliz::IString filterItems )
{
	GLASSERT( ic );
	for( int i=1; i<ic->NumItems(); ++i ) {
		GameItem* item = ic->GetItem(i);
		if ( item->Intrinsic() ) continue;
		if ( !this->CanAddToInventory() ) break;

		if (    (addWeapons && ( item->ToWeapon() || item->ToShield() ))
			 || filterItems == item->IName() )
		{
			ic->RemoveFromInventory( i );
			--i;
			this->AddToInventory( item );
		}
	}

	// AIs will use the "best" item.
	if ( !parentChit->PlayerControlled() ) {
		SortInventory();
	}
	hardpointsModified = true;
	UpdateActive();
}


void ItemComponent::AddToInventory( ItemComponent* ic )
{
	GLASSERT( ic );
	GLASSERT( ic->NumItems() == 1 );
	GLASSERT( this->CanAddToInventory() );
	GameItem* gameItem = ic->itemArr.Pop();
	itemArr.Push( gameItem );
	delete ic;

	// AIs will use the "best" item.
	if ( !parentChit->PlayerControlled() ) {
		SortInventory();
	}
	hardpointsModified = true;
	UpdateActive();

	if ( parentChit && parentChit->GetRenderComponent() ) {
		if ( gameItem->IName() == IStringConst::fruit ) {
			parentChit->GetRenderComponent()->AddDeco( "fruit", STD_DECO );	// will get refreshed if carred.
		}
		else {
			parentChit->GetRenderComponent()->AddDeco( "loot", STD_DECO );
		}
	}
}


GameItem* ItemComponent::RemoveFromInventory( int index )
{
	GLASSERT( index < itemArr.Size() );
	GameItem* item = itemArr[index];
	if ( item->Intrinsic() ) 
		return 0;
	if ( index == 0 )
		return 0;

	itemArr.Remove( index );
	hardpointsModified = true;
	UpdateActive();
	return item;
}


void ItemComponent::Drop( const GameItem* item )
{
	GLASSERT( parentChit->GetSpatialComponent() );
	if ( !parentChit->GetSpatialComponent() )
		return;

	hardpointsModified = true;
	// Can't drop main or intrinsic item
	for( int i=1; i<itemArr.Size(); ++i ) {
		if ( itemArr[i]->Intrinsic() )
			continue;
		if ( itemArr[i] == item ) {
			parentChit->GetLumosChitBag()->NewItemChit( parentChit->GetSpatialComponent()->GetPosition(), 
														itemArr[i], true, true, 0 );
			itemArr.Remove(i);
			UpdateActive();
			return;
		}
	}
	GLASSERT( 0 );	// should have found the item.
}


bool ItemComponent::Swap( int i, int j ) 
{
	if (    i > 0 && j > 0 && i < itemArr.Size() && j < itemArr.Size()
		 && !itemArr[i]->Intrinsic()
		 && !itemArr[j]->Intrinsic() )
	{
		grinliz::Swap( &itemArr[i], &itemArr[j] );
		hardpointsModified = true;
		hardpointsModified = true;
		UpdateActive();
		return true;
	}
	return false;
}


/*static*/ bool ItemComponent::Swap2( ItemComponent* a, int aIndex, ItemComponent* b, int bIndex )
{
	if (    a && b
		 && (aIndex >= 0 && aIndex < a->itemArr.Size() )
		 && (bIndex >= 0 && bIndex < b->itemArr.Size() )
		 && !a->itemArr[aIndex]->Intrinsic()
		 && !b->itemArr[bIndex]->Intrinsic() )
	{
		grinliz::Swap( &a->itemArr[aIndex], &b->itemArr[bIndex] );
		a->UpdateActive();
		a->hardpointsModified = true;
		b->UpdateActive();
		b->hardpointsModified = true;
		return true;
	}
	return false;
}


bool ItemComponent::SwapWeapons()
{
	// The current but NOT intrinsic
	GameItem* current = 0;
	IRangedWeaponItem* ranged = GetRangedWeapon(0);
	IMeleeWeaponItem*  melee  = GetMeleeWeapon();
	if ( ranged && !ranged->GetItem()->Intrinsic() )
		current = ranged->GetItem();
	if ( melee && !melee->GetItem()->Intrinsic() )
		current = melee->GetItem();

	// The reserve is never intrinsic:
	IWeaponItem* reserve = GetReserveWeapon();
	GLASSERT( !reserve->GetItem()->Intrinsic() );

	GLASSERT( current != reserve->GetItem() );

	if ( current && reserve ) {
		int slot0 = FindItem( current->GetItem() );
		int slot1 = FindItem( reserve->GetItem() );
		Swap( slot0, slot1 );

		hardpointsModified = true;
		UpdateActive();
	
		GLOUTPUT(( "Swapped %s -> %s\n",
				   current->GetItem()->BestName(),
				   reserve->GetItem()->BestName() ));
		return true;
	}
	return false;
}


IRangedWeaponItem* ItemComponent::GetRangedWeapon( grinliz::Vector3F* trigger )
{
	// Go backwards, so that we get the NOT intrinsic first.
	if ( ranged && trigger ) {
		RenderComponent* rc = parentChit->GetRenderComponent();
		GLASSERT( rc );
		if ( rc ) {
			Matrix4 xform;
			rc->GetMetaData( ranged->GetItem()->hardpoint, &xform ); 
			Vector3F pos = xform * V3F_ZERO;
			*trigger = pos;
		}
	}
	return ranged;
}


void ItemComponent::SetHardpoints()
{
	if ( !parentChit->GetRenderComponent() ) {
		return;
	}
	RenderComponent* rc = parentChit->GetRenderComponent();
	bool female = strstr( itemArr[0]->Name(), "Female" ) != 0;
	int  team   = itemArr[0]->primaryTeam;

	for( int i=0; i<itemArr.Size(); ++i ) {
		const GameItem* item = itemArr[i];
		bool setProc = (i==0);

		if ( i > 0 && item->hardpoint && !item->Intrinsic() && activeArr[i] ) { 
			setProc = true;
			rc->Attach( item->hardpoint, item->ResourceName() );
		}

		if ( setProc )	// not a built-in
		{
			IString proc = itemArr[i]->keyValues.GetIString( "procedural" );
			int features = 0;
			itemArr[i]->keyValues.Get( ISC::features, &features );

			ProcRenderInfo info;

			if ( !proc.empty() ) {
				AssignProcedural( proc.c_str(), female, item->ID(), team, false, item->Effects(), features, &info );
			}
			rc->SetProcedural( (i==0) ? 0 : itemArr[i]->hardpoint, info );
		}
	}
}


void ItemComponent::ApplyLootLimits()
{
	// Some MOBs gather a lot of loot. Cheat a little, and send
	// the massive loot tax off the reserve.
	GameItem* mainItem = itemArr[0];
	IString mob = mainItem->keyValues.GetIString("mob");
	
	// Lesser.
	int limit = 0;
	int cLimit = 0;

	if (mob == IStringConst::greater) {
		limit = MAX_GREATER_GOLD;
		cLimit = MAX_GREATER_MOB_CRYSTAL;
	}
	else if (mob == IStringConst::lesser) {
		limit = MAX_LESSER_GOLD;
		cLimit = MAX_LESSER_MOB_CRYSTAL;
	}

	if (limit) {
		int d = mainItem->wallet.gold - limit;
		if (d > 0) {
			mainItem->wallet.AddGold(-d);
			ReserveBank::Instance()->bank.AddGold(d);
		}
		for (int i = 0; i<NUM_CRYSTAL_TYPES; ++i) {
			d = mainItem->wallet.crystal[i] - cLimit;
			if (d > 0) {
				mainItem->wallet.AddCrystal(i, -d);
				ReserveBank::Instance()->bank.AddCrystal(i, d);
			}
		}
	}
}
