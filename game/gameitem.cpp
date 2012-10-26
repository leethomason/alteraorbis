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

#include "../xegame/inventorycomponent.h"
#include "../xegame/chit.h"

using namespace grinliz;
using namespace tinyxml2;

#define READ_FLAG( flags, str, name )	{ if ( strstr( str, #name )) flags |= name; }
#define READ_FLOAT_ATTR( ele, name )	{ ele->QueryFloatAttribute( #name, &name ); }
#define READ_INT_ATTR( ele, name )		{ ele->QueryIntAttribute( #name, &name ); }
#define READ_UINT_ATTR( ele, name )		{ ele->QueryUnsignedAttribute( #name, &name ); }


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
	READ_FLAG( flags, f, EFFECT_ENERGY );
	READ_FLAG( flags, f, EFFECT_EXPLOSIVE );
	READ_FLAG( flags, f, EFFECT_FIRE );
	READ_FLAG( flags, f, EFFECT_SHOCK );
	READ_FLAG( flags, f, RENDER_TRAIL );
	READ_FLAG( flags, f, SHIELD );

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

	if ( EFFECT_FIRE )	flags |= IMMUNE_FIRE;

	hardpoint = NO_HARDPOINT;
	const char* h = ele->Attribute( "hardpoint" );
	if ( h ) {
		hardpoint = InventoryComponent::HardpointNameToFlag( h );
		GLASSERT( hardpoint >= 0 );
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


bool GameItem::DoTick( U32 delta )
{
	cooldownTime += delta;
	if ( reloadTime < reload ) {
		reloadTime += delta;

		if ( reloadTime >= reload ) {
			rounds = clipCap;
		}
	}

	bool tick = false;

	if (    TotalHP() 
		 && hpRegen != 0
		 && ( hp != TotalHP() || hp != 0 ) ) 
	{
		hp += hpRegen * (float)delta / 1000.0f;
		tick = true;
	}

	if ( parentChit ) {
		parentChit->SendMessage( ChitMsg( ChitMsg::GAMEITEM_TICK, 0, this ), 0 );
	}
	return !Ready() || Reloading() || tick;	
}


void GameItem::Apply( const GameItem* intrinsic )
{
	if ( intrinsic->flags & EFFECT_FIRE )
		flags |= IMMUNE_FIRE;
//	if ( intrinsic->flags & EFFECT_ENERGY )
//		flags |= IMMUNE_ENERGY;
}


void GameItem::AbsorbDamage( bool inInventory, const DamageDesc& dd, DamageDesc* remain, const char* log )
{
	float absorbed = 0;

	if ( !inInventory ) {
		// just regular item getting hit, that takes damage.
		absorbed = Min( dd.damage, hp );
		hp -= absorbed;
	}
	else {
		// Items in the inventory don't take damage. They
		// may reduce damage for there parent.
		if ( flags & SHIELD ) {
			reloadTime = 0;
			absorbed = Min( dd.damage * absorbsDamage, (float)rounds );
			rounds -= LRintf( absorbed );
			if ( rounds < 0 ) rounds = 0;
		}
		else {
			// Something that straight up reduces damage.
			absorbed = dd.damage * absorbsDamage;
		}
	}
	if ( remain ) {
		remain->damage = dd.damage - absorbed;
		GLASSERT( remain->damage >= 0 );
	}
	if ( absorbed ) {
		if ( inInventory ) {
			GLLOG(( "Damage Absorbed %s absorbed=%.1f ", name.c_str(), absorbed ));
		}
		else {
			GLLOG(( "Damage %s total=%.1f hp=%.1f ", name.c_str(), absorbed, hp ));
		}
	}
}


/*void DamageDesc::Save( const char* prefix, tinyxml2::XMLPrinter* )
{
	GLASSERT( 0 );	// FIXME
}


void DamageDesc::Load( const char* prefix, const tinyxml2::XMLElement* doc )
{
	components.Zero();

	doc->QueryFloatAttribute( "kinetic", &components[KINETIC] );
	doc->QueryFloatAttribute( "energy", &components[ENERGY] );
	doc->QueryFloatAttribute( "fire", &components[FIRE] );
	doc->QueryFloatAttribute( "shock", &components[SHOCK] );
}
*/

void DamageDesc::Log()
{
	GLLOG(( "[damage=%.1f]", damage ));
}
