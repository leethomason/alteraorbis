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

#include "../script/scriptcomponent.h"

#include "componentfactory.h"

#include "../grinliz/glstringutil.h"
#include "../engine/engine.h"

using namespace grinliz;

Component* ComponentFactory::Factory( const char* name, Chit* chit ) const
{
	GLASSERT( name && *name );
	if ( StrEqual( name, "SpatialComponent" )) {
		return new SpatialComponent();
	}
	else if ( StrEqual( name, "PathMoveComponent" )) {
		return new PathMoveComponent( worldMap );
	}
	else if ( StrEqual( name, "ItemComponent" )) {
		GameItem item;
		return new ItemComponent( engine, worldMap, 0 );
	}
	else if ( StrEqual( name, "DebugStateComponent" )) {
		return new DebugStateComponent( worldMap );
	}
	else if ( StrEqual( name, "DebugPathComponent" )) {
		return new DebugPathComponent( engine, worldMap, lumosGame );
	}
	else if ( StrEqual( name, "HealthComponent" )) {
		return new HealthComponent( engine );
	}
	else if ( StrEqual( name, "RenderComponent" )) {
		return new RenderComponent( engine, 0 );
	}
	else if ( StrEqual( name, "CameraComponent" )) {
		return new CameraComponent( &engine->camera );
	}
	else if ( StrEqual( name, "ScriptComponent" )) {
		return new ScriptComponent( this, census );
	}
	else if ( StrEqual( name, "MapSpatialComponent" )) {
		return new MapSpatialComponent( worldMap );
	}
	else if ( StrEqual( name, "AIComponent" )) {
		return new AIComponent( engine, worldMap );
	}
	else if ( StrEqual( name, "GridMoveComponent" )) {
		return new GridMoveComponent( worldMap );
	}
	else if ( StrEqual( name, "VisitorStateComponent" )) {
		return new VisitorStateComponent( worldMap );
	}
	else if ( StrEqual( name, "TrackingMoveComponent" )) {
		return new TrackingMoveComponent( worldMap );
	}
	else if ( StrEqual( name, "PhysicsMoveComponent" )) {
		return new PhysicsMoveComponent( worldMap, true );
	}

	GLASSERT( 0 );
	GLOUTPUT_REL(( "%s", name ));
	return 0;
}