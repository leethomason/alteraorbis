#include "news.h"
#include "gameitem.h"
#include "worldinfo.h"
#include "../xarchive/glstreamer.h"
#include "../xegame/chitbag.h"
#include "../xegame/chit.h"
#include "../script/itemscript.h"

using namespace grinliz;

NewsHistory* StackedSingleton< NewsHistory >::instance = 0;

NewsHistory::NewsHistory( ChitBag* _chitBag ) : date(0), chitBag(_chitBag)
{
	PushInstance( this );
}


NewsHistory::~NewsHistory()
{
	PopInstance( this );
}


void NewsHistory::DoTick( U32 delta )
{
	date += delta;
}

void NewsHistory::Serialize( XStream* xs )
{
	XarcOpen( xs, "NewsHistory" );
	XARC_SER( xs, date );
	XARC_SER_CARRAY( xs, events );
	XarcClose( xs );
}


void NewsHistory::Add( const NewsEvent& event )
{
	NewsEvent* pEvent = events.PushArr(1);
	*pEvent = event;
	pEvent->date = date;
}


const NewsEvent** NewsHistory::Find( int itemID, bool second, int* num )
{
	cache.Clear();
	for( int i=events.Size()-1; i>=0; --i ) {
		if (    events[i].itemID == itemID
			|| (second && events[i].secondItemID == itemID) ) 
		{
			cache.Push( &events[i] );
			if ( events[i].itemID == itemID && events[i].Origin() ) {
				break;
			}
		}
	}
	// They are in inverse order;
	int size = cache.Size();
	for( int i=0; i<size/2; ++i ) {
		Swap( &cache[i], &cache[size-i-1] );
	}
	*num = cache.Size();
	return cache.Mem();
}



NewsEvent::NewsEvent( U32 what, const grinliz::Vector2F& pos, Chit* main, Chit* second )
{
	Clear();
	this->what = what;
	this->pos = pos;
	this->chitID	   = main ? main->ID() : 0;
	this->itemID	   = ( main && main->GetItem() )     ? main->GetItem()->ID()   : 0;
	this->secondItemID = ( second && second->GetItem() ) ? second->GetItem()->ID() : 0;
}


NewsEvent::NewsEvent( U32 what, const grinliz::Vector2F& pos, const GameItem* item, Chit* second )
{
	Clear();
	this->what = what;
	this->pos = pos;
	this->itemID = item->ID();
	this->chitID = second->ID();
	this->secondItemID = second->GetItem() ? second->GetItem()->ID() : 0;
}


grinliz::IString NewsEvent::GetWhat() const
{
	GLASSERT( what >= 0 && what < NUM_WHAT );
	static const char* NAME[] = {
		"",
		"Denizen Created",
		"Denizen Derez",
		"Greater Created",
		"Greater Derez",
		"Lesser Named",
		"Lesser Derez",
		"Forged",
		"Destroyed",
		"Sector Herd",
		"Rampage",
		"Volcano",
		"Pool",
		"Waterfall",
		"Purchase"
	};
	GLASSERT( GL_C_ARRAY_SIZE( NAME ) == NUM_WHAT );
	return grinliz::StringPool::Intern( NAME[what], true );
}


grinliz::IString NewsEvent::IDToName( int id ) const
{
	GLASSERT( ItemDB::Instance() );
	if ( !ItemDB::Instance() ) return IString();

	IString name;

	const GameItem* item = ItemDB::Instance()->Find( id );
	if ( item ) {
		name = item->IFullName();
	}
	else {
		const ItemHistory* history = ItemDB::Instance()->History( id );
		if ( history ) {
			name = history->fullName;
		}
	}
	return name;
}


void NewsEvent::Console( grinliz::GLString* str ) const
{
	*str = "";
	IString wstr = GetWhat();
	Vector2I sector	= ToSector( ToWorld2I( pos ));
	const GameItem* second = ItemDB::Instance()->Find( secondItemID );

	// Not the case with explosives: can kill yourself.
	//GLASSERT( itemID != secondItemID );

	Chit* chit = 0;
	if ( NewsHistory::Instance()) {
		ChitBag* chitBag = NewsHistory::Instance()->GetChitBag();
		if ( chitBag ) {
			chit = chitBag->GetChit( chitID );
		}
	}

	IString itemName   = IDToName( itemID );
	IString secondName = IDToName( secondItemID );

	float age = float( double(date) / double(AGE_IN_MSEC));
	IString domain;
	if ( WorldInfo::Instance() ) {
		const SectorData& sd = WorldInfo::Instance()->GetSector( sector );
		domain = sd.name;
	}

	switch ( what ) {
	case DENIZEN_CREATED:
		str->Format( "%.2f: Denizen %s rezzed at %s.", age, itemName.c_str(), domain.c_str() );
		break;

	case DENIZEN_KILLED:
		str->Format( "%.2f: Denizen %s derez at %s by %s.", age, itemName.c_str(), domain.c_str(), secondName.c_str() );
		break;

	case GREATER_MOB_CREATED:
		str->Format( "%.2f: %s rezzed at %s.", age, itemName.c_str(), domain.c_str() ); 
		break;

	case GREATER_MOB_KILLED:
	case LESSER_NAMED_MOB_KILLED:
		str->Format( "%.2f: %s derez at %s by %s.", age, itemName.c_str(), domain.c_str(), secondName.c_str() ); 
		break;

	case LESSER_MOB_NAMED:
		str->Format( "%.2f: %s has earned a name at %s.", age, itemName.c_str(), domain.c_str() );
		break;

	case FORGED:
		str->Format( "%.2f: %s forged at %s by %s.", age, itemName.c_str(), domain.c_str(), secondName.c_str() );
		break;

	case UN_FORGED:
		str->Format( "%.2f: %s destroyed at %s.", age, itemName.c_str(), domain.c_str() );
		break;

	case PURCHASED:
		str->Format( "%.2f: %s purchased at %s by %s.", age, itemName.c_str(), domain.c_str(), secondName.c_str() );
		break;

	default:
		GLASSERT( 0 );
	}
}


void NewsEvent::Serialize( XStream* xs )
{
	XarcOpen( xs, "NewsEvent" );
	XARC_SER( xs, what );
	XARC_SER( xs, pos );
	XARC_SER( xs, chitID );
	XARC_SER( xs, itemID );
	XARC_SER( xs, secondItemID );
	XARC_SER( xs, date );
	XarcClose( xs );	
}

