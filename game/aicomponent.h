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
struct SectorPort;

// Combat AI: needs refactoring
class AIComponent : public Component
{
private:
	typedef Component super;

public:
	AIComponent( Engine* _engine, WorldMap* _worldMap );
	virtual ~AIComponent();

	virtual const char* Name() const { return "AIComponent"; }
	virtual AIComponent* ToAIComponent() { return this; }

	virtual void Serialize( XStream* xs );
	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	virtual int  DoTick( U32 delta, U32 timeSince );
	virtual void DebugStr( grinliz::GLString* str );
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual void OnChitEvent( const ChitEvent& event );

	// Approximate. enemyList may not be flushed.
	bool AwareOfEnemy() const { return enemyList.Size() > 0; }

	// Tell the AI to take actions. If the "focused" is set,
	// it is given extra priority. The 'dest' must be in the same
	// sector. If 'sector' is also specfied, will do grid travel
	// after the move.
	void Move( const grinliz::Vector2F& dest, const SectorPort* sector, bool focused );
	// Targets a particular enemy.
	void Target( Chit* chit, bool focused );
	// Melt rock in the sector.
	void Melt( const grinliz::Vector2I& rock );

	void EnableDebug( bool enable ) { debugFlag = enable; }

	enum {
		FRIENDLY,
		ENEMY,
		NEUTRAL
	};
	int GetTeamStatus( Chit* other );

	// Top level AI modes. Higher level goals.
	// Translated to immediate goals: MOVE, SHOOT, MELEE
	enum {
		NORMAL_MODE,
		ROCKBREAK_MODE,
		BATTLE_MODE
	};

private:

	enum {
		MAX_TRACK = 8,
	};

	void GetFriendEnemyLists();
	Chit* Closest( const ComponentSet& thisComp, Chit* arr[], int n, 
				   grinliz::Vector2F* pos, float* distance );

	// Compute the line of site
	bool LineOfSight( const ComponentSet& thisComp, Chit* target );
	bool LineOfSight( const ComponentSet& thisComp, const grinliz::Vector2I& mapPos );

	void Think( const ComponentSet& thisComp );	// Choose a new action.
	void ThinkWander( const ComponentSet& thisComp );
	void ThinkBattle( const ComponentSet& thisComp );
	void ThinkRockBreak( const ComponentSet& thisComp );

	grinliz::Vector2F WanderOrigin( const ComponentSet& thisComp ) const;

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
		STAND,			// used to eat plants, reload
		NUM_ACTIONS
	};

	enum { 
		FOCUS_NONE,
		FOCUS_MOVE,
		FOCUS_TARGET
	};

	// A description of the current target, which
	// can either be a chit or a location on the map.
	struct TargetDesc
	{
		TargetDesc() { Clear(); }
	

		int id;
		grinliz::Vector2I mapPos;

		grinliz::Vector3F MapTarget() const {
			GLASSERT( !mapPos.IsZero() );
			grinliz::Vector3F v = { (float)mapPos.x+0.5f, 0.5f, (float)mapPos.y+0.5f };
			return v;
		}

		void Clear() { id = 0; mapPos.Zero(); }
		void Set( int _id ) { id = _id; mapPos.Zero(); }
		void Set( const grinliz::Vector2I& _mapPos ) { mapPos = _mapPos; id = 0; }
	};

	int					aiMode;
	TargetDesc			targetDesc;
	int					currentAction;
	int					focus;
	grinliz::Rectangle2F awareness;
	CTicker				rethink;
	int					friendEnemyAge;
	U32					wanderTime;
	bool				randomWander;
	bool				debugFlag;

	void DoMelee( const ComponentSet& thisComp );
	void DoShoot( const ComponentSet& thisComp );
	void DoMove( const ComponentSet& thisComp );
	int  DoStand( const ComponentSet& thisComp, U32 since );
	bool SectorHerd( const ComponentSet& thisComp );

	grinliz::CArray<int, MAX_TRACK> friendList;
	grinliz::CArray<int, MAX_TRACK> enemyList;
};


#endif // AI_COMPONENT_INCLUDED