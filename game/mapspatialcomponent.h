#ifndef MAP_SPATIAL_COMPONENT
#define MAP_SPATIAL_COMPONENT

#include "../xegame/spatialcomponent.h"

class WorldMap;

// Extra methods that support map positioning.
class MapSpatialComponent : public SpatialComponent
{
public:
	// fixme: need a system to:
	// - create from definition
	// - specify blocks
	//
	MapSpatialComponent( int width, int height, WorldMap* map );
	virtual ~MapSpatialComponent()	{}

	virtual Component*          ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "MapSpatialComponent" ) ) return this;
		return SpatialComponent::ToComponent( name );
	}

	// Set the location of the origin point.
	// 0, 90, 180, 270
	void SetMapPosition( int x, int y, int r );

	const grinliz::Vector2I& GetMapPosition() const { return mapPos; }
	int GetMapRotation() const { return mapRotation; }

private:
	WorldMap* map;
	grinliz::Vector2I mapPos;
	grinliz::Vector2I mapSize;
	int mapRotation;
};

#endif // MAP_SPATIAL_COMPONENT
