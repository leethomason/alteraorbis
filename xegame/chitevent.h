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
	int id;
	grinliz::Rectangle2F bounds2F;

	union {
		int team;
	};

public:
	ChitEvent( int _id=-1 ) : id( _id ) {}
	int ID() const						{ return id; }

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
