#include "spatialcomponent.h"
#include "itemcomponent.h"
#include "rendercomponent.h"
#include "cameracomponent.h"
#include "chitcontext.h"

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
#include "../script/flagscript.h"

#include "../ai/rebuildai.h"
#include "../ai/domainai.h"

#include "componentfactory.h"

#include "../grinliz/glstringutil.h"
#include "../engine/engine.h"
#include "../xegame/chitbag.h"

using namespace grinliz;

Component* ComponentFactory::Factory( const char* _name, const ChitContext* context )
{
	GLASSERT( _name && *_name );
	IString name = StringPool::Intern(_name);

	if ( name == "PathMoveComponent" ) {
		return new PathMoveComponent();
	}
	else if ( name == "ItemComponent" ) {
		return new ItemComponent( 0 );
	}
	else if ( name == "DebugStateComponent" ) {
		return new DebugStateComponent();
	}
	else if ( name == "DebugPathComponent" ) {
		return new DebugPathComponent();
	}
	else if ( name == "HealthComponent" ) {
		return new HealthComponent();
	}
	else if ( name == "RenderComponent" ) {
		return new RenderComponent( 0 );
	}
	else if ( name == "CameraComponent" ) {
		return new CameraComponent( &context->engine->camera );
	}
	else if ( name == "MapSpatialComponent" ) {
		return new MapSpatialComponent();
	}
	else if ( name == "AIComponent" ) {
		return new AIComponent();
	}
	else if ( name == "GridMoveComponent" ) {
		return new GridMoveComponent();
	}
	else if (name == "GameMoveComponent") {
		return new GameMoveComponent();
	}
	else if ( name == "TrackingMoveComponent" ) {
		return new TrackingMoveComponent();
	}
	else if ( name == "PhysicsMoveComponent" ) {
		return new PhysicsMoveComponent();
	}
	else if ( name == "TowComponent" ) {
		return new TowComponent();
	}
	if ( name == "VolcanoScript") {
		return new VolcanoScript();
	}
	else if (name == "CoreScript") {
		return new CoreScript();
	}
	else if (name == "CountDownScript") {
		return new CountDownScript(1000);
	}
	else if (name == "FarmScript") {
		return new FarmScript();
	}
	else if (name == "DistilleryScript") {
		return new DistilleryScript();
	}
	else if (name == "EvalBuildingScript") {
		return new EvalBuildingScript();
	}
	else if (name == "GuardScript") {
		return new GuardScript();
	}
	else if (name == "RebuildAIComponent") {
		return new RebuildAIComponent();
	}
	else if (name == "BatteryComponent") {
		return new BatteryComponent();
	}
	else if (name == "DeityDomainAI") {
		return new DeityDomainAI();
	}
	else if (name == "ForgeDomainAI") {
		return new ForgeDomainAI();
	}
	else if (name == "GobDomainAI") {
		return new GobDomainAI();
	}
	else if (name == "KamakiriDomainAI") {
		return new KamakiriDomainAI();
	}
	else if (name == "HumanDomainAI") {
		return new HumanDomainAI();
	}
	else if (name == "FlagScript") {
		return new FlagScript();
	}


	GLASSERT( 0 );
	GLOUTPUT_REL(( "%s", _name ));
	return 0;
}