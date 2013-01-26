#include "volcanoscript.h"
#include "../engine/serialize.h"

using namespace grinliz;
using namespace tinyxml2;

VolcanoScript::VolcanoScript( WorldMap* p_map )
{
	worldMap = p_map;
	origin.Zero();
}


void VolcanoScript::SetOrigin( int x, int y )
{
	origin.Set( x, y );
}


void VolcanoScript::OnAdd( const ScriptContext& heap )
{
	GLOUTPUT(( "VolcanoScript::OnAdd\n" ));
}


void VolcanoScript::OnRemove( const ScriptContext& heap )
{
	GLOUTPUT(( "VolcanoScript::OnRemove\n" ));
}


void VolcanoScript::Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele )
{
	XEArchive( prn, ele, "origin", origin );
}


void VolcanoScript::Load( const ScriptContext& ctx, const tinyxml2::XMLElement* parent )
{
	const XMLElement* child = parent->FirstChildElement( "VolcanoScript" );
	GLASSERT( child );
	
	origin.Zero();

	if ( child ) {
		Archive( 0, child );
	}
}


void VolcanoScript::Save( const ScriptContext& ctx, tinyxml2::XMLPrinter* printer )
{
	printer->OpenElement( "VolcanoScript" );
	Archive( printer, 0 );
	printer->CloseElement();
}


bool VolcanoScript::DoTick( const ScriptContext& ctx, U32 delta )
{
	return false;
}


