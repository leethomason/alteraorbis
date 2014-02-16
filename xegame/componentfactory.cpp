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

#include "componentfactory.h"

#include "../grinliz/glstringutil.h"
#include "../engine/engine.h"

using namespace grinliz;

Component* ComponentFactory::Factory( const char* name, Chit* chit ) const
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
		return glnew DebugStateComponent( worldMap );
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
		return glnew CameraComponent( &engine->camera );
	}
	else if ( StrEqual( name, "ScriptComponent" )) {
		return glnew ScriptComponent( this );
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

	GLASSERT( 0 );
	GLOUTPUT_REL(( "%s", name ));
	return 0;
}