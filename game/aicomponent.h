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

#ifndef AI_COMPONENT_INCLUDED
#define AI_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../xegame/cticker.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glrectangle.h"
#include "../script/battlemechanics.h"

class WorldMap;
class Engine;
struct ComponentSet;


// Combat AI: needs refactoring
class AIComponent : public Component
{
private:
	typedef Component super;

public:
	AIComponent( Engine* _engine, WorldMap* _worldMap );
	virtual ~AIComponent();

	virtual const char* Name() const { return "AIComponent"; }

	virtual void Serialize( XStream* xs );
	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	virtual int  DoTick( U32 delta, U32 timeSince );
	virtual void DebugStr( grinliz::GLString* str );
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual void OnChitEvent( const ChitEvent& event );

	// Approximate. enemyList may not be flushed.
	bool AwareOfEnemy() const { return enemyList.Size() > 0; }
	void FocusedMove( const grinliz::Vector2F& dest, const grinliz::Vector2I* sector );
	void FocusedTarget( Chit* chit );

	enum {
		WANDER_NONE,
		WANDER_HERD,
		WANDER_CIRCLE
	};
	void SetWanderParams( int mode, const grinliz::Vector2F& pos, float radius ); 
	void EnableDebug( bool enable ) { debugFlag = enable; }

	enum {
		FRIENDLY,
		ENEMY,
		NEUTRAL
	};
	int GetTeamStatus( Chit* other );

	// Top level AI modes.
	enum {
		NORMAL_MODE,
		BATTLE_MODE
	};

private:

	enum {
		MAX_TRACK = 8,
	};

	void GetFriendEnemyLists();
	Chit* Closest( const ComponentSet& thisComp, grinliz::CArray<int, MAX_TRACK>* list );

	bool LineOfSight( const ComponentSet& thisComp, Chit* target );

	void Think( const ComponentSet& thisComp );	// Choose a new action.
	void ThinkWander( const ComponentSet& thisComp );
	void ThinkBattle( const ComponentSet& thisComp );

	// What happens when no other move is working.
	grinliz::Vector2F ThinkWanderRandom( const ComponentSet& thisComp );
	// pseudo-flocking
	grinliz::Vector2F ThinkWanderFlock( const ComponentSet& thisComp );
	// creepy circle pacing
	grinliz::Vector2F ThinkWanderCircle( const ComponentSet& thisComp );


	Engine*		engine;
	WorldMap*	map;

	enum {
		// Possible actions:
		NO_ACTION,
		MOVE,			// Movement, will reload and run&gun if possible.
		MELEE,			// Go to the target and hit it. Melee charge.
		SHOOT,			// Stand ground and shoot.

		WANDER,
	};

	int					aiMode;
	int					currentAction;
	int					currentTarget;
	bool				focusOnTarget;
	bool				focusedMove;
	grinliz::Rectangle2F awareness;
	CTicker				rethink;
	grinliz::Vector2F	wanderOrigin;
	float				wanderRadius;
	U32					wanderTime;
	bool				randomWander;
	int					wanderFlags;
	bool				debugFlag;

	void DoMelee( const ComponentSet& thisComp );
	void DoShoot( const ComponentSet& thisComp );
	void DoMove( const ComponentSet& thisComp );

	grinliz::CArray<int, MAX_TRACK> friendList;
	grinliz::CArray<int, MAX_TRACK> enemyList;
	BattleMechanics battleMechanics;
};


#endif // AI_COMPONENT_INCLUDED