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

#include "../engine/enginelimits.h"

#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glmath.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glrandom.h"

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

		Leads to:
			INTRINSIC_AT_HARDPOINT		hand
			INTRINSIC_FREE				pincer
			HELD_AT_HARDPOINT			sword, shield
			HELD_FREE					amulet

	Hardpoints
	- The possible list of hardpoints lives in the GameItem (as bit flags)
	- The Render component exposes a list of metadata is supports
	- The Inventory component translates between the metadata and hardpoints

	Constraints:
	- There can only be one melee weapon / attack. This simplies the animation,
	  and not having to track which melee hit. (This could be fixed, of course.)
	  The melee weapon must be on the 'trigger' hardpoint.
	- Similarly, the shield must be on the 'shield' hardpoitn
	- An item can only attach to one hardpoint. It would be good if it could attach
	  to either left or right hands, for example.
	  
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


static const float SKILL_NORMALIZE = 0.1f;	// skill of 10 is a multiple 1.0
static const float LEVEL_BONUS     = 0.5f;


class GameStat
{
public:
	GameStat()  {
		Init();	
	}

	void Save( tinyxml2::XMLPrinter* );
	void Load( const tinyxml2::XMLElement* doc );
	void Serialize( XStream* xs );

	void Init() {
		for( int i=0; i<NUM_TRAITS; ++i ) trait[i] = 10;
		exp	= 0;
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

	void AddBattleXP( int killshotLevel )	{ exp += 1 + killshotLevel; }

	// 1.0 is normal, 0.1 very low, 2+ exceptional.
	float NormalLeveledTrait( int t ) const { GLASSERT( t>=0 && t<NUM_TRAITS ); return NormalLeveledSkill( trait[t] ); }
	float Accuracy() const		{ return NormalLeveledSkill( Dexterity() ); }
	float Damage() const		{ return NormalLeveledSkill( Strength() ); }
	float Toughness() const		{ return NormalLeveledSkill( Will() ); }
	
	int LevelStr() const		{ return grinliz::LRintf( LeveledSkill( Strength() )); }
	int LevelWill() const		{ return grinliz::LRintf( LeveledSkill( Will() )); }
	int LevelInt() const		{ return grinliz::LRintf( LeveledSkill( Intelligence() )); }
	int LevelDex() const		{ return grinliz::LRintf( LeveledSkill( Dexterity() )); }
	
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
			NUM_TRAITS
		};

private:
	float LeveledSkill( int value ) const { 
		return ((float)value + (float)Level() * LEVEL_BONUS);
	}
	float NormalLeveledSkill( int value ) const {
		return LeveledSkill( value ) * SKILL_NORMALIZE;
	}
	void Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele );

	int trait[NUM_TRAITS];
	int exp;
};



class IWeaponItem 
{
public:
	virtual GameItem* GetItem() = 0;
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
	}

	GameItem( const GameItem& rhs )			{ CopyFrom( &rhs );	}
	void operator=( const GameItem& rhs )	{ CopyFrom( &rhs );	}

	virtual ~GameItem()	{}
	virtual GameItem* GetItem() { return this; }

	virtual void Save( tinyxml2::XMLPrinter* );
	virtual void Load( const tinyxml2::XMLElement* doc );
	virtual void Serialize( XStream* xs );

	// If an intrinsic sub item has a trait - say, FIRE - that
	// implies that the parent is immune to fire. Apply() sets
	// basic sanity flags.
	void Apply( const GameItem* intrinsic );	

	const char* Name() const			{ return name.c_str(); }
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
		AI_SECTOR_HERD		= (1<<18),		// will herd across sectors
		AI_BINDS_TO_CORE	= (1<<19),
		AI_DOES_WORK		= (1<<20),
		GOLD_PICKUP			= (1<<21),

		CLICK_THROUGH		= (1<<22),		// model is created with flags to ignore world clicking
	};

	// ------ description ------
	grinliz::IString		name;		// name of the item
	grinliz::IString		desc;		// description / alternate name
	grinliz::IString		key;		// modified name, for storage. not serialized.
	grinliz::IString		resource;	// resource used to  render the item
	int		flags;			// flags that define this item; 'constant'
	int		hardpoint;		// id of hardpoint this item attaches to
	float	mass;			// mass (kg)
	float	hpRegen;		// hp / second regenerated (or lost) by this item
	int		primaryTeam;	// who owns this items
	float	meleeDamage;	// a multiplier of the base (effective mass) applied before stats.Damage()
	float	rangedDamage;	// base ranged damage, applied before stats.Damage()
	float	absorbsDamage;	// how much damage this consumes, in the inventory (shield, armor, etc.) 1.0: all, 0.5: half
	int		cooldown;		// time between uses
	int		reload;			// time to reload once clip is used up
	int		clipCap;		// possible rounds in the clip

	GameStat stats;

	// ------- current --------
	bool	isHeld;			// if true, a held item is "in hand", else it is in the pack.
	float	hp;				// current hp for this item
	int		cooldownTime;	// counting down to ready state
	int		reloadTime;		// counting down to ready state
	int		rounds;			// current rounds in the clip
	float	accruedFire;	// how much fire damage built up, not yet applied
	float	accruedShock;	// how much shock damage built up, not yet applied

	struct KeyValue
	{
		bool operator==( const KeyValue& rhs ) const { return key == rhs.key; }

		grinliz::IString	key;
		grinliz::IString	value;
	};
	grinliz::CDynArray<KeyValue>	keyValues;

	grinliz::IString GetValue( const char* name ) const;
	bool GetValue( const char* name, double* value ) const;
	bool GetValue( const char* name, float* value ) const;
	bool GetValue( const char* name, int* value ) const;

	float CalcBoltSpeed() const {
		static const float SPEED = 10.0f;
		float speed = 1.0f;
		GetValue( "speed", &speed );
		return SPEED * speed;
	}

	Chit* parentChit;		// only set when attached to a Component

	// Group all the copy/init in one place!
	void CopyFrom( const GameItem* rhs ) {
		if ( rhs ) {
			name			= rhs->name;
			desc			= rhs->desc;
			key				= rhs->key;
			resource		= rhs->resource;
			flags			= rhs->flags;
			hardpoint		= rhs->hardpoint;
			mass			= rhs->mass;
			hpRegen			= rhs->hpRegen;
			primaryTeam		= rhs->primaryTeam;
			meleeDamage		= rhs->meleeDamage;
			rangedDamage	= rhs->rangedDamage;
			absorbsDamage	= rhs->absorbsDamage;
			cooldown		= rhs->cooldown;
			cooldownTime	= rhs->cooldownTime;
			reload			= rhs->reload;
			reloadTime		= rhs->reloadTime;
			clipCap			= rhs->clipCap;
			rounds			= rhs->rounds;
			stats			= rhs->stats;

			isHeld			= rhs->isHeld;
			hp				= rhs->hp;
			accruedFire		= rhs->accruedFire;
			accruedShock	= rhs->accruedShock;

			keyValues.Clear();
			for( int i=0; i<rhs->keyValues.Size(); ++i ) {
				keyValues.Push( rhs->keyValues[i] );
			}

			parentChit		= 0;	// NOT copied
		}
		else {
			name = grinliz::IString();
			desc = grinliz::IString();
			key  = grinliz::IString();
			resource = grinliz::IString();
			flags = 0;
			hardpoint  = 0;
			mass = 1;
			hpRegen = 0;
			primaryTeam = 0;
			meleeDamage = 1;
			rangedDamage = 0;
			absorbsDamage = 0;
			cooldown = 1000;
			cooldownTime = 0;
			reload = 2000;
			reloadTime = 0;
			clipCap = 0;			// default to no clip and unlimited ammo
			rounds = clipCap;
			stats.Init();
			keyValues.Clear();

			isHeld = false;
			hp = TotalHPF();
			accruedFire = 0;
			accruedShock = 0;

			parentChit = 0;
		}
	}

	void InitState() {
		hp = TotalHPF();
		accruedFire = 0;
		accruedShock = 0;
		cooldownTime = 0;
		reloadTime = 0;
		rounds = clipCap;
	}

	bool Active() const		{ return isHeld || (flags & INTRINSIC); }
	bool Intrinsic() const	{ return (flags & INTRINSIC) != 0; }

	virtual IMeleeWeaponItem*	ToMeleeWeapon()		{ return (flags & MELEE_WEAPON) ? this : 0; }
	virtual IRangedWeaponItem*	ToRangedWeapon()	{ return (flags & RANGED_WEAPON) ? this : 0; }

	virtual IWeaponItem*		ToWeapon()			{ return (flags & (MELEE_WEAPON | RANGED_WEAPON)) ? this : 0; }
	virtual const IWeaponItem*	ToWeapon() const	{ return (flags & (MELEE_WEAPON | RANGED_WEAPON)) ? this : 0; }

	virtual IShield*			ToShield()			{ return ( hardpoint == HARDPOINT_SHIELD ) ? this : 0; }

	int Effects() const { return flags & EFFECT_MASK; }
	int DoTick( U32 delta, U32 since );
	
	// States:
	//		Ready

	//		Cooldown (just used)
	//		Reloading
	//		Out of rounds
	bool CanUse()					{ return !CoolingDown() && !Reloading() && HasRound(); }
	bool Use();

	bool CoolingDown() const		{ return cooldownTime > 0; }

	bool Reloading() const			{ return clipCap > 0 && reloadTime > 0; }
	bool Reload();
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
	int TotalHP() const		{ return grinliz::LRintf( mass*stats.Toughness() ); }
	float TotalHPF() const	{ return (float)TotalHP(); }

	float HPFraction() const	{ 
		float f = hp / TotalHPF(); 
		//GLASSERT( f >= 0 && f <= 1 );
		f = grinliz::Clamp( f, 0.0f, 1.0f ); // FIXME: hack in hp calc
		return f;
	} 

	float ReloadFraction() const {
		if ( Reloading() ) {
			return (float)reloadTime / (float)reload;
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
	void AbsorbDamage( bool inInventory, DamageDesc dd, DamageDesc* remain, const char* log );

private:
	float Delta( U32 delta, float v ) {
		return v * (float)delta * 0.001f;
	}
};



#endif // GAMEITEM_INCLUDED
