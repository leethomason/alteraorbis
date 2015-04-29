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

#include "loosequadtree.h"
#include "map.h"
#include "gpustatemanager.h"

#include "../grinliz/glutil.h"
#include "model.h"

#include <float.h>

using namespace grinliz;

static const int MODEL_BLOCK = 10*1000;

/*
	Tuning: (from Xenowar)
	Unit tree:  Query checked=153, computed=145 of 341 nodes, models=347 [35,44,40,26,606]
	Classic:	Query checked=209, computed=209 of 341 nodes, models=361 [1,36,36,51,627]
	Unit tree has less compution and fewer models, slightly less balanced. 
*/

SpaceTree::SpaceTree( float yMin, float yMax, int _size ) 
{
	GLASSERT( _size >> (DEPTH-1) );
	size = _size;
	treeBounds.Set( 0, yMin, 0, (float)size, yMax, (float)size );
	lightXPerY = 0;
	lightZPerY = 0;
	InitNode();
}


SpaceTree::~SpaceTree()
{
	for (int i = 0; i < NUM_NODES; ++i) {
		GLASSERT(nodeArr[i].root == 0);
		GLASSERT(nodeArr[i].nModels == 0);
	}
}


void SpaceTree::InitNode()
{
	int depth = 0;
	int nodeSize = size;

	while ( depth < DEPTH ) 
	{
		for( int j=0; j<size; j+=nodeSize ) 
		{
			for( int i=0; i<size; i+=nodeSize ) 
			{
				Node* node = GetNode( depth, i, j );
				node->parent = 0;
				node->nModels = 0;
				if ( depth > 0 ) {
					node->parent = GetNode( depth-1, i, j );
				}

				node->aabb.Set( (float)(i), treeBounds.min.y, (float)(j),
								(float)(i+nodeSize), treeBounds.max.y, (float)(j+nodeSize) );
				node->origin.Set( i, j );

				node->depth = depth;
				node->root = 0;

				if ( depth+1 < DEPTH ) {
					node->child[0] = GetNode( depth+1, i,				j );
					node->child[1] = GetNode( depth+1, i+nodeSize/2,	j );
					node->child[2] = GetNode( depth+1, i,				j+nodeSize/2 );
					node->child[3] = GetNode( depth+1, i+nodeSize/2,	j+nodeSize/2 );
				}
				else { 
					node->child[0] = 0;
					node->child[1] = 0;
					node->child[2] = 0;
					node->child[3] = 0;
				}
			}
		}
		++depth;
		nodeSize >>= 1;
	}
}


void SpaceTree::SetLightDir( const Vector3F& light )
{
	GLASSERT( light.y > 0 );

	lightXPerY = -light.x / light.y;
	lightZPerY = -light.z / light.y;
}



void SpaceTree::Update( Model* model )
{
	GLASSERT(model->spaceTree == 0 || model->spaceTree == this);
	if (!model->spaceTree) {
		model->spaceTree = this;
	}

	// This call is very expensive. 3ms (approx bounds) to 20ms in debug mode.
	//Rectangle3F bounds = model->AABB();
	Rectangle3F bounds = model->GetInvariantAABB( lightXPerY, lightZPerY );

	if (model->spaceTreeNode) {
		Node* node = (Node*)model->spaceTreeNode;
		node->Remove(model);
	}
	GLASSERT(model->spaceTreeNext == 0);
	GLASSERT(model->spaceTreePrev == 0);
	GLASSERT(model->spaceTreeNode == 0);

	// Since the tree is somewhat modified from the ideal, start with the 
	// most ideal node and work up. Note that everything fits at the top node.
	int depth = DEPTH-1;
	Node* node = 0;
	
	bounds.DoIntersection( treeBounds );

	Vector3F c = bounds.Center();
	int x = LRint(c.x);
	int z = LRint(c.z);

	while( depth > 0 ) {
		node = GetNode( depth, x, z );
		if ( node->aabb.Contains( bounds ) ) {
			// fits.
			break;
		}
		--depth;
	}
	if (depth == 0) {
		node = nodeArr;	// everything fits at the root.
	}
	node->Add(model);
}



SpaceTree::Node* SpaceTree::GetNode( int depth, int x, int z )
{
	GLASSERT( depth >=0 && depth < DEPTH );
	int size = this->size >> depth;
	int nx = x / size;
	int nz = z / size;

	static const int base[DEPTH] = { 0, 1, 1+4, 1+4+16, 1+4+16+64, 1+4+16+64+256 };
	int dx = (1<<depth);

	nx = Clamp( nx, 0, dx-1 );	// error correction...
	nz = Clamp( nz, 0, dx-1 );	// error correction...

	Node* result = &nodeArr[ base[depth] + nz*dx+nx ];
	GLASSERT( result >= nodeArr && result < &nodeArr[NUM_NODES] );
	return result;
}

void SpaceTree::Query(grinliz::CDynArray<Model*>* models, const Plane* planes, int nPlanes, const Rectangle3F* clipRect, bool includeShadow, int required, int excluded)
{
	nodesVisited = 0;
	planesComputed = 0;
	requiredFlags = required;
	excludedFlags = excluded;
	zones.Clear();

	QueryPlanesRec(models, planes, nPlanes, clipRect, includeShadow, grinliz::INTERSECT, &nodeArr[0], 0);
}


void SpaceTree::Node::Add(Model* model)
{
	GLASSERT(model->spaceTreeNext == 0);
	GLASSERT(model->spaceTreePrev == 0);
	GLASSERT(model->spaceTreeNode == 0);

	if (root) {
		root->spaceTreePrev = model;
	}
	model->spaceTreeNode = this;
	model->spaceTreeNext = root;
	model->spaceTreePrev = 0;
	root = model;

	for (Node* it = this; it; it = it->parent)
		it->nModels++;
}


void SpaceTree::Node::Remove( Model* model ) 
{
	GLASSERT(model->spaceTreeNode == this);
	if (root == model)
		root = model->spaceTreeNext;
	if ( model->spaceTreePrev )
		model->spaceTreePrev->spaceTreeNext = model->spaceTreeNext;
	if ( model->spaceTreeNext )
		model->spaceTreeNext->spaceTreePrev = model->spaceTreePrev;

	model->spaceTreeNode = 0;
	model->spaceTreeNext = 0;
	model->spaceTreePrev = 0;

	for( Node* it=this; it; it=it->parent )
		it->nModels--;
}


#ifdef DEBUG
void SpaceTree::Node::Dump()
{
	for( int i=0; i<depth; ++i )
		GLOUTPUT(( "  " ));
	GLOUTPUT(( "depth=%d nModels=%d (%.1f,%.1f)-(%.1f,%.1f)\n",
				depth, nModels, 
				aabb.min.x, aabb.min.z, 
				aabb.max.x, aabb.max.z ));
}


void SpaceTree::Dump( Node* node )
{
	node->Dump();
	for( int i=0; i<4; ++i )
		if ( node->child[i] )
			Dump( node->child[i] );
}
#endif


void SpaceTree::QueryPlanesRec(grinliz::CDynArray<Model*>* models,
							   const Plane* planes, int nPlanes,
							   const Rectangle3F* clipRect,
							   bool includeShadow,
							   int intersection, const Node* node, U32 positive)
{
#define IS_POSITIVE( pos, i ) ( pos & (1<<i) )
	const int allPositive = (1 << nPlanes) - 1;

	if (intersection == grinliz::POSITIVE)
	{
		// we are fully inside, and don't need to check.
		++nodesVisited;
	}
	else if (intersection == grinliz::INTERSECT)
	{
		const Rectangle3F& aabb = node->aabb;

		if (clipRect) {
			if (!clipRect->Intersect(aabb)) {
				intersection = grinliz::NEGATIVE;
			}
			else if (nPlanes == 0 && clipRect->Contains(aabb)) {
				intersection = grinliz::POSITIVE;
			}
		}
		if (nPlanes && intersection == grinliz::INTERSECT) {
			for (int i = 0; i < nPlanes; ++i) {
				// Assume positive bit is set, check if not. Once we go POSITIVE, we don't need 
				// to check the sub-nodes. They will still be POSITIVE. Saves about 1/2 the Plane
				// and Sphere checks.
				//
				int comp = grinliz::POSITIVE;
				if (IS_POSITIVE(positive, i) == 0) {
					comp = ComparePlaneAABB(planes[i], aabb);
					++planesComputed;
				}

				// If the aabb is negative of any plane, it is culled.
				if (comp == grinliz::NEGATIVE) {
					intersection = grinliz::NEGATIVE;
					break;
				}
				else if (comp == grinliz::POSITIVE) {
					// If the aabb intersects the plane, the result is subtle:
					// intersecting a plane doesn't meen it is in the frustrum. 
					// The intersection of the aabb and the plane is often 
					// completely outside the frustum.

					// If the aabb is positive of ALL the planes then we are in good shape.
					positive |= (1 << i);
				}
			}
			if (positive == allPositive) {
				// All positive is quick:
				intersection = grinliz::POSITIVE;
			}
		}
		++nodesVisited;
	}
	if (intersection != grinliz::NEGATIVE)
	{
		if (node->depth == DEPTH - 1) {
			// Write the data for the voxel test. It
			// needs a record of all the nodes in view.
			Rectangle2I voxel;
			voxel.min = voxel.max = node->origin;
			int c = (size >> (node->depth));
			voxel.max.x += c - 1;
			voxel.max.y += c - 1;
			zones.Push(voxel);
		}

		const int _requiredFlags = requiredFlags;
		const int _excludedFlags = excludedFlags;

		for (Model* m = node->root; m; m = m->spaceTreeNext)
		{
			const int flags = m->Flags();

			if (((_requiredFlags & flags) == _requiredFlags)
				&& ((_excludedFlags & flags) == 0))
			{
				//GLOUTPUT(( "%*s[%d] Testing: 0x%x %s", node->depth, " ", node->depth, (int)m, m->GetResource()->header.name.c_str() ));
				if (intersection == grinliz::INTERSECT) {
					Rectangle3F aabb = m->AABB();
					if (includeShadow && m->CastsShadow()) {
						ExpandForLight(&aabb);
					}
					int compare = grinliz::INTERSECT;

					for (int k = 0; k < nPlanes; ++k) {
						// Since the bounding sphere is in the AABB, we
						// can check for the positive plane again.
						if (IS_POSITIVE(positive, k) == 0) {
							compare = ComparePlaneAABB(planes[k], aabb);
							if (compare == grinliz::NEGATIVE) {
								break;
							}
						}
					}
					if (compare == grinliz::NEGATIVE) {
						//GLOUTPUT(( "...NEGATIVE\n" ));
						continue;
					}
				}
				//GLOUTPUT(( "...yes\n" ));
				models->Push(m);
			}
		}

		if (node->child[0])  {
			// We could, in theory, early out using the number of models. But this makes the
			// zones for the voxel system super-big, which is a big problem.
			for (int i = 0; i < 4; ++i) {
				QueryPlanesRec(models, planes, nPlanes, clipRect, includeShadow, intersection, node->child[i], positive);
			}
		}
	}
#undef IS_POSITIVE
}


Model* SpaceTree::QueryRay( const Vector3F& _origin, 
							const Vector3F& _direction, 
							float length,
							int required, int excluded, const Model* const * ignore,
							HitTestMethod testType,
							Vector3F* intersection ) 
{
	//GLOUTPUT(( "query ray\n" ));
	nodesVisited = 0;
	requiredFlags = required;
	excludedFlags = excluded;

	Vector3F dummy;
	if ( !intersection ) {
		intersection = &dummy;
	}

	Vector3F dir = _direction;
	dir.Normalize();

	Rectangle3F aabb = treeBounds;

	// Where does this ray enter and leave the spaceTree?
	// It enters at 'p0' and leaves at 'p1'
	int p0Test, p1Test;
	Vector3F p0, p1;
	int test = IntersectRayAllAABB( _origin, dir, aabb, &p0Test, &p0, &p1Test, &p1 );
	if ( test != grinliz::INTERSECT ) {
		// Can click outside of AABB pretty commonly, actually.
		return 0;
	}
	Rectangle3F worldBounds;
	worldBounds.FromPair( p0, p1 );

	Rectangle3F rayBounds;
	rayBounds.FromPair( _origin, _origin+dir*length );

	Rectangle3F pathBounds = rayBounds;
	pathBounds.DoIntersection( worldBounds );

	queryCache.Clear();
	Query( &queryCache, 0, 0, &pathBounds, false, required, excluded );

	// We now have a batch of models. Are any of them a hit??
	GLASSERT( testType == TEST_HIT_AABB || testType == TEST_TRI );

	float close = length;
	Model* closeModel = 0;
	Vector3F testInt;
	float t;

	for (int i = 0; i < queryCache.Size(); ++i) {
		Model* root = queryCache[i];

		if ( Ignore( root, ignore ) )
			continue;

		//GLOUTPUT(( "Consider: %s\n", root->GetResource()->header.name ));
		t = FLT_MAX;
		int result = root->IntersectRay((testType == TEST_TRI), p0, dir, &testInt);

		if ( result == grinliz::INTERSECT ) {
			Vector3F delta = p0 - testInt;
			t = delta.Length();
			//GLOUTPUT(( "Hit: %s t=%.2f\n", root->GetResource()->header.name, t ));
		}	

		if ( result == grinliz::INTERSECT ) {
			// Ugly little bug: check for t>=0, else could collide with objects
			// that touch the bounding box but are before the ray starts.
			if ( t >= 0.0f && t <= close ) {
				closeModel = root;
				*intersection = testInt;
				close = t;
			}
		}
	}
	if ( closeModel ) {
		return closeModel;
	}
	return 0;
}


#ifdef DEBUG
void SpaceTree::Draw()
{
	/*
	CompositingShader shader( true );

	for( int i=0; i<NUM_NODES; ++i ) {
		if ( nodeArr[i].hit )
		{
			const Node& node = nodeArr[i];
			if ( node.depth < 4 )
				continue;

			const float y = 0.2f;
			const float offset = 0.05f;
//			float v[12] = {	
//							node.x+offset, y, node.z+offset,
//							node.x+offset, y, (node.z+node.size)-offset,
//							(node.x+node.size)-offset, y, (node.z+node.size)-offset,
//							(node.x+node.size)-offset, y, node.z+offset
//						  };
			GLASSERT( 0 );

			if ( node.hit == 1 )
				shader.SetColor( 1.f, 0.f, 0.f, 0.5f );
			else
				shader.SetColor( 0.f, 1.f, 0.f, 0.5f );

			static const uint16_t index[6] = { 0, 1, 2, 0, 2, 3 };
			shader.Draw( 6, index );
		}
	}
	*/
}
#endif
