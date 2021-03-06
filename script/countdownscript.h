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

#ifndef COUNTDOWN_SCRIPT_INCLUDED
#define COUNTDOWN_SCRIPT_INCLUDED

#include "../xegame/cticker.h"
#include "../xegame/component.h"


class CountDownScript : public Component
{
public:
	CountDownScript( int time ) : timer( time ) {}
	virtual ~CountDownScript()	{}

	virtual void Serialize( XStream* xs );
	virtual int DoTick( U32 delta );
	virtual const char* Name() const { return "CountDownScript"; }

private:
	CTicker timer;
};

#endif // PLANT_SCRIPT_INCLUDED