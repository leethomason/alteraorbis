#include "mapspatialcomponent.h"
#include "worldmap.h"

using namespace grinliz;

MapSpatialComponent::MapSpatialComponent( int width, int height, WorldMap* _map ) : SpatialComponent()
{
	map = _map;
	mapPos.Set( -1, -1 );
	mapSize.Set( width, height );
	mapRotation = 0;
}


void MapSpatialComponent::SetMapPosition( int x, int y, int r )
{
	if ( x != mapPos.x || y != mapPos.y || r != mapRotation ) {

		if ( mapPos.x >= 0 ) {
			Rectangle2I rb;
			rb.Set( mapPos.x, mapPos.y, mapPos.x+mapSize.x-1, mapPos.y+mapSize.y-1 );
			map->ClearBlock( rb );
		}

		float wx, wz;
		GLASSERT( r == 0 || r == 90 || r == 180 || r == 270  );
		mapRotation = r;

		Rectangle2I rb;
		rb.min.Set( x, y );

		if ( mapRotation == 0 || mapRotation == 180 ) {
			wx = (float)x + (float)mapSize.x*0.5f;
			wz = (float)y + (float)mapSize.y*0.5f;
			rb.max.Set( x+mapSize.x-1, y+mapSize.y-1 );
		}
		else {
			wx = (float)x + (float)mapSize.y*0.5f;
			wz = (float)y + (float)mapSize.x*0.5f;
			rb.max.Set( x+mapSize.y-1, y+mapSize.x-1 );
		}		
		map->SetBlock( rb );

		SetPosition( wx, 0, wz );
		SetYRotation( (float)mapRotation );
	}
}




