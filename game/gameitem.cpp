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

#include "../grinliz/glstringutil.h"
#include "../engine/serialize.h"

#include "../tinyxml2/tinyxml2.h"
#include "../xarchive/glstreamer.h"

#include "../xegame/chit.h"
#include "../xegame/istringconst.h"

#include "../script/battlemechanics.h"
#include "../script/itemscript.h"

using namespace grinliz;
using namespace tinyxml2;

#define READ_FLAG( flags, str, name )	{ if ( strstr( str, #name )) flags |= name; }

#define READ_FLOAT_ATTR( n )			else if ( name == #n ) { n = attr->FloatValue(); }
#define READ_INT_ATTR( n )				else if ( name == #n ) { n = attr->IntValue(); }
#define READ_BOOL_ATTR( n )				else if ( name == #n ) { n = attr->BoolValue(); }

#define APPEND_FLAG( flags, cstr, name )	{ if ( flags & name ) { f += #name; f += " "; } }
#define PUSH_ATTRIBUTE( prnt, name )		{ prnt->PushAttribute( #name, name ); }


void GameTrait::Serialize( XStream* xs )
{
	XarcOpen( xs, "traits" );
	XARC_SER_ARR( xs, trait, NUM_TRAITS );
	XARC_SER( xs, exp );

	XarcClose( xs );
}


void GameTrait::Roll( U32 seed )
{
	Random random( seed );
	for( int i=0; i<NUM_TRAITS; ++i ) {
		trait[i] = random.Dice( 3, 6 );
	}
}



void Cooldown::Serialize( XStream* xs, const char* name )
{
	XarcOpen( xs, name );
	XARC_SER( xs, threshold );
	XARC_SER( xs, time );
	XarcClose( xs );
}

int GameItem::idPool = 0;

void GameItem::Serialize( XStream* xs )
{
	XarcOpen( xs, "GameItem" );

	XARC_SER( xs, name );
	XARC_SER( xs, properName );
	XARC_SER( xs, desc );
	XARC_SER( xs, resource );
	XARC_SER( xs, id );
	XARC_SER( xs, mass );
	XARC_SER_DEF( xs, hpRegen, 0.0f );
	XARC_SER_DEF( xs, primaryTeam, 0 );
	XARC_SER_DEF( xs, clipCap, 0 );
	XARC_SER_DEF( xs, rounds, 0 );
	XARC_SER_DEF( xs, meleeDamage, 1.0f );
	XARC_SER_DEF( xs, rangedDamage, 0.0f );
	XARC_SER_DEF( xs, absorbsDamage, 0 );
	XARC_SER_DEF( xs, accruedFire, 0 );
	XARC_SER_DEF( xs, accruedShock, 0 );

	XARC_SER( xs, hardpoint );
	XARC_SER( xs, hp );

	if ( xs->Saving() ) {
		CStr<512> f;
		APPEND_FLAG( flags, f, MELEE_WEAPON );
		APPEND_FLAG( flags, f, RANGED_WEAPON );
		APPEND_FLAG( flags, f, INTRINSIC );
		APPEND_FLAG( flags, f, IMMUNE_FIRE );
		APPEND_FLAG( flags, f, FLAMMABLE );
		APPEND_FLAG( flags, f, IMMUNE_SHOCK );
		APPEND_FLAG( flags, f, SHOCKABLE );
		APPEND_FLAG( flags, f, EFFECT_EXPLOSIVE );
		APPEND_FLAG( flags, f, EFFECT_FIRE );
		APPEND_FLAG( flags, f, EFFECT_SHOCK );
		APPEND_FLAG( flags, f, RENDER_TRAIL );
		APPEND_FLAG( flags, f, INDESTRUCTABLE );
		APPEND_FLAG( flags, f, AI_WANDER_HERD );
		APPEND_FLAG( flags, f, AI_WANDER_CIRCLE );
		APPEND_FLAG( flags, f, AI_EAT_PLANTS );
		APPEND_FLAG( flags, f, AI_HEAL_AT_CORE );
		APPEND_FLAG( flags, f, AI_SECTOR_HERD );
		APPEND_FLAG( flags, f, AI_SECTOR_WANDER );
		APPEND_FLAG( flags, f, AI_USES_BUILDINGS );
		APPEND_FLAG( flags, f, AI_DOES_WORK );
		APPEND_FLAG( flags, f, GOLD_PICKUP );
		APPEND_FLAG( flags, f, ITEM_PICKUP );
		APPEND_FLAG( flags, f, CLICK_THROUGH );

		xs->Saving()->Set( "flags", f.c_str() );
	}
	else {
		const StreamReader::Attribute* attr = xs->Loading()->Get( "flags" );
		if ( attr ) {
			const char* f = xs->Loading()->Value( attr, 0 );
			READ_FLAG( flags, f, MELEE_WEAPON );
			READ_FLAG( flags, f, RANGED_WEAPON );
			READ_FLAG( flags, f, INTRINSIC );
			READ_FLAG( flags, f, IMMUNE_FIRE );
			READ_FLAG( flags, f, FLAMMABLE );
			READ_FLAG( flags, f, IMMUNE_SHOCK );
			READ_FLAG( flags, f, SHOCKABLE );
			READ_FLAG( flags, f, EFFECT_EXPLOSIVE );
			READ_FLAG( flags, f, EFFECT_FIRE );
			READ_FLAG( flags, f, EFFECT_SHOCK );
			READ_FLAG( flags, f, RENDER_TRAIL );
			READ_FLAG( flags, f, INDESTRUCTABLE );
			READ_FLAG( flags, f, AI_WANDER_HERD );
			READ_FLAG( flags, f, AI_WANDER_CIRCLE );
			READ_FLAG( flags, f, AI_EAT_PLANTS );
			READ_FLAG( flags, f, AI_HEAL_AT_CORE );
			READ_FLAG( flags, f, AI_SECTOR_HERD );
			READ_FLAG( flags, f, AI_SECTOR_WANDER );
			READ_FLAG( flags, f, AI_USES_BUILDINGS );
			READ_FLAG( flags, f, AI_DOES_WORK );
			READ_FLAG( flags, f, GOLD_PICKUP );
			READ_FLAG( flags, f, ITEM_PICKUP );
			READ_FLAG( flags, f, CLICK_THROUGH );
		}
	}

	cooldown.Serialize( xs, "cooldown" );
	reload.Serialize( xs, "reload" );
	
	keyValues.Serialize( xs, "keyval" );
	microdb.Serialize( xs, "microdb" );

	traits.Serialize( xs );
	wallet.Serialize( xs );
	XarcClose( xs );

	if ( xs->Loading() ) {
		Track();
	}
}

	
void GameItem::Load( const tinyxml2::XMLElement* ele )
{
	this->CopyFrom( 0 );

	GLASSERT( StrEqual( ele->Name(), "item" ));
	
	name		= StringPool::Intern( ele->Attribute( "name" ));
	properName	= StringPool::Intern( ele->Attribute( "properName" ));
	desc		= StringPool::Intern( ele->Attribute( "desc" ));
	resource	= StringPool::Intern( ele->Attribute( "resource" ));
	id = 0;
	flags = 0;
	const char* f = ele->Attribute( "flags" );

	if ( f ) {
		READ_FLAG( flags, f, MELEE_WEAPON );
		READ_FLAG( flags, f, RANGED_WEAPON );
		READ_FLAG( flags, f, INTRINSIC );
		READ_FLAG( flags, f, IMMUNE_FIRE );
		READ_FLAG( flags, f, FLAMMABLE );
		READ_FLAG( flags, f, IMMUNE_SHOCK );
		READ_FLAG( flags, f, SHOCKABLE );
		READ_FLAG( flags, f, EFFECT_EXPLOSIVE );
		READ_FLAG( flags, f, EFFECT_FIRE );
		READ_FLAG( flags, f, EFFECT_SHOCK );
		READ_FLAG( flags, f, RENDER_TRAIL );
		READ_FLAG( flags, f, INDESTRUCTABLE );
		READ_FLAG( flags, f, AI_WANDER_HERD );
		READ_FLAG( flags, f, AI_WANDER_CIRCLE );
		READ_FLAG( flags, f, AI_EAT_PLANTS );
		READ_FLAG( flags, f, AI_HEAL_AT_CORE );
		READ_FLAG( flags, f, AI_SECTOR_HERD );
		READ_FLAG( flags, f, AI_SECTOR_WANDER );
		READ_FLAG( flags, f, AI_USES_BUILDINGS );
		READ_FLAG( flags, f, AI_DOES_WORK );
		READ_FLAG( flags, f, GOLD_PICKUP );
		READ_FLAG( flags, f, ITEM_PICKUP );
		READ_FLAG( flags, f, CLICK_THROUGH );
	}
	for( const tinyxml2::XMLAttribute* attr = ele->FirstAttribute();
		 attr;
		 attr = attr->Next() )
	{
		IString name = StringPool::Intern( attr->Name() );
		if ( name == "name" || name == "properName" || name == "desc" || name == "resource" || name == "flags" ) {
			// handled above.
		}
		else if ( name == "hardpoint" || name == "hp" ) {
			// handled below
		}
		else if ( name == "cooldown" ) {
			cooldown.SetThreshold( attr->IntValue() );
		}
		else if ( name == "reload" ) {
			reload.SetThreshold( attr->IntValue() );
		}
		READ_FLOAT_ATTR( mass )
		READ_FLOAT_ATTR( hpRegen )
		READ_INT_ATTR( primaryTeam )
		READ_FLOAT_ATTR( meleeDamage )
		READ_FLOAT_ATTR( rangedDamage )
		READ_FLOAT_ATTR( absorbsDamage )
		READ_INT_ATTR( clipCap )
		READ_INT_ATTR( rounds )
		READ_FLOAT_ATTR( accruedFire )
		READ_FLOAT_ATTR( accruedShock )
		else {
			// What is it??? Tricky stuff.
			int integer=0;
			int real=0;
			int str=0;

			for( const char* p=attr->Value(); *p; ++p ) {
				if ( *p == '-' || isdigit(*p) || *p == '+' ) {
					integer++;
				}
				else if ( *p == '.' ) {
					real++;
				}
				else if ( isupper(*p) || islower(*p) ) {
					str++;
				}
			}

			if ( str )	
				keyValues.Set( name.c_str(), "S", StringPool::Intern( attr->Value() ));
			else if ( integer && real )
				keyValues.Set( name.c_str(), "f", atof( attr->Value()));
			else if ( integer )
				keyValues.Set( name.c_str(), "d", atoi( attr->Value()));
			else
				GLASSERT(0);
		}
	}

	if ( flags & EFFECT_FIRE )	flags |= IMMUNE_FIRE;

	hardpoint = 0;
	const char* h = ele->Attribute( "hardpoint" );
	if ( h ) {
		hardpoint = MetaDataToID( h );
		GLASSERT( hardpoint > 0 );
	}

	hp = this->TotalHPF();
	ele->QueryFloatAttribute( "hp", &hp );
	GLASSERT( hp <= TotalHP() );

	GLASSERT( TotalHP() > 0 );
	GLASSERT( hp > 0 );	// else should be destroyed
}


bool GameItem::Use( Chit* parentChit ) {
	if ( CanUse() ) {
		UseRound();
		cooldown.Use();
		if ( parentChit ) {
			parentChit->SetTickNeeded();
		}
		return true;
	}
	return false;
}


bool GameItem::Reload( Chit* parentChit ) {
	if ( CanReload()) {
		reload.Use();
		if ( parentChit ) {
			parentChit->SetTickNeeded();
		}
		return true;
	}
	return false;
}


void GameItem::UseRound() { 
	if ( clipCap > 0 ) { 
		GLASSERT( rounds > 0 ); 
		--rounds; 
	} 
}


// FIXME: pass in items affecting this one:
// The body affects the claw. Environment (water) affects the body.
int GameItem::DoTick( U32 delta )
{
	int tick = VERY_LONG_TICK;

	cooldown.Tick( delta );
	if ( reload.Tick( delta ) ) {
		rounds = clipCap;
	}

	if ( !cooldown.CanUse() || !reload.CanUse() ) {
		tick = 0;
	}

	float savedHP = hp;
	if ( flags & IMMUNE_FIRE )
		accruedFire = 0;
	if ( flags & IMMUNE_SHOCK )
		accruedShock = 0;

	accruedFire  = Min( accruedFire,  EFFECT_ACCRUED_MAX );
	accruedShock = Min( accruedShock, EFFECT_ACCRUED_MAX );

	float maxEffectDamage = Delta( delta, EFFECT_DAMAGE_PER_SEC );
	hp -= Min( accruedFire, maxEffectDamage );
	hp -= Min( accruedShock, maxEffectDamage );

	accruedFire -= maxEffectDamage * ((flags & FLAMMABLE) ? 0.1f : 1.0f);
	accruedFire = Max( 0.0f, accruedFire );
	
	accruedShock -= maxEffectDamage * ((flags & SHOCKABLE) ? 0.5f : 1.0f);
	accruedShock = Max( 0.0f, accruedShock );

	if ( hp > 0 ) {
		hp += Delta( delta, hpRegen );
	}
	hp = Clamp( hp, 0.0f, TotalHPF() );

	if ( flags & INDESTRUCTABLE ) {
		hp = TotalHPF();
	}

	if ( savedHP != hp ) {
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


void GameItem::AbsorbDamage( bool inInventory, DamageDesc dd, DamageDesc* remain, const char* log, const IMeleeWeaponItem* booster, Chit* parentChit )
{
	float absorbed = 0;
	int   effect = dd.effects;

	if ( !inInventory ) {
		// just regular item getting hit, that takes damage.
		absorbed = Min( dd.damage, hp );
		if ( dd.effects & EFFECT_FIRE )
			accruedFire += absorbed;
		if ( dd.effects & EFFECT_SHOCK )
			accruedShock += absorbed;
		hp -= absorbed;
	}
	else {
		// Items in the inventory don't take damage. They
		// may reduce damage for their parent.
		if ( ToShield() ) {
			reload.ResetUnready();
			absorbed = Min( dd.damage * absorbsDamage, (float)rounds );
			
			float cost = absorbed;
			if ( effect & EFFECT_SHOCK ) cost *= 0.5f;
			if ( booster ) {
				float boost = BattleMechanics::ComputeShieldBoost( booster );
				if ( boost > 1.0f ) {
					cost /= boost;
				}
			}

			rounds -= LRintf( cost );

			if ( rounds < 0 ) rounds = 0;

			// If the shield still has power, remove the effect.
			if ( rounds > 0 ) {
				if ( flags & EFFECT_FIRE )
					effect &= (~EFFECT_FIRE);
				if ( flags & EFFECT_SHOCK )
					effect &= (~EFFECT_SHOCK);
			}
		}
		else {
			// Something that straight up reduces damage.
			absorbed = dd.damage * absorbsDamage;
		}
	}
	if ( remain ) {
		remain->damage = dd.damage - absorbed;
		remain->effects = effect;
		GLASSERT( remain->damage >= 0 );
	}
	if ( absorbed ) {
		if ( inInventory ) {
			GLLOG(( "Damage Absorbed %s absorbed=%.1f ", name.c_str(), absorbed ));
		}
		else {
			GLLOG(( "Damage %s total=%.1f hp=%.1f accFire=%.1f accShock=%.1f ", name.c_str(), absorbed, hp, accruedFire, accruedShock ));
		}
	}
	if ( parentChit ) {
		parentChit->SetTickNeeded();
	}
}



int GameItem::GetValue() const
{
	static const float EFFECT_BONUS = 1.5f;
	static const float MELEE_VALUE  = 20;
	static const float RANGED_VALUE = 30;
	static const float SHIELD_VALUE = 20;
	
	if ( value < 0 ) { 
		CArray<int, 16> valueArr;
		valueArr.Push( 0 );
		value = 0;

		if ( ToMeleeWeapon() ) {
			float dptu = BattleMechanics::MeleeDPTU( 0, ToMeleeWeapon() );

			const GameItem& basic = ItemDefDB::Instance()->Get( "ring" );
			float refDPTU = BattleMechanics::MeleeDPTU( 0, basic.ToMeleeWeapon() );

			float v = dptu / refDPTU * MELEE_VALUE;
			if ( 
				flags & GameItem::EFFECT_FIRE ) v *= EFFECT_BONUS;
			if ( flags & GameItem::EFFECT_SHOCK ) v *= EFFECT_BONUS;
			if ( flags & GameItem::EFFECT_EXPLOSIVE ) v *= EFFECT_BONUS;
			valueArr.Push( (int)v );
		}

		if ( ToRangedWeapon() ) {
			float radAt1 = BattleMechanics::ComputeRadAt1( 0, ToRangedWeapon(), false, false );
			float dptu = BattleMechanics::RangedDPTU( ToRangedWeapon(), true );
			float er   = BattleMechanics::EffectiveRange( radAt1 );

			const GameItem& basic = ItemDefDB::Instance()->Get( "blaster" );
			float refRadAt1 = BattleMechanics::ComputeRadAt1( 0, basic.ToRangedWeapon(), false, false );
			float refDPTU = BattleMechanics::RangedDPTU( basic.ToRangedWeapon(), true );
			float refER   = BattleMechanics::EffectiveRange( refRadAt1 );

			float v = ( dptu * er ) / ( refDPTU * refER ) * RANGED_VALUE;
			if ( flags & GameItem::EFFECT_FIRE ) v *= EFFECT_BONUS;
			if ( flags & GameItem::EFFECT_SHOCK ) v *= EFFECT_BONUS;
			if ( flags & GameItem::EFFECT_EXPLOSIVE ) v *= EFFECT_BONUS;
			valueArr.Push( (int)v );
		}

		if ( ToShield() ) {
			int rounds = ClipCap();
			const GameItem& basic = ItemDefDB::Instance()->Get( "shield" );
			int refRounds = basic.ClipCap();

			// Currently, shield effects don't do anything, although that would be cool.
			float v = float(rounds) / float(refRounds) * SHIELD_VALUE;
			valueArr.Push( int(v) );
		}
		if ( !valueArr.Empty() ) {
			value = valueArr.Max();
		}
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
	// use the 'id', not ID() so we don't allocate:
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
	// use the 'id', not ID() so we don't allocate:
	if ( id == 0 ) return;

	ItemDB* db = ItemDB::Instance();
	if ( db ) {
		db->Remove( this );
	}
}



void DamageDesc::Log()
{
	GLLOG(( "[damage=%.1f]", damage ));
}


