#ifndef CHIT_EVENT_INCLUDED
#define CHIT_EVENT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glvector.h"

// Shallow
class ChitEvent
{
public:

	ChitEvent() : id( -1 )	{}
	ChitEvent( int _id ) : id(_id), data0(-1), pData0(0), pData1(0) { 
		normal.Zero(); 
		bounds.Zero(); 
	}

	int id;
	int data0;
	const void* pData0;	
	const void* pData1;
	grinliz::Vector2F    normal;
	grinliz::Rectangle2F bounds;
};


#endif // CHIT_EVENT_INCLUDED
