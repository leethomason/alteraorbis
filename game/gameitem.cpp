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
#define READ_FLOAT_ATTR( ele, name )	{ ele->QueryFloatAttribute( #name, &name ); }
#define READ_INT_ATTR( ele, name )		{ ele->QueryIntAttribute( #name, &name ); }
#define READ_UINT_ATTR( ele, name )		{ ele->QueryUnsignedAttribute( #name, &name ); }


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


void GameItem::Save( tinyxml2::XMLPrinter* )
{
	GLASSERT( 0 );
}


void GameItem::Load( const tinyxml2::XMLElement* ele )
{
	this->CopyFrom( 0 );

	GLASSERT( StrEqual( ele->Name(), "item" ));
	
	name		= StringPool::Intern( ele->Attribute( "name" ));
	resource	= StringPool::Intern( ele->Attribute( "resource" ));
	flags = 0;
	const char* f = ele->Attribute( "flags" );

	READ_FLAG( flags, f, CHARACTER );
	READ_FLAG( flags, f, MELEE_WEAPON );
	READ_FLAG( flags, f, RANGED_WEAPON );
	READ_FLAG( flags, f, INTRINSIC_AT_HARDPOINT );
	READ_FLAG( flags, f, INTRINSIC_FREE );
	READ_FLAG( flags, f, HELD_AT_HARDPOINT );
	READ_FLAG( flags, f, HELD_FREE );
	READ_FLAG( flags, f, IMMUNE_FIRE );
	READ_FLAG( flags, f, FLAMMABLE );
	READ_FLAG( flags, f, EFFECT_EXPLOSIVE );
	READ_FLAG( flags, f, EFFECT_FIRE );
	READ_FLAG( flags, f, EFFECT_SHOCK );
	READ_FLAG( flags, f, RENDER_TRAIL );

	READ_FLOAT_ATTR( ele, mass );
	READ_FLOAT_ATTR( ele, hpPerMass );
	READ_FLOAT_ATTR( ele, hpRegen );
	READ_INT_ATTR( ele, primaryTeam );
	READ_UINT_ATTR( ele, cooldown );
	READ_UINT_ATTR( ele, cooldownTime );
	READ_UINT_ATTR( ele, reload );
	READ_UINT_ATTR( ele, reloadTime );
	READ_INT_ATTR( ele, clipCap );
	READ_INT_ATTR( ele, rounds );
	READ_FLOAT_ATTR( ele, speed );
	READ_FLOAT_ATTR( ele, meleeDamage );
	READ_FLOAT_ATTR( ele, rangedDamage );
	READ_FLOAT_ATTR( ele, absorbsDamage );
	READ_FLOAT_ATTR( ele, accruedFire );
	READ_FLOAT_ATTR( ele, accruedShock );

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

	hp = this->TotalHP();
	ele->QueryFloatAttribute( "hp", &hp );

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
bool GameItem::DoTick( U32 delta )
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
