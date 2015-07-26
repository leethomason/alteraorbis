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
#include "../game/visitor.h"

class WorldMap;
class Engine;
struct ComponentSet;
struct SectorPort;
class WorkQueue;
class RangedWeapon;

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
	virtual void OnAdd( Chit* chit, bool init );
	virtual void OnRemove();

	virtual int  DoTick( U32 delta );
	virtual void DebugStr( grinliz::GLString* str );
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual void OnChitEvent( const ChitEvent& event );

	// Approximate. enemyList may not be flushed.
	bool AwareOfEnemy() const { return enemyList2.Size() > 0; }

	// Tell the AI to take actions. If the "focused" is set,
	// it is given extra priority. The 'dest' must be in the same
	// sector. If 'sector' is also specfied, will do grid travel
	// after the move.
	void Move( const grinliz::Vector2F& dest, bool focused, const grinliz::Vector2F* normal=0 );
	// Returns true if the move is possible.
	bool Move( const SectorPort& sectorport, bool focused );
	void Pickup( Chit* item );
	void Rampage( int dest );
	bool Rampaging() const { return aiMode == AIMode::RAMPAGE_MODE;  }
	void GoSectorHerd(bool focus);	// forces a sector herd

	void Target( Chit* chit, bool focused );
	void Target(const grinliz::Vector2I& voxel, bool focused);
	bool TargetAdjacent(const grinliz::Vector2I& pos, bool focused);

	Chit* GetTarget();
	void MakeAware( const int* enemyIDs, int n );

	bool AtWaypoint();
	
	bool Build( const grinliz::Vector2I& pos, grinliz::IString structure );

	void EnableLog( bool enable )			{ debugLog = enable; }
	void SetSectorAwareness( bool aware )	{ fullSectorAware = aware; }
	void SetVisitorIndex( int i )			{ visitorIndex = i; }

	int  VisitorIndex() const				{ return visitorIndex; }
	VisitorData* GetVisitorData();

	const ai::Needs& GetNeeds() const		{ return needs; }
	ai::Needs* GetNeedsMutable()			{ return &needs; }

	// Core deleting, WorkItem removed, etc.
	void ClearTaskList()						{ taskList.Clear(); }
	const ai::Task* GoalTask() const			{ return taskList.GoalTask(); }

	// Top level AI modes. Higher level goals.
	// Translated to immediate goals: MOVE, SHOOT, MELEE
	enum class AIMode {
		NORMAL_MODE,
		RAMPAGE_MODE,		// a MOB that gets stuck can 'rampage', which means cutting a path through the world.
		BATTLE_MODE,
		NUM_MODES
	};

	// Secondary AI modes.
	enum class AIAction {
		NO_ACTION,
		MOVE,			// Movement, will reload and run&gun if possible.
		MELEE,			// Go to the target and hit it. Melee charge.
		SHOOT,			// Stand ground and shoot.
		NUM_ACTIONS,
	};

private:
	enum {
		MAX_TRACK = 8,
	};

	void Target( int id, bool focused );
	// Process the lists, makes sure they only include valid targets.
	void ProcessFriendEnemyLists(bool tick);

	// Compute the line of site
	bool LineOfSight(Chit* target, const RangedWeapon* weapon );
	bool LineOfSight(const grinliz::Vector2I& voxel );

	void Think();	// Choose a new action.
	void ThinkNormal();
	void ThinkBattle();
	void ThinkVisitor();
	void ThinkRampage();	// process the rampage action
	bool ThinkHungry();
	bool ThinkFruitCollect();
	bool ThinkDelivery();
	bool ThinkRepair();
	bool ThinkFlag();			// same domain "local" flags
	bool ThinkWaypoints();		// squad motion

	bool RampageDone();
	void DoMoraleZero();
	bool TravelHome(bool focus);
	void WorkQueueToTask();			// turn a work item into a task
	void FlushTaskList(U32 delta );	// moves tasks along, mark tasks completed, do special actions

	bool AtHomeCore();
	bool AtFriendlyOrNeutralCore();

	grinliz::Vector2F GetWanderOrigin();
	int GetThinkTime() const { return 500; }
	WorkQueue* GetWorkQueue();
	void FindFruit( const grinliz::Vector2F& origin, grinliz::Vector2F* dest, grinliz::CArray<Chit*, 32 >* arr, bool* nearPath );
	// find the correct position, if building, if mob, etc. Will return Zero if not available (happens with buildings)
	// if 'target' is true, returns the target, else returns the foot.
	// 'id' can be positive (Chit) or negative (voxel)
	grinliz::Vector3F EnemyPos(int id, bool target);

	// Returns true if this action was actually taken.
	bool ThinkCollectNearFruit();
	bool ThinkCriticalShopping();
	bool ThinkNeeds();
	bool ThinkLoot();
	bool ThinkDoRampage();	// whether this MOB should rampage
	bool ThinkGuard();

	// What happens when no other move is working.
	grinliz::Vector2F ThinkWanderRandom();
	// pseudo-flocking
	grinliz::Vector2F ThinkWanderHerd();
	// creepy circle pacing
	grinliz::Vector2F ThinkWanderCircle();

	grinliz::Vector2I RandomPosInRect( const grinliz::Rectangle2I& rect, bool excludeCenter );

	void DoMelee();
	void DoShoot();
	void DoMove();
	bool SectorHerd(bool focus);	// "upper" function: look for dest, etc.
	bool DoSectorHerd(bool focus, const grinliz::Vector2I& sector);	// lower function: go
	bool DoSectorHerd(bool focus, const SectorPort& sectorPort);	// lower function: go
	void EnterNewGrid();

	enum { 
		FOCUS_NONE,
		FOCUS_MOVE,
		FOCUS_TARGET
	};

	AIMode				aiMode;
	AIAction			currentAction;
	int					focus;
	int					lastTargetID;
	CTicker				feTicker;
	U32					wanderTime;
	int					rethink;
	bool				fullSectorAware;
	int					visitorIndex;
	bool				debugLog;
	int					rampageTarget;
	int					destinationBlocked;
	ai::TaskList		taskList;
	CTicker				needsTicker;
	ai::Needs			needs;
	grinliz::Vector2I	lastGrid;	// the environment affects morale and needs - but only check on new grids.

	static const char*	MODE_NAMES[int(AIMode::NUM_MODES)];
	static const char*	ACTION_NAMES[int(AIAction::NUM_ACTIONS)];

	grinliz::CDynArray< Chit* > chitArr;	// temporary, local
	grinliz::CArray<int, MAX_TRACK> friendList2;
	// The first entry is the current target.
	// If an entry is negative, then it is a map location to attack.
	grinliz::CArray<int, MAX_TRACK> enemyList2;
};


#endif // AI_COMPONENT_INCLUDED