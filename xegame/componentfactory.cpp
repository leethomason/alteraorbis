#include "spatialcomponent.h"
#include "inventorycomponent.h"
#include "itemcomponent.h"
#include "rendercomponent.h"
#include "cameracomponent.h"

#include "../game/pathmovecomponent.h"
#include "../game/debugstatecomponent.h"
#include "../game/debugpathcomponent.h"
#include "../game/healthcomponent.h"

#include "componentfactory.h"

#include "../grinliz/glstringutil.h"
#include "../engine/engine.h"

using namespace grinliz;

Component* ComponentFactory::Factory( const char* name, Chit* chit ) const
{
	if ( StrEqual( name, "SpatialComponent" )) {
		return new SpatialComponent();
	}
	else if ( StrEqual( name, "PathMoveComponent" )) {
		return new PathMoveComponent( worldMap );
	}
	else if ( StrEqual( name, "InventoryComponent" )) {
		return new InventoryComponent();
	}
	else if ( StrEqual( name, "ItemComponent" )) {
		GameItem item;
		return new ItemComponent( item );
	}
	else if ( StrEqual( name, "DebugStateComponent" )) {
		return new DebugStateComponent( worldMap );
	}
	else if ( StrEqual( name, "DebugPathComponent" )) {
		return new DebugPathComponent( engine, worldMap, lumosGame );
	}
	else if ( StrEqual( name, "HealthComponent" )) {
		return new HealthComponent();
	}
	else if ( StrEqual( name, "RenderComponent" )) {
		return new RenderComponent( engine, 0 );
	}
	else if ( StrEqual( name, "CameraComponent" )) {
		return new CameraComponent( &engine->camera );
	}

	GLASSERT( 0 );
	return 0;
}