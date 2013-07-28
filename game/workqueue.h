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

class WorkQueue
{
public:
	WorkQueue( WorldMap*, LumosChitBag*, Engine* );
	~WorkQueue();
	void InitSector( const grinliz::Vector2I& _sector ) { sector = _sector; }

	enum {
		NO_ACTION,
		CLEAR,
		BUILD,
		NUM_ACTIONS,
	};

	struct QueueItem {
		QueueItem() : action(NO_ACTION), assigned(0) { pos.Zero(); }
		QueueItem( int p_action, const grinliz::Vector2I& p_pos, grinliz::IString p_structure, int p_id ) 
			: action(p_action), pos(p_pos), assigned(0), structure(p_structure), taskID(p_id) {}

		void Serialize( XStream* xs );

		int					action;		// CLEAR_GRID, etc.
		grinliz::IString	structure;	// what to construct
		grinliz::Vector2I	pos;
		int					assigned;	// id of worker assigned this task.			
		int					taskID;		// id # of this task
	};

	void Serialize( XStream* xs );
	void Add( int action, const grinliz::Vector2I& pos, grinliz::IString structure );	// add an action to do
	void Remove( const grinliz::Vector2I& pos );
	
	const QueueItem*	Find( const grinliz::Vector2I& chitPos );	// find something to do. don't hold pointer!
	void				Assign( int id, const QueueItem* item );	// associate this chit with a job.
	const QueueItem*	GetJob( int chitID );						// get the current job, don't hold pointer!
	const QueueItem*	GetJobByTaskID( int taskID );
	void				ReleaseJob( int chitID );
	void				ClearJobs();

	void DoTick();	// mostly looks for work being complete.
	const grinliz::CDynArray< WorkQueue::QueueItem >& Queue() const { return queue; };	

private:

	void AddImage( const QueueItem& item );
	void RemoveImage( const QueueItem& item );
	void RemoveItem( int index );

	Engine*				engine;
	WorldMap*			worldMap;
	LumosChitBag*		chitBag;
	int					idPool;
	grinliz::Vector2I	sector;
	grinliz::CDynArray< QueueItem >		queue;	// work to do
	grinliz::CDynArray< gamui::Image* > images;	// images to tag the work
	grinliz::CDynArray< Model* >		models;	// models to tag the work
};

#endif // WORKQUEUE_ALTERA_INCLUDED
