#include "gameitem.h"
#include "../grinliz/glstringutil.h"
#include "../tinyxml2/tinyxml2.h"
#include "../xegame/inventorycomponent.h"

using namespace grinliz;
using namespace tinyxml2;

#define READ_FLAG( flags, str, name ) { if ( strstr( str, #name )) flags |= name; }

void GameItem::Save( tinyxml2::XMLPrinter* )
{
	GLASSERT( 0 );
}


void GameItem::Load( const tinyxml2::XMLElement* ele )
{
	this->CopyFrom( 0 );

	GLASSERT( StrEqual( ele->Name(), "item" ));
	
	name		= ele->Attribute( "name" );
	resource	= ele->Attribute( "resource" );
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
	READ_FLAG( flags, f, IMMUNE_ENERGY );
	READ_FLAG( flags, f, EFFECT_FIRE );
	READ_FLAG( flags, f, EFFECT_ENERGY );

	if ( EFFECT_FIRE )	flags |= IMMUNE_FIRE;
	if ( EFFECT_ENERGY)	flags |= IMMUNE_ENERGY;

	ele->QueryFloatAttribute( "mass",			&mass );
	ele->QueryFloatAttribute( "power",			&power );
	ele->QueryIntAttribute( "primaryTeam",		&primaryTeam );
	ele->QueryUnsignedAttribute( "coolDownTime",&coolDownTime );

	const XMLElement* meleeEle = ele->FirstChildElement( "melee" );
	if ( meleeEle ) {
		meleeDamage.Load( "melee", meleeEle );
	}
	const XMLElement* rangedEle = ele->FirstChildElement( "ranged" );
	if ( rangedEle ) {
		rangedDamage.Load( "ranged", rangedEle );
	}

	hardpoint = NO_HARDPOINT;
	const char* h = ele->Attribute( "hardpoint" );
	if ( h ) {
		hardpoint = InventoryComponent::HardpointNameToFlag( h );
		GLASSERT( hardpoint >= 0 );
	}

	hp = mass;
	ele->QueryFloatAttribute( "hp", &hp );
}


void GameItem::Apply( const GameItem* intrinsic )
{
	if ( intrinsic->flags & EFFECT_FIRE )
		flags |= IMMUNE_FIRE;
	if ( intrinsic->flags & EFFECT_ENERGY )
		flags |= IMMUNE_ENERGY;
}


void DamageDesc::Save( const char* prefix, tinyxml2::XMLPrinter* )
{
	GLASSERT( 0 );	// FIXME
}


void DamageDesc::Load( const char* prefix, const tinyxml2::XMLElement* doc )
{
	components.Zero();

	doc->QueryFloatAttribute( "kinetic", &components[KINETIC] );
	doc->QueryFloatAttribute( "energy", &components[ENERGY] );
	doc->QueryFloatAttribute( "fire", &components[FIRE] );
}
