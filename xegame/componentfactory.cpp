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

#include "../script/volcanoscript.h"
#include "../script/plantscript.h"
#include "../script/corescript.h"
#include "../script/countdownscript.h"
#include "../script/farmscript.h"
#include "../script/distilleryscript.h"
#include "../script/evalbuildingscript.h"
#include "../script/guardscript.h"
#include "../script/batterycomponent.h"

#include "../ai/rebuildai.h"

#include "componentfactory.h"

#include "../grinliz/glstringutil.h"
#include "../engine/engine.h"
#include "../xegame/chitbag.h"

using namespace grinliz;

Component* ComponentFactory::Factory( const char* _name, const ChitContext* context )
{
	GLASSERT( _name && *_name );
	IString name = StringPool::Intern(_name);

	if ( name == "SpatialComponent" ) {
		return glnew SpatialComponent();
	}
	else if ( name == "PathMoveComponent" ) {
		return glnew PathMoveComponent();
	}
	else if ( name == "ItemComponent" ) {
		return glnew ItemComponent( 0 );
	}
	else if ( name == "DebugStateComponent" ) {
		return glnew DebugStateComponent();
	}
	else if ( name == "DebugPathComponent" ) {
		return glnew DebugPathComponent();
	}
	else if ( name == "HealthComponent" ) {
		return glnew HealthComponent();
	}
	else if ( name == "RenderComponent" ) {
		return glnew RenderComponent( 0 );
	}
	else if ( name == "CameraComponent" ) {
		return glnew CameraComponent( &context->engine->camera );
	}
	else if ( name == "MapSpatialComponent" ) {
		return glnew MapSpatialComponent();
	}
	else if ( name == "AIComponent" ) {
		return glnew AIComponent();
	}
	else if ( name == "GridMoveComponent" ) {
		return glnew GridMoveComponent();
	}
	else if (name == "GameMoveComponent") {
		return glnew GameMoveComponent();
	}
	else if ( name == "VisitorStateComponent" ) {
		return glnew VisitorStateComponent();
	}
	else if ( name == "TrackingMoveComponent" ) {
		return glnew TrackingMoveComponent();
	}
	else if ( name == "PhysicsMoveComponent" ) {
		return glnew PhysicsMoveComponent();
	}
	else if ( name == "TowComponent" ) {
		return glnew TowComponent();
	}
	if ( name == "VolcanoScript") {
		return glnew VolcanoScript();
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
	else if (name == "RebuildAIComponent") {
		return glnew RebuildAIComponent();
	}
	else if (name == "BatteryComponent") {
		return glnew BatteryComponent();
	}


	GLASSERT( 0 );
	GLOUTPUT_REL(( "%s", name ));
	return 0;
}