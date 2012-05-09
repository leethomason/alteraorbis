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
#include "../grinliz/glperformance.h"
#include <limits.h>

using namespace grinliz;

RenderQueue::RenderQueue()
{
	nState = 0;
	nItem = 0;

	vertexCacheSize = 0;
	vertexCacheCap = 0;
}


RenderQueue::~RenderQueue()
{
	vertexCache.Destroy();
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
		int compare = CompareState( statePool[mid], state );

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
			    && CompareState( statePool[insert], state ) < 0 ) 
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
		GLASSERT( CompareState( statePool[i], statePool[i+1] ) < 0 );
	}
	//GLOUTPUT(( "\n" ));
#endif

	return &statePool[insert];
}


void RenderQueue::Add( Model* model, const ModelAtom* atom, GPUShader* shader, const grinliz::Vector4F& param )
{
	GLASSERT( model );
	GLASSERT( atom );
	GLASSERT( shader );

	if ( nItem == MAX_ITEMS ) {
		GLASSERT( 0 );
		return;
	}

	State s0 = { shader, atom->texture, 0 };

	State* state = FindState( s0 );
	if ( !state ) {
		GLASSERT( 0 );
		return;
	}

	Item* item = &itemPool[nItem++];
	item->model = model;
	item->atom = atom;
	item->param = param;
	
	item->next  = state->root;
	state->root = item;
}


void RenderQueue::Submit( GPUShader* overRideShader, int required, int excluded, const Matrix4* xform )
{
	GRINLIZ_PERFTRACK

	for( int i=0; i<nState; ++i ) {
		GPUShader* shader = overRideShader ? overRideShader : statePool[i].shader;
		GLASSERT( shader );
		if ( !overRideShader ) {
			shader->SetTexture0( statePool[i].texture );
		}

		// Filter out all the items for this RenderState
		itemArr.Clear();
		for( Item* item = statePool[i].root; item; item=item->next ) 
		{
			Model* model = item->model;
			int modelFlags = model->Flags();

			if (    ( (required & modelFlags) == required)
				 && ( (excluded & modelFlags) == 0 ) )
			{
				itemArr.Push( item );
			}
		}

		// For this RenderState, we now have all the items to be drawn.
		// We now want to group the items by RenderAtom, so that the 
		// Atoms which are instanced can be rendered together.
		qsort( itemArr.Mem(), itemArr.Size(), sizeof(Item*), CompareAtom );

		int start = 0;
		int end = 0;

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
			if ( atom->instances > 1 && delta > 1 ) {
				atom->Bind( shader );

				int k=start;
				while( k < end ) {
					int delta = Min( end-k, (int)EL_MAX_INSTANCE );

					for( int index=0; index<delta; ++index ) {
						const Item* item = itemArr[k+index];
						if ( xform ) {
							shader->InstanceMatrix( index, (*xform) * item->model->XForm() );
						}
						else {
							shader->InstanceMatrix( index, item->model->XForm() );
						}
						shader->InstanceParam( index, item->param );
					}
					shader->Draw( delta );
					k += delta;
				}
			} else
			#endif
			{
				atom->Bind( shader );
				for( int k=start; k<end; ++k ) {
					const Item* item = itemArr[k];
					Model* model = item->model;

					shader->PushMatrix( GPUShader::MODELVIEW_MATRIX );
					if ( xform ) {
						shader->MultMatrix( GPUShader::MODELVIEW_MATRIX, *xform );
					}
					shader->MultMatrix( GPUShader::MODELVIEW_MATRIX, model->XForm() );
					shader->SetParam( item->param );
					shader->Draw();
					shader->PopMatrix( GPUShader::MODELVIEW_MATRIX );
				}
			}
			start = end;
		}
	}
}
