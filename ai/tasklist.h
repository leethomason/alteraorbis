#ifndef TASKLIST_INCLUDED
#define TASKLIST_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glstringutil.h"

class Chit;
class WorkQueue;
class WorldMap;

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
		TASK_REMOVE,
		TASK_BUILD,
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

	static Task RemoveTask( const grinliz::Vector2I& pos2i, int taskID=0 ) {
		Task t;
		t.action = TASK_REMOVE;
		t.pos2i = pos2i;
		t.taskID = taskID;
		return t;
	}
	static Task BuildTask( const grinliz::Vector2I& pos2i, const grinliz::IString& structure, int taskID=0 ) {
		Task t;
		t.action = TASK_BUILD;
		t.pos2i = pos2i;
		t.structure = structure;
		t.taskID = taskID;
		return t;
	}


	void Clear() {
		action = 0;
		pos2i.Zero();
		timer = 0;
		data = 0;
		structure = grinliz::IString();
		taskID = 0;
	}

	int					action;		// move, stand, etc.
	grinliz::Vector2I	pos2i;
	int					timer;
	int					data;
	grinliz::IString	structure;
	int					taskID;		// if !0, then attached to a workqueue task
};


class TaskList
{
public:
	TaskList();
	~TaskList();

	grinliz::Vector2I Pos2I() const;
	bool Empty() const { return taskList.Empty(); }
	
	void Push( const Task& task );
	void Clear()						{ taskList.Clear(); }

	bool Standing() const { return !taskList.Empty() && taskList[0].action == Task::TASK_STAND; }
	void DoStanding( int time );

	// WorkQueue is optional, but connects the tasks back to the queue.
	void DoTasks( Chit* chit, WorkQueue* workQueue, WorldMap* map );

private:
	grinliz::CDynArray<Task> taskList;
};

}; // namespace 'ai'

#endif // TASKLIST_INCLUDED

