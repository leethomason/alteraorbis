#include "visitor.h"
#include "../xarchive/glstreamer.h"
#include "worldmap.h"
#include "worldinfo.h"
#include "../grinliz/glrandom.h"
#include "../xegame/istringconst.h"

using namespace grinliz;

void VisitorData::Serialize( XStream* xs )
{
	XarcOpen( xs, "VisitorData" );
	XARC_SER( xs, id );
	XARC_SER( xs, kioskTime );
	XARC_SER_ARR( xs, wants, NUM_VISITS );
	XARC_SER( xs, nWants );
	XARC_SER( xs, nVisits );
	XARC_SER( xs, doneWith );
	XARC_SER_CARRAY( xs, memoryArr );
	XarcClose( xs );
}


VisitorData::VisitorData()
{
	id = 0;
	kioskTime = 0;
	for( int i=0; i<NUM_VISITS; ++i ) wants[i] = 0;
}


void VisitorData::Memory::Serialize( XStream* xs ) 
{
	XarcOpen( xs, "Memory" );
	XARC_SER( xs, sector );
	XARC_SER( xs, kioskType );
	XARC_SER( xs, rating );
	XarcClose( xs );
}



Visitors* Visitors::instance = 0;

Visitors::Visitors()
{
	GLASSERT( instance == 0 );
	instance = this;
	Random random;
	for( int i=0; i<NUM_VISITORS; ++i ) {
		for( int k=0; k<VisitorData::NUM_VISITS; ++k ) {
			visitorData[i].wants[k] = random.Rand(VisitorData::NUM_KIOSK_TYPES);
		}
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


SectorPort Visitors::ChooseDestination( int index, WorldMap* map )
{
	GLASSERT( instance );
	GLASSERT( index >=0 && index <NUM_VISITORS );
	Vector2I sector = { 0, 0 };
	int kioskType = visitorData[index].CurrentKioskWantID();
	
	if ( random.Rand(3)) {
		Random notRandom( index );

		grinliz::CArray< VisitorData::Memory, VisitorData::MEMORY*3 > ideas;
		float score[VisitorData::MEMORY*3];

		int idx[3] = { index, notRandom.Rand( NUM_VISITORS ), notRandom.Rand( NUM_VISITORS ) };

		// Push the memory of this object and 2 friends, then
		// choose one at random based on rating.
		for( int k=0; k<3; ++k ) {
			VisitorData* vd = Visitors::Get( idx[k] );
			for( int i=0; i<vd->memoryArr.Size(); ++i ) {
				if (    vd->memoryArr[i].kioskType == kioskType
					 && vd->memoryArr[i].rating > 0 ) 
				{
					score[ideas.Size()] = (float)vd->memoryArr[i].rating;
					ideas.Push( vd->memoryArr[i] );
				}
			}
		}
		if ( !ideas.Empty() ) {
			int select = random.Select( score, ideas.Size() );
			sector = ideas[select].sector;
		}
	}


	while( true ) {
		const SectorData& sd = map->GetSector( sector );
		if ( sd.ports ) 
			break;
		sector.Set( random.Rand( NUM_SECTORS ), random.Rand( NUM_SECTORS ));
	}

	const SectorData& sd = map->GetSector( sector );
	GLASSERT( sd.ports );

	int ports[4] = { 1, 2, 4, 8 };
	int port = 0;

	random.ShuffleArray( ports, 4 );
	for( int i=0; i<4; ++i ) {
		if ( sd.ports & ports[i] ) {
			port = ports[i];
			break;
		}
	}

	SectorPort sectorPort;
	sectorPort.sector = sector;
	sectorPort.port   = port;
	return sectorPort;
}


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


grinliz::IString VisitorData::CurrentKioskWant()
{
	if ( nWants >= NUM_VISITS )
		return IString();

	switch ( wants[nWants] ) {
	case KIOSK_N:	return IStringConst::kkiosk__n;
	case KIOSK_M:	return IStringConst::kkiosk__m;
	case KIOSK_C:	return IStringConst::kkiosk__c;
	case KIOSK_S:	return IStringConst::kkiosk__s;
	}
	GLASSERT( 0 );	// Bad values?
	return IString();
}


