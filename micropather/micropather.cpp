/*
Copyright (c) 2000-2009 Lee Thomason (www.grinninglizard.com)

Grinning Lizard Utilities.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/

#ifdef _MSC_VER
#pragma warning( disable : 4786 )	// Debugger truncating names.
#pragma warning( disable : 4530 )	// Exception handler isn't used
#endif

//#include <vector>
#include <memory.h>
#include <stdio.h>

//#define DEBUG_PATH
//#define DEBUG_PATH_DEEP
//#define TRACK_COLLISION


#include "micropather.h"
#include "../grinliz/gldebug.h"

using namespace std;
using namespace micropather;

class OpenQueue
{
  public:
	OpenQueue( Graph* _graph )
	{ 
		graph = _graph; 
		sentinel = (PathNode*) sentinelMem;
		sentinel->InitSentinel();
		#ifdef DEBUG
			sentinel->CheckList();
		#endif
	}
	~OpenQueue()	{}

	void Push( PathNode* pNode );
	PathNode* Pop();
	void Update( PathNode* pNode );
    
	bool Empty()	{ return sentinel->next == sentinel; }

  private:
	OpenQueue( const OpenQueue& );	// undefined and unsupported
	void operator=( const OpenQueue& );
  
	PathNode* sentinel;
	int sentinelMem[ ( sizeof( PathNode ) + sizeof( int ) ) / sizeof( int ) ];
	Graph* graph;	// for debugging
};


void OpenQueue::Push( PathNode* pNode )
{
	
	MPASSERT( pNode->inOpen == 0 );
	MPASSERT( pNode->inClosed == 0 );
	
#ifdef DEBUG_PATH_DEEP
	printf( "Open Push: " );
	graph->PrintStateInfo( pNode->state );
	printf( " total=%.1f\n", pNode->totalCost );		
#endif
	
	// Add sorted. Lowest to highest cost path. Note that the sentinel has
	// a value of FLT_MAX, so it should always be sorted in.
	MPASSERT( pNode->totalCost < FLT_MAX );
	PathNode* iter = sentinel->next;
	while ( true )
	{
		if ( pNode->totalCost < iter->totalCost ) {
			iter->AddBefore( pNode );
			pNode->inOpen = 1;
			break;
		}
		iter = iter->next;
	}
	MPASSERT( pNode->inOpen );	// make sure this was actually added.
#ifdef DEBUG
	sentinel->CheckList();
#endif
}

PathNode* OpenQueue::Pop()
{
	MPASSERT( sentinel->next != sentinel );
	PathNode* pNode = sentinel->next;
	pNode->Unlink();
#ifdef DEBUG
	sentinel->CheckList();
#endif
	
	MPASSERT( pNode->inClosed == 0 );
	MPASSERT( pNode->inOpen == 1 );
	pNode->inOpen = 0;
	
#ifdef DEBUG_PATH_DEEP
	printf( "Open Pop: " );
	graph->PrintStateInfo( pNode->state );
	printf( " total=%.1f\n", pNode->totalCost );		
#endif
	
	return pNode;
}

void OpenQueue::Update( PathNode* pNode )
{
#ifdef DEBUG_PATH_DEEP
	printf( "Open Update: " );		
	graph->PrintStateInfo( pNode->state );
	printf( " total=%.1f\n", pNode->totalCost );		
#endif
	
	MPASSERT( pNode->inOpen );
	
	// If the node now cost less than the one before it,
	// move it to the front of the list.
	if ( pNode->prev != sentinel && pNode->totalCost < pNode->prev->totalCost ) {
		pNode->Unlink();
		sentinel->next->AddBefore( pNode );
	}
	
	// If the node is too high, move to the right.
	if ( pNode->totalCost > pNode->next->totalCost ) {
		PathNode* it = pNode->next;
		pNode->Unlink();
		
		while ( pNode->totalCost > it->totalCost )
			it = it->next;
		
		it->AddBefore( pNode );
#ifdef DEBUG
		sentinel->CheckList();
#endif
	}
}


class ClosedSet
{
  public:
	ClosedSet( Graph* _graph )		{ this->graph = _graph; }
	~ClosedSet()	{}

	void Add( PathNode* pNode )
	{
		#ifdef DEBUG_PATH_DEEP
			printf( "Closed add: " );		
			graph->PrintStateInfo( pNode->state );
			printf( " total=%.1f\n", pNode->totalCost );		
		#endif
		#ifdef DEBUG
		MPASSERT( pNode->inClosed == 0 );
		MPASSERT( pNode->inOpen == 0 );
		#endif
		pNode->inClosed = 1;
	}

	void Remove( PathNode* pNode )
	{
		#ifdef DEBUG_PATH_DEEP
			printf( "Closed remove: " );		
			graph->PrintStateInfo( pNode->state );
			printf( " total=%.1f\n", pNode->totalCost );		
		#endif
		MPASSERT( pNode->inClosed == 1 );
		MPASSERT( pNode->inOpen == 0 );

		pNode->inClosed = 0;
	}

  private:
	ClosedSet( const ClosedSet& );
	void operator=( const ClosedSet& );
	Graph* graph;
};


void PathNode::Init(	unsigned _frame,
						float _costFromStart, 
						float _estToGoal, 
						PathNode* _parent )
{
	if ( _frame == 0 || _frame != frame ) {
		frame = _frame;
		costFromStart = _costFromStart;
		estToGoal = _estToGoal;
		CalcTotalCost();
		parent = _parent;
		inOpen = 0;
		inClosed = 0;
	}
}

MicroPather::MicroPather( Graph* _graph, unsigned allocate, bool symmetric )
	:	//pathNodePool( allocate, typicalAdjacent ),
		graph( _graph ),
		frame( 1 ),
		pathCache( allocate, symmetric )
//		checksum( 0 )
{}


MicroPather::~MicroPather()
{
}

	      
void MicroPather::Reset()
{
	pathCache.Reset();
}


void MicroPather::GoalReached( PathNode* node, PathNode* start, PathNode* end, MP_VECTOR< PathNode* > *_path )
{
	MP_VECTOR< PathNode* >& path = *_path;
	path.clear();

	// We have reached the goal.
	// How long is the path? Used to allocate the vector which is returned.
	int count = 1;
	PathNode* it = node;
	while( it->parent )
	{
		++count;
		it = it->parent;
	}

	// Now that the path has a known length, allocate
	// and fill the vector that will be returned.
	if ( count < 3 )
	{
		// Handle the short, special case.
		path.resize(2);
		path[0] = start;
		path[1] = end;
	}
	else
	{
		path.resize(count);

		path[0] = start;
		path[count-1] = end;
		count-=2;
		it = node->parent;

		while ( it->parent )
		{
			path[count] = it;
			it = it->parent;
			--count;
		}
	}
	pathCache.Add( path );

	#ifdef DEBUG_PATH
	printf( "Path: " );
	int counter=0;
	#endif
	for ( unsigned k=0; k<path.size(); ++k )
	{
		#ifdef DEBUG_PATH
		graph->PrintStateInfo( path[k] );
		printf( " " );
		++counter;
		if ( counter == 8 )
		{
			printf( "\n" );
			counter = 0;
		}
		#endif
	}
	#ifdef DEBUG_PATH
	printf( "Cost=%.1f Checksum %d\n", node->costFromStart, checksum );
	#endif
}

/*
void MicroPather::GetNodeNeighbors( PathNode* node )
{
	graph->AdjacentCost( node, &node->adjacent, &node->numAdjacent );
}
*/

#ifdef DEBUG
/*
void MicroPather::DumpStats()
{
	int hashTableEntries = 0;
	for( int i=0; i<HASH_SIZE; ++i )
		if ( hashTable[i] )
			++hashTableEntries;
	
	int pathNodeBlocks = 0;
	for( PathNode* node = pathNodeMem; node; node = node[ALLOCATE-1].left )
		++pathNodeBlocks;
	printf( "HashTableEntries=%d/%d PathNodeBlocks=%d [%dk] PathNodes=%d SolverCalled=%d\n",
			  hashTableEntries, HASH_SIZE, pathNodeBlocks, 
			  pathNodeBlocks*ALLOCATE*sizeof(PathNode)/1024,
			  pathNodeCount,
			  frame );
}
*/
#endif

PathCache::PathCache( int _allocated, bool _symmetric )
{
	mem = new Item[_allocated];
	memset( mem, 0, sizeof(Item)*_allocated );
	allocated = _allocated;
	nItems = 0;
	symmetric = _symmetric;
}


PathCache::~PathCache()
{
	delete [] mem;
}


void PathCache::Reset()
{
	if ( nItems ) {
		memset( mem, 0, sizeof(*mem)*allocated );
		nItems = 0;
	}
}


void PathCache::Add( const MP_VECTOR< PathNode* >& path )
{
	if ( nItems+(int)path.size() > allocated * 2 / 3 ) {
		return;
	}

	PathNode* start = path[0];
	PathNode* end   = path[path.size()-1];

	for( unsigned i=0; i<path.size()-1; ++i ) {
		Item item = { path[i], end, path[i+1], path[i+1]->costFromStart - path[i]->costFromStart };
		AddItem( item );
	}
	if ( symmetric ) {
		for( unsigned i=path.size()-1; i>0; --i ) {
			Item item = { path[i], start, path[i-1], path[i]->costFromStart - path[i-1]->costFromStart };
			AddItem( item );
		}
	}
}


int PathCache::Solve( PathNode* start, PathNode* end, MP_VECTOR< PathNode* >* path, float* totalCost )
{
	Item* item = Find( start, end );
	if ( item ) {
		path->clear();
		path->push_back( start );
		*totalCost = 0;

		for ( ;start != end; start=item->next, item=Find(start, end) ) {
			*totalCost += item->cost;
			path->push_back( item->next );
		}
		return MicroPather::SOLVED;
	}
	return MicroPather::NOT_CACHED;
}


void PathCache::AddItem( const Item& item )
{
	/*
	if ( allocated == 0 ) {
		allocated = 1000;
		mem = new Item[allocated];
	}
	else if ( nItems > (allocated*2/3) ) {
		int oldAllocated = allocated;
		Item* oldMem = mem;

		allocated *= 2;
		mem = new Item[allocated];
		nItems = 0;

		for( int i=0; i<oldAllocated; ++i ) {
			if ( oldMem[i].start ) {
				AddItem( oldMem[i] );
			}
		}
		delete [] oldMem;
	}
	*/
	unsigned index = item.Hash() % allocated;
	while( true ) {
		if ( mem[index].start == 0 ) {
			mem[index] = item;
			break;
		}
		++index;
		if ( index == allocated )
			index = 0;
	}
}


PathCache::Item* PathCache::Find( PathNode* start, PathNode* end )
{
	Item fake = { start, end, 0, 0 };
	unsigned index = fake.Hash() % allocated;
	while( true ) {
		if ( mem[index].start == 0 ) {
			return 0;
		}
		if ( mem[index].start == start && mem[index].end == end ) {
			return mem + index;
		}
		++index;
		if ( index == allocated )
			index = 0;
	}
}


int MicroPather::Solve( PathNode* startNode, PathNode* endNode, MP_VECTOR< PathNode* >* path, float* cost )
{
	// Important to clear() in case the caller doesn't check the return code. There
	// can easily be a left over path  from a previous call.
	path->clear();

	#ifdef DEBUG_PATH
	printf( "Path: " );
	graph->PrintStateInfo( startNode );
	printf( " --> " );
	graph->PrintStateInfo( endNode );
	printf( " min cost=%f\n", graph->LeastCostEstimate( startNode, endNode ) );
	#endif

	*cost = 0.0f;
	++frame;

	if ( startNode == endNode )
		return START_END_SAME;

	int cacheResult = pathCache.Solve( startNode, endNode, path, cost );
	if ( cacheResult == SOLVED || cacheResult == NO_SOLUTION ) {
		GLOUTPUT(( "PathCache hit. result=%s\n", cacheResult == SOLVED ? "solved" : "no_solution" ));
		return cacheResult;
	}
	GLOUTPUT(( "PathCache miss\n" ));

	OpenQueue open( graph );
	ClosedSet closed( graph );
	
	startNode->Init( frame, 0, graph->LeastCostEstimate( startNode, endNode ), 0 );
	open.Push( startNode );	
	stateCostVec.resize(0);

	while ( !open.Empty() )
	{
		PathNode* node = open.Pop();
		
		if ( node == endNode )
		{
			GoalReached( node, startNode, endNode, path );
			*cost = node->costFromStart;
			#ifdef DEBUG_PATH
			DumpStats();
			#endif
			return SOLVED;
		}
		else
		{
			closed.Add( node );

			// We have not reached the goal - add the neighbors.
			int nAdjacent = 0;
			StateCost* pAdjacent = 0;
			graph->AdjacentCost( node, &pAdjacent, &nAdjacent );

			for( int i=0; i<nAdjacent; ++i )
			{
				// Not actually a neighbor, but useful. Filter out infinite cost.
				if ( pAdjacent[i].cost == FLT_MAX ) {
					continue;
				}
				// Does nothing if the frame is the same.
				pAdjacent[i].state->Init(	frame, 
											node->costFromStart+pAdjacent[i].cost, 
											graph->LeastCostEstimate( pAdjacent[i].state, endNode ),
											node );

				PathNode* child = pAdjacent[i].state;
				float newCost = node->costFromStart + pAdjacent[i].cost;

				PathNode* inOpen   = child->inOpen ? child : 0;
				PathNode* inClosed = child->inClosed ? child : 0;
				PathNode* inEither = (PathNode*)( ((MP_UPTR)inOpen) | ((MP_UPTR)inClosed) );

				MPASSERT( inEither != node );
				MPASSERT( !( inOpen && inClosed ) );

				if ( inEither ) {
					if ( newCost < child->costFromStart ) {
						child->parent = node;
						child->costFromStart = newCost;
						child->estToGoal = graph->LeastCostEstimate( child, endNode );
						child->CalcTotalCost();
						if ( inOpen ) {
							open.Update( child );
						}
					}
				}
				else {
					child->parent = node;
					child->costFromStart = newCost;
					child->estToGoal = graph->LeastCostEstimate( child, endNode ),
					child->CalcTotalCost();
					
					MPASSERT( !child->inOpen && !child->inClosed );
					open.Push( child );
				}
			}
		}					
	}
	#ifdef DEBUG_PATH
	DumpStats();
	#endif
	return NO_SOLUTION;		
}	


int MicroPather::SolveForNearStates( PathNode* startState, MP_VECTOR< StateCost >* near, float maxCost )
{
	/*	 http://en.wikipedia.org/wiki/Dijkstra%27s_algorithm

		 1  function Dijkstra(Graph, source):
		 2      for each vertex v in Graph:           // Initializations
		 3          dist[v] := infinity               // Unknown distance function from source to v
		 4          previous[v] := undefined          // Previous node in optimal path from source
		 5      dist[source] := 0                     // Distance from source to source
		 6      Q := the set of all nodes in Graph
				// All nodes in the graph are unoptimized - thus are in Q
		 7      while Q is not empty:                 // The main loop
		 8          u := vertex in Q with smallest dist[]
		 9          if dist[u] = infinity:
		10              break                         // all remaining vertices are inaccessible from source
		11          remove u from Q
		12          for each neighbor v of u:         // where v has not yet been removed from Q.
		13              alt := dist[u] + dist_between(u, v) 
		14              if alt < dist[v]:             // Relax (u,v,a)
		15                  dist[v] := alt
		16                  previous[v] := u
		17      return dist[]
	*/

	++frame;

	OpenQueue open( graph );			// nodes to look at
	ClosedSet closed( graph );

	stateCostVec.resize(0);

	PathNode closedSentinel;
	closedSentinel.Init( frame, FLT_MAX, FLT_MAX, 0 );
	closedSentinel.next = closedSentinel.prev = &closedSentinel;

	startState->Init( frame, 0, 0, 0 );
	open.Push( startState );
	
	while ( !open.Empty() )
	{
		PathNode* node = open.Pop();	// smallest dist
		closed.Add( node );				// add to the things we've looked at
		closedSentinel.AddBefore( node );
			
		if ( node->totalCost > maxCost )
			continue;		// Too far away to ever get here.

		int numAdjacent = 0;
		StateCost *pAdjacent = 0;
		graph->AdjacentCost( node, &pAdjacent, &numAdjacent );

		for( int i=0; i<numAdjacent; ++i )
		{
			MPASSERT( node->costFromStart < FLT_MAX );
			float newCost = node->costFromStart + pAdjacent[i].cost;

			PathNode* inOpen   = pAdjacent[i].state->inOpen   ? pAdjacent[i].state : 0;
			PathNode* inClosed = pAdjacent[i].state->inClosed ? pAdjacent[i].state : 0;
			MPASSERT( !( inOpen && inClosed ) );
			PathNode* inEither = inOpen ? inOpen : inClosed;
			MPASSERT( inEither != node );

			if ( inEither && inEither->costFromStart <= newCost ) {
				continue;	// Do nothing. This path is not better than existing.
			}
			// Groovy. We have new information or improved information.
			PathNode* child = pAdjacent[i].state;
			//MPASSERT( child != newPathNode );	// should never re-process the parent.

			child->parent = node;
			child->costFromStart = newCost;
			child->estToGoal = 0;
			child->totalCost = child->costFromStart;

			if ( inOpen ) {
				open.Update( inOpen );
			}
			else if ( !inClosed ) {
				open.Push( child );
			}
		}
	}	
	near->clear();

	for( PathNode* pNode=closedSentinel.next; pNode != &closedSentinel; pNode=pNode->next ) {
		if ( pNode->totalCost <= maxCost ) {
			StateCost sc;
			sc.cost = pNode->totalCost;
			sc.state = pNode;

			near->push_back( sc );
		}
	}
#ifdef DEBUG
	for( unsigned i=0; i<near->size(); ++i ) {
		for( unsigned k=i+1; k<near->size(); ++k ) {
			MPASSERT( (*near)[i].state != (*near)[k].state );
		}
	}
#endif

	return SOLVED;
}




