#include "visitor.h"
#include "../grinliz/gldebug.h"
#include "../xarchive/glstreamer.h"
#include "worldmap.h"

using namespace grinliz;

void VisitorData::Serialize( XStream* xs )
{
	XarcOpen( xs, "VisitorData" );
	XARC_SER( xs, id );
	XARC_SER( xs, kioskTime );

	if ( xs->Saving() ) {
		XarcSet( xs, "sectorVisited.size", sectorVisited.Size() );
		XarcSetVectorArr( xs, "sectorVisited.mem",  sectorVisited.Mem(), sectorVisited.Size() );
	}
	else {
		int size = 0;
		XarcGet( xs, "sectorVisited.size", size );
		Vector2I* mem = sectorVisited.PushArr( size );
		XarcGetVectorArr( xs, "sectorVisited.mem", mem, size );
	}
	XarcClose( xs );
}


Visitors* Visitors::instance = 0;

Visitors::Visitors()
{
	GLASSERT( instance == 0 );
	instance = this;
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


SectorPort Visitors::ChooseDestination( int visitorIndex, WorldMap* map )
{
	Random r;
	r.SetSeedFromTime();
	SectorPort sp = map->RandomPort( &r );
	return sp;
}


