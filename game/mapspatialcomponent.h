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
class CircuitSim;

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

	virtual void OnAdd( Chit* chit, bool init );
	virtual void OnRemove();
	virtual void Serialize( XStream* xs );
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual int DoTick(U32 time);

	void SetBuilding( int size, bool hasPorch, int circuit );
	int Size() const								{ return size; }
	grinliz::Vector2I MapPosition() const			{ return bounds.min; }
	virtual grinliz::Rectangle2I Bounds() const		{ return bounds; }

	void SetBlocks( bool blocks );
	int Blocks() const			{ return blocks; }

	// Returns the porch, or an empty rectangle if there is none.
	grinliz::Rectangle2I PorchPos() const;
	bool HasPorch() const { return hasPorch > 0; }
	int PorchType() const { return hasPorch;  }
	int CircuitType() const { return hasCircuit; }

	// internal: used by the LumosChitBag to track buildings.
	MapSpatialComponent* nextBuilding;

	// utility method to correctly position
	static void SetMapPosition(Chit* chit, int x, int y);

	// Utility code for dealing with porches
	// 'pos' in minimum coordinates, size=1 or 2, rotation 0-270
	static grinliz::Rectangle2I CalcPorchPos(const grinliz::Vector2I& pos, int size, float rotation);

private:
	void SyncWithSpatial();
	//void UpdatePorch(bool clearPorch);
	static void UpdateGridLayer(WorldMap* map, LumosChitBag* chitBag, CircuitSim* ciruitSim, const grinliz::Rectangle2I& rect);

	int						size;
	bool					blocks;
	int						hasPorch;	// if a building, does it have a porch?
	int						hasCircuit;	
	float					glowTarget;
	float					glow;
	grinliz::Rectangle2I	bounds;
};


#endif // MAP_SPATIAL_COMPONENT
