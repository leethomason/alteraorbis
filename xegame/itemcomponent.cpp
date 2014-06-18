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
#include "../game/team.h"
#include "../game/worldmap.h"
#include "../game/weather.h"

#include "../script/procedural.h"
#include "../script/itemscript.h"

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
	int group = 0, id = 0;
	Team::SplitID(item->team, &group, &id);
	str->AppendFormat( "[Item] %s hp=%.1f/%d lvl=%d tm=%d,%d ", 
		item->Name(), item->hp, item->TotalHP(), item->Traits().Level(),
		group, id );
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
	const ChitContext* context = this->Context();

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
		mainItem->hp = double(mainItem->TotalHP());
		NameItem( mainItem );
		if ( parentChit->GetRenderComponent() ) {
			parentChit->GetRenderComponent()->AddDeco( "levelup", STD_DECO );
		}
	}
}


void ItemComponent::AddBattleXP( const GameItem* loser, bool killshot )
{
	// Loser has to be a MOB. That was a funny bug. kills: plant0 7
	if ( loser->keyValues.GetIString( "mob" ) == IString() ) {
		return;
	}

	GameItem* mainItem = itemArr[0];
	int level = mainItem->Traits().Level();
	int killshotLevel = killshot ? 0 : loser->Traits().Level();
	mainItem->GetTraitsMutable()->AddBattleXP( killshotLevel );

	bool isGreater = (loser->keyValues.GetIString( IStringConst::mob ) == IStringConst::greater);

	if ( mainItem->Traits().Level() > level ) {
		// Level up!
		mainItem->hp = double(mainItem->TotalHP());
		NameItem( mainItem );
		if ( parentChit->GetRenderComponent() ) {
			parentChit->GetRenderComponent()->AddDeco( "levelup", STD_DECO );
		}
	}

	// credit everything in use
	static const int NUM = 3;
	GameItem* weapon[NUM] = { 0, 0, 0 };	// ranged, melee, shield
	weapon[0] = this->GetRangedWeapon(0) ? this->GetRangedWeapon(0)->GetItem() : 0;
	weapon[1] = this->GetMeleeWeapon() ? this->GetMeleeWeapon()->GetItem() : 0;
	weapon[2] = this->GetShield() ? this->GetShield()->GetItem() : 0;

	for (int i = 0; i < NUM; ++i) {
		if (weapon[i]) {
			// instrinsic parts don't level up...that is just
			// really an extension of the main item.
			if ((weapon[i]->flags & GameItem::INTRINSIC) == 0) {
				weapon[i]->GetTraitsMutable()->AddBattleXP(killshotLevel);
				NameItem(weapon[i]);
			}
		}
	}

	if ( killshot ) {
		// Credit the main item and weapon. (But not the shield.)
		mainItem->historyDB.Increment( "Kills" );
		if ( isGreater )
			mainItem->historyDB.Increment( "Greater" );

		for (int i = 0; i < NUM; ++i) {
			if (weapon[i]) {
				weapon[i]->historyDB.Increment("Kills");
				if (isGreater)
					weapon[i]->historyDB.Increment("Greater");
			}
		}
		if ( loser ) {
			CStr< 64 > str;
			str.Format( "Kills: %s", loser->Name() );

			mainItem->historyDB.Increment( str.c_str() );
			for (int i = 0; i < NUM; ++i) {
				if (weapon[i]) {
					weapon[i]->historyDB.Increment(str.c_str());
				}
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
	const ModelResource* res = 0;
	if (mainItem->ResourceName()) {
		res = ModelResourceManager::Instance()->GetModelResource(mainItem->ResourceName(), false);
	}

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
	NewsHistory* history = Context()->chitBag->GetNewsHistory();
	GLASSERT(history);

	Vector2F pos = { 0, 0 };
	if ( parentChit->GetSpatialComponent() ) {
		pos = parentChit->GetSpatialComponent()->GetPosition2D();
	}

	Chit* destroyer = Context()->chitBag->GetChit( lastDamageID );

	int msg = 0;
	if ( item->keyValues.Get( ISC::destroyMsg, &msg ) == 0 ) {
		history->Add( NewsEvent( msg, pos, parentChit, destroyer ));
	}
}


void ItemComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	const ChitContext* context = Context();
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
				v.Normalize();

				//bool knockback = dd.damage > (mainItem.TotalHP() *0.25f);
				bool knockback = ddorig.damage > (mainItem->TotalHP() *0.25f);

				// Ony apply for phyisics or pathmove
				PhysicsMoveComponent* physics  = GET_SUB_COMPONENT( parentChit, MoveComponent, PhysicsMoveComponent );
				MoveComponent* move = parentChit->GetMoveComponent();

				if ( knockback && (physics || move) ) {
					rc->PlayAnimation( ANIM_IMPACT );

					// Rotation
					float r = -300.0f + (float)chit->random.Rand( 600 );

					if ( !physics ) {
						physics = new PhysicsMoveComponent();
						parentChit->Add( physics );	// move components can be stacked (unlike any other component)
					}
					static const float FORCE = 4.0f;
					physics->Add( v*FORCE, r );
				}
			}
		}

		GLLOG(( "Chit %3d '%s' (origin=%d) ", parentChit->ID(), mainItem->Name(), info->originID ));

		DamageDesc dd = ddorig;
		IShield* iShield = this->GetShield();
		GameItem* shield = iShield ? iShield->GetItem() : 0;

		// Check for a shield. Block or reduce damage.
		if (shield && shield->HasRound()) {
			if (shield->flags & GameItem::EFFECT_FIRE)	dd.effects &= (~GameItem::EFFECT_FIRE);
			if (shield->flags & GameItem::EFFECT_SHOCK)	dd.effects &= (~GameItem::EFFECT_SHOCK);

			float rounds = (float)shield->rounds;
			IMeleeWeaponItem* melee = this->GetMeleeWeapon();
			if (melee) {
				// A shield boost effectively reduces the % of damage.
				// (Which means the melee weapons "powers up" the shield.)
				dd.damage /= BattleMechanics::ComputeShieldBoost(melee);
			}

			if (float(shield->rounds) >= dd.damage) {
				shield->rounds -= LRint(dd.damage);
				if (shield->rounds < 0) shield->rounds = 0;
				dd.damage = 0;
			}
			else {
				dd.damage -= (float)shield->rounds;
				shield->rounds = 0;
			}
			shield->reload.ResetUnready();
			shield->GetTraitsMutable()->AddXP(1);

			RenderComponent* rc = parentChit->GetRenderComponent();
			if (rc) {
				ParticleDef def = context->engine->particleSystem->GetPD("shield");
				Vector3F shieldPos = { 0, 0, 0 };
				rc->GetMetaData(HARDPOINT_SHIELD, &shieldPos);
				context->engine->particleSystem->EmitPD(def, shieldPos, V3F_UP, 0);
			}
		}

		itemArr[0]->AbsorbDamage( dd );

		// report XP back to what hit us.
		int originID = info->originID;
		Chit* origin = Context()->chitBag->GetChit( originID );
		if ( origin && origin->GetItemComponent() ) {
			origin->GetItemComponent()->AddBattleXP( mainItem, false ); 
		}
		lastDamageID = originID;

		parentChit->SetTickNeeded();
	}
	else if ( msg.ID() == ChitMsg::CHIT_HEAL ) {
		parentChit->SetTickNeeded();
		float heal = msg.dataF;
		GLASSERT( heal >= 0 );

		mainItem->hp += heal;
		if ( mainItem->hp > double(mainItem->TotalHP()) ) {
			mainItem->hp = double(mainItem->TotalHP());
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
	else if (msg.ID() == ChitMsg::CHIT_DESTROYED_START)
	{
		// Report back to what killed us:
		Chit* origin = Context()->chitBag->GetChit(lastDamageID);
		if (origin && origin->GetItemComponent()) {
			origin->GetItemComponent()->AddBattleXP(this->GetItem(0), true);
		}

		/* A significant GameItem will automatically set it's history in the ItemHistory
		   database when it changes. (Some changes to the item need to be set in the
		   history as well.) The ItemHistory stores what the item is/was not a series of
		   events.

		   The NewsHistory is the series of events, including the creation and destruction
		   of the item. All the dates get inferredd from that.
		   */
		NewsDestroy(mainItem);
	}
	else if (msg.ID() > ChitMsg::CHIT_DESTROYED_START && msg.ID() <= ChitMsg::CHIT_DESTROYED_END) {
		// Note the _START is processed above.
		// However, it's possible gold still gets added. But at the END
		// the chit is in tear-down and the gold/items are gone.
		// Use the intermediate time to put
		// gold and items on the ground. This is something of a hack.

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
				Context()->chitBag->NewWalletChits( pos, w );
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
				Context()->chitBag->NewItemChit( pos, item, true, true, 0 );
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
}


int ItemComponent::ProcessEffect(int delta)
{
	// FIXME: buggy for 2x2 items. Only using one grid.

	// ItemComponent tick frequency much higher than the
	// water sim in WorldMap. Multiply to reduce the frequency.
	static const float CHANCE_FIRE_NORMAL_IC = CHANCE_FIRE_NORMAL * 0.1f;
	static const float CHANCE_FIRE_HIGH_IC   = CHANCE_FIRE_HIGH * 0.1f;
	static const float CHANCE_FIRE_SPREAD_IC = CHANCE_FIRE_SPREAD * 0.1f;

	const float deltaF = float(delta) * 0.001f;

	SpatialComponent* sc = parentChit->GetSpatialComponent();
	if (!sc) return VERY_LONG_TICK;

	Vector2F pos2 = sc->GetPosition2D();
	Vector2I pos2i = ToWorld2I(pos2);
	GameItem* mainItem = itemArr[0];
	IShield* iShield = this->GetShield();
	const GameItem* shield = iShield ? iShield->GetItem() : 0;

	// Look around at map, weather, etc. and sum up
	// the chances of fire, shock, & water
	static const int NDIR = 5;
	static const Vector2I DIR[NDIR] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };

	float fire = 0;
	float shock = 0;
	float water = 0;
	bool underWater = false;

	if (mainItem->flags & GameItem::DAMAGE_UNDER_WATER) {
		const WorldGrid& wg = Context()->worldMap->GetWorldGrid(pos2i);
		if (wg.IsFluid()) {
			RenderComponent* rc = parentChit->GetRenderComponent();
			if (rc) {
				Rectangle3F aabb = rc->MainModel()->AABB();
				float deep = 0.75f * aabb.max.y + 0.25f * aabb.min.y;

				if (wg.FluidHeight() > deep) {
					underWater = true;

					DamageDesc dd = { Travel(float(EFFECT_DAMAGE_PER_SEC), delta), 0 };
					mainItem->AbsorbDamage(dd);
				}
			}
		}
	}

	for (int i = 0; i < NDIR; ++i) {
		// FIXME: return water when out of bounds??
		const WorldGrid& wg = Context()->worldMap->GetWorldGrid(pos2i + DIR[i]);
		float mult = (i == 0) ? 2.0f : 1.0f;	// how much more likely of effect if we are standing on it?

		if (wg.Magma() || (wg.IsFluid() && wg.FluidType() == WorldGrid::FLUID_LAVA))
			fire += mult;
		if ((i == 0) && wg.IsFluid()) 
			water += 1.0;
		if (wg.PlantOnFire()) 
			fire += mult;
		if (wg.PlantOnShock()) 
			shock += mult;
	}
	if ( Weather::Instance() && Weather::Instance()->IsRaining(pos2.x, pos2.y)) {
		water += 1;
	}

	float chanceFire = 0, chanceShock = 0;

	if (mainItem->flags & GameItem::IMMUNE_FIRE)		chanceFire = 0;
	else if (mainItem->flags & GameItem::FLAMMABLE)		chanceFire = CHANCE_FIRE_HIGH_IC;
	else												chanceFire = CHANCE_FIRE_NORMAL_IC;

	if (mainItem->flags & GameItem::IMMUNE_SHOCK)		chanceShock = 0;
	else if (mainItem->flags & GameItem::SHOCKABLE)		chanceShock = CHANCE_FIRE_HIGH_IC;
	else												chanceShock = CHANCE_FIRE_NORMAL_IC;

	float cF = (fire - water) * chanceFire;
	float cS = (shock * (water + 1.0f)) * chanceShock;

	// Shields offer a lot of environmental protection.
	if (shield) {
		if (shield->HasRound()) {
			cF = 0;
			cS = 0;
		}
		if (shield->flags & GameItem::EFFECT_FIRE)
			cF *= 0.5f;	// even if shield depleted!
		if (shield->flags & GameItem::EFFECT_SHOCK)
			cS *= 0.5f;
	}

	if (cF > 0 || cS > 0) {
		if (parentChit->random.Uniform() < cF)
			mainItem->fireTime = EFFECT_MAX_TIME;
		if (parentChit->random.Uniform() < cS)
			mainItem->shockTime = EFFECT_MAX_TIME;
	}

	if (water) {
		// If water==1, this will just do a Travel(delta, EFFECT_MAX_TIME) which
		// halves the effect time, because the time will also be reduced in
		// GameItem::DoTick();
		mainItem->fireTime -= int(float(delta) * water);
		if (mainItem->fireTime < 0) mainItem->fireTime = 0;
	}


	// NOTE: Don't do damage to self. This is
	// done by the GameItem ticker. Just need to be sure
	// the fireTime and/or shockTime is set.

#if 1
	/* Pushing damage back to the environment is SUPER
	   COOL. Also hard to debug.
	*/

	// Push state back to the environment
	if (mainItem->fireTime || mainItem->shockTime) {
		if (parentChit->random.Uniform() < CHANCE_FIRE_SPREAD_IC) {
			DamageDesc ddSpread = { 0, 0 };
			if (mainItem->fireTime) ddSpread.effects |= GameItem::EFFECT_FIRE;
			if (mainItem->shockTime) ddSpread.effects |= GameItem::EFFECT_SHOCK;

			for (int j = 0; j < NDIR; ++j) {
				Vector2I hit2 = pos2i + DIR[j];
				Vector3I hit = { hit2.x, 0, hit2.y };
				Context()->worldMap->VoxelHit(hit, ddSpread);
			}
		}
	}
#endif
	return (fire > 0 || shock > 0 || underWater ) ? 0 : VERY_LONG_TICK;
}


void ItemComponent::DoSlowTick()
{
	GameItem* mainItem = itemArr[0];

	if ( mainItem->flags & GameItem::GOLD_PICKUP ) {
		ComponentSet thisComp( parentChit, Chit::SPATIAL_BIT | ComponentSet::IS_ALIVE );
		if ( thisComp.okay ) {
			CChitArray arr;

			Vector2F pos = parentChit->GetSpatialComponent()->GetPosition2D();
			GoldCrystalFilter goldCrystalFilter;
			Context()->chitBag->QuerySpatialHash( &arr, pos, PICKUP_RANGE, 0, &goldCrystalFilter );
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
	const ChitContext* context = Context();

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
	tick = Min(tick, ProcessEffect(delta));

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


void ItemComponent::OnAdd( Chit* chit, bool init )
{
	GameItem* mainItem = itemArr[0];
	GLASSERT( itemArr.Size() >= 1 );	// the one true item
	super::OnAdd( chit, init );
	hardpointsModified = true;

	if ( Context()->chitBag ) {
		IString mob = mainItem->keyValues.GetIString( "mob" );
		if ( mob == IStringConst::lesser ) {
			Context()->chitBag->census.normalMOBs += 1;
		}
		else if ( mob == IStringConst::greater ) {
			Context()->chitBag->census.greaterMOBs += 1;
		}
	}
	slowTick.SetPeriod( 500 + (chit->ID() & 128));
	UpdateActive();
}


void ItemComponent::OnRemove() 
{
	GameItem* mainItem = itemArr[0];
	if ( Context()->chitBag ) {
		IString mob;
		mainItem->keyValues.Get( ISC::mob, &mob );
		if ( mob == IStringConst::lesser ) {
			Context()->chitBag->census.normalMOBs -= 1;
		}
		else if ( mob == IStringConst::greater ) {
			Context()->chitBag->census.greaterMOBs -= 1;
		}
	}
	super::OnRemove();
}


bool ItemComponent::EmitEffect( const GameItem& it, U32 delta )
{
	const ChitContext* context = Context();
	ParticleSystem* ps = context->engine->particleSystem;
	bool emitted = false;

	ComponentSet compSet( parentChit, Chit::ITEM_BIT | Chit::SPATIAL_BIT | ComponentSet::IS_ALIVE );

	if ( compSet.okay ) {
		if ( it.fireTime > 0 ) {
			ps->EmitPD( "fire", compSet.spatial->GetPosition(), V3F_UP, delta );
			ps->EmitPD( "smoke", compSet.spatial->GetPosition(), V3F_UP, delta );
			emitted = true;
		}
		if ( it.shockTime > 0 ) {
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
			Context()->chitBag->NewItemChit(parentChit->GetSpatialComponent()->GetPosition(),
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
	int  team   = itemArr[0]->team;

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
