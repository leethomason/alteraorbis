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
#include "../ai/aineeds.h"
#include "../ai/tasklist.h"

class WorldMap;
class Engine;
struct ComponentSet;
struct SectorPort;
class WorkQueue;
class IRangedWeaponItem;

namespace ai {
	class TaskList;
};

// Combat AI: needs refactoring
class AIComponent : public Component
{
private:
	typedef Component super;

public:
	AIComponent();
	virtual ~AIComponent();

	virtual const char* Name() const { return "AIComponent"; }
	virtual AIComponent* ToAIComponent() { return this; }

	virtual void Serialize( XStream* xs );
	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	virtual int  DoTick( U32 delta );
	virtual void DebugStr( grinliz::GLString* str );
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual void OnChitEvent( const ChitEvent& event );

	// Approximate. enemyList may not be flushed.
	bool AwareOfEnemy() const { return enemyList.Size() > 0; }

	// Tell the AI to take actions. If the "focused" is set,
	// it is given extra priority. The 'dest' must be in the same
	// sector. If 'sector' is also specfied, will do grid travel
	// after the move.
	void Move( const grinliz::Vector2F& dest, bool focused, const grinliz::Vector2F* normal=0 );
	// Returns true if the move is possible.
	bool Move( const SectorPort& sectorport, bool focused );
	void Pickup( Chit* item );
	void Stand();
	void Rampage( int dest );

	void Target( Chit* chit, bool focused );
	bool RockBreak( const grinliz::Vector2I& pos );
	void MakeAware( const int* enemyIDs, int n );
	
	// Use a null IString for ICE.
	bool Build( const grinliz::Vector2I& pos, grinliz::IString structure );

	void EnableDebug( bool enable )			{ debugFlag = enable; }
	void SetSectorAwareness( bool aware )	{ fullSectorAware = aware; }
	void SetVisitorIndex( int i )			{ visitorIndex = i; }
	int  VisitorIndex() const				{ return visitorIndex; }

	int GetTeamStatus( Chit* other );
	const ai::Needs& GetNeeds() const		{ return needs; }
	ai::Needs* GetNeedsMutable()			{ return &needs; }

	// Top level AI modes. Higher level goals.
	// Translated to immediate goals: MOVE, SHOOT, MELEE
	enum {
		NORMAL_MODE,
		RAMPAGE_MODE,		// a MOB that gets stuck can 'rampage', which means cutting a path through the world.
		ROCKBREAK_MODE,		// weird special mode for attacking rocks. probably can be removed.
		BATTLE_MODE,
		NUM_MODES
	};

	// Secondary AI modes.
	enum {
		NO_ACTION,
		MOVE,			// Movement, will reload and run&gun if possible.
		MELEE,			// Go to the target and hit it. Melee charge.
		SHOOT,			// Stand ground and shoot.

		WANDER,
		STAND,			// used to eat plants, reload
		NUM_ACTIONS,
	};

private:
	enum {
		MAX_TRACK = 8,
	};

	void GetFriendEnemyLists();
	Chit* Closest( const ComponentSet& thisComp, Chit* arr[], int n, 
				   grinliz::Vector2F* pos, float* distance );

	// Compute the line of site
	bool LineOfSight( const ComponentSet& thisComp, Chit* target, IRangedWeaponItem* weapon );
	bool LineOfSight( const ComponentSet& thisComp, const grinliz::Vector2I& voxel );

	void Think( const ComponentSet& thisComp );	// Choose a new action.
	void ThinkWander( const ComponentSet& thisComp );
	void ThinkBattle( const ComponentSet& thisComp );
	void ThinkRockBreak( const ComponentSet& thisComp );
	void ThinkBuild( const ComponentSet& thisComp );
	void ThinkVisitor( const ComponentSet& thisComp );
	void ThinkRampage( const ComponentSet& thisComp );	// process the rampage action

	void WorkQueueToTask(  const ComponentSet& thisComp );	// turn a work item into a task
	void FlushTaskList( const ComponentSet& thisComp, U32 delta );		// moves tasks along, mark tasks completed, do special actions
	void FindRoutineTasks( const ComponentSet& );	// do maintenance, etc.

	grinliz::Vector2F GetWanderOrigin( const ComponentSet& thisComp );
	int GetThinkTime() const { return 500; }
	WorkQueue* GetWorkQueue();

	// Returns true if this action was actually taken.
	bool ThinkWanderEatPlants( const ComponentSet& thisComp );
	bool ThinkWanderHealAtCore( const ComponentSet& thisComp );
	bool ThinkCriticalNeeds( const ComponentSet& thisComp );
	bool ThinkNeeds( const ComponentSet& thisComp );
	bool ThinkLoot( const ComponentSet& thisComp );
	bool ThinkDoRampage( const ComponentSet& thisComp );	// whether this MOB should rampage

	// What happens when no other move is working.
	grinliz::Vector2F ThinkWanderRandom( const ComponentSet& thisComp );
	// pseudo-flocking
	grinliz::Vector2F ThinkWanderFlock( const ComponentSet& thisComp );
	// creepy circle pacing
	grinliz::Vector2F ThinkWanderCircle( const ComponentSet& thisComp );

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
		grinliz::IString  name;

		grinliz::Vector3F MapTarget() const {
			GLASSERT( !mapPos.IsZero() );
			grinliz::Vector3F v = { (float)mapPos.x+0.5f, 0.5f, (float)mapPos.y+0.5f };
			return v;
		}

		void Clear() { id = 0; mapPos.Zero(); name = grinliz::IString(); }
		void Set( int _id ) { id = _id; mapPos.Zero(); name = grinliz::IString(); }
		void Set( const grinliz::Vector2I& _mapPos, grinliz::IString _name = grinliz::IString() ) 
			{ mapPos = _mapPos; id = 0; name = _name; }
		bool HasTarget() const { return id != 0 || !mapPos.IsZero(); }
	};

	int					aiMode;
	TargetDesc			targetDesc;
	int					currentAction;
	int					focus;
	CTicker				feTicker;
	U32					wanderTime;
	int					rethink;
	bool				fullSectorAware;
	int					visitorIndex;
	bool				debugFlag;
	int					rampageTarget;
	int					destinationBlocked;
	ai::TaskList		taskList;
	CTicker				needsTicker;
	ai::Needs			needs;

	static const char*	MODE_NAMES[NUM_MODES];
	static const char*	ACTION_NAMES[NUM_ACTIONS];

	void DoMelee( const ComponentSet& thisComp );
	void DoShoot( const ComponentSet& thisComp );
	void DoMove( const ComponentSet& thisComp );
	bool DoStand( const ComponentSet& thisComp, U32 since );	// return true if doing something
	bool SectorHerd( const ComponentSet& thisComp );

	grinliz::CDynArray< Chit* > chitArr;	// temporary, local
	grinliz::CArray<int, MAX_TRACK> friendList;
	grinliz::CArray<int, MAX_TRACK> enemyList;
};


#endif // AI_COMPONENT_INCLUDED