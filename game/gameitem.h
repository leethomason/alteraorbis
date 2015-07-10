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

#ifndef GAMEITEM_INCLUDED
#define GAMEITEM_INCLUDED

#include "gamelimits.h"
#include "lumosmath.h"
#include "wallet.h"

#include "../engine/enginelimits.h"

#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glmath.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glmicrodb.h"

#include "../tinyxml2/tinyxml2.h"

#include "personality.h"

#include "../xegame/istringconst.h"

class DamageDesc;
class XStream;
class NewsHistory;

class MeleeWeapon;
class RangedWeapon;
class Shield;

/*
	Items and Inventory.

	Almost everything *is* an item. Characters, guns all
	have properties, health, etc. Lots of things are items, and therefore
	have a GameItem in an ItemComponent.

	(Note that walls and plants used to be items, but it over-taxed
	the system. They are now special cased.)

	Characters (and some other things) *have* items, in an inventory.
	- Some are built in: hands, claws, pincers. This is "intrinsic".
	  They are at a hardpoint, but don't render.
	- Some attach to hardpoints: shields, guns.
	- An attached item can over-ride an intrinsic. Gun will override hand.
	- The rest are carried in the pack
		
		Examples:
			- Human
				- "hand" is an item and trigger hardpoint.
				- "sword" overrides hand and attaches to trigger hardpoint
				- "shield" attaches to the shield harpoint (but there is no
				   item that it overrides)

	Hardpoints
	- The possible list is enumerated as EL_NUM_METADATA
	- The ModelResource has a list that a given model actually supports.

	Constraints:
	- Can't be multiples. The first Ranged, Shield, Melee is the one 
	  that gets used.
	- An item can only attach to one hardpoint. (It would be good if it could attach
	  to either left or right hands)
*/

class Chit;
class WeaponItem;
class GameItem;


class DamageDesc
{
public:
	DamageDesc() : damage(1), effects(0) {}
	DamageDesc( float _damage, int _effects ) : damage(_damage), effects(_effects) {}

	float damage;
	int effects;

	void Log();
	float Score() const;	// overall rating of damage (includes efffects)
};


static const float LEVEL_BONUS     = 0.5f;


class GameTrait
{
public:
	GameTrait()  {
		Init();	
	}

	void Serialize( XStream* xs );

	void Init() {
		for( int i=0; i<NUM_TRAITS; ++i ) trait[i] = 10;
		exp	= 0;
	}
		
	void Set( int i, int value ) {
		GLASSERT( i >= 0 && i < NUM_TRAITS );
		GLASSERT( value > 0 && value <= 20 );
		trait[i] = value;
	}

	const int * Get() const {
		return trait;
	}

	int Get( int i ) const {
		GLASSERT( i >= 0 && i < NUM_TRAITS );
		return trait[i];
	}

	// 1-20
	int Strength() const		{ return trait[STR]; }
	int Will() const			{ return trait[WILL]; }
	int Charisma() const		{ return trait[CHR]; }
	int Intelligence() const	{ return trait[INT]; }
	int Dexterity() const		{ return trait[DEX]; }

	int Experience() const		{ return exp; }
	int Level() const			{ return ExperienceToLevel( exp ); }

	void SetExpFromLevel( int level ) {		
		exp = LevelToExperience( level );
		GLASSERT( level == ExperienceToLevel( exp ));
	}
	// Roll the initial values.
	void Roll( U32 seed );

	// Add...XP() should be called from the ItemComponent - 
	// other things need to happen (display items, add hp, etc.)
	void AddBattleXP( int killshotLevel )	{ exp += 1 + killshotLevel; }
	void AddCraftXP( int nCrystals )		{ exp += 8 * nCrystals; }
	void AddXP( int xp )					{ exp += xp; }

	float NormalLeveledTrait( int t ) const {
		GLASSERT( t >= 0 && t < NUM_TRAITS );
		return NormalLeveledSkill( trait[t] );
	}

	float Accuracy() const		{ return NormalLeveledSkill( Dexterity() ); }
	float Damage() const		{ return NormalLeveledSkill( Strength() ); }
	float Toughness() const		{ return NormalLeveledSkill( Will() ); }
	
	// Log base 2: 1 -> 0		exp: 0-31  -> 0 level
	//			   2 -> 1           32-63  -> 1
	//             4 -> 2           64-127 -> 2
	//			   8 -> 3          128-255 -> 3
	static int ExperienceToLevel( int ep )	{ return grinliz::LogBase2( ep/16 ); } 
	static int LevelToExperience( int lp )	{ return 16*(1<<lp); }
					
	enum {	STR,
			WILL,
			CHR,
			INT,
			DEX,
			NUM_TRAITS,

			ALT_DAMAGE		= STR,
			ALT_SPEED		= WILL,
			ALT_COOL		= WILL,
			ALT_CAPACITY	= CHR,
			ALT_RELOAD		= INT,
			ALT_ACCURACY	= DEX,
		};

	bool HasTraits() const {
		if (exp) return true;
		for (int i = 0; i < NUM_TRAITS; ++i) {
			if (trait[i] != 10) return true;
		}
		return false;
	}

private:
	float NormalLeveledSkill( int value ) const {
		return Dice3D6ToMult( value + this->Level() );
	}

	int trait[NUM_TRAITS];
	int exp;
};


class Cooldown
{
public:
	Cooldown( int t=1000 ) : threshold(t), time(t)								{}
	Cooldown( const Cooldown& rhs ) : threshold(rhs.threshold), time(rhs.time)	{}
	void operator=( const Cooldown& rhs ) {
		this->time		= rhs.time;
		this->threshold = rhs.threshold;
	}

	void ResetReady() {
		time = threshold;
	}
	void ResetUnready() {
		time = 0;
	}

	bool CanUse() const { return time >= threshold; }
	float Fraction() const {
		if ( time >= threshold ) return 1.0f;
		return (float)time / (float)threshold;
	}
	bool Use() {
		if ( CanUse() ) {
			time = 0;
			return true;
		}
		return false;
	}
	bool Tick( int delta ) { 
		if (time > threshold) {
			time = threshold;	// error recovery
		}
		else if (time < threshold) {
			time += delta;
			if (time >= threshold) {
				time = threshold;
				return true;
			}
		}
		return false;
	}
	void Serialize( XStream* xs, const char* name);
	
	void SetThreshold( int t )	{ threshold = t; }
	void SetCurrent( int t )	{ time = t; }
	int Threshold() const		{ return threshold; }

private:
	int threshold;
	int time;
};


class GameItem
{
private:
	// ------ description ------
	grinliz::IString		name;		// name of the item
	grinliz::IString		properName;	// the proper name, if the item has one "John"
	grinliz::IString		resource;	// resource used to  render the item

	GameItem(const GameItem& rhs);
	void operator=(const GameItem& rhs);

public:

	static GameItem* Factory(const char* subclass);

	GameItem();

	static bool trackWallet;
	virtual ~GameItem();

	virtual GameItem* Clone() const { return CloneFrom(0); }
	virtual void Roll(int team, const int* values);
	int ID() const;

	virtual void Load( const tinyxml2::XMLElement* doc );
	virtual void Serialize( XStream* xs);

	// If an intrinsic sub item has a trait - say, FIRE - that
	// implies that the parent is immune to fire. Apply() sets
	// basic sanity flags.
	void Apply( const GameItem* intrinsic );	
	
	// is this an item who has a trackable history? I try 
	// to avoid saying what an Item is...but sometimes
	// you need to know.
	bool Significant() const;
	// Get the value of the item. Currently equivalent to being loot.
	// Intrinsic items do return a value; gives a pretty
	// good scale of "how good is the item".
	int  GetValue() const;

	// name:		blaster						humanFemale					The item name - itemdef.xml
	// proper:		Hgar						Trinity						Given name, if assigned. Else empty/null.
	// best:		Hgar						Trinity						Given name, if assigned. Else item name.
	// full:		Hgar (blaster)				Trinity (humanFemale)		Fully formatted.
	// title:		Dragon Slayer				Troll Bane			
	// nameAndTitle:Hgar (blaster) Dragon...	Trinity (humanFemale) Troll Bane

	const char*			Name() const			{ return name.c_str(); }
	grinliz::IString	IName() const			{ return name; }
	const char*			ProperName() const		{ return properName.c_str(); }
	grinliz::IString	IProperName() const		{ return properName; }
	const char*			BestName() const		{ if ( !properName.empty() ) return properName.c_str(); return name.c_str(); }
	grinliz::IString	IBestName() const		{ if ( !properName.empty() ) return properName; return name; }
	grinliz::IString	IFullName() const;
	const char*			ResourceName() const	{ return resource.c_str(); }
	grinliz::IString	IResourceName() const	{ return resource; }

	void SetName( const char* n )				{ name = grinliz::StringPool::Intern( n ); UpdateTrack(); }
	void SetProperName( const char* n )			{ properName = grinliz::StringPool::Intern( n ); UpdateTrack(); }
	void SetProperName( const grinliz::IString& n );
	void SetResource(const char* n)				{ GLASSERT(n && *n);  resource = grinliz::StringPool::Intern(n); }

	bool IsFemale() const {
		// HACK! But only here.
		return !resource.empty() && strstr(resource.c_str(), "emale");
	}

	// Sets this item to be tracked. Also records the team that created it, 
	// for generating colors when applicable.
	void SetSignificant(NewsHistory* history, const grinliz::Vector2F& pos, int creationMsg, int destructionMsg, Chit* creator);

	enum {
		// Type(s) of the item
		INTRINSIC			= (1<<3),				// built in item: pinters for example. Always "held".

		IMMUNE_FIRE			= (1<<6),				// doesn't burn *at all*
		FLAMMABLE			= (1<<7),				// burns until gone (wood)
		IMMUNE_SHOCK		= (1<<8),
		SHOCKABLE			= (1<<9),

		EFFECT_EXPLOSIVE	= (1<<10),
		EFFECT_FIRE			= (1<<11),	
		EFFECT_SHOCK		= (1<<12),
		EFFECT_MASK			= EFFECT_EXPLOSIVE | EFFECT_FIRE | EFFECT_SHOCK,

		RENDER_TRAIL		= (1<<13),		// render a bolt with a 'smoketrail' vs. regular bolt

		INDESTRUCTABLE		= (1<<14),

		AI_WANDER_HERD		= (1<<15),		// basic herding
		AI_WANDER_CIRCLE	= (1<<16),		// creepy circle herding
		AI_WANDER_MASK      = AI_WANDER_HERD | AI_WANDER_CIRCLE,

		AI_SECTOR_HERD		= (1<<17),		// will herd across sectors, as a group
		AI_SECTOR_WANDER	= (1<<18),		// will wander between sectors, as an individual
		AI_USES_BUILDINGS	= (1<<20),		// can use markets, etc. and do transactions. also used as a general "smart enough to use weapons" flag.
		AI_NOT_TARGET		= (1<<21),		// won't be targeted in battle	

		GOLD_PICKUP			= (1<<22),
		ITEM_PICKUP			= (1<<23),		// picks up items. will use if possible.
		HAS_NEEDS			= (1<<24),		// ai should use needs

		DAMAGE_UNDER_WATER  = (1<<25),		// if set, takes damage if submerged
		FLOAT				= (1<<26),		// floats on water and moves with the water flow
		SUBMARINE			= (1<<27),		// isn't moved by the fluid when walking.
		EXPLODES			= (1<<28),		// explodes when destroyed. hell yeah!

		CLICK_THROUGH		= (1<<29),		// model is created with flags to ignore world clicking
		PATH_NON_BLOCKING   = (1<<30),		// doesn't block the pather
	};

	grinliz::IString		key;		// modified name, for storage. not serialized.
	int		flags;			// flags that define this item; 'constant'
	int		hardpoint;		// id of hardpoint this item attaches to
	float	mass;			// mass (kg)
	float	baseHP;
	float	hpRegen;		// hp / second regenerated (or lost) by this item

private:
	int		team;			// which team this item is aligned with (or created it, for items.)
public:
	int		Team() const	{ return team; }
	int		Deity() const;

	void	SetRogue();
	void	SetChaos();
	void	SetTeam(int team);

	Wallet wallet;			// this is either money carried (denizens) or resources bound in (weapons)

	// ------- current --------
	double	hp;				// current hp for this item. concerned float isn't enough if small time slices in use
	int		fireTime;		// time to fire goes out
	int  	shockTime;		// time to shock goes out

	// These are the same, except for use. 'keyValues' are used
	// for simple dynamic extension. speed=2.0 for example.
	// 'microdb' is used to record history, events, etc.
	grinliz::MicroDB keyValues;
	grinliz::MicroDB historyDB;

	bool Intrinsic() const			{ return (flags & INTRINSIC) != 0; }
	grinliz::IString MOB() const	{ return keyValues.GetIString(ISC::mob); }
	bool IsDenizen() const			{ return keyValues.GetIString(ISC::mob) == ISC::denizen; }
	bool IsWorker() const			{ return name == ISC::worker; }

	virtual MeleeWeapon*	ToMeleeWeapon()		{ return 0; }
	virtual RangedWeapon*	ToRangedWeapon()	{ return 0; }
	virtual Shield*			ToShield()			{ return 0; }

	virtual const MeleeWeapon*	ToMeleeWeapon() const	{ return 0; }
	virtual const RangedWeapon*	ToRangedWeapon() const	{ return 0; }
	virtual const Shield*		ToShield() const		{ return 0; }

	// Convenience method to get the natural-industrial scale
	double GetBuildingIndustrial() const;

	int Effects() const { return flags & EFFECT_MASK; }
	virtual int DoTick( U32 delta );
	
	bool OnFire() const  { return fireTime > 0; }
	bool OnShock() const { return shockTime > 0; }

	// Note that the current HP, if it has one, 
	int   TotalHP() const	{ return grinliz::Max( 1, (int)grinliz::LRintf( baseHP*traits.Toughness())); }

	double HPFraction() const	{ 
		double f = hp / double(TotalHP()); 
		f = grinliz::Clamp( f, 0.0, 1.0 ); // FIXME: hack in hp calc
		return f;
	} 

	void FullHeal() {
		hp = TotalHP();
	}

	void Heal(double h) {
		GLASSERT(h >= 0);
		hp += h;
		hp = grinliz::Min(double(TotalHP()), hp);
	}

	void Heal(int h) {
		GLASSERT(h >= 0);
		hp += (double)h;
		hp = grinliz::Min(double(TotalHP()), hp);
	}

	void AbsorbDamage( const DamageDesc& dd );

	static int idPool;

	const GameTrait& Traits() const				{ return traits; }
	GameTrait* GetTraitsMutable()				{ value = -1; return &traits; }
	const Personality& GetPersonality() const	{ return personality; }
	Personality* GetPersonalityMutable()		{ return &personality; }
	
	// Generally should be automatic.
	void UpdateHistory() const					{ UpdateTrack(); }
	void Track() const;

protected:
	GameItem* CloneFrom(GameItem* item) const;

private:
	GameTrait	traits;
	Personality personality;

	// Functions to update the ItemDB
	void UnTrack() const;
	void UpdateTrack() const;

	mutable int	id;						// unique id for this item. not assigned until needed, hence mutable
	mutable grinliz::IString fullName;	// not serialized, but cached
	mutable int value;					// not serialized, but cached
};


class MeleeWeapon : public GameItem
{
	typedef GameItem super;
public:
	MeleeWeapon();
	~MeleeWeapon()	{}
	GameItem* Clone() const;

	virtual MeleeWeapon* ToMeleeWeapon() { return this;  }
	virtual const MeleeWeapon* ToMeleeWeapon() const { return this;  }

	virtual void Roll(int team, const int* roll);
	float Damage() const;
	int CalcMeleeCycleTime() const;	
	float ShieldBoost() const;

	void Serialize( XStream* xs);
	virtual void Load( const tinyxml2::XMLElement* doc );

private:
	float meleeDamage;
};


class RangedWeapon : public GameItem
{
	typedef GameItem super;

public:
	RangedWeapon();
	~RangedWeapon();

	virtual RangedWeapon* ToRangedWeapon() { return this;  }
	virtual const RangedWeapon* ToRangedWeapon() const { return this;  }


	void Serialize( XStream* xs);
	virtual void Load( const tinyxml2::XMLElement* doc );
	virtual void Roll(int team, const int* roll);
	GameItem* Clone() const;

	virtual int DoTick(U32 delta);

	bool CanShoot() const;
	bool Shoot(Chit* parent);
	bool Reload(Chit* parent);
	float BoltSpeed() const;
	int ClipCap() const				{ return clipCap; }
	int ReloadTime() const			{ return reload.Threshold(); }
	int CooldownTime() const		{ return cooldown.Threshold(); }
	float Damage() const;
	float Accuracy() const;

	bool CoolingDown() const		{ return !cooldown.CanUse(); }
	bool Reloading() const			{ return clipCap > 0 && !reload.CanUse(); }
	bool CanReload() const			{ return !CoolingDown() && !Reloading() && (rounds < clipCap); }
	bool HasRound() const			{ GLASSERT( rounds <= clipCap );	return rounds || clipCap == 0; }
	
	float ReloadFraction() const;
	float RoundsFraction() const;

private:
	void Init();
	void UseRound();

	float	 rangedDamage;	// base ranged damage, applied before stats.Damage()
	Cooldown cooldown;		// time between uses.
	Cooldown reload;		// time to reload once clip is used up
	int		 rounds;		// current rounds in the clip
	int		 clipCap;		// possible rounds in the clip
	float	 accuracy;
};


class Shield : public GameItem
{
	typedef GameItem super;

public:
	Shield();
	~Shield();

	virtual Shield* ToShield() { return this;  }
	virtual const Shield* ToShield() const { return this; }

	void Serialize(XStream* xs);
	virtual void Load(const tinyxml2::XMLElement* doc);
	virtual GameItem* Clone() const;
	virtual void Roll(int team, const int* roll);

	virtual int DoTick(U32 delta);

	int ShieldRechargeTime() const	{ return cooldown.Threshold(); }
	// Is shield on and working?
	bool Active() const	{ return charge > 0; }
	void AbsorbDamage(DamageDesc* dd, float boost);
	
	float ChargeFraction() const {
		if (capacity) {
			return float(charge) / float(capacity);
		}
		else
			return 1;
	}
	float Capacity() const { return capacity; }

protected:

private:
	float charge;
	float capacity;
	Cooldown cooldown;
};


#endif // GAMEITEM_INCLUDED
