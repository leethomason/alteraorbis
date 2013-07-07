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

	static grinliz::IString ktrigger;
	static grinliz::IString ktarget;
	static grinliz::IString kalthand;
	static grinliz::IString khead;
	static grinliz::IString kshield;

	static grinliz::IString khumanFemale;
	static grinliz::IString khumanMale;

	static grinliz::IString kring;
	static grinliz::IString klargeRing;
	static grinliz::IString kgold;
	static grinliz::IString kcrystal_green;
	static grinliz::IString kcrystal_red;
	static grinliz::IString kcrystal_blue;
	static grinliz::IString kcrystal_violet;
	static grinliz::IString kcore;

	static int Hardpoint( grinliz::IString str ) {
		if		( str == ktrigger ) return HARDPOINT_TRIGGER;
		else if ( str == kalthand ) return HARDPOINT_ALTHAND;
		else if ( str == khead )	return HARDPOINT_HEAD;
		else if ( str == kshield )	return HARDPOINT_SHIELD;
		GLASSERT( 0 );
		return 0;\
	}
	static grinliz::IString Hardpoint( int i ) {
		switch ( i ) {
		case HARDPOINT_TRIGGER:	return ktrigger;
		case HARDPOINT_ALTHAND: return kalthand;
		case HARDPOINT_HEAD:	return khead;
		case HARDPOINT_SHIELD:	return kshield;
		default:
			GLASSERT( 0 );
			return grinliz::IString();
		}
	}
};

#endif // ISTRING_CONST_INCLUDED
