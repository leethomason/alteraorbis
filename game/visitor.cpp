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
	XarcClose(xs);
}


VisitorData::VisitorData()
{
	id = 0;
	kioskTime = 0;
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


SectorPort Visitors::ChooseDestination(int index, const Web& web, ChitBag* chitBag, WorldMap* map )
{
	GLASSERT( instance );
	GLASSERT( index >=0 && index <NUM_VISITORS );
	VisitorData* visitor = &visitorData[index];
	Chit* visitorChit = chitBag->GetChit(visitor->id);

	SectorPort sectorPort;
	if (!visitorChit) return sectorPort;

	Vector2I thisSector = ToSector(visitorChit->Position());
	const Web::Node* node = web.FindNode(thisSector);
	if (!node || node->child.Empty()) 
		return sectorPort;

	int which = random.Rand(node->child.Size());
	Vector2I sector = node->child[which]->sector;

	const SectorData& sd = map->GetSectorData( sector );
	GLASSERT(sd.ports);
	if (!sd.ports) 
		return sectorPort;

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
