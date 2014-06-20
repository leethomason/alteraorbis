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

#ifndef GRID_MOVE_COMPONENT_INCLUDED
#define GRID_MOVE_COMPONENT_INCLUDED

#include "gamemovecomponent.h"
#include "worldinfo.h"
#include "sectorport.h"

class GridMoveComponent : public GameMoveComponent
{
private:
	typedef GameMoveComponent super;
public:
	GridMoveComponent();
	virtual ~GridMoveComponent();

	virtual const char* Name() const { return "GridMoveComponent"; }
	virtual GridMoveComponent* ToGridMoveComponent() { return this; }

	virtual void DebugStr( grinliz::GLString* str );

	virtual void Serialize( XStream* xs );

	virtual void OnAdd( Chit* chit, bool init );
	virtual void OnRemove();
	virtual int DoTick( U32 delta );
	virtual bool IsMoving() const;
	virtual grinliz::Vector3F Velocity() { return velocity; }

	void SetDest( const SectorPort& sp );

private:
	enum {
		NOT_INIT,
		ON_BOARD,
		TRAVELLING,
		OFF_BOARD,
		DONE
	};
	int					state;
	SectorPort			destSectorPort;
	float				speed;

	// Not serialized:
	grinliz::Vector3F velocity;
	grinliz::CDynArray< GridBlock > path;
};

#endif // GRID_MOVE_COMPONENT_INCLUDED
