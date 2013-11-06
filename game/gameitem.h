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

#include "../engine/enginelimits.h"

#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glmath.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glmicrodb.h"

#include "../tinyxml2/tinyxml2.h"

class DamageDesc;
class XStream;

/*
	Items and Inventory.

	Almost everything *is* an item. Characters, walls, plants, guns all
	have properties, health, etc. Lots of things are items, and therefore
	have a GameItem in an ItemComponent.

	Characters (and some other things) *have* items, in an inventory.
	- Some are built in: hands, claws, pincers. These may be hardpoints.
	- Some attach to hardpoints: shields
	- Some over-ride the built in AND attach to hardpoints: gun, sword
	- Some are carried in the pack
		
		Examples:
			- Human
				- "hand" is an item and trigger hardpoint.
				- "sword" overrides hand and attaches to trigger hardpoint
				- "shield" attaches to the shield harpoint (but there is no
				   item that it overrides)
				- "amulet" is carried, doesn't render, but isn't in pack.
			- Mantis
				- "pincer" is an item, but not a hardpoint

	Hardpoints
	- The possible list of hardpoints lives in the GameItem (as bit flags)
	- The Render component exposes a list of metadata is supports
	- The Inventory component translates between the metadata and hardpoints

	Constraints:
	- There can only be one melee weapon / attack. This simplies the animation,
	  and not having to track which melee hit. (This could be fixed, of course.)
	  The melee weapon must be on the 'trigger' hardpoint.
	- Similarly, the shield must be on the 'shield' hardpoint
	- An item can only attach to one hardpoint. (It would be good if it could attach
	  to either left or right hands)
	- In order to carry/use items, a ModelResource must have both a 'trigger' and
	  and 'shield'. Seems better than adding Yet Another Flag.
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

	int Get( int i ) {
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
	// Roll the initial values, more dice result in better numbers.
	// Minimum value of nDice = NUM_TRAITS
	void Roll( U32 seed, int nDice );

	void AddBattleXP( int killshotLevel )	{ exp += 1 + killshotLevel; }

	float NormalLeveledTrait( int t ) const {
		GLASSERT( t >= 0 && t < NUM_TRAITS );
		return NormalLeveledSkill( trait[t] );
	}

	float Accuracy() const		{ return NormalLeveledSkill( Dexterity() ); }
	float Damage() const		{ return NormalLeveledSkill( Strength() ); }
	float Toughness() const		{ return NormalLeveledSkill( Will() ); }
	
	U32 Hash() const			{ 
		return grinliz::Random::Hash( trait, sizeof(trait[0])*NUM_TRAITS );
	}

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

private:
	float NormalLeveledSkill( int value ) const {
		return Dice3D6ToMult( value + this->Level() );
	}

	int trait[NUM_TRAITS];
	int exp;
};


class IWeaponItem 
{
public:
	virtual GameItem* GetItem() = 0;
	virtual const GameItem* GetItem() const = 0;
};

class IMeleeWeaponItem : virtual public IWeaponItem
{
public:
};

class IRangedWeaponItem : virtual public IWeaponItem
{
public:
};

class IShield
{
public:
	virtual GameItem* GetItem() = 0;
	virtual const GameItem* GetItem() const = 0;
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
	bool Tick( int delta ) { time += delta; 
		if ( time >= threshold && (time-delta) < threshold ) return true;
		return false;
	}
	void Serialize( XStream* xs, const char* name );
	
	void SetThreshold( int t )	{ threshold = t; }
	void SetCurrent( int t )	{ time = t; }
	int Threshold() const		{ return threshold; }

private:
	int threshold;
	int time;
};


// FIXME: memory pool
class GameItem :	private IMeleeWeaponItem, 
					private IRangedWeaponItem,
					private IShield
{
public:
	GameItem( int _flags=0, const char* _name=0, const char* _res=0 )
	{
		CopyFrom(0);
		flags = _flags;
		name = grinliz::StringPool::Intern( _name );
		resource = grinliz::StringPool::Intern( _res );
		id = ++idPool;
	}

	GameItem( const GameItem& rhs )			{ CopyFrom( &rhs );	}
	void operator=( const GameItem& rhs )	{ CopyFrom( &rhs );	}

	virtual ~GameItem()	{}
	virtual GameItem* GetItem() { return this; }
	virtual const GameItem* GetItem() const { return this; }

	virtual void Load( const tinyxml2::XMLElement* doc );
	virtual void Serialize( XStream* xs );

	// If an intrinsic sub item has a trait - say, FIRE - that
	// implies that the parent is immune to fire. Apply() sets
	// basic sanity flags.
	void Apply( const GameItem* intrinsic );	

	const char* Name() const			{ return name.c_str(); }
	const char* ProperName() const		{ return properName.c_str(); }
	const char* Desc() const			{ return desc.c_str(); }
	const char* ResourceName() const	{ return resource.c_str(); }

	enum {
		// Type(s) of the item
		MELEE_WEAPON		= (1<<1),
		RANGED_WEAPON		= (1<<2),

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

		AI_EAT_PLANTS		= (1<<17),		// eats plants to regain health
		AI_HEAL_AT_CORE		= (1<<18),		// stands at a core to regain health (greater monsters, generally)
		AI_SECTOR_HERD		= (1<<19),		// will herd across sectors, as a group
		AI_SECTOR_WANDER	= (1<<20),		// will wander between sectors, as an individual
		AI_BINDS_TO_CORE	= (1<<21),
		AI_DOES_WORK		= (1<<22),

		GOLD_PICKUP			= (1<<23),
		ITEM_PICKUP			= (1<<24),		// picks up items. will use if possible.

		CLICK_THROUGH		= (1<<25),		// model is created with flags to ignore world clicking
	};

	// ------ description ------
	grinliz::IString		name;		// name of the item
	grinliz::IString		properName;	// the proper name, if the item has one "John"
	grinliz::IString		desc;		// description / alternate name
	grinliz::IString		key;		// modified name, for storage. not serialized.
	grinliz::IString		resource;	// resource used to  render the item
	int		id;				// unique id for this item
	static int idPool;
	int		flags;			// flags that define this item; 'constant'
	int		hardpoint;		// id of hardpoint this item attaches to
	float	mass;			// mass (kg)
	float	hpRegen;		// hp / second regenerated (or lost) by this item
	int		primaryTeam;	// who owns this items
	float	meleeDamage;	// a multiplier of the base (effective mass) applied before stats.Damage()
	float	rangedDamage;	// base ranged damage, applied before stats.Damage()
	float	absorbsDamage;	// how much damage this consumes, in the inventory (shield, armor, etc.) 1.0: all, 0.5: half
	Cooldown cooldown;		// time between uses.
	Cooldown reload;		// time to reload once clip is used up
	int		clipCap;		// possible rounds in the clip

	GameTrait traits;

	// ------- current --------
	float	hp;				// current hp for this item
	int		rounds;			// current rounds in the clip
	float	accruedFire;	// how much fire damage built up, not yet applied
	float	accruedShock;	// how much shock damage built up, not yet applied

	// These are the same, except for use. 'keyValues' are used
	// for simple dynamic extension. speed=2.0 for example.
	// 'microdb' is used to record history, events, etc.
	grinliz::MicroDB keyValues;
	grinliz::MicroDB microdb;

	float CalcBoltSpeed() const {
		static const float SPEED = 10.0f;
		float speed = 1.0f;
		keyValues.GetFloat( "speed", &speed );
		return SPEED * speed;
	}

	// Group all the copy/init in one place!
	void CopyFrom( const GameItem* rhs, int useID=-1 ) {
		if ( rhs ) {
			name			= rhs->name;
			properName		= rhs->properName;
			desc			= rhs->desc;
			key				= rhs->key;
			resource		= rhs->resource;
			if ( useID < 0 ) 
				id			= ++idPool;
			else
				id			= useID;
			flags			= rhs->flags;
			hardpoint		= rhs->hardpoint;
			mass			= rhs->mass;
			hpRegen			= rhs->hpRegen;
			primaryTeam		= rhs->primaryTeam;
			meleeDamage		= rhs->meleeDamage;
			rangedDamage	= rhs->rangedDamage;
			absorbsDamage	= rhs->absorbsDamage;
			cooldown		= rhs->cooldown;
			reload			= rhs->reload;
			clipCap			= rhs->clipCap;
			rounds			= rhs->rounds;
			traits			= rhs->traits;

			hp				= rhs->hp;
			accruedFire		= rhs->accruedFire;
			accruedShock	= rhs->accruedShock;

			keyValues		= rhs->keyValues;
			microdb			= rhs->microdb;
		}
		else {
			name = grinliz::IString();
			properName = grinliz::IString();
			desc = grinliz::IString();
			key  = grinliz::IString();
			resource = grinliz::IString();
			id = 0;
			flags = 0;
			hardpoint  = 0;
			mass = 1;
			hpRegen = 0;
			primaryTeam = 0;
			meleeDamage = 1;
			rangedDamage = 0;
			absorbsDamage = 0;
			clipCap = 0;			// default to no clip and unlimited ammo
			rounds = clipCap;
			traits.Init();
			keyValues.Clear();
			microdb.Clear();

			hp = TotalHPF();
			accruedFire = 0;
			accruedShock = 0;
		}
	}

	void InitState() {
		hp = TotalHPF();
		accruedFire = 0;
		accruedShock = 0;
		cooldown.ResetReady();
		reload.ResetReady();
		rounds = clipCap;
	}

	bool Intrinsic() const	{ return (flags & INTRINSIC) != 0; }

	virtual IMeleeWeaponItem*	ToMeleeWeapon()		{ return (flags & MELEE_WEAPON) ? this : 0; }
	virtual IRangedWeaponItem*	ToRangedWeapon()	{ return (flags & RANGED_WEAPON) ? this : 0; }

	virtual const IMeleeWeaponItem*		ToMeleeWeapon() const	{ return (flags & MELEE_WEAPON) ? this : 0; }
	virtual const IRangedWeaponItem*	ToRangedWeapon() const	{ return (flags & RANGED_WEAPON) ? this : 0; }

	virtual IWeaponItem*		ToWeapon()			{ return (flags & (MELEE_WEAPON | RANGED_WEAPON)) ? this : 0; }
	virtual const IWeaponItem*	ToWeapon() const	{ return (flags & (MELEE_WEAPON | RANGED_WEAPON)) ? this : 0; }

	virtual IShield*			ToShield()			{ return ( hardpoint == HARDPOINT_SHIELD ) ? this : 0; }
	virtual const IShield*		ToShield() const	{ return ( hardpoint == HARDPOINT_SHIELD ) ? this : 0; }

	int Effects() const { return flags & EFFECT_MASK; }
	int DoTick( U32 delta, U32 since );
	
	// States:
	//		Ready

	//		Cooldown (just used)
	//		Reloading
	//		Out of rounds
	bool CanUse()					{ return !CoolingDown() && !Reloading() && HasRound(); }
	bool Use( Chit* parent );

	bool CoolingDown() const		{ return !cooldown.CanUse(); }

	bool Reloading() const			{ return clipCap > 0 && !reload.CanUse(); }
	bool Reload( Chit* parent );
	bool CanReload() const			{ return !CoolingDown() && !Reloading() && (rounds < clipCap); }

	int Rounds() const { return rounds; }
	int ClipCap() const { return clipCap; }
	bool HasRound() const { 
		GLASSERT( rounds <= clipCap );
		return rounds || clipCap == 0; 
	}
	void UseRound();
	bool OnFire() const  { return (!(flags & IMMUNE_FIRE)) && accruedFire > 0; }
	bool OnShock() const { return (!(flags & IMMUNE_SHOCK)) && accruedShock > 0; }

	// Note that the current HP, if it has one, 
	int   TotalHP() const	{ return grinliz::Max( 1, (int)grinliz::LRintf( mass*traits.Toughness())); }
	float TotalHPF() const	{ return (float)TotalHP(); }

	float HPFraction() const	{ 
		float f = hp / TotalHPF(); 
		//GLASSERT( f >= 0 && f <= 1 );
		f = grinliz::Clamp( f, 0.0f, 1.0f ); // FIXME: hack in hp calc
		return f;
	} 

	float ReloadFraction() const {
		if ( Reloading() ) {
			return reload.Fraction();
		}
		return 1.0f;
	}

	float RoundsFraction() const {
		if ( clipCap ) {
			return (float)rounds / (float)clipCap;
		}
		return 1.0f;
	}

	// Absorb damage.'remain' is how much damage passes through the shield
	void AbsorbDamage( bool inInventory, DamageDesc dd, DamageDesc* remain, const char* log, const IMeleeWeaponItem* booster, Chit* parent );

private:
	float Delta( U32 delta, float v ) {
		return v * (float)delta * 0.001f;
	}
};



#endif // GAMEITEM_INCLUDED
