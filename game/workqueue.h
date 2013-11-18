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

#ifndef WORKQUEUE_ALTERA_INCLUDED
#define WORKQUEUE_ALTERA_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"

#include "../gamui/gamui.h"

class WorldMap;
class XStream;
class LumosChitBag;
class Model;
class Engine;
struct Wallet;

class WorkQueue
{
public:
	WorkQueue( WorldMap*, LumosChitBag*, Engine* );
	~WorkQueue();
	void InitSector( const grinliz::Vector2I& _sector ) { sector = _sector; }

	struct QueueItem {
		QueueItem() : action(0), assigned(0), taskID(0), model(0) { pos.Zero(); }

		void Serialize( XStream* xs );

		int					action;		// BuildScript::CLEAR, ICE, VAULT, etc.
		grinliz::Vector2I	pos;
		int					assigned;	// id of worker assigned this task.			
		int					taskID;		// id # of this task
		Model*				model;		// model used to show map work location
	};

	void Serialize( XStream* xs );
	void AddAction( const grinliz::Vector2I& pos, int action );	// add an action to do
	//void AddClear( const grinliz::Vector2I& pos );
	void Remove( const grinliz::Vector2I& pos );
	
	const QueueItem*	Find( const grinliz::Vector2I& chitPos );	// find something to do. don't hold pointer!
	void				Assign( int id, const QueueItem* item );	// associate this chit with a job.
	const QueueItem*	GetJob( int chitID );						// get the current job, don't hold pointer!
	const QueueItem*	GetJobByTaskID( int taskID );
	void				ReleaseJob( int chitID );
	void				ClearJobs();

	void DoTick();	// mostly looks for work being complete.
	const grinliz::CDynArray< WorkQueue::QueueItem >& Queue() const { return queue; };	

	// Can this task be done at the location?
	// Stuff moves, commands change, etc.
	static bool TaskCanComplete(	WorldMap* worldMap, 
									LumosChitBag* chitBag,
									const grinliz::Vector2I& pos, 
									int action,
									const Wallet& availableFunds );

	bool TaskCanComplete( const WorkQueue::QueueItem& item );
	static int  CalcTaskSize( const grinliz::IString& structure );

private:

	void AddImage( QueueItem* item );
	void RemoveImage( QueueItem* item );
	void RemoveItem( int index );
	void SendNotification( const grinliz::Vector2I& pos );

	Engine*				engine;
	WorldMap*			worldMap;
	LumosChitBag*		chitBag;
	int					idPool;
	grinliz::Vector2I	sector;
	grinliz::CDynArray< QueueItem >		queue;	// work to do
};

#endif // WORKQUEUE_ALTERA_INCLUDED
