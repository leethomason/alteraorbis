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

#include "gameitem.h"
#include "news.h"
#include "team.h"

#include "../grinliz/glstringutil.h"
#include "../engine/serialize.h"
#include "../engine/model.h"

#include "../tinyxml2/tinyxml2.h"
#include "../xarchive/glstreamer.h"

#include "../xegame/chit.h"
#include "../xegame/istringconst.h"
#include "../xegame/rendercomponent.h"

#include "../script/battlemechanics.h"
#include "../script/itemscript.h"

using namespace grinliz;
using namespace tinyxml2;

#define READ_FLAG( flags, str, name )	{ if ( strstr( str, #name )) flags |= name; }

#define READ_ATTR( n )			else if ( name == #n ) { ele->QueryAttribute(#n, &n); }

#define APPEND_FLAG( flags, cstr, name )	{ if ( flags & name ) { f += #name; f += " "; } }
#define PUSH_ATTRIBUTE( prnt, name )		{ prnt->PushAttribute( #name, name ); }


void GameTrait::Serialize( XStream* xs )
{
	XarcOpen( xs, "traits" );
	if (xs->Loading() || (xs->Saving() && HasTraits())) {
		XARC_SER_ARR(xs, trait, NUM_TRAITS);
		XARC_SER(xs, exp);
	}
	XarcClose( xs );
}


void GameTrait::Roll( U32 seed )
{
	Random random( seed );
	random.Rand();
	for( int i=0; i<NUM_TRAITS; ++i ) {
		trait[i] = random.Dice( 3, 6 );
	}
}



void Cooldown::Serialize( XStream* xs, const char* name )
{
	XarcOpen( xs, name );
	XARC_SER_DEF( xs, threshold, 1000 );
	XARC_SER_DEF( xs, time, 1000 );
	XarcClose( xs );
}

int GameItem::idPool = 0;
bool GameItem::trackWallet = true;


GameItem* GameItem::Factory(const char* subclass)
{
	if (StrEqual(subclass, "GameItem")) {
		return new GameItem();
	}
	else if (StrEqual(subclass, "RangedWeapon")) {
		return new RangedWeapon();
	}
	else if (StrEqual(subclass, "MeleeWeapon")) {
		return new MeleeWeapon();
	}
	else if (StrEqual(subclass, "Shield")) {
		return new Shield();
	}
	GLASSERT(0);
	return 0;
}


GameItem::GameItem()
{
	id = 0;
	flags = 0;
	hardpoint  = 0;
	mass = 1;
	baseHP = 1;
	hpRegen = 0;
	team = 0;
	traits.Init();
	personality.Init();
	GLASSERT(wallet.IsEmpty());
	wallet.Set(0, 0);
	keyValues.Clear();
	historyDB.Clear();

	hp = double(TotalHP());
	fireTime = 0;
	shockTime = 0;
	value			= -1;
}


GameItem::~GameItem()
{ 
	if (trackWallet) {
		GLASSERT(wallet.IsEmpty());
	}
	UnTrack(); 
}


GameItem* GameItem::CloneFrom(GameItem* item) const
{
	if (!item) item = new GameItem();
	item->name = name;
	item->properName = properName;
	item->resource = resource;
	item->key = key;
	item->flags = flags;
	item->hardpoint = hardpoint;
	item->mass = mass;
	item->baseHP = baseHP;
	item->hpRegen = hpRegen;
	item->team = team;
	GLASSERT(wallet.IsEmpty());
	item->hp = hp;
	// fireTime, shockTime not copied
	item->keyValues = keyValues;
	// History *not* copied.
	item->traits = traits;
	item->personality = personality;

	if (this->hardpoint) {
		GLASSERT(Intrinsic() || !resource.empty());
	}

	return item;
}


void GameItem::Roll(int newTeam, const int *roll)
{
	GLASSERT(team == 0);
	team = newTeam;
	for( int i=0; i<GameTrait::NUM_TRAITS; ++i ) {
		GetTraitsMutable()->Set( i, roll[i] );
	}
}


void GameItem::Serialize( XStream* xs )
{
	XarcOpen( xs, "GameItem" );

	XARC_SER( xs, name );
	XARC_SER_DEF( xs, properName, IString() );
	XARC_SER( xs, resource );
	XARC_SER(xs, flags);
	XARC_SER_DEF( xs, hardpoint, 0 );
	XARC_SER( xs, mass );
	XARC_SER( xs, baseHP );
	XARC_SER_DEF( xs, hpRegen, 0.0f );
	XARC_SER_DEF( xs, team, 0 );
	XARC_SER( xs, hp );
	XARC_SER_DEF( xs, fireTime, 0 );
	XARC_SER_DEF( xs, shockTime, 0 );
	XARC_SER( xs, id );

	wallet.Serialize( xs );
	keyValues.Serialize( xs, "keyval" );
	historyDB.Serialize( xs, "historyDB" );
	traits.Serialize( xs );
	personality.Serialize( xs );

	XarcClose( xs );
}


int GameItem::ToMessage(const char* m)
{
	static const char* names[] = {
		"NONE",
		"DENIZEN_CREATED",
		"DENIZEN_KILLED",
		"GREATER_MOB_CREATED",
		"GREATER_MOB_KILLED",
		"DOMAIN_CREATED",
		"DOMAIN_DESTROYED",
		"FORGED",
		"UN_FORGED",
		0
	};

	if (!m) return 0;
	for (int i = 0; names[i]; ++i) {
		if (StrEqual(names[i], m))
			return i;
	}
	return 0;
}

	
void GameItem::Load(const tinyxml2::XMLElement* ele)
{
	name = StringPool::Intern(ele->Attribute("name"));
	properName = StringPool::Intern(ele->Attribute("properName"));
	resource = StringPool::Intern(ele->Attribute("resource"));

	GLASSERT(!name.empty());

//	const char* cMsg = ele->Attribute("startTrack");
//	const char* dMsg = ele->Attribute("endTrack");
//	GLASSERT((!cMsg && !dMsg) || (cMsg && dMsg));
//	createMsg = ToMessage(cMsg);
//	destroyMsg = ToMessage(dMsg);

	id = 0;
	flags = 0;
	const char* f = ele->Attribute("flags");

	if (f) {
		READ_FLAG(flags, f, INTRINSIC);
		READ_FLAG(flags, f, IMMUNE_FIRE);
		READ_FLAG(flags, f, FLAMMABLE);
		READ_FLAG(flags, f, IMMUNE_SHOCK);
		READ_FLAG(flags, f, SHOCKABLE);
		READ_FLAG(flags, f, EFFECT_EXPLOSIVE);
		READ_FLAG(flags, f, EFFECT_FIRE);
		READ_FLAG(flags, f, EFFECT_SHOCK);
		READ_FLAG(flags, f, RENDER_TRAIL);
		READ_FLAG(flags, f, INDESTRUCTABLE);
		READ_FLAG(flags, f, AI_WANDER_HERD);
		READ_FLAG(flags, f, AI_WANDER_CIRCLE);
		READ_FLAG(flags, f, AI_USES_BUILDINGS);
		READ_FLAG(flags, f, AI_NOT_TARGET);
		READ_FLAG(flags, f, GOLD_PICKUP);
		READ_FLAG(flags, f, ITEM_PICKUP);
		READ_FLAG(flags, f, HAS_NEEDS);
		READ_FLAG(flags, f, DAMAGE_UNDER_WATER);
		READ_FLAG(flags, f, FLOAT);
		READ_FLAG(flags, f, SUBMARINE);
		READ_FLAG(flags, f, EXPLODES);
		READ_FLAG(flags, f, CLICK_THROUGH);
		READ_FLAG(flags, f, PATH_NON_BLOCKING);
	}
	for (const tinyxml2::XMLAttribute* attr = ele->FirstAttribute();
		 attr;
		 attr = attr->Next())
	{
		IString name = StringPool::Intern(attr->Name());
		if (name == "name" || name == "properName" || name == "desc" || name == "resource" || name == "flags") {
			// handled above.
		}
		else if (name == "hardpoint" || name == "hp") {
			// handled below
		}
		READ_ATTR(mass)
		READ_ATTR(baseHP)
		READ_ATTR(hpRegen)
		READ_ATTR(team)
		READ_ATTR(fireTime)
		READ_ATTR(shockTime)
		else {
			// What is it??? Tricky stuff.
			int integer = 0;
			int real = 0;
			int str = 0;

			for (const char* p = attr->Value(); *p; ++p) {
				if (*p == '-' || isdigit(*p) || *p == '+') {
					integer++;
				}
				else if (*p == '.') {
					real++;
				}
				else if (isupper(*p) || islower(*p)) {
					str++;
				}
			}

			if (str)
				keyValues.Set(name.c_str(), StringPool::Intern(attr->Value()));
			else if (integer && real)
				keyValues.Set(name.c_str(), (float)atof(attr->Value()));
			else if (integer)
				keyValues.Set(name.c_str(), atoi(attr->Value()));
			else
				GLASSERT(0);
		}
	}

	if (flags & EFFECT_FIRE)	flags |= IMMUNE_FIRE;
	GLASSERT((flags & INDESTRUCTABLE) || (baseHP > 1) || (flags & INTRINSIC));

	hardpoint = 0;
	const char* h = ele->Attribute( "hardpoint" );
	if ( h ) {
		hardpoint = MetaDataToID( h );
		GLASSERT( hardpoint > 0 );
		GLASSERT((flags & INTRINSIC) || (!resource.empty()));
	}

	hp = double(this->TotalHP());
	ele->QueryAttribute( "hp", &hp );
	GLASSERT( hp <= TotalHP() );

	GLASSERT( TotalHP() > 0 );
	GLASSERT( hp > 0 );	// else should be destroyed
}


RangedWeapon::RangedWeapon()
{
	rangedDamage = 1;
	clipCap = 6;
	accuracy = 1;
	rounds = clipCap;
	cooldown.ResetReady();
	reload.ResetReady();
}


RangedWeapon::~RangedWeapon()
{}


GameItem* RangedWeapon::Clone() const
{
	RangedWeapon* r = new RangedWeapon();
	CloneFrom(r);
	r->rangedDamage = rangedDamage;
	r->cooldown = cooldown;
	r->reload = reload;
	r->rounds = rounds;
	r->clipCap = clipCap;
	r->accuracy = accuracy;

	GLASSERT(r->rangedDamage > 0);
	GLASSERT(r->rounds > 0);
	GLASSERT(r->clipCap > 0);
	GLASSERT(r->accuracy > 0);

	return r;
}



void RangedWeapon::Serialize(XStream* xs)
{
	XarcOpen( xs, "RangedWeapon" );

	// FIXME: attributes need to be serialized before objects.
	XARC_SER(xs, clipCap);
	XARC_SER(xs, rounds);
	XARC_SER(xs, rangedDamage);
	XARC_SER(xs, accuracy);

	super::Serialize(xs);

	cooldown.Serialize(xs, "cooldown");
	reload.Serialize(xs, "reload");
	XarcClose(xs);
}


void RangedWeapon::Load(const tinyxml2::XMLElement* ele)
{
	super::Load(ele);
	ele->QueryAttribute("clipCap", &clipCap);
	ele->QueryAttribute("rangedDamage", &rangedDamage);
	ele->QueryAttribute("accuracy", &accuracy);

	int c = 0, r = 0;
	ele->QueryAttribute("cooldown", &c);
	ele->QueryAttribute("reload", &r);

	if (c) cooldown.SetThreshold(c);
	if (r) reload.SetThreshold(r);

	rounds = clipCap;
}


void RangedWeapon::Roll(int newTeam, const int* roll)
{
	super::Roll(newTeam, roll);

	// Accuracy and Damage are effected by traits + level.
	// Speed, ClipCap, Reload by traits. (And only at creation.)
	// Accuracy:	DEX, level		
	// Damage:		STR, level, rangedDamage
	//
	// Fixed at creation:
	// Cooldown:	WILL
	// ClipCap:		CHR		
	// Reload:		INT

	float cool = (float)cooldown.Threshold();
	// Fire rate can get a little out of control; tune
	// it down by a factor of 0.5
	cool *= (0.5f + 0.5f*Dice3D6ToMult(Traits().Get(GameTrait::ALT_COOL)));
	// FIXME: should be set by constants from itemdef, as these limits
	// are pulled from that range. Fastest weapon (pulse) at 300 cooldown,
	// so limit to 1/5 second.
	cooldown.SetThreshold( Clamp( (int)cool, 200, 3000 ));

	float fcc = (float)clipCap;
	fcc *= Dice3D6ToMult( Traits().Get( GameTrait::ALT_CAPACITY ));
	clipCap = Clamp( (int)fcc, 1, 100 );

	float fr = (float)reload.Threshold();
	fr /= Dice3D6ToMult( Traits().Get( GameTrait::ALT_RELOAD ));
	reload.SetThreshold( Clamp( (int)fr, 100, 3000 ));

	cooldown.ResetReady();
	rounds = clipCap;
	reload.ResetReady();
}


int RangedWeapon::DoTick(U32 delta)
{
	int tick = super::DoTick(delta);

	cooldown.Tick(delta);
	if (reload.Tick(delta)) {
		rounds = clipCap;
	}

	if (!cooldown.CanUse() || !reload.CanUse()) {
		tick = 0;
	}
	return tick;
}


float RangedWeapon::BoltSpeed() const
{
	static const float SPEED = 10.0f;
	float speed = 1.0f;
	keyValues.Get(ISC::speed, &speed);
	return SPEED * speed;
}


float RangedWeapon::Damage() const
{
	return rangedDamage * Traits().Damage();
}

float RangedWeapon::Accuracy() const
{
	return accuracy * Traits().Accuracy();
}


bool RangedWeapon::CanShoot() const 
{
	return !CoolingDown() && !Reloading() && HasRound();
}


bool RangedWeapon::Shoot(Chit* parentChit) {
	if (CanShoot()) {
		UseRound();
		cooldown.Use();
		if (parentChit) {
			parentChit->SetTickNeeded();
		}
		return true;
	}
	return false;
}


bool RangedWeapon::Reload( Chit* parentChit ) 
{
	if ( CanReload()) {
		reload.Use();
		if ( parentChit ) {
			parentChit->SetTickNeeded();
			RenderComponent* rc = parentChit->GetRenderComponent();
			if (rc) {
				rc->AddDeco("reload", STD_DECO);
			}
		}
		return true;
	}
	return false;
}


void RangedWeapon::UseRound() { 
	if ( clipCap > 0 ) { 
		GLASSERT( rounds > 0 ); 
		--rounds; 
	} 
}


float RangedWeapon::ReloadFraction() const
{
	return float(reload.Fraction());
}


float RangedWeapon::RoundsFraction() const
{
	if (clipCap) {
		return float(rounds) / float(clipCap);
	}
	return 1;
}


MeleeWeapon::MeleeWeapon()
{
	meleeDamage = 10;
}


GameItem* MeleeWeapon::Clone() const
{
	MeleeWeapon* m = new MeleeWeapon();
	CloneFrom(m);
	m->meleeDamage = meleeDamage;
	return m;
}


void MeleeWeapon::Serialize(XStream* xs)
{
	XarcOpen( xs, "MeleeWeapon" );
	XARC_SER(xs, meleeDamage);
	super::Serialize(xs);
	XarcClose(xs);
}


void MeleeWeapon::Load(const tinyxml2::XMLElement* ele)
{
	ele->QueryAttribute("meleeDamage", &meleeDamage);
	super::Load(ele);
}


void MeleeWeapon::Roll(int newTeam, const int* traits)
{
	super::Roll(newTeam, traits);
	// nothing set on Roll.
}


float MeleeWeapon::Damage() const
{
	return meleeDamage * Traits().Damage();
}

float MeleeWeapon::ShieldBoost() const
{
	float value = 1.0f;
	keyValues.Get( ISC::shieldBoost, &value ); 
	float boost = value * Traits().NormalLeveledTrait( GameTrait::CHR );
	return Max(boost, 1.0f);
}


int MeleeWeapon::CalcMeleeCycleTime() const
{
	int meleeTime = 1000;
	if ( !IResourceName().empty() ) {
		const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( ResourceName(), false );
		if ( res ) {
			IString animName = res->header.animation;
			if ( !animName.empty() ) {
				const AnimationResource* animRes = AnimationResourceManager::Instance()->GetResource( animName.c_str() );
				if ( animRes ) {
					meleeTime = animRes->Duration( ANIM_MELEE );
				}
			}
		}
	}
	return meleeTime;
}


int GameItem::DoTick( U32 delta )
{
	int tick = VERY_LONG_TICK;
	if (delta > MAX_FRAME_TIME) delta = MAX_FRAME_TIME;
	//float deltaF = float(delta) * 0.001f;

	double savedHP = hp;
	if ( flags & IMMUNE_FIRE )		fireTime = 0;
	if ( flags & IMMUNE_SHOCK )		shockTime = 0;

	if (fireTime || shockTime) {
		fireTime = Min(fireTime, EFFECT_MAX_TIME);
		shockTime = Min(shockTime, EFFECT_MAX_TIME);

		double effectDamage = Travel(EFFECT_DAMAGE_PER_SEC,  delta);

		if (fireTime) hp -= effectDamage;
		if (shockTime) hp -= effectDamage;

		shockTime -= (flags & SHOCKABLE) ? delta/2 : delta;
		fireTime  -= (flags & FLAMMABLE) ? delta/2 : delta;
		if (fireTime < 0) fireTime = 0;
		if (shockTime < 0) shockTime = 0;
	}
	if ( hp > 0 ) {
		hp += Travel( hpRegen, delta);
	}
	hp = Clamp( hp, 0.0, double(TotalHP()) );

	if ( flags & INDESTRUCTABLE ) {
		hp = double(TotalHP());
	}

	if ( savedHP != hp || fireTime || shockTime ) {
		tick = 0;
	}
	return tick;	
}


void GameItem::Apply( const GameItem* intrinsic )
{
	if ( intrinsic->flags & EFFECT_FIRE )
		flags |= IMMUNE_FIRE;
	if ( intrinsic->flags & EFFECT_SHOCK )
		flags |= IMMUNE_SHOCK;
}


double GameItem::GetBuildingIndustrial() const
{
	IString istr = keyValues.GetIString(ISC::zone);
	if (istr == ISC::industrial)
		return 1;
	if (istr == ISC::natural)
		return -1;
	return 0;
}


void GameItem::AbsorbDamage( const DamageDesc& dd )
{
	// just regular item getting hit, that takes damage.
	if (dd.effects & EFFECT_FIRE) fireTime = EFFECT_MAX_TIME;
	if (dd.effects & EFFECT_SHOCK) shockTime = EFFECT_MAX_TIME;

	hp -= dd.damage;
	if ( hp < 0 ) hp = 0;
}


int GameItem::GetValue() const
{
	if (value >= 0) {
		return value;
	}

	value = 0;

	static const float EFFECT_BONUS = 1.5f;
	static const float MELEE_VALUE  = 20;
	static const float RANGED_VALUE = 30;
	static const float SHIELD_VALUE = 20;

	if (ToRangedWeapon()) {
		float radAt1 = BattleMechanics::ComputeRadAt1(0, ToRangedWeapon(), false, false);
		float dptu = BattleMechanics::RangedDPTU(ToRangedWeapon(), true);
		float er = BattleMechanics::EffectiveRange(radAt1);

		const GameItem& basic = ItemDefDB::Instance()->Get("blaster");
		float refRadAt1 = BattleMechanics::ComputeRadAt1(0, basic.ToRangedWeapon(), false, false);
		float refDPTU = BattleMechanics::RangedDPTU(basic.ToRangedWeapon(), true);
		float refER = BattleMechanics::EffectiveRange(refRadAt1);

		float v = (dptu * er) / (refDPTU * refER) * RANGED_VALUE;
		if (flags & GameItem::EFFECT_FIRE) v *= EFFECT_BONUS;
		if (flags & GameItem::EFFECT_SHOCK) v *= EFFECT_BONUS;
		if (flags & GameItem::EFFECT_EXPLOSIVE) v *= EFFECT_BONUS;
		value = LRintf(v);
	}

	if (ToMeleeWeapon()) {
		float dptu = BattleMechanics::MeleeDPTU(0, ToMeleeWeapon());

		const GameItem& basic = ItemDefDB::Instance()->Get("ring");
		float refDPTU = BattleMechanics::MeleeDPTU(0, basic.ToMeleeWeapon());

		float v = dptu / refDPTU * MELEE_VALUE;
		if (flags & GameItem::EFFECT_FIRE) v *= EFFECT_BONUS;
		if (flags & GameItem::EFFECT_SHOCK) v *= EFFECT_BONUS;
		if (flags & GameItem::EFFECT_EXPLOSIVE) v *= EFFECT_BONUS;

		value = LRintf(v);
	}

	const Shield* shield = ToShield();
	if (shield) {
		const GameItem& basic = ItemDefDB::Instance()->Get("shield");
		const Shield* refShield = basic.ToShield();
		GLASSERT(refShield);
		
		// Reduce importance of shield recharge time
		float vRef = refShield->Capacity() / sqrtf((float)refShield->ShieldRechargeTime());
		float vItem = shield->Capacity()   / sqrtf((float)shield->ShieldRechargeTime());
		float v = vItem * SHIELD_VALUE / vRef;

		if (flags & GameItem::EFFECT_FIRE)  v *= EFFECT_BONUS;
		if (flags & GameItem::EFFECT_SHOCK) v *= EFFECT_BONUS;

		value = LRintf(v);
	}
	return value;
}


int GameItem::ID() const { 
	if ( !id ) {
		id = ++idPool; 
		// id is now !0, so start tracking.
		Track();	
	}
	return id; 
}


void GameItem::Track() const
{
	// id will be !0 if actually in use.
	// and calling ID() will allocate one.
	if ( id == 0 ) return;

	ItemDB* db = ItemDB::Instance();
	if ( db ) {
		db->Add( this );
	}
}


void GameItem::UpdateTrack() const 
{
	// use the 'id', not ID() so we don't allocate:
	if ( id == 0 ) return;

	ItemDB* db = ItemDB::Instance();
	if ( db ) {
		db->Update( this );
	}
}


void GameItem::UnTrack() const
{
	// id will be !0 if actually in use.
	// and calling ID() will allocate one.
	if (id == 0) return;

	ItemDB* db = ItemDB::Instance();
	if ( db ) {
		db->Remove( this );
	}
}


void GameItem::SetProperName( const grinliz::IString& n ) 
{ 
	fullName = IString();	// clear cache
	properName = n; 
	UpdateTrack();
}


bool GameItem::Significant() const
{
	int msg = 0;
	if ( keyValues.Get( ISC::destroyMsg, &msg ) == 0 ) {
		return true;
	}
	if (team && name == ISC::core) {
		return true;
	}

	return false;
}


IString GameItem::IFullName() const
{
	if ( fullName.empty() ) {
		if ( properName.empty() ) {
			fullName = name;
		}
		else {
			GLString n;
			if (Team::IsDeity(team)) {
				n.Format("%s", properName.safe_str());
			}
			else if (IName() == ISC::core && Team::IsDenizen(team)) {
				n.Format("%s (%s)", properName.safe_str(), Team::Instance()->TeamName(team).safe_str());
			}
			else {
				n.Format("%s (%s)", properName.safe_str(), name.safe_str());
			}
			fullName = StringPool::Intern(n.safe_str());
		}
	}
	return fullName;
}


void GameItem::SetSignificant(NewsHistory* history, const Vector2F& pos, int creationMsg, int destructionMsg, const GameItem* creator)
{
	// Mark this item as important with a destroyMsg:
	keyValues.Set(ISC::destroyMsg, destructionMsg);

	if (creator && team == 0) {
		team = creator->Team();
	}
	NewsEvent news(creationMsg, pos, this->ID(), creator ? creator->ID() : 0);
	history->Add(news);
}


float DamageDesc::Score() const
{
	float score = damage;
	if ( effects & GameItem::EFFECT_FIRE ) score *= 1.5f;
	if ( effects & GameItem::EFFECT_SHOCK ) score *= 2.0f;
	if ( effects & GameItem::EFFECT_EXPLOSIVE ) score *= 2.5f;
	return score;
}


void DamageDesc::Log()
{
//	GLLOG(( "[damage=%.1f]", damage ));
}


Shield::Shield()
{
	charge = 0;
	capacity = 10;
	cooldown.SetThreshold(10);
	cooldown.ResetReady();
}


Shield::~Shield()
{}


GameItem* Shield::Clone() const
{
	Shield* s = new Shield();
	CloneFrom(s);
	s->charge = charge;
	s->capacity = capacity;
	s->cooldown = cooldown;

	GLASSERT(s->capacity);
	return s;
}

void Shield::Serialize(XStream* xs)
{
	XarcOpen( xs, "Shield" );

	// FIXME: attributes need to be serialized before objects.
	XARC_SER(xs, charge);
	XARC_SER(xs, capacity);

	super::Serialize(xs);

	cooldown.Serialize(xs, "cooldown");
	XarcClose(xs);
}


void Shield::Load(const tinyxml2::XMLElement* ele)
{
	super::Load(ele);
	ele->QueryAttribute("capacity", &capacity);
	charge = capacity;

	int c = 0;
	ele->QueryAttribute("cooldown", &c);

	if (c) cooldown.SetThreshold(c);
}


int Shield::DoTick(U32 delta)
{
	int tick = super::DoTick(delta);
	if (cooldown.CanUse()) {
		charge += Travel(capacity / 2.0f, delta);
		if (charge > capacity)
			charge = capacity;
	}
	cooldown.Tick(delta);

	if (!cooldown.CanUse() || charge < capacity) {
		return 0;
	}
	return tick;
}


void Shield::Roll(int newTeam, const int* traits)
{
	super::Roll(newTeam, traits);
	float fc = capacity;
	fc *= Dice3D6ToMult(Traits().Get(GameTrait::ALT_CAPACITY));
	capacity = fc;

	float fcd = (float)cooldown.Threshold();
	fcd /= Dice3D6ToMult(Traits().Get(GameTrait::ALT_COOL));
	cooldown.SetThreshold(int(fcd));

	charge = capacity;
	cooldown.ResetReady();
}


void Shield::AbsorbDamage(DamageDesc* dd, float boost)
{
	if (Active()) {
		if (flags & EFFECT_FIRE)	dd->effects &= (~EFFECT_FIRE);
		if (flags & EFFECT_SHOCK)	dd->effects &= (~EFFECT_SHOCK);

		if (charge >= dd->damage) {
			charge -= dd->damage;
			dd->damage = 0;
		}
		else {
			dd->damage -= charge;
			charge = 0;
		}
	}
	cooldown.ResetUnready();
}


void GameItem::SetRogue()
{
	GLASSERT(IsDenizen()); 
	team = Team::Group(team);
}


void GameItem::SetChaos()
{
	GLASSERT(IsDenizen());
	team = TEAM_CHAOS;
}


void GameItem::SetTeam(int newTeam)
{
	GLASSERT(IName() != ISC::core);

	GLASSERT(    (Team::IsDenizen(team) && Team::IsDenizen(newTeam))
			  || (!Team::IsDenizen(team) && !Team::IsDenizen(newTeam))
			  || (Team::IsDeity(newTeam)));

	if (IsDenizen()) {
		GLASSERT(Team::IsDenizen(team));
		GLASSERT(Team::IsDenizen(newTeam));
		GLASSERT((newTeam == team) || Team::ID(team) == 0);
		GLASSERT(Team::ID(newTeam) != 0);
	}
	team = newTeam;
}


int GameItem::Deity() const
{
	switch (Team::Group(Team())) {
		case TEAM_TROLL:	
		return DEITY_TRUULGA;
		
		case TEAM_HOUSE:	
		return DEITY_R1K;

		case TEAM_GOB:
		return DEITY_Q;

		case TEAM_KAMAKIRI:
		return DEITY_KAXA;
	}
	return 0;
}
