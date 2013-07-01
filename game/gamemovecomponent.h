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

protected:
	// Keep from hitting world objects.
	void ApplyBlocks( grinliz::Vector2F* pos, bool* forceApplied );

	WorldMap* map;
};


#endif // GAME_MOVE_COMPONENT_INCLUDED
