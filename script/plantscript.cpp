#include "plantscript.h"
#include "itemscript.h"

#include "../engine/serialize.h"
#include "../engine/model.h"

#include "../xegame/itemcomponent.h"
#include "../xegame/chit.h"
#include "../xegame/rendercomponent.h"

#include "../grinliz/glstringutil.h"

using namespace grinliz;

PlantScript::PlantScript( Engine* p_engine, WorldMap* p_map, int p_type ) : 
	engine( p_engine ), 
	worldMap( p_map ), 
	type( p_type )
{
	stage = 0;
	age = 0;
	ageAtStage = 0;
}


void PlantScript::SetRenderComponent( Chit* chit )
{
	CStr<10> str = "plant0.0";
	str[5] = '0' + type;
	str[7] = '0' + stage;

	if ( chit->GetRenderComponent() ) {
		const ModelResource* res = chit->GetRenderComponent()->MainResource();
		if ( res && res->header.name == str.c_str() ) {
			// No change!
			return;
		}
	}

	if ( chit->GetRenderComponent() ) {
		chit->Remove( chit->GetRenderComponent() );
	}

	chit->Add( new RenderComponent( engine, str.c_str() ));
}


void PlantScript::Init( const ScriptContext& ctx )
{
	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	const GameItem& treeItem = itemDefDB->Get( "tree" );
	ctx.chit->Add( new ItemComponent( treeItem ));

	SetRenderComponent( ctx.chit );
}


void PlantScript::Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele )
{
	XE_ARCHIVE( type );
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


