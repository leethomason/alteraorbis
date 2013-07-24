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
	void SetMapPosition( int x, int y, int cx, int cy );

	grinliz::Vector2I MapPosition() const	{ return bounds.min; }
	grinliz::Rectangle2I Bounds() const		{ return bounds; }

	void SetMode( int mode );
	int Mode() const			{ return mode; }
	void SetBuilding( bool b )	{ building = b; }
	bool Building() const		{ return building; }

	// Assumes there is a porch; to actually query,
	// use the 'porch' property on the GameItem
	grinliz::Vector2I PorchPos() const;

private:
	int						mode;
	bool					building;	// is this a building?
	grinliz::Rectangle2I	bounds;
	WorldMap*				worldMap;
};


#endif // MAP_SPATIAL_COMPONENT
