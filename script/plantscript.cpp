#include "plantscript.h"
#include "itemscript.h"
#include "../engine/serialize.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/chit.h"
#include "../xegame/rendercomponent.h"

PlantScript::PlantScript( Engine* p_engine, WorldMap* p_map, int p_type ) : 
	engine( p_engine ), 
	worldMap( p_map ), 
	type( p_type )
{
	stage = 0;
	age = 0;
	ageAtStage = 0;
}


void PlantScript::Init( const ScriptContext& ctx )
{
	SetRenderComponent();

	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	const GameItem& treeItem = itemDefDB->Get( "tree" );
	ctx.chit->Add( new ItemComponent( treeItem ));
	ctx.chit->Add( new RenderComponent( engine, "plant1.3" ));
}


void PlantScript::Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele )
{
	XE_ARCHIVE( stage );
	XE_ARCHIVE( age );
	XE_ARCHIVE( ageAtStage );
}


void PlantScript::Load( const ScriptContext& ctx, const tinyxml2::XMLElement* element )
{
	stage = 0;
	age = 0;
	ageAtStage = 0;
	GLASSERT( element );
	Archive( 0, element );
}


void PlantScript::Save( const ScriptContext& ctx, tinyxml2::XMLPrinter* printer )
{
	printer->OpenElement( "PlantScript" );
	Archive( printer, 0 );
	printer->CloseElement();
}


bool PlantScript::DoTick( const ScriptContext& ctx, U32 delta )
{
	return true;
}


