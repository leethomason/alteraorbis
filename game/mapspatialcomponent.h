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
// Not mutable; once added, can't be moved.
class MapSpatialComponent : public SpatialComponent
{
private:
	typedef SpatialComponent super;
public:
	MapSpatialComponent();
	virtual ~MapSpatialComponent()	{}

	virtual const char* Name() const { return "MapSpatialComponent"; }
	virtual MapSpatialComponent*	ToMapSpatialComponent()			{ return this; }

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual void Serialize( XStream* xs );

	// WARNING must be called before OnAdd
	void SetMapPosition( int x, int y, int cx, int cy );
	void SetBuilding( bool b, bool hasPorch );
	// -- end before OnAdd warning --

	grinliz::Vector2I MapPosition() const	{ return bounds.min; }
	grinliz::Rectangle2I Bounds() const		{ return bounds; }
	virtual void SetPosRot( const grinliz::Vector3F& v, const grinliz::Quaternion& quat );

	void SetMode( int mode );	// GRID_IN_USE or GRID_BLOCKED
	int Mode() const			{ return mode; }
	bool Building() const		{ return building; }

	// Returns the porch, or an empty rectangle if there is none.
	grinliz::Rectangle2I PorchPos() const;

	// internal: used by the LumosChitBag to track buildings.
	MapSpatialComponent* nextBuilding;

private:
	void UpdatePorch( const grinliz::Rectangle2I& bounds, WorldMap* worldMap, LumosChitBag* bag );
	void UpdateBlock( WorldMap* map );	// can't use the context - used after OnRemove

	int						mode;
	bool					building;	// is this a building?
	bool					hasPorch;	// if a building, does it have a porch?
	grinliz::Rectangle2I	bounds;
};


#endif // MAP_SPATIAL_COMPONENT
