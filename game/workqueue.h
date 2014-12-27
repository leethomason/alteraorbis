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
#include "../grinliz/glrectangle.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"

#include "../gamui/gamui.h"

class WorldMap;
class XStream;
class LumosChitBag;
class Model;
class Engine;
class Wallet;
class Chit;


/*
	WorkQueue is higher level than the Task.
	The Task will reach back and check to see if
	the work item is still valid.
*/
class WorkQueue
{
public:
	WorkQueue();
	~WorkQueue();
	void InitSector( Chit* _parent, const grinliz::Vector2I& _sector );
	struct QueueItem {
		QueueItem() : buildScriptID(0), assigned(0), model(0), rotation(0), variation(0) { pos.Zero(); }

		void Serialize( XStream* xs );

		// Accounts for the size of the structure.
		grinliz::Rectangle2I Bounds() const;

		int					buildScriptID;	// BuildScript::CLEAR, ICE, VAULT, etc.
		grinliz::Vector2I	pos;
		int					assigned;		// id of worker assigned this task.			
		Model*				model;			// model used to show map work location
		float				rotation;
		int					variation;		// which PAVE
	};

	void Serialize( XStream* xs );
	// Manages what jobs there are to do:
	bool AddAction(const grinliz::Vector2I& pos, int buildScriptID, float rotation, int variation);	// add an action to do
	void Remove( const grinliz::Vector2I& pos );

	bool HasJob() const				{ return !queue.Empty(); }
	bool HasAssignedJob() const;
	bool HasJobAt(const grinliz::Vector2I& v);
	
	// Manages which chits are doing a job:
	const QueueItem*	Find( const grinliz::Vector2I& chitPos );	// find something to do. don't hold pointer!
	void				Assign( int id, const QueueItem* item );	// associate this chit with a job.
	const QueueItem*	GetJob( int chitID );						// get the current job, don't hold pointer!

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

	bool TaskIsComplete(const WorkQueue::QueueItem& item);

	static int  CalcTaskSize( const grinliz::IString& structure );

private:

	void AddImage( QueueItem* item );
	void RemoveImage( QueueItem* item );
	void RemoveItem( int index );
	void SendNotification( const grinliz::Vector2I& pos );

	Chit				*parentChit;
	grinliz::Vector2I	sector;
	grinliz::CDynArray< QueueItem >		queue;	// work to do
};

#endif // WORKQUEUE_ALTERA_INCLUDED
