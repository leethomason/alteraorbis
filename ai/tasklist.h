#ifndef TASKLIST_INCLUDED
#define TASKLIST_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glstringutil.h"

class Chit;
class WorkQueue;
class WorldMap;
class Engine;
struct ComponentSet;

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
		TASK_USE_BUILDING
	};

	Task() { Clear(); }
	
	static Task MoveTask( const grinliz::Vector2F& pos2, int taskID=0 ) 
	{
		grinliz::Vector2I pos2i = { (int)pos2.x, (int)pos2.y };
		return MoveTask( pos2i, taskID );
	}

	static Task MoveTask( const grinliz::Vector2I& pos2i, int taskID=0 )
	{
		Task t;
		t.action = TASK_MOVE;
		t.pos2i  = pos2i;
		t.taskID = taskID;
		return t;
	}
	// FIXME: many stands don't work if you move off the healing place, building, etc.
	// Check and use location.
	static Task StandTask( int time, int taskID=0 ) {
		Task t;
		t.action = TASK_STAND;
		t.timer = time;
		t.taskID = taskID;
		return t;
	}
		
	// Assumes move first.
	static Task PickupTask( int chitID, int taskID=0 ) {
		Task t;
		t.action = TASK_PICKUP;
		t.data = chitID;
		t.taskID = taskID;
		return t;
	}

	static Task UseBuildingTask( int taskID=0 ) {
		Task t;
		t.action = TASK_USE_BUILDING;
		t.taskID = taskID;
		return t;
	}

	static Task RemoveTask( const grinliz::Vector2I& pos2i, int taskID=0 );

	static Task BuildTask( const grinliz::Vector2I& pos2i, int buildScriptID, int taskID=0 ) {
		Task t;
		t.action = TASK_BUILD;
		t.pos2i = pos2i;
		t.buildScriptID = buildScriptID;
		t.taskID = taskID;
		return t;
	}

	void Clear() {
		action = 0;
		pos2i.Zero();
		timer = 0;
		data = 0;
		buildScriptID = 0;
		taskID = 0;
	}

	int					action;		// move, stand, etc.
	int					buildScriptID;
	grinliz::Vector2I	pos2i;
	int					timer;
	int					data;
	int					taskID;		// if !0, then attached to a workqueue task
};


// A TaskList is a set of Tasks to do one thing. (Visit a Vault, for example.)
// The WorkQueue is a list of all the TaskLists to do.
class TaskList
{
public:
	TaskList( WorldMap* _map, Engine* _engine )	: worldMap(_map), engine(_engine) {}
	~TaskList()	{}

	grinliz::Vector2I Pos2I() const;
	bool Empty() const { return taskList.Empty(); }
	
	void Push( const Task& task );
	void Clear()						{ taskList.Clear(); }

	bool Standing() const { return !taskList.Empty() && taskList[0].action == Task::TASK_STAND; }
	void DoStanding( int time );

	// WorkQueue is optional, but connects the tasks back to the queue.
	void DoTasks( Chit* chit, WorkQueue* workQueue, U32 delta );
	grinliz::IString LastBuildingUsed() const { return lastBuildingUsed; }

private:
	void UseBuilding( const ComponentSet& thisComp, Chit* building, const grinliz::IString& buildingName );
	void GoShopping(  const ComponentSet& thisComp, Chit* market );
	bool UseFactory(  const ComponentSet& thisComp, Chit* factory, int tech );

	// chat, basically, between denizens
	void SocialPulse( const ComponentSet& thisComp, const grinliz::Vector2F& origin );

	WorldMap*	worldMap;
	Engine*		engine;
	grinliz::IString lastBuildingUsed;	// should probably be serialized, if this was serialized.
	grinliz::CDynArray<Task> taskList;
};

}; // namespace 'ai'

#endif // TASKLIST_INCLUDED

