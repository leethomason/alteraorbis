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

#include "renderQueue.h"
#include "model.h"    
#include "texture.h"
#include "gpustatemanager.h"
#include "shadermanager.h"
//#include "../grinliz/glperformance.h"
#include "../grinliz/glgeometry.h"
#include <limits.h>

using namespace grinliz;

RenderQueue::RenderQueue()
{
}


RenderQueue::~RenderQueue()
{
}

bool RenderQueue::CompareItem::Less( const Item* s0, const Item* s1 )
{
	// Priority list:

	GLASSERT( sizeof(Item) < 100 );	// just a sanity check.	
	int result = s1->state.StateFlags() - s0->state.StateFlags();
	if ( result ) return result > 0;

	result = (int)((SPTR)s1->atom - (SPTR)s0->atom);
	if ( result ) return result > 0;

	result = (int)((SPTR)s1->atom->texture - (SPTR)s0->atom->texture);
	if ( result ) return result > 0;

	result = s1->state.ShaderFlags() - s0->state.ShaderFlags();
	return result > 0;
}


void RenderQueue::Add(	Model* model, 
						const ModelAtom* atom, 
						const GPUState& state, 
						const Vector4F& color,
						const Vector4F& filter,
						const Vector4F& control,
						const ModelAux* aux )
{
	GLASSERT( model );
	GLASSERT( atom );

	Item* item = itemPool.PushArr(1);
	item->state = state;
	item->model = model;
	item->atom = atom;
	item->color = color;
	item->boneFilter = filter;
	item->control = control;
	item->aux = aux;

	GLASSERT( itemPool.Size() < 10*1000 );	// sanity, infinite loop detection
}


void RenderQueue::Submit(	int modelRequired, 
							int modelExcluded, 
							const Matrix4* xform )
{
	//GRINLIZ_PERFTRACK
	itemArr.Clear();

	for( int i=0; i<itemPool.Size(); ++i ) {

		Item* item = &itemPool[i];
		Model* model = item->model;
		int modelFlags = model->Flags();

		if (    ( (modelRequired & modelFlags) == modelRequired)
			 && ( (modelExcluded & modelFlags) == 0 ) )
		{
			itemArr.Push( item );
		}
	}

	Sort<Item*, CompareItem>( itemArr.Mem(), itemArr.Size() );

	int start = 0;
	int end = 0;
	GPUStream		stream;
	GPUStreamData   data;

	Matrix4		instanceMatrix[EL_MAX_INSTANCE];
	Vector4F	instanceColorParam[EL_MAX_INSTANCE];
	Vector4F	instanceBoneFilter[EL_MAX_INSTANCE];
	Vector4F	instanceControlParam[EL_MAX_INSTANCE];
	Vector4F	instanceTexture0XForm[EL_MAX_INSTANCE];
	Vector4F	instanceTexture0Clip[EL_MAX_INSTANCE];
	Matrix4		instanceTexture0ColorMap[EL_MAX_INSTANCE];
	Matrix4		instanceBone[EL_MAX_INSTANCE*EL_MAX_BONES];

	//GLOUTPUT(( "Batch:\n" ));
	while( start < itemArr.Size() ) {
		// Get a range;
		end = start + 1;
		while(    end < itemArr.Size() 
			   && itemArr[end]->IsInstance( itemArr[start] ) ) 
		{
			++end;
		}
		int delta = end - start;
		const ModelAtom* atom = itemArr[start]->atom;
		GPUState* state = &itemArr[start]->state;

		//GLOUTPUT(( "  n=%d state=%d shader=%d texture=%s atom=%x\n", 
		//			delta, state->StateFlags(), state->ShaderFlags(),
		//			atom->texture->Name(), atom ));

		// The 2 paths is a PITA, both
		// for startup time and debugging. If instancing
		// in use, always instance.
		{
			atom->Bind( &stream, &data );
			data.matrix = instanceMatrix;
			data.colorParam = instanceColorParam;
			data.boneFilter = instanceBoneFilter;
			data.controlParam = instanceControlParam;
			data.texture0XForm = instanceTexture0XForm;
			data.texture0Clip = instanceTexture0Clip;
			data.texture0ColorMap = instanceTexture0ColorMap;
			data.bones  = instanceBone;
			GLASSERT( data.texture0 );	// not required, but not sure it works without

			int k=start;

			while( k < end ) {
				int delta = Min( end-k, (int)EL_MAX_INSTANCE );

				for( int index=0; index<delta; ++index ) {
					const Item* item = itemArr[k+index];
					if ( xform ) {
						instanceMatrix[index] = (*xform) * item->model->XForm();
					}
					else {
						instanceMatrix[index] = item->model->XForm();
					}
					instanceColorParam[index] = item->color;
					instanceBoneFilter[index] = item->boneFilter;
					instanceControlParam[index] = item->control;

					if ( item->aux ) {
						for( int i=0; i<EL_MAX_BONES; ++i ) {
							instanceBone[index*EL_MAX_BONES+i]	= item->aux->boneMats[i];
						}
						instanceTexture0XForm[index]	= item->aux->texture0XForm;
						instanceTexture0Clip[index]		= item->aux->texture0Clip;
						instanceTexture0ColorMap[index] = item->aux->texture0ColorMap;
					}
				}
				GPUDevice::Instance()->Draw( *state, stream, data, 0, atom->nIndex, delta );
				k += delta;
			}
		}
		start = end;
	}
}
