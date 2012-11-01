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

#ifndef RENDERQUEUE_INCLUDED
#define RENDERQUEUE_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glcontainer.h"

#include "enginelimits.h"
#include "vertex.h"
#include "gpustatemanager.h"

class Model;
struct ModelAtom;
class Texture;


/* 
	The prevailing wisdom for GPU performance is to group the submission by 1)render state
	and 2) texture. In general I question this for tile based GPUs, but it's better to start
	with a queue architecture rather than have to retrofit later.

	RenderQueue queues up everything to be rendered. Flush() commits, and should be called 
	at the end of the render pass. Flush() may also be called automatically if the queues 
	are full.
*/
class RenderQueue
{
public:
	enum {
		MAX_STATE  = 128,
		MAX_ITEMS  = 1024,
	};

	RenderQueue();
	~RenderQueue();

	void Add(	Model* model,					// Can be chaned: billboard rotation will be set.
				const ModelAtom* atom, 
				const GPUState& state,
				const grinliz::Vector4F& param, 
				const grinliz::Matrix4* param4,
				const BoneData* boneData  );


	/* If a shader is passed it, it will override the shader set by the Add. */
	void Submit(	int modelRequired, 
					int modelExcluded,
					const grinliz::Matrix4* xform );

	bool Empty() { return nState == 0 && nItem == 0; }
	void Clear() { nState = 0; nItem = 0; }

private:
	struct Item {
		Model*					model;
		const ModelAtom*		atom;	
		grinliz::Vector4F		param;			// per instance data (vec4)
		const grinliz::Matrix4*	param4;			// per instance data (matrix)
		const BoneData*			boneData;
		Item*					next;
	};

	struct State {
		GPUState	state;
		Texture*	texture0;
		Item*		root;		// list of items in this state.
	};

	static int CompareState( const State& s0, const State& s1 ) 
	{
		GLASSERT( sizeof(GPUState) < 100 );	// just a sanity check.
		if ( s0.state == s1.state ) {
			if ( s0.texture0 == s1.texture0 )
				return 0;
			return ( s0.texture0 < s1.texture0 ) ? -1 : 1;
		}
		if ( s0.state.SortOrder() == s1.state.SortOrder() ) {
			return s0.state.Hash() < s1.state.Hash() ? -1 : 1;
		}
		return s0.state.SortOrder() - s1.state.SortOrder();
	}

	static int CompareAtom( const void* vi0, const void* vi1 ) 
	{
		const Item** i0 = (const Item**)vi0;
        const Item** i1 = (const Item**)vi1;
        return (int)((*i0)->atom) - (int)((*i1)->atom);
	}

	State* FindState( const State& state );

	int nState;
	int nItem;

	State statePool[MAX_STATE];
	Item  itemPool[MAX_ITEMS];
	grinliz::CArray< Item*, MAX_ITEMS > itemArr;
};


#endif //  RENDERQUEUE_INCLUDED