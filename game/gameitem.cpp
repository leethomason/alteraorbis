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
#include "../script/procedural.h"

#include "../xegame/inventorycomponent.h"
#include "../xegame/chit.h"

using namespace grinliz;
using namespace tinyxml2;

#define READ_FLAG( flags, str, name )	{ if ( strstr( str, #name )) flags |= name; }

#define READ_FLOAT_ATTR( n )			else if ( name == #n ) { n = attr->FloatValue(); }
#define READ_INT_ATTR( n )				else if ( name == #n ) { n = attr->IntValue(); }

#define APPEND_FLAG( flags, cstr, name )	{ if ( flags & name ) f += #name; f += " "; }
#define PUSH_ATTRIBUTE( prnt, name )		{ prnt->PushAttribute( #name, name ); }

// FIXME: make this way, way simpler
float AbilityCurve( float yAt0, float yAt1, float yAt16, float yAt32, float x )
{
	GLASSERT( grinliz::InRange( x, 0.0f, 32.0f ));
	if ( x < 1 ) {
		return grinliz::Lerp( yAt0, yAt1, x );
	}
	else if ( x < 16 ) {
		return grinliz::Lerp( yAt1, yAt16, (x-1.0f)/15.0f );
	}
	return grinliz::Lerp( yAt16, yAt32, (x-16.0f)/16.0f );
}


void GameStat::Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele )
{
	XEArchiveT( prn, ele, "STR",  trait[STR] );
	XEArchiveT( prn, ele, "WILL", trait[WILL] );
	XEArchiveT( prn, ele, "CHR",  trait[CHR] );
	XEArchiveT( prn, ele, "INT",  trait[INT] );
	XEArchiveT( prn, ele, "DEX",  trait[DEX] );
	XEArchiveT( prn, ele, "exp",  exp );
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
	APPEND_FLAG( flags, f, CHARACTER );
	APPEND_FLAG( flags, f, MELEE_WEAPON );
	APPEND_FLAG( flags, f, RANGED_WEAPON );
	APPEND_FLAG( flags, f, INTRINSIC_AT_HARDPOINT );
	APPEND_FLAG( flags, f, INTRINSIC_FREE );
	APPEND_FLAG( flags, f, HELD_AT_HARDPOINT );
	APPEND_FLAG( flags, f, HELD_FREE );
	APPEND_FLAG( flags, f, IMMUNE_FIRE );
	APPEND_FLAG( flags, f, FLAMMABLE );
	APPEND_FLAG( flags, f, IMMUNE_SHOCK );
	APPEND_FLAG( flags, f, SHOCKABLE );
	APPEND_FLAG( flags, f, EFFECT_EXPLOSIVE );
	APPEND_FLAG( flags, f, EFFECT_FIRE );
	APPEND_FLAG( flags, f, EFFECT_SHOCK );
	APPEND_FLAG( flags, f, RENDER_TRAIL );
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

	if ( hardpoint != NO_HARDPOINT ) {
		printer->PushAttribute( "hardpoint", InventoryComponent::HardpointFlagToName( hardpoint ).c_str() );
	}
	if ( procedural != PROCEDURAL_NONE ) {
		printer->PushAttribute( "procedural", ItemGen::ToName( procedural ).c_str() );
	}
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
		READ_FLAG( flags, f, CHARACTER );
		READ_FLAG( flags, f, MELEE_WEAPON );
		READ_FLAG( flags, f, RANGED_WEAPON );
		READ_FLAG( flags, f, INTRINSIC_AT_HARDPOINT );
		READ_FLAG( flags, f, INTRINSIC_FREE );
		READ_FLAG( flags, f, HELD_AT_HARDPOINT );
		READ_FLAG( flags, f, HELD_FREE );
		READ_FLAG( flags, f, IMMUNE_FIRE );
		READ_FLAG( flags, f, FLAMMABLE );
		READ_FLAG( flags, f, IMMUNE_SHOCK );
		READ_FLAG( flags, f, SHOCKABLE );
		READ_FLAG( flags, f, EFFECT_EXPLOSIVE );
		READ_FLAG( flags, f, EFFECT_FIRE );
		READ_FLAG( flags, f, EFFECT_SHOCK );
		READ_FLAG( flags, f, RENDER_TRAIL );
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
		else {
			KeyValue kv = { name, attr->DoubleValue() };
			keyValues.Push( kv );
		}
	}

	if ( flags & EFFECT_FIRE )	flags |= IMMUNE_FIRE;

	hardpoint = NO_HARDPOINT;
	const char* h = ele->Attribute( "hardpoint" );
	if ( h ) {
		hardpoint = InventoryComponent::HardpointNameToFlag( h );
		GLASSERT( hardpoint >= 0 );
	}
	procedural = PROCEDURAL_NONE;
	const char* p = ele->Attribute( "procedural" );
	if ( p ) {
		procedural = ItemGen::ToID( StringPool::Intern( p ) );
	}

	stats.Load( ele );

	hp = this->TotalHP();
	ele->QueryFloatAttribute( "hp", &hp );
	GLASSERT( hp <= TotalHP() );

	GLASSERT( TotalHP() > 0 );
	GLASSERT( hp > 0 );	// else should be destroyed
}


bool GameItem::Use() {
	if ( Ready() && HasRound() ) {
		UseRound();
		cooldownTime = 0;
		if ( parentChit ) {
			parentChit->SetTickNeeded();
		}
		return true;
	}
	return false;
}


bool GameItem::Reload() {
	if ( CanReload()) {
		reloadTime = 0;
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
	bool tick = false;

	cooldownTime += delta;
	if ( reloadTime < reload ) {
		reloadTime += delta;
		tick = true;

		if ( reloadTime >= reload ) {
			rounds = clipCap;
		}
	}

	float savedHP = hp;
	if ( accruedFire > 0 ) {
		int debug=1;
	}
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
	hp = Clamp( hp, 0.0f, TotalHP() );

	tick = tick || (savedHP != hp);

	if ( parentChit ) {
		parentChit->SendMessage( ChitMsg( ChitMsg::GAMEITEM_TICK, 0, this ), 0 );
	}
	return tick ? 0 : VERY_LONG_TICK;	
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
			reloadTime = 0;
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
}


void DamageDesc::Log()
{
	GLLOG(( "[damage=%.1f]", damage ));
}
