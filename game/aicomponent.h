#ifndef AI_COMPONENT_INCLUDED
#define AI_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glrectangle.h"

class WorldMap;
class Engine;

// Combat AI: needs refactoring
class AIComponent : public Component
{
public:
	AIComponent( Engine* _engine, WorldMap* _worldMap );
	virtual ~AIComponent();

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "AIComponent" ) ) return this;
		return Component::ToComponent( name );
	}

	virtual bool NeedsTick()					{ return true; }
	virtual void DoTick( U32 delta );
	virtual void DebugStr( grinliz::GLString* str );
	virtual void OnChitMsg( Chit* chit, int id, const ChitEvent* event );

private:
	enum {
		FRIENDLY,
		ENEMY,
		NEUTRAL,
		
		MAX_TRACK = 8,

	};

	void UpdateCombatInfo( const grinliz::Rectangle2F* _zone=0 );
	void UpdateChitData();	// brings chit pointers in friend/enemy list up to date			
	int GetTeamStatus( Chit* other );

	Engine*		engine;
	WorldMap*	map;
	U32			combatInfoAge;

	struct ChitData {
		int   chitID;
		Chit* chit;			// *only* valid after Purge() or UpdateCombatInfo()	
		float pathDistance;
		float range;
	};
	grinliz::CArray<ChitData, MAX_TRACK> friendList;
	grinliz::CArray<ChitData, MAX_TRACK> enemyList;
};


#endif // AI_COMPONENT_INCLUDED