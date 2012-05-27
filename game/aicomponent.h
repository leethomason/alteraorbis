#ifndef AI_COMPONENT_INCLUDED
#define AI_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glrectangle.h"

class WorldMap;

// Combat AI: needs refactoring
class AIComponent : public Component
{
public:
	AIComponent( WorldMap* _worldMap, int _team );
	virtual ~AIComponent();

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "AIComponent" ) ) return this;
		return Component::ToComponent( name );
	}

	virtual bool NeedsTick()					{ return true; }
	virtual void DoTick( U32 delta );
	virtual void DebugStr( grinliz::GLString* str );

private:
	enum {
		FRIENDLY,
		ENEMY,
		NEUTRAL,
		
		MAX_TRACK = 8,

	};

	void UpdateCombatInfo( const grinliz::Rectangle2F* _zone=0 );
	void UpdateChitData();	// brings chit pointers in friend/enemy list up to date			
	int GetTeamStatus( const AIComponent* other );

	WorldMap* map;
	int team;
	U32 combatInfoAge;

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