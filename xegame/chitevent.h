#ifndef CHIT_EVENT_INCLUDED
#define CHIT_EVENT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glvector.h"


// Shallow/copied
// All data needs to be in the base class.
class ChitEvent
{
protected:
	int id;		// the message sent
	int data;	// if there is obvious data - not warranting a sub-class - put it here
	grinliz::Rectangle2F bounds2F;

	union {
		int team;
	};

public:
	ChitEvent( int _id=-1, int _data=0 ) : id( _id ), data( _data ) {}
	int ID() const			{ return id; }
	int Data() const		{ return data; }

	enum {
		AWARENESS
	};
};


class AwarenessChitEvent : public ChitEvent {
public:
	AwarenessChitEvent( int _team, const grinliz::Rectangle2F& _bounds ) : ChitEvent( ChitEvent::AWARENESS ) {
		bounds2F = _bounds;
		team = _team;
	}
	const grinliz::Rectangle2F& Bounds() const { return bounds2F; }
	int Team() const { return team; }
};


#endif // CHIT_EVENT_INCLUDED
