#include "news.h"
#include "../xarchive/glstreamer.h"
#include "../xegame/chitbag.h"
#include "../xegame/chit.h"
#include "gameitem.h"
#include "../script/itemscript.h"

using namespace grinliz;

NewsHistory* NewsHistory::instance = 0;

NewsHistory::NewsHistory() : date(0)
{
	GLASSERT( !instance );
	instance = this;
}


NewsHistory::~NewsHistory()
{
	GLASSERT( instance );
	instance = 0;
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


NewsEvent::NewsEvent( U32 what, const grinliz::Vector2F& pos, Chit* main, Chit* second )
{
	Clear();
	this->what = what;
	this->pos = pos;
	this->chitID	   = main ? main->ID() : 0;
	this->itemID	   = ( main && main->GetItem() )     ? main->GetItem()->ID()   : 0;
	this->secondItemID = ( second && second->GetItem() ) ? second->GetItem()->ID() : 0;
}


grinliz::IString NewsEvent::GetWhat() const
{
	GLASSERT( what >= 0 && what < NUM_WHAT );
	static const char* NAME[NUM_WHAT] = {
		"",
		"Denizen Created",
		"Denizen Derez",
		"Greater Created",
		"Greater Derez",
		"Sector Herd",
		"Volcano",
		"Pool",
		"Waterfall"
	};
	return grinliz::StringPool::Intern( NAME[what], true );
}


void NewsEvent::Console( grinliz::GLString* str, ChitBag* chitBag ) const
{
	IString wstr = GetWhat();
	Vector2I sector = ToSector( ToWorld2I( pos ));
	const GameItem* item = ItemDB::Instance()->Find( itemID );
	const GameItem* second = ItemDB::Instance()->Find( secondItemID );
	IString chitName, altChitName;
	if ( item ) {
		chitName = item->IBestName();
	}
	if ( second ) {
		altChitName = second->IBestName();
	}

	switch ( what ) {
	case DENIZEN_CREATED:
	case GREATER_MOB_CREATED:
		str->Format( "%s: %s at domain %x%x", wstr.c_str(), chitName.c_str(), sector.x, sector.y ); 
		break;

	case DENIZEN_KILLED:
	case GREATER_MOB_KILLED:
		str->Format( "%s: %s at domain %x%x by %s", wstr.c_str(), chitName.c_str(), sector.x, sector.y, altChitName.c_str() ); 
		break;

	default:
		GLASSERT( 0 );
	}
}


void NewsEvent::Serialize( XStream* xs )
{
	XARC_SER( xs, what );
	XARC_SER( xs, pos );
	XARC_SER( xs, chitID );
	XARC_SER( xs, itemID );
	XARC_SER( xs, secondItemID );
	XARC_SER( xs, date );
}

