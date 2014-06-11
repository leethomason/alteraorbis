#ifndef TASKLIST_INCLUDED
#define TASKLIST_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glstringutil.h"
#include "../xegame/cticker.h"

class Chit;
class WorldMap;
class Engine;
struct ComponentSet;
class ChitBag;

namespace ai {

/*	Attempt to put in a meta-language:
	Worker:		MOVE x,y
				STAND 1000 x,y	NOTE: x,y is of *task* not where standing
				REMOVE rock
*/
class Task
{
public:
	enum {
		TASK_MOVE = 1024,
		TASK_STAND,
		TASK_BUILD,		// any BuildScript action
		TASK_PICKUP,
		TASK_USE_BUILDING,
		TASK_REPAIR_BUILDING
	};

	Task() { Clear(); }
	
	static Task MoveTask( const grinliz::Vector2F& pos2 ) 
	{
		grinliz::Vector2I pos2i = { (int)pos2.x, (int)pos2.y };
		return MoveTask( pos2i );
	}

	static Task MoveTask( const grinliz::Vector2I& pos2i )
	{
		Task t;
		t.action = TASK_MOVE;
		t.pos2i  = pos2i;
		return t;
	}
	// Check and use location.
	static Task StandTask( int time ) {
		Task t;
		t.action = TASK_STAND;
		t.timer = time;
		return t;
	}
		
	// Assumes move first.
	static Task PickupTask( int chitID ) {
		Task t;
		t.action = TASK_PICKUP;
		t.data = chitID;
		return t;
	}

	static Task UseBuildingTask() {
		Task t;
		t.action = TASK_USE_BUILDING;
		return t;
	}

	static Task RepairTask(int buildingID) {
		Task t;
		t.action = TASK_REPAIR_BUILDING;
		t.data = buildingID;
		return t;
	}

	static Task RemoveTask( const grinliz::Vector2I& pos2i );

	static Task BuildTask( const grinliz::Vector2I& pos2i, int buildScriptID, int rotation) {
		Task t;
		t.action = TASK_BUILD;
		t.pos2i = pos2i;
		t.buildScriptID = buildScriptID;
		t.data = rotation;
		return t;
	}

	void Clear() {
		action = 0;
		pos2i.Zero();
		timer = 0;
		data = 0;
		buildScriptID = 0;
	}

	void Serialize( XStream* xs );

	int					action;		// move, stand, etc.
	int					buildScriptID;
	grinliz::Vector2I	pos2i;
	int					timer;
	int					data;
};


// A TaskList is a set of Tasks to do one thing. (Use a Vault, for example.)
// The WorkQueue is a list of all the TaskLists to do. (1. Move, 2. Stand, 3. Use Vault
class TaskList
{
public:
	TaskList()	: worldMap(0), engine(0), chitBag(0), socialTicker(2000) {}
	~TaskList()	{ Clear();  }

	void Init( WorldMap* _map, Engine* _engine, ChitBag* _cb ) {
		worldMap = _map;
		engine = _engine;
		chitBag = _cb;
	}

	void Serialize( XStream* xs );

	grinliz::Vector2I Pos2I() const;
	bool Empty() const { return taskList.Empty(); }
	const Task* GoalTask() const {
		if (!Empty()) return &taskList[taskList.Size() - 1];
		return 0;
	}
	
	void Push( const Task& task );
	void Clear();

	bool Standing() const { return !taskList.Empty() && taskList[0].action == Task::TASK_STAND; }
	// Using is a building is actually instantaneous: is the AI standing in order to use the building?
	bool UsingBuilding() const;

	void DoTasks( Chit* chit, U32 delta );

	grinliz::IString LastBuildingUsed() const { return lastBuildingUsed; }

private:
	void UseBuilding( const ComponentSet& thisComp, Chit* building, const grinliz::IString& buildingName );

	void GoShopping(const ComponentSet& thisComp, Chit* market);
	void GoExchange(const ComponentSet& thisComp, Chit* market);
	bool UseFactory(  const ComponentSet& thisComp, Chit* factory, int tech );
	bool DoStanding( const ComponentSet& thisComp, int time );

	// chat, basically, between denizens
	void SocialPulse( const ComponentSet& thisComp, const grinliz::Vector2F& origin );

	// Remove the 1st task.
	void Remove();

	WorldMap*			worldMap;
	Engine*				engine;
	ChitBag*			chitBag;
	grinliz::IString	lastBuildingUsed;	// should probably be serialized, if TaskList was serialized.
	CTicker				socialTicker;
	grinliz::CDynArray<Task> taskList;
};

}; // namespace 'ai'

#endif // TASKLIST_INCLUDED

	