#ifndef CHIT_EVENT_INCLUDED
#define CHIT_EVENT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glvector.h"

class AwarenessChitEvent;

class ChitEvent
{
protected:
	grinliz::Rectangle2F aoe;	// how broadly this event is broadcast

protected:
	int data;	// if there is obvious data - not warranting a sub-class - put it here
	int itemFilter;

	union {
		int team;
	};

public:
	ChitEvent( const grinliz::Rectangle2F& areaOfEffect,
		       int _data=0 ) 
		: aoe( areaOfEffect ), data( _data ), itemFilter(0) {}

	void SetItemFilter( int flag )	{ itemFilter = flag; }
	int  GetItemFilter() const		{ return itemFilter; }
	
	const grinliz::Rectangle2F& AreaOfEffect() const { return aoe; }
	int Data() const		{ return data; }

	virtual const AwarenessChitEvent* ToAwareness() const { return 0; }
};


class AwarenessChitEvent : public ChitEvent {
public:
	AwarenessChitEvent( int _team, const grinliz::Rectangle2F& _bounds ) : ChitEvent( _bounds ) {
		aoe = _bounds;
		team = _team;
	}
	const grinliz::Rectangle2F& Bounds() const { return aoe; }
	int Team() const { return team; }

	virtual const AwarenessChitEvent* ToAwareness() const { return this; }
};


#endif // CHIT_EVENT_INCLUDED
