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
	MapSpatialComponent( WorldMap* map );
	virtual ~MapSpatialComponent()	{}

	virtual const char* Name() const { return "MapSpatialComponent"; }
	virtual MapSpatialComponent*	ToMapSpatialComponent()			{ return this; }

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual void Serialize( XStream* xs );

	// WARNING must be called before OnAdd
	void SetMapPosition( int x, int y );

	grinliz::Vector2I MapPosition() const { 
		grinliz::Vector2I v = { (int) position.x, (int)position.z };
		return v;
	}
	// FIXME: currently always 1x1
	grinliz::Rectangle2I Bounds() const {
		grinliz::Rectangle2I b;
		b.min = b.max = MapPosition();
		return b;
	}

	void SetMode( int mode );
	int Mode() const { return mode; }
	void SetBuilding( bool b )	{ building = b; }
	bool Building() const		{ return building; }

	grinliz::Vector2I PorchPos() const;

private:
	int mode;
	bool building;			// is this a building?
	WorldMap* worldMap;
};


#endif // MAP_SPATIAL_COMPONENT
