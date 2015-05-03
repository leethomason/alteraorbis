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

#include "chitbag.h"
#include "chit.h"
#include "chitcontext.h"

#include "spatialcomponent.h"
#include "rendercomponent.h"
#include "itemcomponent.h"
#include "cameracomponent.h"

#include "../engine/model.h"
#include "../engine/engine.h"
#include "../Shiny/include/Shiny.h"
#include "../tinyxml2/tinyxml2.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;
using namespace tinyxml2;

ChitBag::ChitBag(const ChitContext& c) : chitContext(c)
{
	idPool = 0;
	frame = 0;
	bagTime = 0;
#ifdef USE_SPACIAL_HASH
#else
	memset( spatialHash, 0, sizeof(*spatialHash)*SIZE2 );
#endif
	memRoot = 0;
	activeCamera = 0;
	newsHistory = new NewsHistory(this);
	areaOfInterest.Set(0, 0, 0, 0, 0, 0);
}


ChitBag::~ChitBag()
{
	delete newsHistory;
	newsHistory = 0;

	DeleteAll();
	RenderComponent::textLabelPool.FreePool();
	RenderComponent::imagePool.FreePool();
	RenderComponent::hudPool.FreePool();
}


void ChitBag::DeleteAll()
{
	GameItem::trackWallet = false;
	chitID.Clear();
	while ( !blocks.Empty() ) {
		Chit* block = blocks.Pop();
		delete [] block;
	}
	GameItem::trackWallet = true;
	bolts.Clear();
}


void ChitBag::Serialize(XStream* xs)
{
	XarcOpen(xs, "ChitBag");
	XARC_SER(xs, activeCamera);
	XARC_SER(xs, bagTime);
	newsHistory->Serialize(xs);

	if (xs->Saving()) {
		XarcOpen(xs, "Chits");
		for (int i = 0; i < chitID.Size(); ++i) {
			Chit* chit = chitID.GetValue(i);
			XarcOpen(xs, "id");
			xs->Saving()->Set("id", chit->ID());
			XarcClose(xs);
			chit->Serialize(xs);
		}
		XarcClose(xs);

		XarcOpen(xs, "Bolts");
		for (int i = 0; i < bolts.Size(); ++i) {
			bolts[i].Serialize(xs);
		}
		XarcClose(xs);
	}
	else {
		idPool = 0;

		XarcOpen(xs, "Chits");
		while (xs->Loading()->HasChild()) {

			int id = 0;
			XarcOpen(xs, "id");
			XARC_SER_KEY(xs, "id", id);
			XarcClose(xs);
			idPool = Max(id, idPool);

			//GLOUTPUT(( "loading chit id=%d\n", id ));
			Chit* c = this->NewChit(id);
			c->Serialize(xs);
		}
		XarcClose(xs);

		XarcOpen(xs, "Bolts");
		while (xs->Loading()->HasChild()) {
			Bolt* b = bolts.PushArr(1);
			b->Serialize(xs);
		}
		XarcClose(xs);
	}
	XarcClose(xs);
}


Chit* ChitBag::NewChit( int id )
{
	if ( !memRoot ) {
		// Allocate a new block.
		Chit* block = new Chit[BLOCK_SIZE];

		// Hook up the free memory pointers.
		block[BLOCK_SIZE-1].next = 0;
		for( int i=BLOCK_SIZE-2; i>=0; --i ) {
			block[i].next = &block[i+1];
		}
		memRoot = &block[0];
		blocks.Push( block );
	}
	Chit* c = memRoot;
	memRoot = memRoot->next;
	c->next = 0;
	c->Init( id ? id : (++idPool), this );
	GLASSERT( chitID.Query( c->ID(), 0 ) == false );

	chitID.Add( c->ID(), c );
	return c;
}


void ChitBag::DeleteChit( Chit* chit ) 
{
	GLASSERT( chitID.Query( chit->ID(), 0 ));
	chitID.Remove( chit->ID() );
	chit->Free();
	// Link back in to free pool.
	chit->next = memRoot;
	memRoot = chit;
}


Chit* ChitBag::GetChit( int id )
{
	Chit* c = 0;
	if (id) {
		chitID.Query(id, &c);
	}
	return c;
}


void ChitBag::QueueDelete( Chit* chit )
{
#ifdef DEBUG
	ItemComponent* ic = chit->GetItemComponent();
	if (ic) {
		for (int i = 0; i < ic->NumItems(); ++i) {
			GLASSERT(ic->GetItem(i)->wallet.IsEmpty());
		}
	}
#endif
	deleteList.Push( chit->ID() );
}


bool ChitBag::IsQueuedForDelete(Chit* chit)
{
	return deleteList.Find(chit->ID()) >= 0;
}


void ChitBag::QueueRemoveAndDeleteComponent( Component* comp )
{
	CompID* c = compDeleteList.PushArr(1);
	c->chitID = comp->ParentChit()->ID();
	c->compID = comp->ID();
}


void ChitBag::DeferredDelete( Component* comp )
{
	GLASSERT( comp->ParentChit() == 0 );	// already removed. Did you mean QueueRemoveAndDeleteComponent()?
	zombieDeleteList.Push( comp );
}


Bolt* ChitBag::NewBolt()
{
	Bolt bolt;
	Bolt* b = bolts.PushArr(1);
	*b = bolt;
	return b;
}


CameraComponent* ChitBag::GetCamera( Engine* engine )
{
	Chit* c = GetChit( activeCamera );
	if( c ) {
		CameraComponent* cc = GET_GENERAL_COMPONENT( c, CameraComponent );
		if ( cc )
			return cc;
	}
	if ( c ) { 
		c->QueueDelete(); 
	}
	c = NewChit();
	activeCamera = c->ID();
	CameraComponent* cc = new CameraComponent( &engine->camera );
	c->Add( cc );
	return cc;
}


int ChitBag::NumBlocks() const
{
	return blocks.Size();
}


void ChitBag::GetBlockPtrs( int b, grinliz::CDynArray<Chit*>* arr ) const
{
	arr->Clear();
	Chit* block = blocks[b];

	for( int j=0; j<BLOCK_SIZE; ++j ) {
		Chit* c = block + j;
		int id = c->ID();
		if ( id ) {
			arr->Push( c );
		}
	}
}


void ChitBag::ProcessDeleteList()
{
	while (!deleteList.Empty()) {
		int id = deleteList.Pop();
		Chit* chit = this->GetChit(id);
		if (chit) {
			DeleteChit(chit);
		}
	}

	while (!compDeleteList.Empty()) {
		CompID compID = compDeleteList.Pop();
		Chit* chit = this->GetChit(compID.chitID);
		if (chit) {
			Component* c = chit->GetComponent(compID.compID);
			if (c) {
				chit->Remove(c);
				delete c;
			}
		}
	}
}


void ChitBag::PushCurrentNews(const CurrentNews& news)
{
	if (currentNews.Size() > 40) {
		currentNews.PopFront();
	}
	currentNews.Push(news);
}



void ChitBag::DoTick( U32 delta )
{
	PROFILE_FUNC();
	bagTime += delta;
	nTicked = 0;
	frame++;

	newsHistory->DoTick(delta);
	bool useAOI = areaOfInterest.Volume() > 0;

	// Events.
	// Ticks.
	// Bolts.
	// probably correct in that order.

	// Process in order, and remember event queue can mutate as we go.
	while ( !events.Empty() ) {
		ChitEvent e = events.Pop();

		ChitAcceptAll acceptAll;
		QuerySpatialHash( &hashQuery, e.AreaOfEffect(), 0, &acceptAll );
		for( int j=0; j<hashQuery.Size(); ++j ) {
			hashQuery[j]->OnChitEvent( e );
		}
	}

	for( int i=0; i<blocks.Size(); ++i ) {
		Chit* block = blocks[i];
		for( int j=0; j<BLOCK_SIZE; ++j ) {
			Chit* c = block + j;
			int id = c->ID();
			if (id && id != activeCamera) {

				c->timeToTick -= delta;
				c->timeSince += delta;

				if (useAOI) {
					// The big challenged is "clumping", where 2000 chits get
					// processed one frame, and then 500 the next. Tried
					// different time strategies and don't yet have a fix.
					// However using IDs and just processing alternating
					// halves outside the AOI seems to be very stable. 
					if (((id + frame) & 1) && !areaOfInterest.Contains(c->Position())) {
						continue;
					}
				}

				if (c->timeToTick <= 0) {
					++nTicked;
					// Performance note: disabling the DoTick drops the 
					// time of this function to 0.3ms, about 1% of the
					// time. So the memory walk and blocks are fast.
					// It's all in the DoTick() itself.
					c->DoTick();
					GLASSERT( c->timeToTick >= 0 );
				}
				// Clear out anything deleted by calling
				// the components. Can't clear out
				// while handling the components - could
				// delete something being Ticked
				ProcessDeleteList();
			}
		}
	}

	if ( chitContext.engine ) {
		Bolt::TickAll( &bolts, delta, chitContext.engine, this );
	}

	// Make sure the camera is updated last so it doesn't "drag"
	// what it's tracking. The next time this happens I should
	// put in a priority system.
	Chit* camera = GetChit( activeCamera );
	if ( camera ) {
		camera->timeToTick = 0;
		camera->timeSince = delta;
		camera->DoTick();
	}

	// Final flush, just to be sure.
	ProcessDeleteList();

	while ( !zombieDeleteList.Empty() ) {
		Component* c = zombieDeleteList.Pop();
		GLASSERT( c && c->ParentChit() == 0 );
		delete c;
	}

	if (debugTick.Delta(delta)) {
		GLString str;
		str.Format("Spatial Hash: %d nAlloc=%d nBuck=%d probe=%d eff=%.2f den=%.2f",
				   spatialHash.NumItems(), spatialHash.NumAlloc(),
				   spatialHash.NumAllocated(), spatialHash.NumProbes(), spatialHash.Efficiency(), spatialHash.Density());
		//GLOUTPUT(("%s\n", str.safe_str()));
		static int count = 0;
		if (count >= 10 && count < 20) {
			GLOUTPUT_REL(("%s\n", str.safe_str()));
		}
		++count;
		spatialHash.ClearMetrics();
	}
}


void ChitBag::HandleBolt( const Bolt& bolt, const ModelVoxel& mv )
{
}


bool ChitHasMoveComponent::Accept( Chit* chit )
{
	return chit->GetMoveComponent() != 0;
}


bool ChitHasAIComponent::Accept( Chit* chit )
{
	return chit->GetAIComponent() != 0;
}


bool MultiFilter::Accept( Chit* chit ) 
{
	if ( anyAll == MATCH_ANY ) {
		for( int i=0; i<filters.Size(); ++i ) {
			if ( filters[i]->Accept( chit )) {
				return true;
			}
		}
		return false;
	}
	else {
		for( int i=0; i<filters.Size(); ++i ) {
			if ( !filters[i]->Accept( chit )) {
				return false;
			}
		}
		return true;
	}
}


void ChitBag::AddToSpatialHash(Chit* chit, int x, int y)
{
	GLASSERT(chit);
	if (x == 0 && y == 0) return;	// sentinel
	//GLOUTPUT(("Add %x at %d,%d\n", chit, x, y));

#ifdef USE_SPACIAL_HASH
	Vector2I pos = { x >> SHIFT, y >> SHIFT };
	spatialHash.Add(pos, chit);
#else
	U32 index = HashIndex(x, y);
	chit->next = spatialHash[index];
	spatialHash[index] = chit;
#endif
}


void ChitBag::RemoveFromSpatialHash(Chit* chit, int x, int y)
{
	GLASSERT(chit);
	if (x == 0 && y == 0) return;	// sentinel

	//GLOUTPUT(("Rmv %x at %d,%d\n", chit, x, y));

#ifdef USE_SPACIAL_HASH
	Vector2I pos = { x >> SHIFT, y >> SHIFT };
	spatialHash.Remove(pos, chit);
#else
	U32 index = HashIndex(x, y);
	Chit* prev = 0;
	for (Chit* it = spatialHash[index]; it; prev = it, it = it->next) {
		if (it == chit) {
			if (prev) {
				prev->next = it->next;
				it->next = 0;
				return;
			}
			else {
				spatialHash[index] = it->next;
				it->next = 0;
				return;
			}
		}
	}
	GLASSERT(0);
#endif
}


void ChitBag::UpdateSpatialHash(Chit* c, int x0, int y0, int x1, int y1)
{
	if (   (x0>>SHIFT) != (x1>>SHIFT) 
		|| (y0>>SHIFT) != (y1>>SHIFT)) 
	{
		RemoveFromSpatialHash(c, x0, y0);
		AddToSpatialHash(c, x1, y1);
	}
}


void ChitBag::QuerySpatialHash(grinliz::CDynArray<Chit*>* array,
							   const grinliz::Rectangle2F& searchBounds,
							   const Chit* ignore,
							   IChitAccept* accept)
{
	Rectangle2I queryBounds;
	queryBounds.Set(int(searchBounds.min.x), int(searchBounds.min.y), int(searchBounds.max.x), int(searchBounds.max.y));

	InnerQuerySpatialHash(array, queryBounds, searchBounds, ignore, accept);
}


void ChitBag::QuerySpatialHash(	CChitArray* arr,
								const grinliz::Rectangle2F& r, 
								const Chit* ignoreMe,
								IChitAccept* accept )
{
	QuerySpatialHash( &cachedQuery, r, ignoreMe, accept );
	arr->Clear();
	for( int i=0; i<cachedQuery.Size() && arr->HasCap(); ++i ) {
		arr->Push( cachedQuery[i] );
	}
}


void ChitBag::QuerySpatialHash(grinliz::CDynArray<Chit*>* array,
							   const grinliz::Rectangle2I& queryBounds,
							   const Chit* ignoreMe,
							   IChitAccept* filter)
{
	Rectangle2F searchBounds = ToWorld2F(queryBounds);
	InnerQuerySpatialHash(array, queryBounds, searchBounds, ignoreMe, filter);
	// Need to get the edge cases correct: we need to be in
	// the integer bounds, not floating point.
	array->Filter(queryBounds, [](const Rectangle2I& bounds, Chit* chit) {
		return bounds.Contains(ToWorld2I(chit->Position()));
	});
}


void ChitBag::QuerySpatialHash(CChitArray* arr,
							   const grinliz::Rectangle2I& queryBounds,
							   const Chit* ignoreMe,
							   IChitAccept* filter)
{
	QuerySpatialHash(&cachedQuery, queryBounds, ignoreMe, filter);
	arr->Clear();
	for( int i=0; i<cachedQuery.Size() && arr->HasCap(); ++i ) {
		arr->Push( cachedQuery[i] );
	}
}


void ChitBag::InnerQuerySpatialHash(grinliz::CDynArray<Chit*>* array,
									const grinliz::Rectangle2I& queryBounds,
									const grinliz::Rectangle2F& searchBounds,
									const Chit* ignoreMe,
									IChitAccept* accept)
{
	GLASSERT(accept);
	GLASSERT(array);
	array->Clear();

	U32 i0 = queryBounds.min.x >> SHIFT;
	U32 j0 = queryBounds.min.y >> SHIFT;
	U32 i1 = queryBounds.max.x >> SHIFT;
	U32 j1 = queryBounds.max.y >> SHIFT;

	for (U32 j = j0; j <= j1; ++j) {
		for (U32 i = i0; i <= i1; ++i) {
			Vector2I pos = { i, j };
			spatialHash.Query(pos, array);
		}
	}
	for (int i = 0; i < array->Size(); ++i) {
		Chit* chit = (*array)[i];
		if (chit != ignoreMe && searchBounds.Contains(ToWorld2F(chit->Position())) && accept->Accept(chit)) {
			// Do nothing. This is valid.
		}
		else {
			array->SwapRemove(i);
			--i;
		}
	}
}


void ChitBag::SetTickNeeded(const grinliz::Rectangle2F& bounds)
{
	ChitAcceptAll all;
	QuerySpatialHash(&cachedQuery, bounds, 0, &all);
	for (int i = 0; i < cachedQuery.Size(); ++i) {
		cachedQuery[i]->SetTickNeeded();
	}
}


/*static*/ void ChitBag::Test()
{
	ChitContext dummyContext;
	ChitBag* chitBag = new ChitBag(dummyContext);

	Chit* chit0 = chitBag->NewChit();
	Chit* chit1 = chitBag->NewChit();
	Chit* chit2 = chitBag->NewChit();
	Chit* chit3 = chitBag->NewChit();

	chit0->SetPosition(10, 0, 10);
	chit1->SetPosition(10.5f, 0, 10);
	chit2->SetPosition(11, 0, 11);
	chit3->SetPosition(12, 0, 12);

	CChitArray arr;
	Rectangle2F r;
	Rectangle2I ri;
	ChitAcceptAll all;

	r.Set(10, 10, 11, 11);
	chitBag->QuerySpatialHash(&arr, r, 0, &all);
	GLTEST(arr.Size() == 3);

	ri.Set(10, 10, 10, 10);
	chitBag->QuerySpatialHash(&arr, ri, 0, &all);
	GLTEST(arr.Size() == 2);	// should NOT pick 11,11

	Vector2F center = { 10, 10 };
	chitBag->QuerySpatialHash(&arr, center, 0.7f, 0, &all);
	GLTEST(arr.Size() == 2);
	chitBag->QuerySpatialHash(&arr, center, 1.0f, 0, &all);
	GLTEST(arr.Size() == 3);
	chitBag->QuerySpatialHash(&arr, center, 2.0f, 0, &all);
	GLTEST(arr.Size() == 4);

	chitBag->DeleteChit(chit0);
	chitBag->DeleteChit(chit1);
	chitBag->DeleteChit(chit2);
	chitBag->DeleteChit(chit3);
	delete chitBag;
	GLOUTPUT(("ChitBag test done."));
}

