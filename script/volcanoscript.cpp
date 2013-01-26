#include "volcanoscript.h"
#include "../engine/serialize.h"
#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"

using namespace grinliz;
using namespace tinyxml2;

VolcanoScript::VolcanoScript( WorldMap* p_map )
{
	worldMap = p_map;
	origin.Zero();
}


void VolcanoScript::Init( const ScriptContext& heap )
{
	SpatialComponent* sc = heap.chit->GetSpatialComponent();
	GLASSERT( sc );
	if ( sc ) {
		Vector2F pos = sc->GetPosition2D();
		origin.Set( (int)pos.x, (int)pos.y );
	}
	GLOUTPUT(( "VolcanoScript::Init. pos=%d,%d\n", origin.x, origin.y ));
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


