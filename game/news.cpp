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
}


NewsHistory::~NewsHistory()
{
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


void NewsHistory::Add( const NewsEvent& e )
{
	NewsEvent event = e;
	event.date = date ? date : 1;	// don't write 0 date. confuses later logic.

	if ( event.what < NewsEvent::START_CURRENT ) {
		events.Push( event );
	}
	if ( !current.HasCap() ) {
		current.PopFront();
	}
	current.Push( event );
}


const NewsEvent** NewsHistory::Find( int itemID, bool second, int* num, NewsHistory::Data* data )
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
	if ( num ) {
		*num = cache.Size();
	}
	if ( data ) {
		for( int i=0; i<cache.Size(); ++i ) {
			int what = cache[i]->what;
			switch( what ) {
			case NewsEvent::DENIZEN_CREATED:
			case NewsEvent::GREATER_MOB_CREATED:
			case NewsEvent::D_LESSER_MOB_NAMED:
			case NewsEvent::DOMAIN_CREATED:
				GLASSERT( data->bornOrNamed == 0 );
				data->bornOrNamed = cache[i]->date;
				break;
			case NewsEvent::DENIZEN_KILLED:
			case NewsEvent::GREATER_MOB_KILLED:
			case NewsEvent::D_LESSER_NAMED_MOB_KILLED:
			case NewsEvent::DOMAIN_DESTROYED:
				GLASSERT( data->bornOrNamed );
				GLASSERT( data->died == 0 );
				data->died = cache[i]->date;
				break;
			default:
				break;
			}
		}
	}
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
		"Domain Created",
		"Domain Destroyed",
		"Forged",
		"Destroyed",
		"Purchase",
		"Starvation",
		"Blood Rage",
		"Vision Quest",

		"",	// placeholder to mark minor events

		"Sector Herd",
		"Rampage",
		"Volcano",
		"Pool",
		"Waterfall",
		"Greater Summoned"
	};
	GLASSERT( GL_C_ARRAY_SIZE( NAME ) == NUM_WHAT );
	return grinliz::StringPool::Intern( NAME[what], true );
}


grinliz::IString NewsEvent::IDToName( int id, bool shortName ) const
{
	if ( id == 0 ) return IString();

	GLASSERT( ItemDB::Instance() );
	if ( !ItemDB::Instance() ) return IString();

	IString name;

	const GameItem* item = ItemDB::Instance()->Find( id );
	if ( item ) {
		if (shortName) {
			if (!item->IProperName().empty())
				name = item->IProperName();
			else
				name = item->IName();
		}
		else {
			name = item->IFullName();
		}
	}
	else {
		const ItemHistory* history = ItemDB::Instance()->History( id );
		if ( history ) {
			name = history->titledName;
		}
	}
	return name;
}


void NewsEvent::Console( grinliz::GLString* str, ChitBag* chitBag, int shortNameID ) const
{
	*str = "";
	IString wstr = GetWhat();
	Vector2I sector	= ToSector( ToWorld2I( pos ));
	const GameItem* second = ItemDB::Instance()->Find( secondItemID );

	// Not the case with explosives: can kill yourself.
	//GLASSERT( itemID != secondItemID );

	Chit* chit = 0;
	if ( chitBag ) {
		chit = chitBag->GetChit( chitID );
	}

	IString itemName   = IDToName( itemID, itemID == shortNameID );
	IString secondName = IDToName( secondItemID, secondItemID == shortNameID );
	if ( secondName.empty() ) secondName = StringPool::Intern( "[unknown]" );

	float age = float( double(date) / double(AGE_IN_MSEC));
	IString domain;
	if ( WorldInfo::Instance() ) {
		const SectorData& sd = WorldInfo::Instance()->GetSector( sector );
		domain = sd.name;
	}

	switch ( what ) {
	case DENIZEN_CREATED:
		str->Format( "%.2f: Denizen %s created at %s.", age, itemName.c_str(), domain.c_str() );
		break;

	case DENIZEN_KILLED:
		str->Format( "%.2f: Denizen %s de-rez at %s by %s.", age, itemName.c_str(), domain.c_str(), secondName.c_str() );
		break;

	case GREATER_MOB_CREATED:
		str->Format( "%.2f: %s created at %s.", age, itemName.c_str(), domain.c_str() ); 
		break;

	case DOMAIN_CREATED:
		str->Format("%.2f: %s created.", age, domain.c_str());
		break;

	case GREATER_MOB_KILLED:
	case D_LESSER_NAMED_MOB_KILLED:
		str->Format( "%.2f: %s de-rez at %s by %s.", age, itemName.c_str(), domain.c_str(), secondName.c_str() ); 
		break;

	case D_LESSER_MOB_NAMED:
		str->Format( "%.2f: %s has earned a name at %s.", age, itemName.c_str(), domain.c_str() );
		break;

	case DOMAIN_DESTROYED:
		str->Format("%.2f: %s destroyed.", age, domain.c_str());
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

	case STARVATION:
		str->Format( "%.2f: %s has been overcome by starvation at %s.", age, itemName.c_str(), domain.c_str() );
		break;

	case BLOOD_RAGE:
		str->Format( "%.2f: a distraught %s is overcome by blood rage at %s.", age, itemName.c_str(), domain.c_str() );
		break;

	case VISION_QUEST:
		str->Format( "%.2f: %s is consumed by despair at %s and leaves for a vision quest.", age, itemName.c_str(), domain.c_str() );
		break;

	case GREATER_SUMMON_TECH:
		str->Format("%.2f: %s is called to %s by the allure of Tech.", age, itemName.c_str(), domain.c_str());
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

