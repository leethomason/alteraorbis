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
#include "chitcontext.h"

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

#include "../audio/xenoaudio.h"

using namespace grinliz;

ItemComponent::ItemComponent( GameItem* item ) :
	hardpointsModified(true), 
	slowTick(500),
	lastDamageID(0), 
	debugEnabled(false), 
	hardpointsComputed(false)
{
	for (int i = 0; i < EL_NUM_METADATA; ++i) {
		hasHardpoint[i] = false;
		hardpoint.Push(0);
	}

	// item can be null if loading.
	if ( item ) {
		GLASSERT( !item->IName().empty() );
		itemArr.Push( item );	
	}
	for (int i = 0; i < EL_NUM_METADATA; ++i) {
		hasHardpoint[i] = false;
	}
}


ItemComponent::~ItemComponent()
{
	for( int i=0; i<itemArr.Size(); ++i ) {
		delete itemArr[i];
	}
}


void ItemComponent::InitFrom(const GameItem* items[], int nItems)
{
	GLASSERT(itemArr.Size() == 0 || nItems == 0);
	for (int i = 0; i < nItems;  ++i) {
		itemArr.Push(items[i]->Clone());
	}
}


void ItemComponent::DebugStr( grinliz::GLString* str )
{
	const GameItem* item = itemArr[0];
	int group = 0, id = 0;
	Team::SplitID(item->Team(), &group, &id);
	str->AppendFormat( "[Item] %s hp=%.1f/%d tm=%d,%d ", 
		item->Name(), item->hp, item->TotalHP(),
		group, id );
}


void ItemComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, "ItemComponent" );
	int nItems = itemArr.Size();
	XARC_SER( xs, nItems );

	if ( xs->Loading() ) {
		for ( int i=0; i<nItems; ++i  ) {
			GameItem* pItem = GameItem::Factory(xs->Loading()->PeekElement());
			pItem->Serialize(xs);
			itemArr.Push( pItem );
			// Needs to be fully loaded before we can track.
			pItem->Track();
		}
	}
	else {
		for (int i = 0; i < nItems; ++i) {
			itemArr[i]->Serialize(xs);
			GLASSERT(!itemArr[i]->IName().empty());
		}
	}

	this->EndSerialize( xs );
	UseBestItems();
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
				item->SetProperName(context->chitBag->NameGen(nameGen.c_str(), item->ID()));
			}

			/*
			This happens so rarely it isn't worth the bugs.
			// An item that wasn't significant is now significant.
			// In practice, this means adding tracking for lesser mobs.
			if ( item->keyValues.GetIString( "mob" ) == "lesser" ) {
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


int ItemComponent::PowerRating(bool current) const
{
	// damage we can deal * damage we can take
	int power = current ? int(itemArr[0]->hp) : itemArr[0]->TotalHP();
	const MeleeWeapon* melee = this->QuerySelectMelee();
	const RangedWeapon* ranged = this->QuerySelectRanged();
	if (melee)
		power *= melee->GetValue();
	if (ranged)
		power *= ranged->GetValue() * 2;

	// nominal: 50 * 10 * 20 * 2 = 10,000
	// reduce nominal to 100:
	power /= 100;

	if (power < 1)
		power = 1;
	return power;
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
	if (loser->keyValues.GetIString(ISC::mob) == IString()) {
		return;
	}

	GameItem* mainItem = itemArr[0];
	int level = mainItem->Traits().Level();
	int killshotLevel = killshot ? 0 : loser->Traits().Level();
	mainItem->GetTraitsMutable()->AddBattleXP( killshotLevel );

	bool isGreater = (loser->keyValues.GetIString( ISC::mob ) == ISC::greater);

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
	weapon[0] = this->GetRangedWeapon(0) ? this->GetRangedWeapon(0) : 0;
	weapon[1] = this->GetMeleeWeapon() ? this->GetMeleeWeapon() : 0;
	weapon[2] = this->GetShield() ? this->GetShield() : 0;

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
	Validate();
	return hardpoint.Find(const_cast<GameItem*>(item)) >= 0;
}


void ItemComponent::NewsDestroy( const GameItem* item )
{
	NewsHistory* history = Context()->chitBag->GetNewsHistory();
	GLASSERT(history);

	Vector2F pos = ToWorld2F(parentChit->Position());
	Chit* destroyer = Context()->chitBag->GetChit( lastDamageID );

	int msg = 0;
	if ( item->keyValues.Get( ISC::destroyMsg, &msg ) == 0 ) {
		history->Add( NewsEvent( msg, pos, parentChit->GetItemID(), destroyer ? destroyer->GetItemID() : 0, parentChit->Team() ));
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
//				float len = v.Length();
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

//		GLLOG(( "Chit %3d '%s' (origin=%d) ", parentChit->ID(), mainItem->Name(), info->originID ));
//		if (parentChit->PlayerControlled()) {
//			int debug = 1;
//		}

		DamageDesc dd = ddorig;
		Shield* shield = this->GetShield();

		// Check for a shield. Block or reduce damage.
		if (shield) {
			bool active = shield->Active();
			float boost = 1.0f;
			if (this->GetMeleeWeapon()) {
				boost = this->GetMeleeWeapon()->ShieldBoost();
			}
			shield->AbsorbDamage(&dd, boost);
			shield->GetTraitsMutable()->AddXP(1);

			RenderComponent* rc = parentChit->GetRenderComponent();
			if (active && rc) {
				ParticleDef def = context->engine->particleSystem->GetPD(ISC::shield);
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

		Vector3F v = parentChit->Position();
		context->engine->particleSystem->EmitPD( ISC::heal, v, V3F_UP, 30 );
	}
	else if ( msg.ID() == ChitMsg::CHIT_TRACKING_ARRIVED ) {
		Chit* gold = (Chit*)msg.Ptr();
		GoldCrystalFilter filter;
		if (    filter.Accept( gold )		// it really is gold/crystal
			 && gold->GetItem()	)			// it has an item component
		{
			mainItem->wallet.Deposit(gold->GetWallet(), gold->GetWallet()->Gold(), gold->GetWallet()->Crystal());
			// Need to delete the gold, else it will track to us again!
			gold->DeRez();

			if ( parentChit->GetRenderComponent() ) {
				parentChit->GetRenderComponent()->AddDeco( "loot", STD_DECO );
			}
		}
	}
	else if (msg.ID() == ChitMsg::CHIT_DESTROYED)
	{
		// Exploding!
		if (mainItem->flags & GameItem::EXPLODES) {
			int effect = 0;
			if (mainItem->fireTime) effect |= GameItem::EFFECT_FIRE;
			if (mainItem->shockTime) effect |= GameItem::EFFECT_SHOCK;

			Vector3F pos = parentChit->Position();

			DamageDesc dd( MASS_TO_EXPLOSION_DAMAGE * float(mainItem->mass), effect );
			BattleMechanics::GenerateExplosion(dd, pos, parentChit->ID(), Context()->engine, Context()->chitBag, Context()->worldMap);
		}

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
		   of the item. All the dates get inferred from that.
		*/
		NewsDestroy(mainItem);

		// Drop our wallet on the ground or send to the Reserve?
		// MOBs drop items, as does anything with sub-items
		// or carrying crystal.
		MOBIshFilter mobFilter;

		Vector3F pos = parentChit->Position();

		while( itemArr.Size() > 1 ) {
			const GameItem* remove = itemArr[itemArr.Size() - 1];
			if (remove->Intrinsic())
				break;
			GameItem* item = this->RemoveFromInventory(remove);
			GLASSERT( !item->IName().empty() );
			Context()->chitBag->NewItemChit( pos, item, true, true, 0 );
		}

		// Mobs drop gold and crystal. (Should cores as well?)
		// Everything drops crystal.
		while (parentChit->GetWallet()->NumCrystals()) {
			Context()->chitBag->NewCrystalChit(pos, parentChit->GetWallet(), true);
		}

		if ( mobFilter.Accept( parentChit )) {
			if (!parentChit->GetWallet()->IsEmpty()) {
				Context()->chitBag->NewWalletChits(pos, parentChit->GetWallet());
			}
		}
		if (ReserveBank::Instance()) {	// null in battle mode
			ReserveBank::Instance()->wallet.DepositAll(parentChit->GetWallet());
		}
		GLASSERT(parentChit->GetWallet()->IsEmpty());
		parentChit->GetWallet()->SetClosed();
	}
	else {
		super::OnChitMsg( chit, msg );
	}
	Validate();
}


void ItemComponent::OnChitEvent( const ChitEvent& event )
{
}

// Note that changes here should be echoed
// in the WorldMap::ProcessEffect()
int ItemComponent::ProcessEffect(int delta)
{
	// FIXME: buggy for 2x2 items. Only using one grid.

	// ItemComponent tick frequency much higher than the
	// water sim in WorldMap. Multiply to reduce the frequency.
	static const float CHANCE_FIRE_NORMAL_IC = CHANCE_FIRE_NORMAL * 0.1f;
	static const float CHANCE_FIRE_HIGH_IC   = CHANCE_FIRE_HIGH * 0.1f;
	static const float CHANCE_FIRE_SPREAD_IC = CHANCE_FIRE_SPREAD * 0.1f;

//	const float deltaF = float(delta) * 0.001f;

	Vector2F pos2 = ToWorld2F(parentChit->Position());
	Vector2I pos2i = ToWorld2I(pos2);
	if (pos2i.IsZero()) return VERY_LONG_TICK;	// if we are at origin, not really for processing.

	GameItem* mainItem = itemArr[0];

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

	Rectangle2I bounds = Context()->worldMap->Bounds();
	for (int i = 0; i < NDIR; ++i) {
		Vector2I v = pos2i + DIR[i];
		if (!bounds.Contains(v)) {
			continue;
		}
		const WorldGrid& wg = Context()->worldMap->GetWorldGrid(pos2i + DIR[i]);
		float mult = (i == 0) ? 2.0f : 1.0f;	// how much more likely of effect if we are standing on it?

		if (i == 0 && wg.IsGrid())
			break;	// grid travel; don't pick up effects

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
	if (cF > 0 || cS > 0) {
		Shield* shield = this->GetShield();
		if (shield) {
			if (shield->Active()) {
				cF = 0;
				cS = 0;
			}
			if (shield->flags & GameItem::EFFECT_FIRE)
				cF *= 0.5f;	// even if shield depleted!
			if (shield->flags & GameItem::EFFECT_SHOCK)
				cS *= 0.5f;
		}
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

	if (mainItem->flags & GameItem::GOLD_PICKUP) {
		CChitArray arr;

		Vector2F pos = ToWorld2F(parentChit->Position());
		GoldCrystalFilter goldCrystalFilter;
		Context()->chitBag->QuerySpatialHash(&arr, pos, PICKUP_RANGE, 0, &goldCrystalFilter);
		for (int i = 0; i < arr.Size(); ++i) {
			Chit* gold = arr[i];
			GLASSERT(parentChit != gold);
			TrackingMoveComponent* tc = GET_SUB_COMPONENT(gold, MoveComponent, TrackingMoveComponent);
			if (!tc) {
				tc = new TrackingMoveComponent();
				tc->SetTarget(parentChit->ID());
				gold->Add(tc);
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
//	const ChitContext* context = Context();

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
		if (( i==0 || ItemActive(itemArr[i]) ) && EmitEffect( *mainItem, delta )) 
		{
			tick = 0;
		}
	}

	if (mainItem->keyValues.GetIString(ISC::mob) == ISC::denizen) {
		if ( parentChit->GetRenderComponent() ) {
			const GameItem* fruit = this->FindItem( ISC::fruit );
			if ( fruit ) {
				parentChit->GetRenderComponent()->AddDeco( "fruit", STD_DECO/2 );
			}
			const GameItem* elixir = this->FindItem(ISC::elixir);
			if (elixir) {
				parentChit->GetRenderComponent()->AddDeco("elixir", STD_DECO / 2);
			}
			// don't remove - can't see fruit being eaten!
			//else {
			//	parentChit->GetRenderComponent()->RemoveDeco( "fruit" );
			//}
		}
	}	

	return tick;
}


void ItemComponent::InformCensus(bool add)
{
	GameItem* mainItem = itemArr[0];
	if (Context()->chitBag) {
		IString mob = mainItem->keyValues.GetIString(ISC::mob);
		IString core = mainItem->IName();

		if (mainItem->IName() == ISC::fruit && mainItem->IProperName() == ISC::wildFruit) {
			if (add)
				Context()->chitBag->census.wildFruit += 1;
			else
				Context()->chitBag->census.wildFruit -= 1;
			GLASSERT(Context()->chitBag->census.wildFruit >= 0);
		}
		else if (!mob.empty()) {
			if (add)
				Context()->chitBag->census.AddMOB(mainItem->IName());
			else
				Context()->chitBag->census.RemoveMOB(mainItem->IName());
		}
		else if (core == "core") {
			int team = parentChit->Team();
			if (team) {
				IString team = Team::Instance()->TeamName(Team::Group(parentChit->Team()));
				if (add)
					Context()->chitBag->census.AddCore(team);
				else
					Context()->chitBag->census.RemoveCore(team);
			}
		}
	}
}


void ItemComponent::OnAdd( Chit* chit, bool init )
{
//	GameItem* mainItem = itemArr[0];
	GLASSERT( itemArr.Size() >= 1 );	// the one true item
	super::OnAdd( chit, init );
	hardpointsModified = true;
	InformCensus(true);

	slowTick.SetPeriod( 500 + (chit->ID() & 128));
	UseBestItems();
}


void ItemComponent::OnRemove() 
{
//	GameItem* mainItem = itemArr[0];
	InformCensus(false);
	super::OnRemove();
}


bool ItemComponent::EmitEffect(const GameItem& it, U32 delta)
{
	const ChitContext* context = Context();
	ParticleSystem* ps = context->engine->particleSystem;
	bool emitted = false;

	if (it.fireTime > 0) {
		ps->EmitPD(ISC::fire, parentChit->Position(), V3F_UP, delta);
		ps->EmitPD(ISC::smoke, parentChit->Position(), V3F_UP, delta);
		emitted = true;
	}
	if (it.shockTime > 0) {
		ps->EmitPD(ISC::shock, parentChit->Position(), V3F_UP, delta);
		emitted = true;
	}
	return emitted;
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


int ItemComponent::NumCarriedItems(const IString& item) const
{
	int count = 0;
	for( int i=1; i<itemArr.Size(); ++i ) {
		if ( !itemArr[i]->Intrinsic() && itemArr[i]->IName() == item )
			++count;
	}
	return count;
}


const GameItem* ItemComponent::ItemToSell() const
{
	CArray<const GameItem*, 3> keepers;
	keepers.Push(QuerySelectWeapon(SELECT_MELEE));
	keepers.Push(QuerySelectWeapon(SELECT_RANGED));
	keepers.Push(GetShield());

	// returns 0 or the cheapest item that can be sold
	int val = INT_MAX;
	const GameItem* best = 0;

	for (int i = 1; i < itemArr.Size(); ++i) {
		GameItem* item = itemArr[i];
		if (!item->Intrinsic() && (keepers.Find(item) < 0) && item->GetValue()) {
			if (item->GetValue() < val) {
				best = item;
				val = item->GetValue();
			}
		}
	}
	return best;
}


bool ItemComponent::CanAddToInventory()
{
	return itemArr.Size() < INVERTORY_SLOTS;
}


void ItemComponent::AddToInventory( GameItem* item )
{
	GLASSERT( this->CanAddToInventory() );
	itemArr.Push( item );

	// AIs will use the "best" item.
	UseBestItems();
}


int ItemComponent::TransferInventory( ItemComponent* ic, bool addWeapons, grinliz::IString filterItems )
{
	GLASSERT( ic );
	int nTransfer = 0;
	for( int i=1; i<ic->NumItems(); ++i ) {
		GameItem* item = ic->itemArr[i];
		if ( item->Intrinsic() ) continue;
		if ( !this->CanAddToInventory() ) break;

		if (    (addWeapons && ( item->ToMeleeWeapon() || item->ToRangedWeapon() || item->ToShield() ))
			 || filterItems == item->IName() )
		{
			ic->RemoveFromInventory( item );
			--i;
			this->AddToInventory( item );
			++nTransfer;
		}
	}
	return nTransfer;
}


void ItemComponent::AddToInventory( ItemComponent* ic )
{
	GLASSERT( ic );
	GLASSERT( ic->NumItems() == 1 );
	GLASSERT( this->CanAddToInventory() );
	GameItem* gameItem = ic->itemArr.Pop();
	AddToInventory(gameItem);
	delete ic;

	if ( parentChit && parentChit->GetRenderComponent() ) {
		if ( gameItem->IName() == ISC::fruit ) {
			parentChit->GetRenderComponent()->AddDeco( "fruit", STD_DECO );	// will get refreshed if carred.
		}
		else if (gameItem->IName() == ISC::elixir) {
			parentChit->GetRenderComponent()->AddDeco("elixir", STD_DECO);	// will get refreshed if carred.
		}
		else {
			parentChit->GetRenderComponent()->AddDeco( "loot", STD_DECO );
		}
	}
}


GameItem* ItemComponent::RemoveFromInventory( const GameItem* citem )
{
	Validate();
	int index = itemArr.Find(const_cast<GameItem*>(citem));
	GLASSERT(index > 0);
	if (index <= 0) return 0;

	GameItem* item = itemArr[index];
	GLASSERT(item == citem);
	GLASSERT(!item->Intrinsic());

	if ( item->Intrinsic() ) 
		return 0;

	bool needSort = false;
	int hpIndex = hardpoint.Find(item);
	if (hpIndex >= 0) {
		hardpoint[hpIndex] = 0;
		needSort = true;
	}

	itemArr.Remove( index );
	if (needSort) {
		UseBestItems();
	}
	Validate();
	return item;
}


void ItemComponent::Drop(const GameItem* citem)
{
	GameItem* item = RemoveFromInventory(citem);
	if (item) {
		Context()->chitBag->NewItemChit(parentChit->Position(), item, true, true, 0);
		return;
	}
	GLASSERT(0);	// should have found the item.
}


bool ItemComponent::Swap( int i, int j ) 
{
	if (    i > 0 && j > 0 && i < itemArr.Size() && j < itemArr.Size()
		 && !itemArr[i]->Intrinsic()
		 && !itemArr[j]->Intrinsic() )
	{
		grinliz::Swap( &itemArr[i], &itemArr[j] );
		UseBestItems();
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
		a->UseBestItems();
		b->UseBestItems();
		return true;
	}
	return false;
}


class ItemValueCompare
{
public:
	static bool Less( const GameItem* v0, const GameItem* v1 )
	{
		int val0 = v0->GetValue();
		int val1 = v1->GetValue();
		// sort descending: (FIXME: should really add a flag for this, in the Sort)
		return val0 > val1;
	}
};


void ItemComponent::SortInventory()
{
	for (int i = 1; i<itemArr.Size(); ++i) {
		if (itemArr[i]->Intrinsic())
			continue;
		if (itemArr.Size() > i + 1) {
			grinliz::Sort((const GameItem**)&itemArr[i], itemArr.Size() - i, ItemValueCompare::Less);
		}
		break;
	}
}


const RangedWeapon* ItemComponent::QuerySelectRanged() const
{
	const GameItem* item = QuerySelectWeapon(SELECT_RANGED);
	if (item) {
		GLASSERT(item->ToRangedWeapon());
		return item->ToRangedWeapon();
	}
	return 0;
}


const MeleeWeapon* ItemComponent::QuerySelectMelee() const
{
	const GameItem* item = QuerySelectWeapon(SELECT_MELEE);
	if (item) {
		GLASSERT(item->ToMeleeWeapon());
		return item->ToMeleeWeapon();
	}
	return 0;
}


bool ItemComponent::IsBetterItem(const GameItem* item) const
{
	if (item->ToMeleeWeapon()) {
		const MeleeWeapon* melee = QuerySelectMelee();
		return !melee || (item->GetValue() > melee->GetValue());
	}
	if (item->ToRangedWeapon()) {
		const RangedWeapon* ranged = QuerySelectRanged();
		return !ranged || (item->GetValue() > ranged->GetValue());
	}
	if (item->ToShield()) {
		const Shield* shield = GetShield();
		return !shield || (item->GetValue() > shield->GetValue());
	}
	return false;
}


const GameItem* ItemComponent::QuerySelectWeapon(int type) const
{
	if (type == SELECT_MELEE && this->GetMeleeWeapon()) return this->GetMeleeWeapon();
	if (type == SELECT_RANGED && this->GetRangedWeapon(0)) return this->GetRangedWeapon(0);

	bool usesWeapons = (GetItem()->flags & GameItem::AI_USES_BUILDINGS) != 0;
	const GameItem* result = 0;

	if (type == SELECT_MELEE) {
		for (int i = 1; i < itemArr.Size(); ++i) {
			GameItem* item = itemArr[i];
			MeleeWeapon* melee = item->ToMeleeWeapon();
			if (melee && (usesWeapons || melee->Intrinsic())) {
				if (hasHardpoint[melee->hardpoint]) {
					result = melee;
					if (!usesWeapons || !melee->Intrinsic()) {
						break;
					}
				}
			}
		}
	}
	else if (type == SELECT_RANGED) {
		for (int i = 1; i < itemArr.Size(); ++i) {
			RangedWeapon* ranged = itemArr[i]->ToRangedWeapon();
			if (ranged && (usesWeapons || ranged->Intrinsic())) {
				if (hasHardpoint[ranged->hardpoint]) {
					result = ranged;
					if (!usesWeapons || !ranged->Intrinsic()) {
						break;
					}
				}
			}
		}
	}
	return result;
}


const GameItem* ItemComponent::SelectWeapon(int type)
{
	GameItem* weapon = const_cast<GameItem*>(QuerySelectWeapon(type));
	if (weapon && hardpoint[weapon->hardpoint] != weapon) {
		if (debugEnabled && parentChit) {
			GLOUTPUT(("Weapon select. chitID=%d weapon=%s\n", parentChit->ID(), weapon ? weapon->Name() : "[none]"));
		}
		hardpoint[weapon->hardpoint] = weapon;
		hardpointsModified = true;
	}
	return weapon;
}


void ItemComponent::ComputeHardpoints()
{
	if (!hardpointsComputed) {
		const ModelResource* modelRes = ModelResourceManager::Instance()->GetModelResource(GetItem()->ResourceName(), false);
		if (modelRes) {
			for (int i = 1; i < EL_NUM_METADATA; ++i) {
				if (modelRes->header.metaData[i].InUse()) {
					hasHardpoint[i] = true;
				}
			}
		}
		hardpointsComputed = true;
	}
}


void ItemComponent::UseBestItems()
{
	ComputeHardpoints();

	bool player = !parentChit || (parentChit == Context()->chitBag->GetAvatar());
	bool usesWeapons = (GetItem()->flags & GameItem::AI_USES_BUILDINGS) != 0;

	if (!player) {
		SortInventory();
	}
	for (int i = 0; i < EL_NUM_METADATA; ++i) {
		hardpoint[i] = 0;
	}

	// First pass: assign the intrinsics.
	for (int i = 1; i < itemArr.Size(); ++i) {
		GameItem* item = itemArr[i];
		if (!item->Intrinsic())
			break;
		if (item->hardpoint) {
			GLASSERT(hasHardpoint[item->hardpoint]);
			GLASSERT(hardpoint[item->hardpoint] == 0);
			hardpoint[item->hardpoint] = item;
		}
	}

	// Now assigned the carried weapons.
	if (usesWeapons) {
		for (int i = 1; i < itemArr.Size(); ++i) {
			GameItem* item = itemArr[i];

			if (item->Intrinsic()) continue;	// dealt with this already.

			int h = item->hardpoint;
			if (h && hasHardpoint[h] && (!hardpoint[h] || hardpoint[h]->Intrinsic())) {
				hardpoint[h] = item;
			}
		}
	}
	Validate();
	hardpointsModified = true;
}


RangedWeapon* ItemComponent::GetRangedWeapon( grinliz::Vector3F* trigger ) const
{
	Validate();
	for (int i = 1; i < itemArr.Size(); ++i) {
		GameItem* item = itemArr[i];
		if (item->ToRangedWeapon() && hardpoint.Find(item) >= 0) {
			RangedWeapon* ranged = item->ToRangedWeapon();
			if (trigger) {
				RenderComponent* rc = parentChit->GetRenderComponent();
				GLASSERT( rc );
				if ( rc ) {
					Matrix4 xform;
					rc->GetMetaData( ranged->hardpoint, &xform ); 
					Vector3F pos = xform * V3F_ZERO;
					*trigger = pos;
				}
			}
			return item->ToRangedWeapon();
		}
	}
	return 0;
}


MeleeWeapon* ItemComponent::GetMeleeWeapon() const
{
	Validate();
	for (int i = 1; i < itemArr.Size(); ++i) {
		GameItem* item = itemArr[i];
		if (item->ToMeleeWeapon() && hardpoint.Find(item) >= 0) {
			return item->ToMeleeWeapon();
		}
	}
	return 0;
}


Shield* ItemComponent::GetShield() const
{
	Validate();
	for (int i = 1; i < itemArr.Size(); ++i) {
		GameItem* item = itemArr[i];
		if (item->ToShield() && hardpoint.Find(item) >= 0) {
			return item->ToShield();
		}
	}
	return 0;
}


void ItemComponent::SetHardpoints()
{
	if ( !parentChit->GetRenderComponent() ) {
		return;
	}
	Validate();
	RenderComponent* rc = parentChit->GetRenderComponent();

	GameItem* mainItem = itemArr[0];
	if (mainItem->keyValues.Has(ISC::procedural)) {
		ProcRenderInfo info;
		AssignProcedural(mainItem, &info);
		rc->SetProcedural(0, info);
	}

	for (int i = 1; i < hardpoint.Size(); ++i) {
		GameItem* item = hardpoint[i];
		
		if (item) {
			GLASSERT(item->hardpoint == i);
			if (item->Intrinsic()) {
				rc->Attach(item->hardpoint, 0);
			}
			else {
				GLASSERT(!item->IResourceName().empty());
				rc->Attach(item->hardpoint, item->ResourceName());

				if (item->keyValues.Has(ISC::procedural)) {
					ProcRenderInfo info;
					AssignProcedural(item, &info);
					rc->SetProcedural((i == 0) ? 0 : item->hardpoint, info);
				}
			}
		}
	}
}


void ItemComponent::Validate() const
{
#ifdef DEBUG
	bool usesWeapons = (GetItem()->flags & GameItem::AI_USES_BUILDINGS) != 0;

	for (int i = 0; i < hardpoint.Size(); ++i) {
		const GameItem* item = hardpoint[i];
		if (item) {
			GLASSERT(item->Intrinsic() || usesWeapons);
			GLASSERT(this->FindItem(item));
		}
	}

	GLASSERT(hardpoint[0] == 0);

#endif
}


void ItemComponent::ApplyLootLimits()
{
	// Some MOBs gather a lot of loot. Cheat a little, and send
	// the massive loot tax off the reserve.
	GameItem* mainItem = itemArr[0];
	IString mob = mainItem->keyValues.GetIString(ISC::mob);
	
	// Lesser.
	int limit = 0;
	int cLimit = 0;

	if (mob == ISC::greater) {
		limit = MAX_GREATER_GOLD;
		cLimit = MAX_GREATER_MOB_CRYSTAL;
	}
	else if (mob == ISC::lesser) {
		limit = MAX_LESSER_GOLD;
		cLimit = MAX_LESSER_MOB_CRYSTAL;
	}

	if (limit) {
		ReserveBank* bank = ReserveBank::Instance();
		int d = mainItem->wallet.Gold() - limit;
		if (d > 0) {
			bank->wallet.Deposit(&mainItem->wallet, d);
		}
		for (int i = 0; i<NUM_CRYSTAL_TYPES; ++i) {
			d = mainItem->wallet.Crystal(i) - cLimit;
			if (d > 0) {
				int crystal[NUM_CRYSTAL_TYPES] = { 0 };
				crystal[i] = d;
				bank->wallet.Deposit(&mainItem->wallet, 0, crystal);
			}
		}
	}
}
