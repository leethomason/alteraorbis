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

#ifndef ISTRING_CONST_INCLUDED
#define ISTRING_CONST_INCLUDED

#include "../grinliz/glstringutil.h"
#include "../game/gamelimits.h"

class IStringConst
{
public:
	static void Init();
	
	static grinliz::IString main;
	static grinliz::IString trigger;
	static grinliz::IString target;
	static grinliz::IString althand;
	static grinliz::IString head;
	static grinliz::IString shield;

	static grinliz::IString humanFemale;
	static grinliz::IString humanMale;
	static grinliz::IString worker;

	static grinliz::IString ring;
	static grinliz::IString gold;
	static grinliz::IString crystal_green;
	static grinliz::IString crystal_red;
	static grinliz::IString crystal_blue;
	static grinliz::IString crystal_violet;
	static grinliz::IString core;

	static grinliz::IString arachnoid;
	static grinliz::IString mantis;
	static grinliz::IString redMantis;
	static grinliz::IString cyclops;
	static grinliz::IString fireCyclops;
	static grinliz::IString shockCyclops;

	// double underscore -> '.'
	static grinliz::IString kiosk__n;
	static grinliz::IString kiosk__m;
	static grinliz::IString kiosk__c;
	static grinliz::IString kiosk__s;

	static int Hardpoint( grinliz::IString str ) {
		if		( str == trigger ) return HARDPOINT_TRIGGER;
		else if ( str == althand ) return HARDPOINT_ALTHAND;
		else if ( str == head )	return HARDPOINT_HEAD;
		else if ( str == shield )	return HARDPOINT_SHIELD;
		GLASSERT( 0 );
		return 0;
	}
	/*func*/ static grinliz::IString Hardpoint( int i ) {
		switch ( i ) {
		case HARDPOINT_TRIGGER:	return trigger;
		case HARDPOINT_ALTHAND: return althand;
		case HARDPOINT_HEAD:	return head;
		case HARDPOINT_SHIELD:	return shield;
		default:
			GLASSERT( 0 );
			return grinliz::IString();
		}
	}
};

#endif // ISTRING_CONST_INCLUDED
