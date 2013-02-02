#include "plantscript.h"

PlantScript::PlantScript( WorldMap* p_map, int p_type ) : worldMap( p_map ), type( p_type )
{
	stage = 0;
	age = 0;
	ageAtStage = 0;
}


void PlantScript::Init( const ScriptContext& ctx )
{
	SetRenderComponent();
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


void PlantScript::Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele )
{

}

