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

#ifndef VISITOR_STATE_COMPONENT_INCLUDED
#define VISITOR_STATE_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../gamui/gamui.h"
#include "visitor.h"

class WorldMap;

class VisitorStateComponent : public Component
{
private:
	typedef Component super;

public:
	VisitorStateComponent();
	~VisitorStateComponent();

	virtual const char* Name() const { return "VisitorStateComponent"; }

	virtual void Serialize( XStream* xs );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual int DoTick( U32 delta );
	void OnChitMsg( Chit* chit, const ChitMsg& msg );

private:
	gamui::Image		wants[VisitorData::NUM_VISITS];
	gamui::DigitalBar	bar;	// FIXME: this is a pretty general need to have here. Probably should be head decos.
	bool needsInit;
};


#endif // VISITOR_STATE_COMPONENT_INCLUDED

