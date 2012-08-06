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
private:
	grinliz::Rectangle2F aoe;	// how broadly this event is broadcast

protected:
	int id;		// the message sent
	int data;	// if there is obvious data - not warranting a sub-class - put it here
	grinliz::Rectangle2F bounds2F;

	union {
		int team;
	};

public:
	ChitEvent( const grinliz::Rectangle2F& areaOfEffect,
		       int _id, int _data=0 ) 
		: aoe( areaOfEffect ), id( _id ), data( _data ) {}
	
	const grinliz::Rectangle2F& AreaOfEffect() const { return aoe; }
	int ID() const			{ return id; }
	int Data() const		{ return data; }

	enum {
		AWARENESS
	};
};


class AwarenessChitEvent : public ChitEvent {
public:
	AwarenessChitEvent( int _team, const grinliz::Rectangle2F& _bounds ) : ChitEvent( _bounds, ChitEvent::AWARENESS ) {
		bounds2F = _bounds;
		team = _team;
	}
	const grinliz::Rectangle2F& Bounds() const { return bounds2F; }
	int Team() const { return team; }
};


#endif // CHIT_EVENT_INCLUDED
