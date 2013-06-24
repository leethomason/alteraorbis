#include "visitor.h"
#include "../grinliz/gldebug.h"
#include "../xarchive/glstreamer.h"


void VisitorData::Serialize( XStream* xs )
{
	XarcOpen( xs, "VisitorData" );
	XARC_SER( xs, id );
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





