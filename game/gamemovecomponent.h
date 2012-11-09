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

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "GameMoveComponent" ) ) return this;
		return super::ToComponent( name );
	}

	WorldMap* GetWorldMap() { return map; }

protected:
	// Keep from hitting world objects.
	void ApplyBlocks( grinliz::Vector2F* pos, bool* forceApplied, bool* isStuck );

	WorldMap* map;
};


#endif // GAME_MOVE_COMPONENT_INCLUDED
