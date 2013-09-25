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

#ifndef GAME_MOVE_COMPONENT_INCLUDED
#define GAME_MOVE_COMPONENT_INCLUDED

#include "../xegame/component.h"

class WorldMap;

class GameMoveComponent : public MoveComponent
{
private:
	typedef MoveComponent super;

public:
	GameMoveComponent( WorldMap* _map ) : map( _map ) {
	}
	virtual ~GameMoveComponent()	{}

	virtual const char* Name() const { return "GameMoveComponent"; }
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	WorldMap* GetWorldMap() { return map; }
	float Speed() const;

protected:
	// Keep from hitting world objects.
	void ApplyBlocks( grinliz::Vector2F* pos, bool* forceApplied );

	WorldMap* map;
};


#endif // GAME_MOVE_COMPONENT_INCLUDED
