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
	A simple grouping queue. States are ordered (see CompareState) in prioritizing:
	- pass (opaque, blend)
	- state (which defines the fixed state)
	- texture

	Within a run of items, ModelAtoms are grouped together and rendererd via instancing.
*/
class RenderQueue
{
public:
	RenderQueue();
	~RenderQueue();

	void Add(	Model* model,
				const ModelAtom* atom, 
				const GPUState& state,
				const grinliz::Vector4F& color,
				const grinliz::Vector4F& filter,
				const grinliz::Vector4F& control,
				const grinliz::Matrix4* param4,
				const BoneData* boneData  );


	/* If a shader is passed it, it will override the shader set by the Add. */
	void Submit(	int modelRequired, 
					int modelExcluded,
					const grinliz::Matrix4* xform );

	bool Empty() { return itemPool.Empty(); }
	void Clear() { itemPool.Clear(); itemArr.Clear(); }

private:
	struct Item {
		GPUState				state;
		Model*					model;
		const ModelAtom*		atom;	
		grinliz::Vector4F		color;			// per instance data (vec4)
		grinliz::Vector4F		boneFilter;		// per instance filter
		grinliz::Vector4F		control;		// per instance data 
		const grinliz::Matrix4*	param4;			// per instance data (matrix)
		const BoneData*			boneData;

		bool IsInstance( const Item* item ) {
			return    state == item->state 
				   && atom == item->atom;
		}
	};

	class CompareItem
	{
	public:
		static bool Less( const Item* s0, const Item* s1 );
	};

	grinliz::CDynArray< Item >  itemPool;
	grinliz::CDynArray< Item* > itemArr;
};


#endif //  RENDERQUEUE_INCLUDED