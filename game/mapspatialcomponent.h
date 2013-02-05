/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MAP_SPATIAL_COMPONENT
#define MAP_SPATIAL_COMPONENT

#include "../xegame/spatialcomponent.h"

class WorldMap;

// Marks the WorldGrid inUse.
// FIXME: not mutable; once added, can't be moved.
//		  can this be fixed with a "mutable" flag on SpatialComponent?
class MapSpatialComponent : public SpatialComponent
{
private:
	typedef SpatialComponent super;
public:
	enum {
		USES_GRID = 1,
		BLOCKS_GRID
	};
	MapSpatialComponent( WorldMap* map );
	virtual ~MapSpatialComponent()	{}

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "MapSpatialComponent" ) ) return this;
		return super::ToComponent( name );
	}

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual void Load( const tinyxml2::XMLElement* element );
	virtual void Save( tinyxml2::XMLPrinter* printer );

	// WARNING must be called before OnAdd
	void SetMapPosition( int x, int y );

	grinliz::Vector2I MapPosition() const;
	void SetMode( int mode );

private:
	void Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele );
	bool justLoaded;
	int mode;
	WorldMap* worldMap;
};


#endif // MAP_SPATIAL_COMPONENT
