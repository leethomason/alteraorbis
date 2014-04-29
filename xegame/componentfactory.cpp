#include "spatialcomponent.h"
#include "itemcomponent.h"
#include "rendercomponent.h"
#include "cameracomponent.h"

#include "../game/pathmovecomponent.h"
#include "../game/debugstatecomponent.h"
#include "../game/debugpathcomponent.h"
#include "../game/healthcomponent.h"
#include "../game/mapspatialcomponent.h"
#include "../game/aicomponent.h"
#include "../game/gridmovecomponent.h"
#include "../game/visitorstatecomponent.h"
#include "../game/physicsmovecomponent.h"
#include "../game/towcomponent.h"

#include "../script/scriptcomponent.h"
#include "../script/volcanoscript.h"
#include "../script/plantscript.h"
#include "../script/corescript.h"
#include "../script/countdownscript.h"
#include "../script/farmscript.h"
#include "../script/distilleryscript.h"
#include "../script/evalbuildingscript.h"
#include "../script/guardscript.h"


#include "componentfactory.h"

#include "../grinliz/glstringutil.h"
#include "../engine/engine.h"
#include "../xegame/chitbag.h"

using namespace grinliz;

Component* ComponentFactory::Factory( const char* name, const ChitContext* context )
{
	GLASSERT( name && *name );
	if ( StrEqual( name, "SpatialComponent" )) {
		return glnew SpatialComponent();
	}
	else if ( StrEqual( name, "PathMoveComponent" )) {
		return glnew PathMoveComponent();
	}
	else if ( StrEqual( name, "ItemComponent" )) {
		return glnew ItemComponent( 0 );
	}
	else if ( StrEqual( name, "DebugStateComponent" )) {
		return glnew DebugStateComponent( context->worldMap );
	}
	else if ( StrEqual( name, "DebugPathComponent" )) {
		return glnew DebugPathComponent();
	}
	else if ( StrEqual( name, "HealthComponent" )) {
		return glnew HealthComponent();
	}
	else if ( StrEqual( name, "RenderComponent" )) {
		return glnew RenderComponent( 0 );
	}
	else if ( StrEqual( name, "CameraComponent" )) {
		return glnew CameraComponent( &context->engine->camera );
	}
	else if ( StrEqual( name, "MapSpatialComponent" )) {
		return glnew MapSpatialComponent();
	}
	else if ( StrEqual( name, "AIComponent" )) {
		return glnew AIComponent();
	}
	else if ( StrEqual( name, "GridMoveComponent" )) {
		return glnew GridMoveComponent();
	}
	else if ( StrEqual( name, "VisitorStateComponent" )) {
		return glnew VisitorStateComponent();
	}
	else if ( StrEqual( name, "TrackingMoveComponent" )) {
		return glnew TrackingMoveComponent();
	}
	else if ( StrEqual( name, "PhysicsMoveComponent" )) {
		return glnew PhysicsMoveComponent( true );
	}
	else if ( StrEqual( name, "TowComponent" )) {
		return glnew TowComponent();
	}
	if (name == "VolcanoScript") {
		return glnew VolcanoScript();
	}
	else if (name == "PlantScript") {
		return glnew PlantScript(0);
	}
	else if (name == "CoreScript") {
		return glnew CoreScript();
	}
	else if (name == "CountDownScript") {
		return glnew CountDownScript(1000);
	}
	else if (name == "FarmScript") {
		return glnew FarmScript();
	}
	else if (name == "DistilleryScript") {
		return glnew DistilleryScript();
	}
	else if (name == "EvalBuildingScript") {
		return glnew EvalBuildingScript();
	}
	else if (name == "GuardScript") {
		return glnew GuardScript();
	}

	GLASSERT( 0 );
	GLOUTPUT_REL(( "%s", name ));
	return 0;
}