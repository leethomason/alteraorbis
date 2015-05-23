#include "news.h"
#include "gameitem.h"
#include "worldinfo.h"
#include "team.h"
#include "worldmap.h"
#include "../xarchive/glstreamer.h"
#include "../xegame/chitbag.h"
#include "../xegame/chit.h"
#include "../xegame/chitcontext.h"
#include "../script/itemscript.h"

using namespace grinliz;

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

	events.Push( event );
}


const NewsEvent** NewsHistory::FindItem( int firstItemID, int secondItemID, int* num, NewsHistory::Data* data )
{
	cache.Clear();
	for (int i = events.Size() - 1; i >= 0; --i) {
		if ((firstItemID && events[i].firstItemID == firstItemID)
			|| (secondItemID && events[i].secondItemID == secondItemID))
		{
			cache.Push(&events[i]);
			if (firstItemID && (events[i].firstItemID == firstItemID && events[i].Origin())) {
				break;
			}
		}
	}
	// They are in inverse order;
	cache.Reverse();

	if ( num ) {
		*num = cache.Size();
	}
	if ( data ) {
		for( int i=0; i<cache.Size(); ++i ) {
			int what = cache[i]->what;
			switch( what ) {
			case NewsEvent::DENIZEN_CREATED:
			case NewsEvent::GREATER_MOB_CREATED:
			case NewsEvent::DOMAIN_CREATED:
			case NewsEvent::FORGED:
				GLASSERT( data->born == 0 );
				data->born = cache[i]->date;
				break;
			case NewsEvent::DENIZEN_KILLED:
			case NewsEvent::GREATER_MOB_KILLED:
			case NewsEvent::DOMAIN_DESTROYED:
			case NewsEvent::UN_FORGED:
				GLASSERT(data->born);
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



NewsEvent::NewsEvent(U32 what, const grinliz::Vector2F& pos, int first, int second, int team)
{
	Clear();
	this->what = what;
	this->pos = pos;
	this->firstItemID = first;
	this->secondItemID = second;
	this->team = team;
}


grinliz::IString NewsEvent::GetWhat() const
{
	GLASSERT( what >= 0 && what < NUM_WHAT );
	static const char* NAME[] = {
		"", 
		"Denizen " MOB_Created,
		"Denizen " MOB_Destroyed,
		"Greater " MOB_Created,
		"Greater " MOB_Destroyed,
		"Domain " MOB_Created,
		"Domain " MOB_Destroyed,
		"Roque Joins",
		"Forged",
		MOB_Destroyed,
		"Purchase",
		"Starvation",
		"Blood Rage",
		"Vision Quest",
		"Greater Summoned",
		"Domain Conquered",
	};
	GLASSERT( GL_C_ARRAY_SIZE( NAME ) == NUM_WHAT );
	return grinliz::StringPool::Intern( NAME[what], true );
}


grinliz::IString NewsEvent::IDToName( int id, bool shortName )
{
	if ( id == 0 ) return IString();

	GLASSERT( ItemDB::Instance() );
	if ( !ItemDB::Instance() ) return IString();

	IString name;

	const GameItem* item = ItemDB::Instance()->Active( id );
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
		ItemHistory history;
		if ( ItemDB::Instance()->Registry(id, &history)) {
			name = history.titledName;
		}
	}
	return name;
}


void NewsEvent::Console(GLString* str, ChitBag* chitBag, int shortNameID) const
{
	*str = "";
	//IString wstr = GetWhat();
	Vector2I sector = ToSector(ToWorld2I(pos));

	const GameItem* first  = ItemDB::Instance()->Active(firstItemID);
	//const GameItem* second = ItemDB::Instance()->Active(secondItemID);

	IString firstName  = IDToName(firstItemID,  firstItemID == shortNameID);
	IString secondName = IDToName(secondItemID, secondItemID == shortNameID);
	if (firstName.empty())  firstName  = StringPool::Intern("[unknown]");
	if (secondName.empty()) secondName = StringPool::Intern("[unknown]");

	float age = float(double(date) / double(AGE_IN_MSEC));
	IString domain;
	if (chitBag->Context()->worldMap) {
		const SectorData& sd = chitBag->Context()->worldMap->GetSectorData(sector);
		domain = sd.name;
	}

	IString teamName = Team::TeamName(team);

	switch (what) {
		case DENIZEN_CREATED:
		str->Format("%.2f: Denizen %s " MOB_created " at %s.", age, firstName.c_str(), domain.c_str());
		break;

		case DENIZEN_KILLED:
		str->Format("%.2f: Denizen %s " MOB_destroyed " at %s by %s.", age, firstName.c_str(), domain.c_str(), secondName.c_str());
		break;

		case GREATER_MOB_CREATED:
		// They get created at the center, then sent. So the domain is meaningless.
		str->Format("%.2f: %s " MOB_created ".", age, firstName.c_str());
		break;

		case DOMAIN_CREATED:
		// "taken over" is interesting; a domain getting created is not.
		*str = "";
		break;

		case ROGUE_DENIZEN_JOINS_TEAM:
		str->Format("%.2f: Denizen %s joins %s at %s.", age, firstName.c_str(), teamName.safe_str(), domain.c_str());
		break;

		case GREATER_MOB_KILLED:
		str->Format("%.2f: %s " MOB_destroyed " at %s by %s.", age, firstName.c_str(), domain.c_str(), secondName.c_str());
		break;

		case DOMAIN_DESTROYED:
		if (team) {
			str->Format("%.2f: %s domain %s " MOB_destroyed " by %s.", age, teamName.safe_str(), domain.safe_str(), secondName.safe_str());
		}
		else {
			str->Format("%.2f: %s " MOB_destroyed ".", age, domain.c_str());
		}
		break;

		case FORGED:
		str->Format("%.2f: %s forged %s at %s.", age, secondName.safe_str(), firstName.c_str(), domain.safe_str());
		break;

		case UN_FORGED:
		str->Format("%.2f: %s " MOB_destroyed " %s at %s.", age, secondName.c_str(), firstName.c_str(), domain.c_str());
		break;

		case PURCHASED:
		if (first) {
			str->Format("%.2f: %s purchased %s at %s for %d (%d tax).", age, secondName.c_str(), firstName.c_str(), domain.c_str(),
						first->GetValue(), int(float(first->GetValue() * SALES_TAX)));
		}
		else {
			str->Format("%.2f: %s purchased %s at %s.", age, secondName.c_str(), firstName.c_str(), domain.c_str());
		}
		break;

		case STARVATION:
		str->Format("%.2f: %s has been overcome by starvation at %s.", age, firstName.c_str(), domain.c_str());
		break;

		case BLOOD_RAGE:
		str->Format("%.2f: a distraught %s is overcome by blood rage at %s.", age, firstName.c_str(), domain.c_str());
		break;

		case VISION_QUEST:
		str->Format("%.2f: %s is consumed by despair at %s and leaves for a vision quest.", age, firstName.c_str(), domain.c_str());
		break;

		case GREATER_SUMMON_TECH:
		str->Format("%.2f: %s is called to %s by the siren song of Tech.", age, firstName.c_str(), domain.c_str());
		break;

		case DOMAIN_CONQUER:
		str->Format("%.2f: %s is occupied by team %s.", age, domain.safe_str(), teamName.safe_str() );
		break;

		default:
		GLASSERT(0);
	}
}


void NewsEvent::Serialize(XStream* xs)
{
	XarcOpen(xs, "NewsEvent");
	XARC_SER(xs, what);
	XARC_SER(xs, pos);
	XARC_SER(xs, firstItemID);
	XARC_SER(xs, secondItemID);
	XARC_SER(xs, date);
	XARC_SER(xs, team);
	XarcClose(xs);
}

