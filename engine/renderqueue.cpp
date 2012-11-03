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
#include "../grinliz/glperformance.h"
#include <limits.h>

using namespace grinliz;

RenderQueue::RenderQueue()
{
	nState = 0;
	nItem = 0;
}


RenderQueue::~RenderQueue()
{
}


RenderQueue::State* RenderQueue::FindState( const State& state )
{
	int low = 0;
	int high = nState - 1;
	int answer = -1;
	int mid = 0;
	int insert = 0;		// Where to put the new state. 

	while (low <= high) {
		mid = low + (high - low) / 2;
		int compare = CompareState( state, statePool[mid] );

		if (compare > 0) {
			high = mid - 1;
		}
		else if ( compare < 0) {
			insert = mid;	// since we know the mid is less than the state we want to add,
							// we can move insert up at least to that index.
			low = mid + 1;
		}
		else {
           answer = mid;
		   break;
		}
	}
	if ( answer >= 0 ) {
		return &statePool[answer];
	}
	if ( nState == MAX_STATE ) {
		return 0;
	}

	if ( nState == 0 ) {
		insert = 0;
	}
	else {
		while (    insert < nState
			    && CompareState( state, statePool[insert] ) < 0 ) 
		{
			++insert;
		}
	}

	// move up
	for( int i=nState; i>insert; --i ) {
		statePool[i] = statePool[i-1];
	}
	// and insert
	statePool[insert] = state;
	statePool[insert].root = 0;
	nState++;

#ifdef DEBUG
	for( int i=0; i<nState-1; ++i ) {
		//GLOUTPUT(( " %d:%d:%x", statePool[i].state.flags, statePool[i].state.textureID, statePool[i].state.atom ));
		GLASSERT( CompareState( statePool[i], statePool[i+1] ) > 0 );
	}
	//GLOUTPUT(( "\n" ));
#endif

	return &statePool[insert];
}


void RenderQueue::Add(	Model* model, 
						const ModelAtom* atom, 
						const GPUState& s, 
						const Vector4F& param, 
						const Matrix4* param4,
						const BoneData* boneData  )
{
	GLASSERT( model );
	GLASSERT( atom );

	if ( nItem == MAX_ITEMS ) {
		GLASSERT( 0 );
		return;
	}

	State s0 = { s, atom->texture, 0 };

	State* state = FindState( s0 );
	if ( !state ) {
		GLASSERT( 0 );
		return;
	}

	Item* item = &itemPool[nItem++];
	item->model = model;
	item->atom = atom;
	item->param = param;
	item->param4 = param4;
	item->boneData = boneData;
	
	item->next  = state->root;
	state->root = item;
}


void RenderQueue::Submit(	int modelRequired, 
							int modelExcluded, 
							const Matrix4* xform )
{
	//GRINLIZ_PERFTRACK

	for( int i=0; i<nState; ++i ) {
		GPUState* state = &statePool[i].state;
		GLASSERT( state );
		//GLOUTPUT(( "%d state=%d shader=%d texture=%x\n", i, state->StateFlags(), state->ShaderFlags(), statePool[i].texture0 ));

		// Filter out all the items for this RenderState
		itemArr.Clear();
		for( Item* item = statePool[i].root; item; item=item->next ) 
		{
			Model* model = item->model;
			int modelFlags = model->Flags();

			if (    ( (modelRequired & modelFlags) == modelRequired)
				 && ( (modelExcluded & modelFlags) == 0 ) )
			{
				itemArr.Push( item );
			}
		}

		// For this RenderState, we now have all the items to be drawn.
		// We now want to group the items by RenderAtom, so that the 
		// Atoms which are instanced can be rendered together.
		qsort( (void*)itemArr.Mem(), itemArr.Size(), sizeof(Item*), CompareAtom );

		int start = 0;
		int end = 0;
		GPUStream		stream;
		GPUStreamData   data;

		while( start < itemArr.Size() ) {
			// Get a range;
			end = start + 1;
			while(    end < itemArr.Size() 
				   && itemArr[end]->atom == itemArr[start]->atom ) 
			{
				++end;
			}
			int delta = end - start;
			const ModelAtom* atom = itemArr[start]->atom;

#ifdef XENOENGINE_INSTANCING
			// The 2 paths is a PITA, both
			// for startup time and debugging. If instancing
			// in use, always instance.
			{
				Matrix4		instanceMatrix[EL_MAX_INSTANCE];
				Vector4F	instanceParam[EL_MAX_INSTANCE];
				Matrix4		instanceParam4[EL_MAX_INSTANCE];
				BoneData	instanceBone[EL_MAX_INSTANCE];

				atom->Bind( &stream, &data );
				data.matrix = instanceMatrix;
				data.param  = instanceParam;
				data.param4 = instanceParam4;
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
						instanceParam[index] = item->param;
						if ( item->param4 ) {
							instanceParam4[index] = *item->param4;
						}
						if ( item->boneData ) {
							instanceBone[index] = *item->boneData;
						}
					}
					state->Draw( stream, data, atom->nIndex, delta );
					k += delta;
				}
			}
#else
			{
				atom->Bind( shader );
				for( int k=start; k<end; ++k ) {
					const Item* item = itemArr[k];
					Model* model = item->model;

					shader->PushMatrix( GPUState::MODELVIEW_MATRIX );
					if ( xform ) {
						shader->MultMatrix( GPUState::MODELVIEW_MATRIX, *xform );
					}
					shader->MultMatrix( GPUState::MODELVIEW_MATRIX, model->XForm() );
					shader->SetParam( item->param );
					if ( item->hasParam4 ) {
						shader->InstanceParam4( 0, item->param4 );
					}
					if ( item->hasBoneData ) {
						shader->InstanceBones( 0, item->boneData );
					}
					shader->Draw();
					shader->PopMatrix( GPUState::MODELVIEW_MATRIX );
				}
			}
#endif
			start = end;
		}
	}
}
