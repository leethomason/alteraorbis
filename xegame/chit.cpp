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

#include "chit.h"
#include "../game/lumoschitbag.h"
#include "component.h"
#include "spatialcomponent.h"
#include "rendercomponent.h"
#include "itemcomponent.h"
#include "../game/aicomponent.h"
#include "../game/healthcomponent.h"
#include "componentfactory.h"
//#include "../script/scriptcomponent.h"	// FIXME: should be in xegame directory

#include "../grinliz/glstringutil.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;
using namespace tinyxml2;

Chit::Chit( int _id, ChitBag* bag ) : next( 0 ), chitBag( bag ), id( _id ), playerControlled( false ), timeToTick( 0 ), timeSince( 0 ),
destroyed(0)
{
	Init( _id, bag );
}


void Chit::Init( int _id, ChitBag* _chitBag )
{
	GLASSERT( chitBag == 0 );
	GLASSERT( id == 0 );
	GLASSERT( next == 0 );

	id = _id;
	chitBag = _chitBag;
	destroyed = 0;

	for( int i=0; i<NUM_SLOTS; ++i ) {
		slot[i] = 0;
	}
	random.SetSeed( _id );
	timeToTick = 0;
	timeSince = 0;
	playerControlled = false;
}


Chit::~Chit()
{
	Free();
}


void Chit::Free()
{
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] ) {
			slot[i]->OnRemove();
			delete slot[i];
			slot[i] = 0;
		}
	}
	id = 0;
	next = 0;
	chitBag = 0;
//	listeners.Clear();
}


void Chit::Serialize(XStream* xs)

{
	XarcOpen( xs, "Chit" );
	XARC_SER( xs, id );
	XARC_SER( xs, timeSince );
	XARC_SER_DEF( xs, playerControlled, false );

	// Data Binding
	XARC_SER(xs, destroyed);

	if ( xs->Saving() ) {
		for( int i=0; i<NUM_SLOTS; ++i ) {
			if ( slot[i] && slot[i]->WillSerialize() ) {
				XarcOpen( xs, slot[i]->Name() );
				slot[i]->Serialize( xs );
				XarcClose( xs );
			}
		}
	}
	else {
		while ( xs->Loading()->HasChild() ) {
			const char* n = xs->Loading()->OpenElement();
			Component* component = ComponentFactory::Factory(n, Context());
			component->Serialize( xs );
			this->Add( component, true );
			xs->Loading()->CloseElement();
		}
	}
	XarcClose( xs );
}


bool Chit::StackedMoveComponent() const
{
	for (int i = 0; i < NUM_GENERAL; ++i) {
		if (general[i] && general[i]->ToMoveComponent())
			return true;
	}
	return false;
}


void Chit::Add( Component* c, bool loading )
{
	if ( c->ToSpatialComponent()) {
		GLASSERT( spatialComponent == 0 );
		spatialComponent = c->ToSpatialComponent();
	}
	else if ( c->ToMoveComponent()) {
		// Move components are special: they can 
		// be stacked.
		// BUT, on load, don't stack so we
		// don't change the order.
		if (!loading) {
			if (moveComponent) {
				// NOT loading, push new move component.
				int i = 0;
				for (i = 0; i < NUM_GENERAL; ++i) {
					if (general[i] == 0) {
						general[i] = moveComponent;
						break;
					}
				}
				GLASSERT(i < NUM_GENERAL);
			}
			moveComponent = c->ToMoveComponent();
		}
		else {
			if (moveComponent) {
				// Loading, put in general.
				int i = 0;
				for (i = 0; i < NUM_GENERAL; ++i) {
					if (general[i] == 0) {
						general[i] = c;
						break;
					}
				}
				GLASSERT(i < NUM_GENERAL);
			}
			else {
				moveComponent = c->ToMoveComponent();
			}
		}
	}
	else if ( c->ToItemComponent()) {
		GLASSERT( itemComponent == 0 );
		itemComponent = c->ToItemComponent();
	}
	else if ( c->ToAIComponent()) {
		GLASSERT( aiComponent == 0 );
		aiComponent = c->ToAIComponent();
	}
	else if ( c->ToHealthComponent()) {
		GLASSERT( healthComponent == 0 );
		healthComponent = c->ToHealthComponent();
	}
	else if ( c->ToRenderComponent()) {
		GLASSERT( renderComponent == 0 );
		renderComponent = c->ToRenderComponent();
	}
	else {
		int i = 0;
		for ( ; i < NUM_GENERAL; ++i) {
			if (!general[i]) {
				general[i] = c;
				break;
			}
		}
		GLASSERT(i < NUM_GENERAL);
	}
	c->OnAdd( this, !loading );
	timeToTick = 0;

	GLASSERT(!spatialComponent || spatialComponent->ToSpatialComponent());
	GLASSERT(!moveComponent || moveComponent->ToMoveComponent());
	GLASSERT(!itemComponent || itemComponent->ToItemComponent());
	GLASSERT(!aiComponent || aiComponent->ToAIComponent());
	GLASSERT(!healthComponent || healthComponent->ToHealthComponent());
	GLASSERT(!renderComponent || renderComponent->ToRenderComponent());
}


void Chit::Remove( Component* c )
{
	timeToTick = 0;
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] == c ) {
			c->OnRemove();
			slot[i] = 0;

			// Special handling for move:
			if (i == MOVE_SLOT) {
				for (int k = NUM_GENERAL - 1; k >= 0; --k) {
					if (general[k] && general[k]->ToMoveComponent()) {
						slot[i] = general[k];
						general[k] = 0;
						break;
					}
				}
			}
			GLASSERT(!spatialComponent || spatialComponent->ToSpatialComponent());
			GLASSERT(!moveComponent || moveComponent->ToMoveComponent());
			GLASSERT(!itemComponent || itemComponent->ToItemComponent());
			GLASSERT(!aiComponent || aiComponent->ToAIComponent());
			GLASSERT(!healthComponent || healthComponent->ToHealthComponent());
			GLASSERT(!renderComponent || renderComponent->ToRenderComponent());
			return;
		}
	}
	GLASSERT( 0 );	// not found
}


Component* Chit::GetComponent(const char* name)
{
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] && StrEqual( name, slot[i]->Name())) {
			return slot[i];
		}
	}
	return 0;
}


Component* Chit::GetComponent( int id )
{
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] && slot[i]->ID() == id ) {
			return slot[i];
		}
	}
	return 0;
}


void Chit::DoTick()
{
	timeToTick = VERY_LONG_TICK;
	GLASSERT( timeSince >= 0 );
	bool hasMove = moveComponent != 0;

	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] ) { 
			// Look for an inactive move component:
			if (hasMove && i >= GENERAL_SLOT && i < GENERAL_SLOT + NUM_GENERAL) {
				if (slot[i]->ToMoveComponent())
					continue;
			}

			int t = slot[i]->DoTick( timeSince );
			timeToTick = Min( timeToTick, t );
		}
	}

	timeSince = 0;
}


void Chit::OnChitEvent( const ChitEvent& event )
{
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] ) {
			slot[i]->OnChitEvent( event );
		}
	}
}


void Chit::SendMessage( const ChitMsg& msg, Component* exclude )
{
	// Components
	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] && slot[i] != exclude ) {
			slot[i]->OnChitMsg( this, msg );
			//GLOUTPUT(( "return\n" ));
		}
	}

	switch (msg.ID()) {
	case ChitMsg::CHIT_DESTROYED_START:
	case ChitMsg::CHIT_DESTROYED_END:
	case ChitMsg::CHIT_DAMAGE:
	case ChitMsg::TRIGGER_DETECTOR_CIRCUIT:
		Context()->chitBag->SendMessage(this, msg);
		break;

	default:
		break;
	}
}


GameItem* Chit::GetItem()
{
	ItemComponent* pIC = GetItemComponent();
	if ( pIC ) {
		return pIC->GetItem();
	}
	return 0;
}


void Chit::QueueDelete()
{
	if (GetItem() && GameItem::trackWallet) {
		GLASSERT(GetItem()->wallet.IsEmpty());
	}
	Context()->chitBag->QueueDelete( this );
}


void Chit::DeRez()
{
	GameItem* item = GetItem();
	if (item) {
		if ((item->flags & GameItem::INDESTRUCTABLE) == 0) {
			// Since this is removing...don't blow up.
			item->flags &= ~GameItem::EXPLODES;
			item->hp = 0;
			SetTickNeeded();
			return;
		}
	}
	this->QueueDelete();
}

void Chit::DebugStr( GLString* str )
{
	str->Format( "Chit=%x ", this );

	for( int i=0; i<NUM_SLOTS; ++i ) {
		if ( slot[i] ) {
			unsigned size = str->size();
			slot[i]->DebugStr( str );
			if ( size == str->size() ) {
				str->AppendFormat( "[] " );
			}
		}
	}
}


int Chit::Team() const
{
	const GameItem* item = GetItem();
	if ( item ) 
		return item->team;
	return 0;
}


const ChitContext* Chit::Context() const
{
	GLASSERT(chitBag);
	return chitBag->Context();
}


ComponentSet::ComponentSet( Chit* _chit, int bits )
{
	Zero();
	if ( _chit ) {
		chit = _chit;
		int error = 0;
		if ( bits & Chit::SPATIAL_BIT ) {
			spatial = chit->GetSpatialComponent();
			if ( !spatial ) ++error;
		}
		if ( bits & Chit::AI_BIT ) {
			ai = chit->GetAIComponent();
			if ( !ai ) ++error;
		}
		if ( bits & Chit::MOVE_BIT ) {
			move = chit->GetMoveComponent();
			if ( !move ) ++error;
		}
		if ( bits & Chit::ITEM_BIT ) {
			itemComponent = chit->GetItemComponent();
			if ( !itemComponent ) 
				++error;
			else
				item = itemComponent->GetItem();
		}
		{
			// Always check if alive:
			ItemComponent* ic = chit->GetItemComponent();
			if ( !ic ) alive = true;	// weird has-no-health-must-be-alive case
			if ( ic ) {
				alive = ic->GetItem()->hp > 0;
			}
			// But only set error if requested
			if ( bits & ComponentSet::IS_ALIVE ) {
				if ( !alive )
					++error;
			}
		}
		if ( (bits & Chit::RENDER_BIT) || (bits & NOT_IN_IMPACT) ) {
			render = chit->GetRenderComponent();
			if ( !render ) ++error;

			if ( render &&(bits & NOT_IN_IMPACT )) {
				if ( render->MainModel() ) {
					int type = render->CurrentAnimation();
					if ( type == ANIM_IMPACT )
						++error;
				}
			}
		}

		if ( error ) 
			Zero();
		okay = !error;
	}
}


void ComponentSet::Zero()
{
	okay = false;
	chit = 0;
	spatial = 0;
	move = 0;
	itemComponent = 0;
	item = 0;
	render = 0;
	ai = 0;
}


bool InSameSector(Chit* a, Chit* b)
{
	SpatialComponent* sca = a->GetSpatialComponent();
	SpatialComponent* scb = b->GetSpatialComponent();
	if (sca && scb) {
		return sca->GetSector() == scb->GetSector();
	}
	return false;
}
