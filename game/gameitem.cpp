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
	if ( parentChit ) {
		parentChit->SendMessage( ChitMsg( ChitMsg::GAMEITEM_TICK, 0, this ), 0 );
	}
	return !Ready() || Reloading();	
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
	float absorb = inInventory ? this->absorbsDamage : 1.0f;

	if ( absorb != 0 ) {
		float d = dd.damage * absorb;
		if ( d > 0 && TotalHP() ) {
			d = Min( d, hp );
			hp -= d;
			if ( parentChit ) parentChit->SetTickNeeded();
		}
		if ( remain ) {
			remain->damage = Max( remain->damage - d, 0.0f );
		}
		GLLOG(( "Damage %s total=%.1f hp=%.1f", inInventory ? "Inventory" : "", d, hp ));
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
