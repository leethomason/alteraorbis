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

#include "../tinyxml2/tinyxml2.h"
#include "../xarchive/glstreamer.h"
#include "../script/procedural.h"

#include "../xegame/chit.h"
#include "../xegame/istringconst.h"

using namespace grinliz;
using namespace tinyxml2;

#define READ_FLAG( flags, str, name )	{ if ( strstr( str, #name )) flags |= name; }

#define READ_FLOAT_ATTR( n )			else if ( name == #n ) { n = attr->FloatValue(); }
#define READ_INT_ATTR( n )				else if ( name == #n ) { n = attr->IntValue(); }
#define READ_BOOL_ATTR( n )				else if ( name == #n ) { n = attr->BoolValue(); }

#define APPEND_FLAG( flags, cstr, name )	{ if ( flags & name ) { f += #name; f += " "; } }
#define PUSH_ATTRIBUTE( prnt, name )		{ prnt->PushAttribute( #name, name ); }


void GameStat::Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele )
{
	XEArchiveT( prn, ele, "STR",  trait[STR] );
	XEArchiveT( prn, ele, "WILL", trait[WILL] );
	XEArchiveT( prn, ele, "CHR",  trait[CHR] );
	XEArchiveT( prn, ele, "INT",  trait[INT] );
	XEArchiveT( prn, ele, "DEX",  trait[DEX] );
	XEArchiveT( prn, ele, "exp",  exp );
}



void GameStat::Serialize( XStream* xs )
{
	XarcOpen( xs, "stat" );
	if ( xs->Saving() ) {
		xs->Saving()->SetArr( "trait", trait, NUM_TRAITS );
	}
	else {
		const StreamReader::Attribute* attr = xs->Loading()->Get( "trait" );
		GLASSERT( attr );
		xs->Loading()->Value( attr, trait, NUM_TRAITS );
	}
	XARC_SER( xs, exp );

	XarcClose( xs );
}


void GameStat::Save( tinyxml2::XMLPrinter* printer )
{
	printer->OpenElement( "GameStat" );
	Archive( printer, 0 );
	printer->CloseElement();
}


void GameStat::Load( const tinyxml2::XMLElement* parent )
{
	const tinyxml2::XMLElement* ele = parent->FirstChildElement( "GameStat" );
	if ( ele ) {
		Archive( 0, ele );
	}
}


void GameStat::Roll( U32 seed )
{
	Random random( seed );
	for( int i=0; i<NUM_TRAITS; ++i ) {
		trait[i] = random.Dice( 3, 6 );
	}
}


void GameItem::Serialize( XStream* xs )
{
	XarcOpen( xs, "GameItem" );

	XARC_SER( xs, name );
	XARC_SER( xs, desc );
	XARC_SER( xs, resource );
	XARC_SER( xs, mass );
	XARC_SER_DEF( xs, hpRegen, 0.0f );
	XARC_SER_DEF( xs, primaryTeam, 0 );
	XARC_SER_DEF( xs, cooldown, 0 );
	XARC_SER_DEF( xs, cooldownTime, 0 );
	XARC_SER_DEF( xs, reload, 0 );
	XARC_SER_DEF( xs, reloadTime, 0 );
	XARC_SER_DEF( xs, clipCap, 0 );
	XARC_SER_DEF( xs, rounds, 0 );
	XARC_SER_DEF( xs, meleeDamage, 1.0f );
	XARC_SER_DEF( xs, rangedDamage, 0.0f );
	XARC_SER_DEF( xs, absorbsDamage, 0 );
	XARC_SER_DEF( xs, accruedFire, 0 );
	XARC_SER_DEF( xs, accruedShock, 0 );

	XARC_SER( xs, hardpoint );
	XARC_SER( xs, isHeld );
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
		APPEND_FLAG( flags, f, AI_SECTOR_HERD );
		APPEND_FLAG( flags, f, AI_BINDS_TO_CORE );
		APPEND_FLAG( flags, f, AI_DOES_WORK );
		//APPEND_FLAG( flags, f, VISITOR );
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
			READ_FLAG( flags, f, AI_SECTOR_HERD );
			READ_FLAG( flags, f, AI_BINDS_TO_CORE );
			READ_FLAG( flags, f, AI_DOES_WORK );
			//READ_FLAG( flags, f, VISITOR );
			READ_FLAG( flags, f, CLICK_THROUGH );
		}
	}

	XarcOpen( xs, "keyval" );
	int n = keyValues.Size();
	XARC_SER( xs, n );
	if ( xs->Loading() ) {
		GLASSERT( keyValues.Empty() );
		keyValues.PushArr( n );
	}
	for( int i=0; i<n; ++i ) {
		XarcOpen( xs, "key" );
		XARC_SER_KEY( xs, "k", keyValues[i].key   );
		XARC_SER_KEY( xs, "v", keyValues[i].value );
		XarcClose( xs );
	}
	XarcClose( xs );

	stats.Serialize( xs );
	XarcClose( xs );
}


void GameItem::Save( tinyxml2::XMLPrinter* printer )
{
	printer->OpenElement( "item" );

	printer->PushAttribute( "name", name.c_str() );
	if ( !desc.empty() ) {
		printer->PushAttribute( "desc", desc.c_str() );
	}
	if ( !resource.empty() ) {
		printer->PushAttribute( "resource", resource.c_str() );
	}
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
	APPEND_FLAG( flags, f, AI_SECTOR_HERD );
	APPEND_FLAG( flags, f, AI_BINDS_TO_CORE );
	APPEND_FLAG( flags, f, AI_DOES_WORK );
	//APPEND_FLAG( flags, f, VISITOR );
	APPEND_FLAG( flags, f, CLICK_THROUGH );
	printer->PushAttribute( "flags", f.c_str() );
	
	PUSH_ATTRIBUTE( printer, mass );
	PUSH_ATTRIBUTE( printer, hpRegen );
	PUSH_ATTRIBUTE( printer, primaryTeam );
	PUSH_ATTRIBUTE( printer, cooldown );
	PUSH_ATTRIBUTE( printer, cooldownTime );
	PUSH_ATTRIBUTE( printer, reload );
	PUSH_ATTRIBUTE( printer, reloadTime );
	PUSH_ATTRIBUTE( printer, clipCap );
	PUSH_ATTRIBUTE( printer, rounds );
	PUSH_ATTRIBUTE( printer, meleeDamage );
	PUSH_ATTRIBUTE( printer, rangedDamage );
	PUSH_ATTRIBUTE( printer, absorbsDamage );
	PUSH_ATTRIBUTE( printer, accruedFire );
	PUSH_ATTRIBUTE( printer, accruedShock );

	for( int i=0; i<keyValues.Size(); ++i ) {
		printer->PushAttribute( keyValues[i].key.c_str(), keyValues[i].value );
	}

	if ( hardpoint != 0 ) {
		printer->PushAttribute( "hardpoint", IStringConst::Hardpoint( hardpoint ).c_str() );
	}
	printer->PushAttribute( "isHeld", isHeld );
	GLASSERT( hp <= TotalHP() );
	printer->PushAttribute( "hp", hp );

	stats.Save( printer );

	printer->CloseElement();	// item
}
	
void GameItem::Load( const tinyxml2::XMLElement* ele )
{
	this->CopyFrom( 0 );

	GLASSERT( StrEqual( ele->Name(), "item" ));
	
	name		= StringPool::Intern( ele->Attribute( "name" ));
	desc		= StringPool::Intern( ele->Attribute( "desc" ));
	resource	= StringPool::Intern( ele->Attribute( "resource" ));
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
		READ_FLAG( flags, f, AI_SECTOR_HERD );
		READ_FLAG( flags, f, AI_BINDS_TO_CORE );
		READ_FLAG( flags, f, AI_DOES_WORK );
		//READ_FLAG( flags, f, VISITOR );
		READ_FLAG( flags, f, CLICK_THROUGH );
	}
	for( const tinyxml2::XMLAttribute* attr = ele->FirstAttribute();
		 attr;
		 attr = attr->Next() )
	{
		IString name = StringPool::Intern( attr->Name() );
		if ( name == "name" || name == "desc" || name == "resource" || name == "flags" ) {
			// handled above.
		}
		else if ( name == "hardpoint" || name == "procedural" || name == "hp" ) {
			// handled below
		}
		READ_FLOAT_ATTR( mass )
		READ_FLOAT_ATTR( hpRegen )
		READ_INT_ATTR( primaryTeam )
		READ_FLOAT_ATTR( meleeDamage )
		READ_FLOAT_ATTR( rangedDamage )
		READ_FLOAT_ATTR( absorbsDamage )
		READ_INT_ATTR( cooldown )
		READ_INT_ATTR( reload )
		READ_INT_ATTR( clipCap )
		READ_INT_ATTR( cooldownTime )
		READ_INT_ATTR( reloadTime )
		READ_INT_ATTR( rounds )
		READ_FLOAT_ATTR( accruedFire )
		READ_FLOAT_ATTR( accruedShock )
		READ_BOOL_ATTR( isHeld )
		else {
			KeyValue kv = { name, attr->DoubleValue() };
			keyValues.Push( kv );
		}
	}

	if ( flags & EFFECT_FIRE )	flags |= IMMUNE_FIRE;

	hardpoint = 0;
	const char* h = ele->Attribute( "hardpoint" );
	if ( h ) {
		hardpoint = IStringConst::Hardpoint( StringPool::Intern( h ) );
		GLASSERT( hardpoint > 0 );
	}
	stats.Load( ele );

	hp = this->TotalHPF();
	ele->QueryFloatAttribute( "hp", &hp );
	GLASSERT( hp <= TotalHP() );

	GLASSERT( TotalHP() > 0 );
	GLASSERT( hp > 0 );	// else should be destroyed
}


bool GameItem::Use() {
	if ( CanUse() ) {
		UseRound();
		cooldownTime = cooldown;
		if ( parentChit ) {
			parentChit->SetTickNeeded();
		}
		return true;
	}
	return false;
}


bool GameItem::Reload() {
	if ( CanReload()) {
		reloadTime = reload;
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
int GameItem::DoTick( U32 delta, U32 sinec )
{
	int tick = VERY_LONG_TICK;

	if ( cooldownTime > 0 ) {
		cooldownTime = Max( cooldownTime-(int)delta, 0 );
		tick = 0;
	}

	if ( reloadTime > 0 ) {
		reloadTime = Max( reloadTime-(int)delta, 0 );
		tick = 0;

		if ( reloadTime == 0 ) {
			rounds = clipCap;
		}
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

	hp += Delta( delta, hpRegen );
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


void GameItem::AbsorbDamage( bool inInventory, DamageDesc dd, DamageDesc* remain, const char* log )
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
			reloadTime = reload;
			absorbed = Min( dd.damage * absorbsDamage, (float)rounds );
			rounds -= LRintf( absorbed );
			// Shock does extra damage to shields.
			if ( effect & EFFECT_SHOCK ) {
				rounds -= LRintf( absorbed * 0.5f );
			}
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


void DamageDesc::Log()
{
	GLLOG(( "[damage=%.1f]", damage ));
}
