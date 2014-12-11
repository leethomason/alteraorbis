#include "visitor.h"
#include "../xarchive/glstreamer.h"
#include "worldmap.h"
#include "worldinfo.h"
#include "lumoschitbag.h"
#include "../script/corescript.h"
#include "../grinliz/glrandom.h"
#include "../xegame/istringconst.h"
#include "../xegame/spatialcomponent.h"

using namespace grinliz;

void VisitorData::Serialize(XStream* xs)
{
	XarcOpen(xs, "VisitorData");
	XARC_SER(xs, id);
	XARC_SER(xs, kioskTime);
	XARC_SER(xs, want);
	XarcClose(xs);
}


VisitorData::VisitorData()
{
	id = 0;
	kioskTime = 0;
}


/*
void VisitorData::Memory::Serialize( XStream* xs ) 
{
	XarcOpen( xs, "Memory" );
	XARC_SER( xs, sector );
	XARC_SER( xs, kioskType );
	XARC_SER( xs, rating );
	XarcClose( xs );
}
*/


Visitors* Visitors::instance = 0;

Visitors::Visitors()
{
	GLASSERT( instance == 0 );
	instance = this;
	for( int i=0; i<NUM_VISITORS; ++i ) {
		visitorData[i].want = i % VisitorData::NUM_KIOSK_TYPES;
	}
}


Visitors::~Visitors()
{
	GLASSERT( instance == this );
	instance = 0;
}


void Visitors::Serialize( XStream* xs )
{
	XarcOpen( xs, "Visitors" );
	for( int i=0; i<NUM_VISITORS; ++i ) {
		visitorData[i].Serialize( xs );
	}
	XarcClose( xs );
}


VisitorData* Visitors::Get( int index )
{
	GLASSERT( instance );
	GLASSERT( index >=0 && index <NUM_VISITORS );
	return &instance->visitorData[index];
}


SectorPort Visitors::ChooseDestination(int index, const Web& web, ChitBag* chitBag, WorldMap* map )
{
	GLASSERT( instance );
	GLASSERT( index >=0 && index <NUM_VISITORS );
	VisitorData* visitor = &visitorData[index];
	Chit* visitorChit = chitBag->GetChit(visitor->id);

	SectorPort sectorPort;
	if (!visitorChit) return sectorPort;

	Vector2I thisSector = visitorChit->GetSpatialComponent()->GetSector();
	const Web::Node* node = web.FindNode(thisSector);
	if (!node || node->adjacent.Empty()) return sectorPort;

	int which = random.Rand(node->adjacent.Size());
	Vector2I sector = node->adjacent[which]->sector;

	const SectorData& sd = map->GetSectorData( sector );
	if (!sd.ports) return sectorPort;

	int ports[4] = { 1, 2, 4, 8 };
	int port = 0;

	random.ShuffleArray( ports, 4 );
	for( int i=0; i<4; ++i ) {
		if ( sd.ports & ports[i] ) {
			port = ports[i];
			break;
		}
	}
	sectorPort.sector = sector;
	sectorPort.port   = port;
	return sectorPort;
}


/*
void VisitorData::DidVisitKiosk( const grinliz::Vector2I& sector )
{
	bool added = false;
	int kioskType = CurrentKioskWantID();

	for( int i=0; i<memoryArr.Size(); ++i ) {
		if ( memoryArr[i].sector == sector ) {
			memoryArr[i].rating = 1;
			memoryArr[i].kioskType = kioskType;
			added = true;
			break;
		}
	}
	if ( !added && memoryArr.HasCap() ) {
		Memory m;
		m.rating = 1;
		m.kioskType = kioskType;
		m.sector = sector;
		memoryArr.Push( m );
	}
	++nWants;
	++nVisits;
	doneWith = sector;
}
*/

/*
void VisitorData::NoKiosk( const grinliz::Vector2I& sector )
{
	++nVisits;
	doneWith = sector;

	for( int i=0; i<memoryArr.Size(); ++i ) {
		if ( memoryArr[i].sector == sector ) {
			memoryArr.SwapRemove( i );
			break;
		}
	}
}
*/

grinliz::IString VisitorData::CurrentKioskWant()
{
	switch ( want ) {
	case KIOSK_N:	return ISC::kiosk__n;
	case KIOSK_M:	return ISC::kiosk__m;
	case KIOSK_C:	return ISC::kiosk__c;
	case KIOSK_S:	return ISC::kiosk__s;
	}
	GLASSERT( 0 );	// Bad values?
	return IString();
}


