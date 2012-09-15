/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
