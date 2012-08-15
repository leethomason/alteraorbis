#include "gameitem.h"
#include "../grinliz/glstringutil.h"
#include "../tinyxml2/tinyxml2.h"

using namespace grinliz;
using namespace tinyxml2;

#define READ_FLAG( flags, str, name ) { if ( strstr( str, #name )) flags |= name; }

void GameItem::Save( tinyxml2::XMLPrinter* )
{
	GLASSERT( 0 );
}


void GameItem::Load( const tinyxml2::XMLElement* ele )
{
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
	READ_FLAG( flags, f, HARDPOINT_TRIGGER );
	READ_FLAG( flags, f, HARDPOINT_ALTHAND );
	READ_FLAG( flags, f, HARDPOINT_HEAD );
	READ_FLAG( flags, f, HARDPOINT_SHIELD );
	READ_FLAG( flags, f, IMMUNE_FIRE );
	READ_FLAG( flags, f, FLAMMABLE );
	READ_FLAG( flags, f, IMMUNE_ENERGY );
	READ_FLAG( flags, f, EFFECT_FIRE );
	READ_FLAG( flags, f, EFFECT_ENERGY );

	if ( EFFECT_FIRE )	flags |= IMMUNE_FIRE;
	if ( EFFECT_ENERGY)	flags |= IMMUNE_ENERGY;

	primaryTeam = 0;
	ele->QueryIntAttribute( "primaryTeam", &primaryTeam );
	coolDownTime = 1000;
	ele->QueryUnsignedAttribute( "coolDownTime", &coolDownTime );

	const char* hardpoint = ele->Attribute( "hardpoint" );
	if ( StrEqual( hardpoint, "trigger" ))	flags |= HARDPOINT_TRIGGER;
	if ( StrEqual( hardpoint, "althand" ))	flags |= HARDPOINT_ALTHAND;
	if ( StrEqual( hardpoint, "head" ))		flags |= HARDPOINT_HEAD;
	if ( StrEqual( hardpoint, "shield" ))	flags |= HARDPOINT_SHIELD;
}


void GameItem::Apply( const GameItem* intrinsic )
{
	if ( intrinsic->flags & EFFECT_FIRE )
		flags |= IMMUNE_FIRE;
	if ( intrinsic->flags & EFFECT_ENERGY )
		flags |= IMMUNE_ENERGY;
}


