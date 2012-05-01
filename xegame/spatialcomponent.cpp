#include "spatialcomponent.h"
#include "chit.h"
#include "chitbag.h"

void SpatialComponent::SetPosition( float x, float y, float z )
{
	position.Set( x, y, z ); 
	RequestUpdate();	// Renders triggers off update. May want to reconsider that.

}
