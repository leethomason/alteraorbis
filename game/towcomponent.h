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

#ifndef Tow_COMPONENT_INCLUDED
#define Tow_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glmath.h"


// Used to control an item that
// the Chit is moving. (So this
// component controlls a different
// chit.)
class TowComponent : public Component
{
private:
	typedef Component super;
public:
	TowComponent() : towingID(0) {} 

	virtual const char* Name() const { return "TowComponent"; }

	virtual void OnAdd( Chit*, bool init );
	virtual void OnRemove();

	virtual void Serialize( XStream* xs );

	virtual void DebugStr( grinliz::GLString* str );
	virtual int DoTick( U32 delta );

	void SetTowingID( int id )	{ towingID = id; }

private:
	int towingID;
};

#endif // Tow_COMPONENT_INCLUDED