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

#ifndef LOOSEQUADTREE_INCLUDED
#define LOOSEQUADTREE_INCLUDED

#include "enginelimits.h"
#include "model.h"
#include "../grinliz/glmemorypool.h"


/*
	A tree for culling models. Used to be
	a loose tree. But made "standard" for
	debugging.

	May need a future tweak.
*/
class SpaceTree
{
public:
	enum {
		DEPTH = 6,
		MAX_ZONES = 1024,
		NUM_NODES = 1+4+16+64+256+1024
	};

	SpaceTree( float yMin, float yMax, int size );
	~SpaceTree();

	void SetLightDir( const grinliz::Vector3F& light );
	
	Model* AllocModel( const ModelResource* );
	void   FreeModel( Model* );

	// Called whenever a model moves. (Usually called automatically be the model.)
	void   Update( Model* );

	// Returns all the models in the planes.
	// Limits to BOTH planes and rectangle. Only one
	// needs to be specified.
	Model* Query( const grinliz::Plane* planes, int nPlanes, 
				  const grinliz::Rectangle3F* rectangle,
				  bool includeShadow,
				  int requiredFlags, int excludedFlags );

	// Returns all the valid areas from the last query. (used by voxel engine.)
	const grinliz::CArray<grinliz::Rectangle2I, MAX_ZONES>& Zones() const	{ return zones; }

	// Returns the FIRST model impacted.
	Model* QueryRay( const grinliz::Vector3F& origin, 
					 const grinliz::Vector3F& direction,	// does not need to be unit length, will be normalized
					 float length,							// maximum distance from origin to intersection or FLT_MAX
					 int required, int excluded, const Model* const * ignore,
					 HitTestMethod method,
					 grinliz::Vector3F* intersection );

#ifdef DEBUG
	// Draws debugging info about the spacetree.
	void Draw();
	void Dump() { Dump( nodeArr ); }
#endif

private:
	void ExpandForLight( grinliz::Rectangle3F* r ) {
		if ( lightXPerY > 0 )
			r->max.x += lightXPerY * r->max.y;
		else
			r->min.x += lightXPerY * r->max.y;

		if ( lightZPerY > 0 )
			r->max.z += lightXPerY * r->max.y;
		else
			r->min.z += lightXPerY * r->max.y;
	}

	struct Node;
#ifdef DEBUG
	void Dump( Node* node );
#endif

	struct Item {
		Model model;	// Must be first! Gets cast back to Item in destructor. When a model gets allocated,
						// this is where the memory for it is stored.
		Node* node;
		Item* next;		// used in the node list.
		Item* prev;
	};

	struct Node
	{
		grinliz::Rectangle3F aabb;	// for testing
		grinliz::Vector2I origin;	// for voxels

		int depth;
		int nModels;
		Item* root;
		Node* parent;
		Node* child[4];

#ifdef DEBUG
		mutable int hit;
#endif

		void Add( Item* item );
		void Remove( Item* item );
#ifdef DEBUG
		void Dump();
#endif
	};

	bool Ignore( const Model* m, const Model* const * ignore ) {
		if ( ignore ) {
			while ( *ignore ) {
				if ( *ignore == m )
					return true;
				++ignore;
			}
		}
		return false;
	}

	void InitNode();
	void QueryPlanesRec( const grinliz::Plane* planes, int nPlanes, const grinliz::Rectangle3F* rect, bool includeShadow, int intersection, const Node* node, U32 positive );

	grinliz::Rectangle3F treeBounds;
	Model* modelRoot;
	int size;

	float lightXPerY;
	float lightZPerY;

	int nodesVisited;
	int planesComputed;
	int spheresComputed;
	int modelsFound;

	int requiredFlags;
	int excludedFlags;

	grinliz::MemoryPool modelPool;

	Node* GetNode( int depth, int x, int z ); 
	Node nodeArr[NUM_NODES];
	grinliz::CArray<grinliz::Rectangle2I, MAX_ZONES> zones;
};

#endif // LOOSEQUADTREE_INCLUDED